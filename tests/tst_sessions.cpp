// /new, /session and the session list (opencode sessions, including the TUI's), through the mock server.
#include "appconfig.h"
#include "conversationmodel.h"
#include "opencodeclient.h"
#include "sessionstore.h"

#include "mockopencodeserver.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest>

static QByteArray bytes(const char *s) { return QByteArray(s); }

class TstSessions : public QObject {
    Q_OBJECT
private slots:
    void parseSessionsOrdersByLastUseAndFallsBack()
    {
        const QVariantList list = OpenCodeClient::parseSessions(QJsonDocument::fromJson(bytes(R"({"data":[
            {"id":"ses_a","title":"Older","directory":"/p/a","time":{"updated":1000}},
            {"id":"ses_b","location":{"directory":"/p/b"},"time":{"updated":3000}},
            {"id":"ses_c","time":{"updated":2000}},
            {"title":"no id"}]})")));
        QCOMPARE(list.size(), 3);                                     // the entry without an id is skipped
        QCOMPARE(list.at(0).toMap().value("key").toString(), QString("ses_b"));
        QCOMPARE(list.at(0).toMap().value("sub").toString(), QString("/p/b"));  // directory from location
        QCOMPARE(list.at(1).toMap().value("key").toString(), QString("ses_c"));
        QCOMPARE(list.at(1).toMap().value("label").toString(), QString("(untitled)"));
        QCOMPARE(list.at(2).toMap().value("key").toString(), QString("ses_a"));
    }

    void newSessionDoesNotResumeTheLastOne()
    {
        MockOpenCodeServer server;
        QVERIFY(server.start());
        server.setEventFrames({QStringLiteral(R"({"type":"server.connected","properties":{}})")});
        server.setJson("/api/command", bytes(R"({"data":[]})"));
        server.setJson("/api/model", bytes(R"({"data":[]})"));
        server.setJson("/api/model/default", bytes(R"({"data":{}})"));
        server.setJson("/api/agent", bytes(R"({"data":[]})"));
        server.setJson("/api/session/ses_old", bytes(R"({"data":{"id":"ses_old"}})"));
        server.setJson("/api/session/ses_old/message", bytes(R"({"data":[],"cursor":{"next":null}})"));
        server.setJson("/api/session", bytes(R"({"id":"ses_new"})"));

        AppConfig config;
        config.setServerUrl(server.baseUrl());
        config.setServerPassword(QString());
        config.setApiFlavor(QStringLiteral("v2"));
        config.setAutoStartServer(false);
        config.setLastSessionId(QStringLiteral("ses_old"));

        OpenCodeClient client(&config);
        client.start();
        QTRY_COMPARE_WITH_TIMEOUT(client.sessionId(), QStringLiteral("ses_old"), 5000);   // resumed on start

        client.newSession();
        QTRY_COMPARE_WITH_TIMEOUT(client.sessionId(), QStringLiteral("ses_new"), 5000);
        QCOMPARE(config.lastSessionId(), QStringLiteral("ses_new"));
    }

    void switchSessionLoadsTheOtherSessionHistory()
    {
        MockOpenCodeServer server;
        QVERIFY(server.start());
        server.setEventFrames({QStringLiteral(R"({"type":"server.connected","properties":{}})")});
        server.setJson("/api/command", bytes(R"({"data":[]})"));
        server.setJson("/api/model", bytes(R"({"data":[]})"));
        server.setJson("/api/model/default", bytes(R"({"data":{}})"));
        server.setJson("/api/agent", bytes(R"({"data":[]})"));
        // The mock matches paths without the query string, so GET /api/session (the list) and POST /api/session
        // share one key. The client resumes ses_mock on start (lastSessionId), so no POST is needed here.
        server.setJson("/api/session", bytes(R"({"data":[
            {"id":"ses_other","title":"Made in the TUI","directory":"/p","time":{"updated":9000}},
            {"id":"ses_mock","title":"Qute","time":{"updated":1000}}]})"));
        server.setJson("/api/session/ses_mock", bytes(R"({"data":{"id":"ses_mock"}})"));
        server.setJson("/api/session/ses_mock/message", bytes(R"({"data":[],"cursor":{"next":null}})"));
        server.setJson("/api/session/ses_other", bytes(R"({"data":{"id":"ses_other"}})"));
        server.setJson("/api/session/ses_other/message", bytes(R"({"data":[],"cursor":{"next":null}})"));

        AppConfig config;
        config.setServerUrl(server.baseUrl());
        config.setServerPassword(QString());
        config.setApiFlavor(QStringLiteral("v2"));
        config.setAutoStartServer(false);
        config.setLastSessionId(QStringLiteral("ses_mock"));

        OpenCodeClient client(&config);
        SessionStore store(&client, &config);
        client.start();
        QTRY_COMPARE_WITH_TIMEOUT(client.sessionId(), QStringLiteral("ses_mock"), 5000);

        QVariantList listed;
        QObject::connect(&client, &OpenCodeClient::sessionsListed, [&listed](const QVariantList &l) { listed = l; });
        client.listSessions();
        QTRY_COMPARE_WITH_TIMEOUT(listed.size(), 2, 5000);
        QCOMPARE(listed.first().toMap().value("key").toString(), QString("ses_other"));   // a TUI session is in the list

        store.conversation()->applyUserPrompt(QStringLiteral("old chat"));
        QCOMPARE(store.conversation()->rowCount(), 1);

        client.switchSession(QStringLiteral("ses_other"));
        QCOMPARE(client.sessionId(), QStringLiteral("ses_other"));
        QCOMPARE(store.conversation()->rowCount(), 0);                                    // previous view is cleared
        QCOMPARE(config.lastSessionId(), QStringLiteral("ses_other"));

        QTRY_VERIFY_WITH_TIMEOUT([&server] {
            for (const MockOpenCodeServer::Request &r : server.requests())
                if (r.method == "GET" && r.path.startsWith("/api/session/ses_other/message")) return true;
            return false;
        }(), 5000);
    }

    void switchSessionRejectsBadIds()
    {
        AppConfig config;
        OpenCodeClient client(&config);
        QString error;
        QObject::connect(&client, &OpenCodeClient::requestFailed, [&error](const QString &m) { error = m; });
        client.switchSession(QStringLiteral("../../etc"));
        QVERIFY(!error.isEmpty());
        QCOMPARE(client.sessionId(), QString());
    }

    // F1.4: running a server command posts the documented body to the current session.
    void promptWithFilesPostsFlatBodyWithFiles()
    {
        MockOpenCodeServer server;
        QVERIFY(server.start());
        server.setEventFrames({QStringLiteral(R"({"type":"server.connected","properties":{}})")});
        server.setJson("/api/model", bytes(R"({"data":[]})"));
        server.setJson("/api/model/default", bytes(R"({"data":{}})"));
        server.setJson("/api/agent", bytes(R"({"data":[]})"));
        server.setJson("/api/session/ses_mock", bytes(R"({"data":{"id":"ses_mock"}})"));
        server.setJson("/api/session/ses_mock/message", bytes(R"({"data":[],"cursor":{"next":null}})"));
        server.setJson("/api/session/ses_mock/prompt", bytes(R"({"data":{}})"));

        AppConfig config;
        config.setServerUrl(server.baseUrl());
        config.setServerPassword(QString());
        config.setApiFlavor(QStringLiteral("v2"));
        config.setAutoStartServer(false);
        config.setLastSessionId(QStringLiteral("ses_mock"));

        OpenCodeClient client(&config);
        client.start();
        QTRY_COMPARE_WITH_TIMEOUT(client.sessionId(), QStringLiteral("ses_mock"), 5000);

        QVariantList files;
        files << QVariantMap{{"uri", "file:///tmp/a.txt"}, {"name", "a.txt"}};
        files << QVariantMap{{"uri", "data:image/png;base64,iVBORw0="}, {"name", "shot.png"}};
        QVERIFY(client.sendPromptWithFiles(QStringLiteral("what is in these"), files, QVariantList{QVariantMap{{"name", "build"}}}));

        QTRY_VERIFY_WITH_TIMEOUT([&server] {
            for (const MockOpenCodeServer::Request &r : server.requests())
                if (r.method == "POST" && r.path == "/api/session/ses_mock/prompt") return true;
            return false;
        }(), 5000);
        for (const MockOpenCodeServer::Request &r : server.requests()) {
            if (r.path != "/api/session/ses_mock/prompt") continue;
            const QJsonObject body = QJsonDocument::fromJson(r.body).object();
            QCOMPARE(body.value("text").toString(), QStringLiteral("what is in these"));
            QVERIFY(!body.contains("prompt"));          // flat body, not the old nested form
            const QJsonArray sent = body.value("files").toArray();
            QCOMPARE(sent.size(), 2);
            QCOMPARE(sent.at(0).toObject().value("uri").toString(), QStringLiteral("file:///tmp/a.txt"));
            QCOMPARE(sent.at(1).toObject().value("uri").toString(), QStringLiteral("data:image/png;base64,iVBORw0="));
            QCOMPARE(sent.at(1).toObject().value("name").toString(), QStringLiteral("shot.png"));
            QCOMPARE(body.value("agents").toArray().at(0).toObject().value("name").toString(), QStringLiteral("build"));
        }
    }

    void compactPostsToNativeEndpoint()
    {
        MockOpenCodeServer server;
        QVERIFY(server.start());
        server.setEventFrames({QStringLiteral(R"({"type":"server.connected","properties":{}})")});
        server.setJson("/api/model", bytes(R"({"data":[]})"));
        server.setJson("/api/model/default", bytes(R"({"data":{}})"));
        server.setJson("/api/agent", bytes(R"({"data":[]})"));
        server.setJson("/api/session/ses_mock", bytes(R"({"data":{"id":"ses_mock"}})"));
        server.setJson("/api/session/ses_mock/message", bytes(R"({"data":[],"cursor":{"next":null}})"));
        server.setJson("/api/session/ses_mock/compact", bytes(R"({"data":{"type":"compaction"}})"));

        AppConfig config;
        config.setServerUrl(server.baseUrl());
        config.setServerPassword(QString());
        config.setApiFlavor(QStringLiteral("v2"));
        config.setAutoStartServer(false);
        config.setLastSessionId(QStringLiteral("ses_mock"));

        OpenCodeClient client(&config);
        client.start();
        QTRY_COMPARE_WITH_TIMEOUT(client.sessionId(), QStringLiteral("ses_mock"), 5000);
        client.compactSession();
        QTRY_VERIFY_WITH_TIMEOUT([&server] {
            for (const MockOpenCodeServer::Request &r : server.requests())
                if (r.method == "POST" && r.path == "/api/session/ses_mock/compact") return true;
            return false;
        }(), 5000);
    }

    void serverCommandPostsNameAndText()
    {
        MockOpenCodeServer server;
        QVERIFY(server.start());
        server.setEventFrames({QStringLiteral(R"({"type":"server.connected","properties":{}})")});
        server.setJson("/api/command", bytes(R"({"data":[{"name":"review","description":"Review the code"}]})"));
        server.setJson("/api/model", bytes(R"({"data":[]})"));
        server.setJson("/api/model/default", bytes(R"({"data":{}})"));
        server.setJson("/api/agent", bytes(R"({"data":[]})"));
        server.setJson("/api/session/ses_mock", bytes(R"({"data":{"id":"ses_mock"}})"));
        server.setJson("/api/session/ses_mock/message", bytes(R"({"data":[],"cursor":{"next":null}})"));
        server.setJson("/api/session/ses_mock/command", bytes("{}"), 204);

        AppConfig config;
        config.setServerUrl(server.baseUrl());
        config.setServerPassword(QString());
        config.setApiFlavor(QStringLiteral("v2"));
        config.setAutoStartServer(false);
        config.setLastSessionId(QStringLiteral("ses_mock"));

        OpenCodeClient client(&config);
        client.start();
        QTRY_COMPARE_WITH_TIMEOUT(client.sessionId(), QStringLiteral("ses_mock"), 5000);

        client.runServerCommand(QStringLiteral("review"), QStringLiteral("src/core"), QVariantList{}, QVariantList{});
        QTRY_VERIFY_WITH_TIMEOUT([&server] {
            for (const MockOpenCodeServer::Request &r : server.requests())
                if (r.method == "POST" && r.path == "/api/session/ses_mock/command") return true;
            return false;
        }(), 5000);
        // Regression: the body key is "command" (opencode 2 answers 400 "Missing key [command]" to "name").
        for (const MockOpenCodeServer::Request &r : server.requests()) {
            if (r.path != "/api/session/ses_mock/command") continue;
            const QJsonObject body = QJsonDocument::fromJson(r.body).object();
            QCOMPARE(body.value("command").toString(), QStringLiteral("review"));
            QCOMPARE(body.value("text").toString(), QStringLiteral("src/core"));
            QVERIFY(body.contains("files"));
        }

        QJsonObject body;
        for (const MockOpenCodeServer::Request &r : server.requests())
            if (r.method == "POST" && r.path == "/api/session/ses_mock/command")
                body = QJsonDocument::fromJson(r.body).object();
        QCOMPARE(body.value("command").toString(), QStringLiteral("review"));
        QCOMPARE(body.value("text").toString(), QStringLiteral("src/core"));
        QVERIFY(body.value("files").isArray());
        QVERIFY(body.value("agents").isArray());
        QVERIFY(body.value("skills").isArray());
    }
};

QTEST_MAIN(TstSessions)
#include "tst_sessions.moc"
