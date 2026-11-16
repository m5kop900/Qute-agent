#include "appconfig.h"
#include "opencodecatalog.h"
#include "opencodeclient.h"
#include "sessionstore.h"

#include "mockopencodeserver.h"

#include <QJsonDocument>
#include <QtTest>

static QJsonDocument doc(const char *json) { return QJsonDocument::fromJson(json); }

class TstCatalog : public QObject {
    Q_OBJECT
private slots:
    void parseCommandsAcceptsArrayAndMap()
    {
        const QVariantList a = OpenCodeCatalog::parseCommands(doc(R"({"data":[{"name":"init","description":"Init the project"}]})"));
        QCOMPARE(a.size(), 1);
        QCOMPARE(a.first().toMap().value("name").toString(), QString("init"));
        QCOMPARE(a.first().toMap().value("description").toString(), QString("Init the project"));

        const QVariantList b = OpenCodeCatalog::parseCommands(doc(R"({"review":{"description":"Review"}})"));
        QCOMPARE(b.size(), 1);
        QCOMPARE(b.first().toMap().value("name").toString(), QString("review"));
    }

    void parseModelsFlattensProvidersAndVariants()
    {
        const QVariantList models = OpenCodeCatalog::parseModels(
            doc(R"({"providers":[{"id":"anthropic","models":[
                    {"id":"claude","name":"Claude","variants":["low","high"]},
                    {"id":"claude-haiku"}]}]})"));
        QCOMPARE(models.size(), 2);
        const QVariantMap first = models.at(0).toMap();
        QCOMPARE(first.value("providerID").toString(), QString("anthropic"));
        QCOMPARE(first.value("modelID").toString(), QString("claude"));
        QCOMPARE(first.value("name").toString(), QString("Claude"));
        QCOMPARE(first.value("id").toString(), QString("anthropic/claude"));
        QCOMPARE(first.value("variants").toList().size(), 2);
        QCOMPARE(first.value("variants").toList().first().toMap().value("id").toString(), QString("low"));
        QCOMPARE(first.value("variants").toList().first().toMap().value("label").toString(), QString("Low"));
        // A model without a name still gets one from its id.
        QCOMPARE(models.at(1).toMap().value("name").toString(), QString("claude-haiku"));
    }

    void parseModelsCarriesLimitsAndFlags()
    {
        const QVariantList models = OpenCodeCatalog::parseModels(doc(R"({"data":[
            {"id":"m","providerID":"p","name":"M","type":"chat","enabled":true,
             "limit":{"context":1000,"output":100},"variants":[{"id":"high","settings":{}}]}]})"));
        QCOMPARE(models.size(), 1);
        const QVariantMap m = models.first().toMap();
        QCOMPARE(m.value("contextLimit").toLongLong(), qint64(1000));
        QCOMPARE(m.value("outputLimit").toLongLong(), qint64(100));
        QCOMPARE(m.value("enabled").toBool(), true);
        QCOMPARE(m.value("type").toString(), QString("chat"));
        QCOMPARE(m.value("variants").toList().first().toMap().value("label").toString(), QString("High"));
    }

    void parseDefaultModelSingleObject()
    {
        const QVariantList models = OpenCodeCatalog::parseModels(
            doc(R"({"data":{"id":"n","providerID":"k","modelID":"n","name":"N","limit":{"context":5000}}})"));
        QCOMPARE(models.size(), 1);
        QCOMPARE(models.first().toMap().value("modelID").toString(), QString("n"));
        QCOMPARE(models.first().toMap().value("contextLimit").toLongLong(), qint64(5000));
    }

    void parseModelsAcceptsFlatArray()
    {
        const QVariantList models = OpenCodeCatalog::parseModels(
            doc(R"({"data":[{"providerID":"openai","modelID":"gpt","name":"GPT"}]})"));
        QCOMPARE(models.size(), 1);
        QCOMPARE(models.first().toMap().value("id").toString(), QString("openai/gpt"));
    }

    void parseAgents()
    {
        const QVariantList agents = OpenCodeCatalog::parseAgents(
            doc(R"({"data":[{"id":"build","name":"Build","description":"Build things","mode":"primary"}]})"));
        QCOMPARE(agents.size(), 1);
        QCOMPARE(agents.first().toMap().value("id").toString(), QString("build"));
        QCOMPARE(agents.first().toMap().value("mode").toString(), QString("primary"));
        QCOMPARE(agents.first().toMap().value("hidden").toBool(), false);
    }

    void parseFilesAcceptsStringsAndObjects()
    {
        const QVariantList files = OpenCodeCatalog::parseFiles(
            doc(R"({"data":["/tmp/a.md",{"path":"/tmp/dir/b.txt","name":"bee.txt"}]})"));
        QCOMPARE(files.size(), 2);
        QCOMPARE(files.at(0).toMap().value("name").toString(), QString("a.md"));
        QCOMPARE(files.at(1).toMap().value("name").toString(), QString("bee.txt"));
    }

    void catalogRefreshesThroughTheClientAndMockServer()
    {
        MockOpenCodeServer server;
        QVERIFY(server.start());
        server.setEventFrames({QStringLiteral(R"({"type":"server.connected","properties":{}})")});
        server.setJson("/api/command", QByteArrayLiteral(R"({"data":[{"name":"init","description":"Init"}]})"));
        server.setJson("/api/model", QByteArrayLiteral(R"({"data":[
            {"id":"claude","providerID":"anthropic","name":"Claude","type":"chat","enabled":true,
             "limit":{"context":100000,"output":8192},"variants":[{"id":"high"},{"id":"low"}]},
            {"id":"gpt","providerID":"openai","name":"GPT","type":"chat","enabled":true,
             "limit":{"context":50000,"output":4096},"variants":[]}]})"));
        server.setJson("/api/model/default", QByteArrayLiteral(
                           R"({"data":{"id":"gpt","providerID":"openai","name":"GPT","limit":{"context":50000}}})"));
        server.setJson("/api/agent", QByteArrayLiteral(R"({"data":[
            {"id":"build","name":"Build","mode":"primary"},
            {"id":"plan","name":"Plan","mode":"primary"},
            {"id":"general","name":"General","mode":"subagent"},
            {"id":"title","name":"Title","mode":"primary","hidden":true}]})"));
        server.setJson("/api/fs/find", QByteArrayLiteral(R"({"data":[{"path":"/tmp/readme.md"}]})"));
        server.setJson("/api/session/ses_mock", QByteArrayLiteral(
            R"({"data":{"id":"ses_mock","agent":"build",
                        "model":{"id":"claude","providerID":"anthropic","variant":"high"}}})"));
        server.setJson("/api/session/ses_mock/agent", QByteArrayLiteral("{}"), 204);
        server.setJson("/api/session/ses_mock/model", QByteArrayLiteral("{}"), 204);

        AppConfig config;
        config.setServerUrl(server.baseUrl());
        config.setServerPassword(QString());
        config.setApiFlavor(QStringLiteral("v2"));
        config.setAutoStartServer(false);

        OpenCodeClient client(&config);
        OpenCodeCatalog catalog(&client);
        SessionStore store(&client, &config, &catalog);
        client.start();

        auto request = [&server](const QString &method, const QString &path) {
            for (const MockOpenCodeServer::Request &r : server.requests())
                if (r.method == method && r.path == path)
                    return r;
            return MockOpenCodeServer::Request{};
        };

        QTRY_VERIFY_WITH_TIMEOUT(client.connected(), 5000);
        QTRY_COMPARE_WITH_TIMEOUT(catalog.commands().size(), 1, 5000);
        QTRY_COMPARE_WITH_TIMEOUT(catalog.models().size(), 2, 5000);
        QTRY_COMPARE_WITH_TIMEOUT(catalog.agents().size(), 4, 5000);
        QTRY_COMPARE_WITH_TIMEOUT(catalog.primaryAgents().size(), 2, 5000);   // hidden + subagents filtered
        QCOMPARE(catalog.primaryAgents().first().toMap().value("id").toString(), QString("build"));
        QCOMPARE(catalog.defaultModel().value("modelID").toString(), QString("gpt"));
        QCOMPARE(catalog.modelByRef("anthropic", "claude").value("contextLimit").toLongLong(), qint64(100000));
        QCOMPARE(client.sessionId(), QString("ses_mock"));

        catalog.searchFiles(QStringLiteral("readme"));
        QTRY_COMPARE_WITH_TIMEOUT(catalog.files().size(), 1, 5000);

        bool sawCommand = false;
        bool sawSearch = false;
        for (const MockOpenCodeServer::Request &r : server.requests()) {
            sawCommand = sawCommand || (r.method == "GET" && r.path == "/api/command");
            sawSearch = sawSearch || (r.method == "GET" && r.path.startsWith("/api/fs/find?query=readme"));
        }
        QVERIFY(sawCommand);
        QVERIFY(sawSearch);

        // The message-history transport that SessionStore reconciles from (F0.1).
        server.setJson("/api/session/ses_mock/message", QByteArrayLiteral(
            R"({"data":[{"id":"m1","type":"assistant","agent":"build",
                         "model":{"id":"claude","providerID":"anthropic","variant":"high"},
                         "tokens":{"input":5000,"output":1000,"reasoning":4000,
                                   "cache":{"read":10000,"write":0}}}]})"));
        QJsonArray history;
        QObject::connect(&client, &OpenCodeClient::messageHistoryReceived,
                         [&history](const QJsonArray &messages) { history = messages; });
        client.requestMessageHistory(5);
        QTRY_COMPARE_WITH_TIMEOUT(history.size(), 1, 5000);
        QCOMPARE(history.first().toObject().value("type").toString(), QString("assistant"));

        // Agent + model come from GET /api/session/{id}; the context ring uses the last assistant message.
        QTRY_COMPARE_WITH_TIMEOUT(store.activeAgent(), QString("build"), 5000);
        QTRY_COMPARE_WITH_TIMEOUT(store.activeModel(), QString("anthropic/claude#high"), 5000);
        QTRY_COMPARE_WITH_TIMEOUT(store.contextTokens(), qint64(20000), 5000);
        QTRY_COMPARE_WITH_TIMEOUT(store.contextPercent(), 20, 5000);
        QCOMPARE(store.contextLimit(), qint64(100000));

        // Switching goes through the session endpoints and updates the store from the synthetic event.
        store.selectAgent(QStringLiteral("plan"));
        QTRY_VERIFY_WITH_TIMEOUT(!request("POST", "/api/session/ses_mock/agent").body.isEmpty(), 5000);
        QCOMPARE(QJsonDocument::fromJson(request("POST", "/api/session/ses_mock/agent").body)
                     .object().value("agent").toString(), QString("plan"));
        QCOMPARE(store.activeAgent(), QString("plan"));

        store.selectModel(QStringLiteral("openai"), QStringLiteral("gpt"), QString());
        QTRY_VERIFY_WITH_TIMEOUT(!request("POST", "/api/session/ses_mock/model").body.isEmpty(), 5000);
        const QJsonObject ref = QJsonDocument::fromJson(request("POST", "/api/session/ses_mock/model").body)
                                    .object().value("model").toObject();
        QCOMPARE(ref.value("id").toString(), QString("gpt"));
        QCOMPARE(ref.value("providerID").toString(), QString("openai"));
        QVERIFY(!ref.contains("variant"));                       // auto: the server picks the default
        QCOMPARE(store.activeModel(), QString("openai/gpt"));
        QTRY_COMPARE_WITH_TIMEOUT(store.contextPercent(), 40, 5000);   // 20000 / 50000
    }
};

QTEST_GUILESS_MAIN(TstCatalog)
#include "tst_catalog.moc"
