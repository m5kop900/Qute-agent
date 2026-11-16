#include "appconfig.h"
#include "opencodeclient.h"

#include <QFileInfo>
#include <QJsonDocument>
#include <QtTest>

class TstClientParse : public QObject {
    Q_OBJECT
private slots:
    // A server that is not running and will not be started must explain itself instead of showing
    // "Connecting to opencode…" forever (the exact symptom that made F0 impossible to diagnose).
    void unreachableServerWithoutAutoStartExplainsWhy()
    {
        AppConfig config;
        config.setServerUrl(QStringLiteral("http://127.0.0.1:59871"));   // nothing listens here
        config.setServerPassword(QString());
        config.setAutoStartServer(false);
        config.setApiFlavor(QStringLiteral("v2"));

        OpenCodeClient client(&config);
        client.start();

        QTRY_VERIFY_WITH_TIMEOUT(!client.lastError().isEmpty(), 8000);
        QVERIFY(!client.connected());
        QVERIFY2(client.lastError().contains(QStringLiteral("Auto-start server")), qPrintable(client.lastError()));
        QVERIFY2(client.lastError().contains(config.serverUrl()), qPrintable(client.lastError()));
        QVERIFY2(client.lastError().contains(QStringLiteral("--port 59871")), qPrintable(client.lastError()));
    }

    void spawnedServerThatExitsIsReported()
    {
        if (!QFileInfo::exists(QStringLiteral("/bin/false")))
            QSKIP("/bin/false not available on this platform");

        AppConfig config;
        config.setServerUrl(QStringLiteral("http://127.0.0.1:59872"));
        config.setServerPassword(QString());
        config.setAutoStartServer(true);
        config.setServerCommand(QStringLiteral("/bin/false"));           // starts, then exits immediately
        config.setApiFlavor(QStringLiteral("v2"));

        OpenCodeClient client(&config);
        client.start();

        QTRY_VERIFY_WITH_TIMEOUT(client.lastError().contains(QStringLiteral("exited"))
                                     || client.lastError().contains(QStringLiteral("crashed")), 8000);
        QVERIFY(!client.connected());
    }
    void v1MessageListKeepsOnlyTextAfterLastUser()
    {
        const auto doc = QJsonDocument::fromJson(R"([
          {"info":{"role":"user"},"parts":[{"type":"text","text":"old q"}]},
          {"info":{"role":"assistant"},"parts":[{"type":"text","text":"old a"}]},
          {"info":{"role":"user"},"parts":[{"type":"text","text":"new q"}]},
          {"info":{"role":"assistant"},"parts":[{"type":"tool"},{"type":"text","text":"part one"},{"type":"text","text":"part two"}]}
        ])");
        QCOMPARE(OpenCodeClient::assistantTextAfterLastUser(OpenCodeClient::extractArray(doc)), QString("part one\n\npart two"));
    }
    void v2WrappedListWithTypeField()
    {
        const auto doc = QJsonDocument::fromJson(R"({"data":[
          {"type":"user","content":[{"type":"text","text":"q"}]},
          {"type":"assistant","content":[{"type":"text","text":"answer"}]}
        ]})");
        const QJsonArray arr = OpenCodeClient::extractArray(doc);
        QCOMPARE(arr.size(), 2);
        QCOMPARE(OpenCodeClient::assistantTextAfterLastUser(arr), QString("answer"));
    }
    void v2ProjectionIgnoresReasoningAndToolContent()
    {
        const auto doc = QJsonDocument::fromJson(
            "{\"data\":[{\"type\":\"user\",\"content\":[{\"type\":\"text\",\"text\":\"run ls\"}]},"
            "{\"type\":\"assistant\",\"agent\":\"build\",\"content\":["
            "{\"type\":\"reasoning\",\"text\":\"let me think\"},"
            "{\"type\":\"tool\",\"id\":\"call_1\",\"name\":\"bash\"},"
            "{\"type\":\"text\",\"text\":\"Here are the files.\"}]}],\"cursor\":{\"next\":null}}");
        const QJsonArray arr = OpenCodeClient::extractArray(doc);
        QCOMPARE(arr.size(), 2);
        QCOMPARE(OpenCodeClient::assistantTextAfterLastUser(arr), QString("Here are the files."));
    }
    void ignoresSyntheticAndOtherTypes()
    {
        const auto doc = QJsonDocument::fromJson(R"([
          {"type":"user","content":[{"type":"text","text":"q"}]},
          {"type":"synthetic","content":[{"type":"text","text":"hidden"}]},
          {"type":"assistant","content":[{"type":"text","text":"ok","synthetic":false}]}
        ])");
        QCOMPARE(OpenCodeClient::assistantTextAfterLastUser(OpenCodeClient::extractArray(doc)), QString("ok"));
    }
};

QTEST_GUILESS_MAIN(TstClientParse)
#include "tst_clientparse.moc"
