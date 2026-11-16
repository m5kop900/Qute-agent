#include "conversationmodel.h"

#include <QFile>
#include <QJsonDocument>
#include <QtTest>

static QJsonArray loadArray(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(f.readAll()).array();
}

static QJsonArray loadMessages(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(f.readAll()).object().value("data").toArray();
}

// Compare the fields that must be identical between the live event projection and the server projection.
static QVariantList snapshot(const ConversationModel &m)
{
    QVariantList out;
    for (const ConversationModel::Item &it : m.items()) {
        out << QVariantMap{{"kind", it.kind}, {"text", it.text}, {"name", it.name},
                           {"status", it.status}, {"detail", it.detail}};
    }
    return out;
}

class TstConversationModel : public QObject {
    Q_OBJECT
private slots:
    void eventReplayMatchesProjection()
    {
        const QString eventsPath = QFINDTESTDATA("fixtures/v2-turn-events.json");
        const QString messagesPath = QFINDTESTDATA("fixtures/v2-turn-messages.json");
        QVERIFY(!eventsPath.isEmpty());
        QVERIFY(!messagesPath.isEmpty());
        const QJsonArray events = loadArray(eventsPath);
        QVERIFY(!events.isEmpty());

        ConversationModel live;
        live.applyUserPrompt(QStringLiteral("run the tests"));
        for (const QJsonValue &v : events) {
            const QJsonObject e = v.toObject();
            live.applyEvent(e.value("type").toString(), e.value("properties").toObject());
        }

        ConversationModel projected;
        projected.applyMessages(loadMessages(messagesPath));

        QCOMPARE(snapshot(live), snapshot(projected));
    }

    void streamingTextIsBuiltFromDeltas()
    {
        ConversationModel m;
        m.applyEvent("session.text.started", QJsonObject{{"assistantMessageID", "m1"}, {"ordinal", 0}});
        m.applyEvent("session.text.delta", QJsonObject{{"assistantMessageID", "m1"}, {"ordinal", 0}, {"delta", "Hel"}});
        m.applyEvent("session.text.delta", QJsonObject{{"assistantMessageID", "m1"}, {"ordinal", 0}, {"delta", "lo"}});
        QCOMPARE(m.rowCount(), 1);
        QCOMPARE(m.items().first().kind, QString("assistant-text"));
        QCOMPARE(m.items().first().text, QString("Hello"));
        QVERIFY(m.items().first().streaming);
        m.applyEvent("session.text.ended", QJsonObject{{"assistantMessageID", "m1"}, {"ordinal", 0}, {"text", "Hello!"}});
        QCOMPARE(m.items().first().text, QString("Hello!"));
        QVERIFY(!m.items().first().streaming);
    }

    void reasoningBecomesItsOwnItem()
    {
        ConversationModel m;
        m.applyEvent("session.reasoning.started", QJsonObject{{"assistantMessageID", "m1"}, {"ordinal", 0}});
        m.applyEvent("session.reasoning.ended", QJsonObject{{"assistantMessageID", "m1"}, {"ordinal", 0}, {"text", "think"}});
        QCOMPARE(m.rowCount(), 1);
        QCOMPARE(m.items().first().kind, QString("reasoning"));
        QCOMPARE(m.items().first().text, QString("think"));
    }

    void toolLifecycleAndInput()
    {
        ConversationModel m;
        m.applyEvent("session.tool.called", QJsonObject{{"id", "c1"}, {"name", "bash"},
                                                        {"input", QJsonObject{{"command", "ls"}}}});
        QCOMPARE(m.items().first().kind, QString("tool"));
        QCOMPARE(m.items().first().name, QString("bash"));
        QCOMPARE(m.items().first().status, QString("running"));
        QCOMPARE(m.items().first().detail, QString("{\"command\":\"ls\"}"));
        m.applyEvent("session.tool.success", QJsonObject{{"id", "c1"}});
        QCOMPARE(m.items().first().status, QString("ok"));
        QCOMPARE(m.items().first().name, QString("bash"));       // not wiped by the id-only update
    }

    void shellIsAToolItem()
    {
        ConversationModel m;
        m.applyEvent("session.shell.started", {});
        m.applyEvent("session.shell.ended", {});
        QCOMPARE(m.rowCount(), 1);
        QCOMPARE(m.items().first().name, QString("shell"));
        QCOMPARE(m.items().first().status, QString("ok"));
    }

    void permissionRequestSurvivesReconcile()
    {
        ConversationModel m;
        m.applyEvent("permission.asked", QJsonObject{{"id", "p1"}, {"action", "bash"}, {"resources", QJsonArray{"rm -rf build"}}});
        QCOMPARE(m.items().first().kind, QString("request"));
        QCOMPARE(m.items().first().requestKind, QString("permission"));
        QCOMPARE(m.items().first().status, QString("pending"));

        const QString messagesPath = QFINDTESTDATA("fixtures/v2-turn-messages.json");
        m.applyMessages(loadMessages(messagesPath));
        // The projection has no permission entry, but a still-pending request must not disappear.
        bool found = false;
        for (const ConversationModel::Item &it : m.items())
            found = found || (it.kind == "request" && it.status == "pending");
        QVERIFY(found);

        m.applyEvent("permission.replied", QJsonObject{{"id", "p1"}});
        for (const ConversationModel::Item &it : m.items())
            if (it.kind == "request")
                QCOMPARE(it.status, QString("answered"));
    }

    void formRequestAndCancel()
    {
        ConversationModel m;
        m.applyEvent("form.created", QJsonObject{{"id", "f1"}, {"title", "Which database?"}});
        QCOMPARE(m.items().first().requestKind, QString("form"));
        QCOMPARE(m.items().first().text, QString("Which database?"));
        m.applyEvent("form.cancelled", QJsonObject{{"id", "f1"}});
        QCOMPARE(m.items().first().status, QString("cancelled"));
    }

    void errorBecomesNotice()
    {
        ConversationModel m;
        m.applyEvent("session.error", QJsonObject{{"error", QJsonObject{{"name", "ProviderAuthError"}}}});
        QCOMPARE(m.rowCount(), 1);
        QCOMPARE(m.items().first().kind, QString("notice"));
        QCOMPARE(m.items().first().text, QString("ProviderAuthError"));
    }

    void v1PartEventsAreProjected()
    {
        ConversationModel m;
        m.applyEvent("message.part.updated", QJsonObject{{"part", QJsonObject{{"id", "pr1"}, {"type", "text"}, {"text", "hi"}}}});
        m.applyEvent("message.part.updated", QJsonObject{{"part", QJsonObject{{"id", "t1"}, {"type", "tool"}, {"tool", "bash"},
                                                                              {"state", QJsonObject{{"status", "running"}}}}}});
        m.applyEvent("message.part.updated", QJsonObject{{"part", QJsonObject{{"id", "t1"}, {"type", "tool"}, {"tool", "bash"},
                                                                              {"state", QJsonObject{{"status", "completed"}}}}}});
        QCOMPARE(m.rowCount(), 2);
        QCOMPARE(m.items().at(0).text, QString("hi"));
        QCOMPARE(m.items().at(1).name, QString("bash"));
        QCOMPARE(m.items().at(1).status, QString("ok"));
    }

    void userPromptItem()
    {
        ConversationModel m;
        m.applyUserPrompt("hello");
        QCOMPARE(m.rowCount(), 1);
        QCOMPARE(m.items().first().kind, QString("user"));
        QCOMPARE(m.items().first().text, QString("hello"));
    }

    void applyMessagesReplacesPreviousHistory()
    {
        ConversationModel m;
        m.applyUserPrompt("old");
        m.applyEvent("session.text.started", QJsonObject{{"assistantMessageID", "old"}, {"ordinal", 0}});
        const QString messagesPath = QFINDTESTDATA("fixtures/v2-turn-messages.json");
        m.applyMessages(loadMessages(messagesPath));
        QCOMPARE(m.rowCount(), 4);      // user + text + tool + text
    }
};

QTEST_GUILESS_MAIN(TstConversationModel)
#include "tst_conversationmodel.moc"
