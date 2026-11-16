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
    void v2ToolsAreWorkingUntilTheLastCallFinishes()
    {
        AgentState a; a.setConnected(true); a.promptSent();
        a.handleEvent("session.tool.called", J(R"({"id":"c1","name":"bash"})"));
        QCOMPARE(a.stateName(), QString("working"));
        QCOMPARE(a.detail(), QString("bash"));
        a.handleEvent("session.tool.called", J(R"({"id":"c2","name":"read"})"));
        a.handleEvent("session.tool.success", J(R"({"id":"c1"})"));
        QCOMPARE(a.stateName(), QString("working"));          // c2 is still running
        a.handleEvent("session.tool.failed", J(R"({"id":"c2"})"));
        QCOMPARE(a.stateName(), QString("thinking"));
    }
    void v2ToolInputStartAlreadyCountsAsWorking()
    {
        AgentState a; a.setConnected(true); a.promptSent();
        a.handleEvent("session.tool.input.started", J(R"({"id":"c1"})"));
        QCOMPARE(a.stateName(), QString("working"));
        a.handleEvent("session.tool.success", J(R"({"id":"c1"})"));
        QCOMPARE(a.stateName(), QString("thinking"));
    }
    void v2PermissionRequestAndReply()
    {
        AgentState a; a.setConnected(true); a.promptSent();
        a.handleEvent("permission.asked", J(R"({"id":"per_1","sessionID":"s1","action":"bash","resources":["rm -rf build"]})"));
        QCOMPARE(a.stateName(), QString("waiting"));
        QCOMPARE(a.requestKind(), QString("permission"));
        QCOMPARE(a.permissionId(), QString("per_1"));
        QCOMPARE(a.detail(), QString("bash: rm -rf build"));
        // a tool call arriving while a permission is pending must not leave "waiting"
        a.handleEvent("session.tool.called", J(R"({"id":"c1","name":"bash"})"));
        QCOMPARE(a.stateName(), QString("waiting"));
        a.handleEvent("permission.replied", J(R"({"id":"per_1"})"));
        QCOMPARE(a.stateName(), QString("thinking"));
        QVERIFY(a.requestKind().isEmpty());
        QVERIFY(a.permissionId().isEmpty());
    }
    void v2PermissionUsesMessageWhenPresent()
    {
        AgentState a; a.setConnected(true);
        a.handleEvent("permission.asked", J(R"({"id":"p","message":"Allow editing main.cpp?","action":"edit"})"));
        QCOMPARE(a.detail(), QString("Allow editing main.cpp?"));
    }
    void v2FormMakesItWaitAndCancelResumes()
    {
        AgentState a; a.setConnected(true); a.promptSent();
        a.handleEvent("form.created", J(R"({"id":"f1","title":"Which database?"})"));
        QCOMPARE(a.stateName(), QString("waiting"));
        QCOMPARE(a.requestKind(), QString("form"));
        QCOMPARE(a.permissionId(), QString("f1"));
        QCOMPARE(a.detail(), QString("Which database?"));
        a.handleEvent("form.cancelled", J(R"({"id":"f1"})"));
        QCOMPARE(a.stateName(), QString("thinking"));
    }
    void v2ReasoningAndTextAreOnlyHints()
    {
        AgentState a; a.setConnected(true); a.promptSent();
        a.handleEvent("session.reasoning.started", {});
        QCOMPARE(a.detail(), QString("reasoning"));
        a.handleEvent("session.text.delta", J(R"({"delta":"hi"})"));
        QCOMPARE(a.detail(), QString("writing"));
        a.handleEvent("session.tool.called", J(R"({"id":"c1","name":"edit"})"));
        // a running tool wins over text hints
        a.handleEvent("session.text.delta", J(R"({"delta":"x"})"));
        QCOMPARE(a.stateName(), QString("working"));
    }
    void v2InterruptGoesBackToIdleNotDone()
    {
        AgentState a; a.setConnected(true); a.promptSent();
        a.handleEvent("session.execution.interrupted", {});
        QCOMPARE(a.stateName(), QString("idle"));
    }
    void v2ExecutionFailedIsAnError()
    {
        AgentState a; a.setConnected(true); a.promptSent();
        a.handleEvent("session.execution.failed", J(R"({"error":{"message":"model not found"}})"));
        QCOMPARE(a.stateName(), QString("error"));
        QCOMPARE(a.detail(), QString("model not found"));
    }
    void v2ShellCountsAsWorking()
    {
        AgentState a; a.setConnected(true); a.promptSent();
        a.handleEvent("session.shell.started", {});
        QCOMPARE(a.stateName(), QString("working"));
        a.handleEvent("session.shell.ended", {});
        QCOMPARE(a.stateName(), QString("thinking"));
    }
    void fullV2TurnWalksThroughAllStates()
    {
        AgentState a; a.setConnected(true);
        QCOMPARE(a.stateName(), QString("idle"));
        a.promptSent();                                                          QCOMPARE(a.stateName(), QString("thinking"));
        a.handleEvent("session.tool.called", J(R"({"id":"c1","name":"bash"})")); QCOMPARE(a.stateName(), QString("working"));
        a.handleEvent("permission.asked", J(R"({"id":"p1","action":"bash"})")); QCOMPARE(a.stateName(), QString("waiting"));
        a.handleEvent("permission.replied", J(R"({"id":"p1"})"));               QCOMPARE(a.stateName(), QString("thinking"));
        a.handleEvent("session.tool.success", J(R"({"id":"c1"})"));             QCOMPARE(a.stateName(), QString("thinking"));
        a.handleEvent("session.execution.succeeded", {});                       QCOMPARE(a.stateName(), QString("done"));
        QTRY_COMPARE_WITH_TIMEOUT(a.stateName(), QString("idle"), 4500);
        a.setConnected(false);                                                   QCOMPARE(a.stateName(), QString("offline"));
        a.setConnected(true);                                                    QCOMPARE(a.stateName(), QString("idle"));
    }
    void v2TextEventsDoNotBreakState()
    {
        // V2 streams text via session.text.* events (not message.part.*); they must not
        // corrupt the state machine.
        AgentState a; a.setConnected(true); a.promptSent();
        QCOMPARE(a.stateName(), QString("thinking"));
        a.handleEvent("session.text.started", J(R"({"assistantMessageID":"m1","ordinal":0})"));
        QCOMPARE(a.stateName(), QString("thinking"));
        a.handleEvent("session.text.delta", J(R"({"assistantMessageID":"m1","ordinal":0,"delta":"hi"})"));
        QCOMPARE(a.stateName(), QString("thinking"));
        a.handleEvent("session.text.ended", J(R"({"assistantMessageID":"m1","ordinal":0,"text":"hi"})"));
        QCOMPARE(a.stateName(), QString("thinking"));
    }
    void offlineThenRecovers()
    {
        AgentState a;
        QCOMPARE(a.stateName(), QString("offline"));
        a.setConnected(false);
        QCOMPARE(a.stateName(), QString("offline"));
        a.setConnected(true);
        QCOMPARE(a.stateName(), QString("idle"));
        a.promptSent();
        QCOMPARE(a.stateName(), QString("thinking"));
        a.handleEvent("session.status", J(R"({"status":{"type":"idle"}})"));
        QCOMPARE(a.stateName(), QString("done"));
        // the settle timer (2.5 s) moves done -> idle by itself
        QTRY_COMPARE_WITH_TIMEOUT(a.stateName(), QString("idle"), 4500);
    }
};

QTEST_GUILESS_MAIN(TstAgentState)
#include "tst_agentstate.moc"
