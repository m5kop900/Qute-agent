#include "agentstate.h"
#include <QJsonDocument>
#include <QtTest>

static QJsonObject J(const char *json) { return QJsonDocument::fromJson(json).object(); }

class TstAgentState : public QObject {
    Q_OBJECT
private slots:
    void startsOfflineAndGoesIdle()
    {
        AgentState a;
        QCOMPARE(a.stateName(), QString("offline"));
        a.setConnected(true);
        QCOMPARE(a.stateName(), QString("idle"));
        a.setConnected(false);
        QCOMPARE(a.stateName(), QString("offline"));
    }
    void promptMakesItThink()
    {
        AgentState a; a.setConnected(true);
        a.promptSent();
        QCOMPARE(a.stateName(), QString("thinking"));
    }
    void toolRunningThenCompleted()
    {
        AgentState a; a.setConnected(true); a.promptSent();
        a.handleEvent("message.part.updated", J(R"({"part":{"type":"tool","tool":"bash","state":{"status":"running"}}})"));
        QCOMPARE(a.stateName(), QString("working"));
        QCOMPARE(a.detail(), QString("bash"));
        a.handleEvent("message.part.updated", J(R"({"part":{"type":"tool","tool":"bash","state":{"status":"completed"}}})"));
        QCOMPARE(a.stateName(), QString("thinking"));
    }
    void permissionFlow()
    {
        AgentState a; a.setConnected(true); a.promptSent();
        a.handleEvent("permission.updated", J(R"({"id":"perm1","title":"Run rm -rf build"})"));
        QCOMPARE(a.stateName(), QString("waiting"));
        QCOMPARE(a.permissionId(), QString("perm1"));
        a.handleEvent("permission.replied", {});
        QCOMPARE(a.stateName(), QString("thinking"));
        QVERIFY(a.permissionId().isEmpty());
    }
    void statusIdleMeansDone()
    {
        AgentState a; a.setConnected(true); a.promptSent();
        a.handleEvent("session.status", J(R"({"status":{"type":"idle"}})"));
        QCOMPARE(a.stateName(), QString("done"));
    }
    void v2ExecutionEvents()
    {
        AgentState a; a.setConnected(true);
        a.handleEvent("session.execution.started", {});
        QCOMPARE(a.stateName(), QString("thinking"));
        a.handleEvent("session.execution.succeeded", {});
        QCOMPARE(a.stateName(), QString("done"));
    }
    void errorState()
    {
        AgentState a; a.setConnected(true);
        a.handleEvent("session.error", J(R"({"error":{"name":"ProviderAuthError"}})"));
        QCOMPARE(a.stateName(), QString("error"));
    }
};

QTEST_GUILESS_MAIN(TstAgentState)
#include "tst_agentstate.moc"
