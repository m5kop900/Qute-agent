#include "mcppresets.h"
#include "opencodeconfig.h"
#include <QtTest>

using Format = OpencodeConfig::Format;

class TstOpencodeConfig : public QObject {
    Q_OBJECT
private slots:
    void emptyFileHasNoFormat()
    {
        OpencodeConfig c;
        QVERIFY(c.loadFromJson("{}"));
        QVERIFY(!c.detectFormat().has_value());
    }
    void detectsDialects()
    {
        OpencodeConfig v1, v2;
        QVERIFY(v1.loadFromJson(R"({"mcp":{"x":{"type":"local","command":["a"]}}})"));
        QCOMPARE(*v1.detectFormat(), Format::V1);
        QVERIFY(v2.loadFromJson(R"({"mcp":{"servers":{"x":{"type":"local","command":["a"]}}}})"));
        QCOMPARE(*v2.detectFormat(), Format::V2);
    }
    void parsesJsoncWithCommentsAndTrailingCommas()
    {
        OpencodeConfig c;
        QVERIFY(c.loadFromJson("{\n // c\n \"model\": \"a/b\",\n}"));
        QVERIFY(c.hadComments());
        QCOMPARE(c.root().value("model").toString(), QString("a/b"));
    }
    void rejectsBrokenJson()
    {
        OpencodeConfig c;
        QString err;
        QVERIFY(!c.loadFromJson("{ nope", &err));
        QVERIFY(!err.isEmpty());
    }
    void addsMcpV1AndKeepsOtherKeys()
    {
        OpencodeConfig c;
        // Avoid "//" on this line: moc's preprocessor treats it as a comment inside macros.
        QVERIFY(c.loadFromJson(R"({"theme":"x","mcp":{"old":{"type":"remote","url":"https:o","enabled":true}}})"));
        c.setMcp(OpencodeConfig::McpSpec{"playwright", false, {"npx", "@playwright/mcp@latest"}, {}, {}, {}}, Format::V1);
        const QJsonObject mcp = c.root().value("mcp").toObject();
        QVERIFY(mcp.contains("old"));
        const QJsonObject pw = mcp.value("playwright").toObject();
        QCOMPARE(pw.value("type").toString(), QString("local"));
        QCOMPARE(pw.value("enabled").toBool(), true);
        QCOMPARE(pw.value("command").toArray().size(), 2);
        QCOMPARE(c.root().value("theme").toString(), QString("x"));
        QVERIFY(c.root().contains("$schema"));
    }
    void addsMcpV2UnderServersWithoutEnabled()
    {
        OpencodeConfig c;
        QVERIFY(c.loadFromJson("{}"));
        OpencodeConfig::McpSpec s;
        s.name = "context7"; s.remote = true; s.url = "https://mcp.context7.com/mcp";
        s.headers.insert("CONTEXT7_API_KEY", "{env:CONTEXT7_API_KEY}");
        c.setMcp(s, Format::V2);
        const QJsonObject srv = c.root().value("mcp").toObject().value("servers").toObject().value("context7").toObject();
        QCOMPARE(srv.value("type").toString(), QString("remote"));
        QVERIFY(!srv.contains("enabled"));
        QCOMPARE(srv.value("headers").toObject().value("CONTEXT7_API_KEY").toString(), QString("{env:CONTEXT7_API_KEY}"));
        QVERIFY(c.hasMcp("context7"));
    }
    void switchingDialectNeverDuplicatesAServer()
    {
        OpencodeConfig c;
        QVERIFY(c.loadFromJson("{}"));
        OpencodeConfig::McpSpec s; s.name = "x"; s.command = {"npx", "x"};
        c.setMcp(s, Format::V1);
        c.setMcp(s, Format::V2);
        QVERIFY(!c.root().value("mcp").toObject().contains("x"));
        QVERIFY(c.root().value("mcp").toObject().value("servers").toObject().contains("x"));
    }
    void removeMcp()
    {
        OpencodeConfig c;
        QVERIFY(c.loadFromJson(R"({"mcp":{"a":{"type":"local","command":["x"]},"servers":{"b":{"type":"local","command":["y"]}}}})"));
        QCOMPARE(c.mcpNames().size(), 2);
        QVERIFY(c.removeMcp("a"));
        QVERIFY(c.removeMcp("b"));
        QVERIFY(!c.removeMcp("nope"));
    }
    void openAiCompatibleProviderV1()
    {
        OpencodeConfig c;
        QVERIFY(c.loadFromJson("{}"));
        OpencodeConfig::ProviderSpec p;
        p.id = "local"; p.name = "Local"; p.baseUrl = "http://127.0.0.1:8080/v1";
        p.models << OpencodeConfig::ModelSpec{"qwen", "qwen", 32768, 8192};
        c.setProvider(p, Format::V1);
        const QJsonObject e = c.root().value("provider").toObject().value("local").toObject();
        QCOMPARE(e.value("npm").toString(), QString("@ai-sdk/openai-compatible"));
        QCOMPARE(e.value("options").toObject().value("baseURL").toString(), QString("http://127.0.0.1:8080/v1"));
        QVERIFY(!e.value("options").toObject().contains("apiKey"));
        QCOMPARE(e.value("models").toObject().value("qwen").toObject().value("limit").toObject().value("context").toInt(), 32768);
    }
    void openAiCompatibleProviderV2()
    {
        OpencodeConfig c;
        QVERIFY(c.loadFromJson("{}"));
        OpencodeConfig::ProviderSpec p;
        p.id = "local"; p.baseUrl = "http://x/v1"; p.apiKeyRef = "{file:/k}";
        p.models << OpencodeConfig::ModelSpec{"m", "m", 0, 0};
        c.setProvider(p, Format::V2);
        const QJsonObject e = c.root().value("providers").toObject().value("local").toObject();
        QCOMPARE(e.value("package").toString(), QString("aisdk:@ai-sdk/openai-compatible"));
        QCOMPARE(e.value("settings").toObject().value("baseURL").toString(), QString("http://x/v1"));
        QCOMPARE(e.value("settings").toObject().value("apiKey").toString(), QString("{file:/k}"));
        QVERIFY(!c.root().contains("provider"));
    }
    void presetsExist()
    {
        for (const char *id : {"context7", "playwright", "sequential-thinking", "github"})
            QVERIFY(McpPresets::find(id) != nullptr);
        QVERIFY(McpPresets::find("playwright")->command.contains("@playwright/mcp@latest"));
        QVERIFY(McpPresets::find("context7")->remote);
    }
};

QTEST_APPLESS_MAIN(TstOpencodeConfig)
#include "tst_opencodeconfig.moc"
