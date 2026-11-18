// F2.1: Build/Plan mode switching. Pure cycle logic plus a round trip through the mock server
// (POST /api/session/{id}/agent), i.e. what Shift+Tab does in the overlay.
#include "appconfig.h"
#include "opencodecatalog.h"
#include "opencodeclient.h"
#include "sessionstore.h"

#include "mockopencodeserver.h"

#include <QtTest>

class TstModes : public QObject {
    Q_OBJECT
private slots:
    void nextAgentCyclesAndWraps()
    {
        const QStringList ids{QStringLiteral("build"), QStringLiteral("plan")};
        QCOMPARE(SessionStore::nextAgent(QStringLiteral("build"), ids), QStringLiteral("plan"));
        QCOMPARE(SessionStore::nextAgent(QStringLiteral("plan"), ids), QStringLiteral("build"));
        // Unset agent starts at the first one, an unknown agent falls back to the first one too.
        QCOMPARE(SessionStore::nextAgent(QString(), ids), QStringLiteral("plan"));
        QCOMPARE(SessionStore::nextAgent(QStringLiteral("nope"), ids), QStringLiteral("build"));
        // Nothing to switch to with fewer than two primary agents.
        QVERIFY(SessionStore::nextAgent(QStringLiteral("build"), {QStringLiteral("build")}).isEmpty());
        QVERIFY(SessionStore::nextAgent(QStringLiteral("build"), {}).isEmpty());
    }

    void shiftTabSendsAgentSwitchToServer()
    {
        MockOpenCodeServer server;
        QVERIFY(server.start());
        server.setEventFrames({QStringLiteral(R"({"type":"server.connected","properties":{}})")});
        server.setJson("/api/command", QByteArrayLiteral(R"({"data":[]})"));
        server.setJson("/api/model", QByteArrayLiteral(R"({"data":[]})"));
        server.setJson("/api/model/default", QByteArrayLiteral(R"({"data":{}})"));
        server.setJson("/api/agent", QByteArrayLiteral(R"({"data":[
            {"id":"build","name":"Build","mode":"primary"},
            {"id":"plan","name":"Plan","mode":"primary"},
            {"id":"general","name":"General","mode":"subagent"}]})"));
        server.setJson("/api/session/ses_mock", QByteArrayLiteral(R"({"data":{"id":"ses_mock","agent":"build"}})"));
        server.setJson("/api/session/ses_mock/agent", QByteArrayLiteral("{}"), 204);

        AppConfig config;
        config.setServerUrl(server.baseUrl());
        config.setServerPassword(QString());
        config.setApiFlavor(QStringLiteral("v2"));
        config.setAutoStartServer(false);

        OpenCodeClient client(&config);
        OpenCodeCatalog catalog(&client);
        SessionStore store(&client, &config, &catalog);
        client.start();

        QTRY_VERIFY_WITH_TIMEOUT(client.connected(), 5000);
        QTRY_COMPARE_WITH_TIMEOUT(catalog.primaryAgents().size(), 2, 5000);
        QTRY_COMPARE_WITH_TIMEOUT(client.sessionId(), QStringLiteral("ses_mock"), 5000);

        auto agentPosts = [&server] {
            QStringList bodies;
            for (const MockOpenCodeServer::Request &r : server.requests())
                if (r.method == "POST" && r.path == "/api/session/ses_mock/agent")
                    bodies << QString::fromUtf8(r.body);
            return bodies;
        };

        store.cycleAgent();                                            // build -> plan
        QTRY_COMPARE_WITH_TIMEOUT(agentPosts().size(), 1, 5000);
        QVERIFY(agentPosts().first().contains(QStringLiteral("\"agent\":\"plan\"")));
        QCOMPARE(store.activeAgent(), QStringLiteral("plan"));

        store.cycleAgent();                                            // plan -> build
        QTRY_COMPARE_WITH_TIMEOUT(agentPosts().size(), 2, 5000);
        QVERIFY(agentPosts().last().contains(QStringLiteral("\"agent\":\"build\"")));
        QCOMPARE(store.activeAgent(), QStringLiteral("build"));
    }
};

QTEST_MAIN(TstModes)
#include "tst_modes.moc"
