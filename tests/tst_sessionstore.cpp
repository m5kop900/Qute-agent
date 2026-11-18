#include "sessionstore.h"

#include "appconfig.h"
#include "conversationmodel.h"

#include <QFile>
#include <QJsonDocument>
#include <QtTest>

static QJsonArray loadMessages(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(f.readAll()).object().value("data").toArray();
}

class TstSessionStore : public QObject {
    Q_OBJECT
private slots:
    void promptSetsBusyAndAddsUserItem()
    {
        SessionStore store(nullptr, nullptr);
        QVERIFY(!store.busy());
        store.onPromptSent(QStringLiteral("hello"));
        QVERIFY(store.busy());
        QCOMPARE(store.conversation()->rowCount(), 1);
        QCOMPARE(store.conversation()->items().first().kind, QString("user"));
    }

    void idleAndExecutionEventsClearBusy()
    {
        SessionStore store(nullptr, nullptr);
        store.onPromptSent(QStringLiteral("x"));
        store.onEvent("session.status", QJsonObject{{"status", QJsonObject{{"type", "idle"}}}});
        QVERIFY(!store.busy());

        store.onEvent("session.execution.started", {});
        QVERIFY(store.busy());
        store.onEvent("session.execution.succeeded", {});
        QVERIFY(!store.busy());

        store.onEvent("session.status", QJsonObject{{"status", QJsonObject{{"type", "busy"}}}});
        QVERIFY(store.busy());
        store.onEvent("session.status", QJsonObject{{"status", QJsonObject{{"type", "idle"}}}});
        QVERIFY(!store.busy());
    }

    void usageIsParsedTolerantly()
    {
        SessionStore store(nullptr, nullptr);
        store.applySessionInfo(QJsonObject{{"tokens", QJsonObject{{"input", 100}, {"output", 50}}}, {"cost", 0.02}});
        QCOMPARE(store.usage().value("input").toDouble(), 100.0);
        QCOMPARE(store.usage().value("output").toDouble(), 50.0);
        QCOMPARE(store.usage().value("total").toDouble(), 150.0);
        QCOMPARE(store.usage().value("cost").toDouble(), 0.02);

        // wrapped in {"info": {...}} and usage.tokens
        store.applySessionInfo(QJsonObject{{"info", QJsonObject{{"usage", QJsonObject{{"tokens", QJsonObject{{"input", 10}, {"output", 5}}}}}}}});
        QCOMPARE(store.usage().value("total").toDouble(), 15.0);
    }

    void agentAndModelFollowEvents()
    {
        SessionStore store(nullptr, nullptr);
        store.onEvent("session.agent.selected", QJsonObject{{"agent", "plan"}});
        QCOMPARE(store.activeAgent(), QString("plan"));

        store.onEvent("session.model.selected",
                      QJsonObject{{"model", QJsonObject{{"providerID", "anthropic"}, {"modelID", "claude"}, {"variant", "high"}}}});
        QCOMPARE(store.activeModel(), QString("anthropic/claude#high"));
        QCOMPARE(store.activeModelProvider(), QString("anthropic"));
        QCOMPARE(store.activeModelId(), QString("claude"));
        QCOMPARE(store.activeVariant(), QString("high"));

        store.onEvent("session.model.selected", QJsonObject{{"model", "openai/gpt"}});
        QCOMPARE(store.activeModel(), QString("openai/gpt"));
        QCOMPARE(store.activeModelProvider(), QString("openai"));
        QCOMPARE(store.activeModelId(), QString("gpt"));
        QCOMPARE(store.activeVariant(), QString());
    }

    void contextTokensUseLastAssistantMessageWithUsage()
    {
        SessionStore store(nullptr, nullptr);
        QJsonArray messages;
        messages.append(QJsonObject{{"id", "m1"}, {"type", "assistant"},
                                    {"tokens", QJsonObject{{"input", 100}, {"output", 50},
                                                           {"cache", QJsonObject{{"read", 850}, {"write", 0}}}}}});
        messages.append(QJsonObject{{"id", "m2"}, {"type", "assistant"},
                                    {"tokens", QJsonObject{{"input", 1000}, {"output", 500}, {"reasoning", 500},
                                                           {"cache", QJsonObject{{"read", 8000}, {"write", 0}}}}}});
        // Newest message is still streaming: null tokens must be skipped, not read as zero.
        messages.append(QJsonObject{{"id", "m3"}, {"type", "assistant"}, {"tokens", QJsonValue::Null}});
        store.onMessages(messages);
        QCOMPARE(store.contextTokens(), qint64(10000));
        QCOMPARE(store.contextPercent(), -1);        // no catalog -> no limit

        store.onEvent("session.step.ended",
                      QJsonObject{{"tokens", QJsonObject{{"input", 10}, {"output", 5},
                                                         {"cache", QJsonObject{{"read", 85}, {"write", 0}}}}}});
        QCOMPARE(store.contextTokens(), qint64(100));
    }

    void contextMathMatchesOpencode()
    {
        QCOMPARE(SessionStore::tokenTotal(QJsonObject{{"input", 1}, {"output", 2}, {"reasoning", 3},
                                                      {"cache", QJsonObject{{"read", 4}, {"write", 5}}}}),
                 qint64(15));
        QCOMPARE(SessionStore::tokenTotal({}), qint64(0));
        QCOMPARE(SessionStore::percentFor(0, 0), -1);
        QCOMPARE(SessionStore::percentFor(10, 0), -1);
        QCOMPARE(SessionStore::percentFor(0, 1000), 0);
        QCOMPARE(SessionStore::percentFor(250, 1000), 25);
        QCOMPARE(SessionStore::percentFor(1000, 1000), 100);
        QCOMPARE(SessionStore::percentFor(1, 3), 33);
        QCOMPARE(SessionStore::percentFor(2, 3), 67);
    }

    void modelAndAgentAreDerivedFromHistory()
    {
        SessionStore store(nullptr, nullptr);
        const QString path = QFINDTESTDATA("fixtures/v2-turn-messages.json");
        QVERIFY(!path.isEmpty());
        store.onMessages(loadMessages(path));
        QCOMPARE(store.conversation()->rowCount(), 4);
        QCOMPARE(store.activeAgent(), QString("build"));
        QCOMPARE(store.activeModel(), QString("anthropic/claude-sonnet"));
    }

    void workDirFollowsConfig()
    {
        AppConfig config;
        config.setWorkDir(QStringLiteral("/tmp/project"));
        SessionStore store(nullptr, &config);
        QCOMPARE(store.workDir(), QString("/tmp/project"));
        config.setWorkDir(QStringLiteral("/tmp/other"));
        QCOMPARE(store.workDir(), QString("/tmp/other"));
    }

    // F2.4: ContextUsage edge cases. Missing or zero limits give -1 (unknown, never 0 %), zero or negative
    // usage gives 0, and overflow is reported as-is (the ring clamps the drawing, not the number).
    void contextPercentEdgeCases()
    {
        QCOMPARE(SessionStore::percentFor(100, 0), -1);
        QCOMPARE(SessionStore::percentFor(100, -5), -1);
        QCOMPARE(SessionStore::percentFor(0, 1000), 0);
        QCOMPARE(SessionStore::percentFor(-5, 1000), 0);
        QCOMPARE(SessionStore::percentFor(800, 1000), 80);
        QCOMPARE(SessionStore::percentFor(950, 1000), 95);
        QCOMPARE(SessionStore::percentFor(1500, 1000), 150);
    }
};

QTEST_GUILESS_MAIN(TstSessionStore)
#include "tst_sessionstore.moc"
