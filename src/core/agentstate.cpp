#include "agentstate.h"

#include <QJsonArray>

AgentState::AgentState(QObject *parent) : QObject(parent)
{
    m_settle.setSingleShot(true);
    connect(&m_settle, &QTimer::timeout, this, [this] {
        if (m_state == State::Done || m_state == State::Error)
            set(State::Idle);
    });
}

QString AgentState::nameOf(State s)
{
    switch (s) {
    case State::Offline:  return QStringLiteral("offline");
    case State::Idle:     return QStringLiteral("idle");
    case State::Thinking: return QStringLiteral("thinking");
    case State::Working:  return QStringLiteral("working");
    case State::Waiting:  return QStringLiteral("waiting");
    case State::Done:     return QStringLiteral("done");
    case State::Error:    return QStringLiteral("error");
    }
    return QStringLiteral("idle");
}

void AgentState::set(State s, const QString &detail)
{
    if (s == m_state && detail == m_detail)
        return;
    m_state = s;
    m_detail = detail;
    if (s != State::Waiting) {
        m_permissionId.clear();
        m_requestKind.clear();
    }
    if (s != State::Working && s != State::Thinking && s != State::Waiting)
        m_tools.clear();
    emit stateChanged();
}

void AgentState::setConnected(bool connected)
{
    if (!connected)
        set(State::Offline);
    else if (m_state == State::Offline)
        set(State::Idle);
}

void AgentState::promptSent()
{
    m_settle.stop();
    m_tools.clear();
    set(State::Thinking);
}

void AgentState::finish()
{
    set(State::Done);
    m_settle.start(2500);
}

QString AgentState::firstString(const QJsonObject &o, std::initializer_list<const char *> keys)
{
    for (const char *k : keys) {
        const QString v = o.value(QLatin1String(k)).toString();
        if (!v.isEmpty())
            return v;
    }
    return {};
}

QString AgentState::toolName(const QJsonObject &p)
{
    QString n = firstString(p, {"name", "tool", "toolName", "title"});
    if (n.isEmpty())
        n = firstString(p.value("call").toObject(), {"name", "tool"});
    return n;
}

void AgentState::setWaiting(const QString &kind, const QString &id, const QString &detail)
{
    m_permissionId = id;                 // set() would clear these for non-waiting states, so assign directly
    m_requestKind = kind;
    m_state = State::Waiting;
    m_detail = detail;
    emit stateChanged();
}

// OpenCode 2 reports tools as session.tool.input.* / called / progress / success / failed (+ session.shell.*).
// Several tools can run at once, so "working" lasts until the LAST running call has finished.
void AgentState::handleToolEvent(const QString &type, const QJsonObject &p)
{
    const QString key = firstString(p, {"id", "callID", "toolCallID", "toolID"});
    const QString name = toolName(p);
    if (type == QLatin1String("session.tool.success") || type == QLatin1String("session.tool.failed")) {
        m_tools.remove(key.isEmpty() ? QStringLiteral("?") : key);
        if (m_state == State::Working && m_tools.isEmpty())
            set(State::Thinking);
        return;
    }
    // input.started / input.ended / called / progress: a tool is in flight
    m_tools.insert(key.isEmpty() ? QStringLiteral("?") : key);
    if (m_state != State::Waiting)
        set(State::Working, name.isEmpty() ? (m_state == State::Working ? m_detail : QString()) : name);
}

void AgentState::handleEvent(const QString &type, const QJsonObject &p)
{
    if (m_state == State::Offline)
        return;   // stream says it is alive again via setConnected(true)

    const bool free = (m_state != State::Working && m_state != State::Waiting);   // may be overridden by "thinking" hints

    // ---- turn lifecycle (V1 + V2) --------------------------------------------------------------------------------
    if (type == QLatin1String("session.status")) {
        const QString st = p.value("status").toObject().value("type").toString();
        if (st == QLatin1String("busy")) {
            if (free) set(State::Thinking);
        } else if (st == QLatin1String("retry")) {
            set(State::Thinking, QStringLiteral("retrying"));
        } else if (st == QLatin1String("idle")) {
            finish();
        }
    } else if (type == QLatin1String("session.idle") || type == QLatin1String("session.execution.succeeded")) {
        finish();
    } else if (type == QLatin1String("session.execution.started")) {
        if (free) set(State::Thinking);
    } else if (type == QLatin1String("session.execution.interrupted")) {
        m_tools.clear();                              // the user pressed Stop: not an error, not a success
        set(State::Idle);
    } else if (type == QLatin1String("session.execution.failed")) {
        const QJsonObject err = p.value("error").toObject();
        set(State::Error, firstString(err, {"message", "name", "type"}).isEmpty() ? firstString(p, {"message"}) : firstString(err, {"message", "name", "type"}));
        m_settle.start(4000);
    } else if (type == QLatin1String("session.error")) {
        const QJsonObject err = p.value("error").toObject();
        set(State::Error, err.value("data").toObject().value("message").toString(err.value("name").toString()));
        m_settle.start(4000);
    } else if (type == QLatin1String("session.retry.scheduled")) {
        if (m_state != State::Waiting) set(State::Thinking, QStringLiteral("retrying"));

    // ---- user input needed: permission (V1 + V2) and question forms (V2) ---------------------------------------------
    } else if (type == QLatin1String("permission.updated") || type == QLatin1String("permission.asked")) {
        // V1: {id, title, ...}  V2: Permission.Request {id, sessionID, action, resources[], message?, metadata?, source?}
        QString title = firstString(p, {"title", "message", "permission"});
        if (title.isEmpty()) {
            const QString action = firstString(p, {"action"});
            const QString res = p.value("resources").toArray().isEmpty() ? QString() : p.value("resources").toArray().first().toString();
            title = res.isEmpty() ? action : action + QStringLiteral(": ") + res;
        }
        setWaiting(QStringLiteral("permission"), firstString(p, {"id", "requestID"}), title);
    } else if (type == QLatin1String("form.created")) {
        // V2 replaced the question tool by forms: Form.Info {id, sessionID, title, fields[]}
        setWaiting(QStringLiteral("form"), firstString(p, {"id", "formID"}),
                   firstString(p, {"title", "message"}).isEmpty() ? QStringLiteral("opencode asks a question") : firstString(p, {"title", "message"}));
    } else if (type == QLatin1String("permission.replied") || type == QLatin1String("form.replied") || type == QLatin1String("form.cancelled")) {
        set(State::Thinking);

    // ---- OpenCode 2 granular events ----------------------------------------------------------------------------------
    } else if (type.startsWith(QLatin1String("session.tool."))) {
        // input.delta fires per token: treat it like input.started (a tool call is being prepared)
        handleToolEvent(type, p);
    } else if (type == QLatin1String("session.shell.started")) {
        m_tools.insert(QStringLiteral("shell"));
        if (m_state != State::Waiting) set(State::Working, QStringLiteral("shell"));
    } else if (type == QLatin1String("session.shell.ended")) {
        m_tools.remove(QStringLiteral("shell"));
        if (m_state == State::Working && m_tools.isEmpty()) set(State::Thinking);
    } else if (type == QLatin1String("session.step.started")) {
        if (free) set(State::Thinking);
    } else if (type.startsWith(QLatin1String("session.reasoning."))) {
        if (free) set(State::Thinking, QStringLiteral("reasoning"));
    } else if (type == QLatin1String("session.text.started") || type == QLatin1String("session.text.delta")) {
        if (free) set(State::Thinking, QStringLiteral("writing"));
    } else if (type == QLatin1String("session.compaction.started")) {
        if (free) set(State::Thinking, QStringLiteral("compacting"));

    // ---- OpenCode 1 part events --------------------------------------------------------------------------------------
    } else if (type == QLatin1String("message.part.updated")) {
        const QJsonObject part = p.value("part").toObject();
        const QString pt = part.value("type").toString();
        if (pt == QLatin1String("tool")) {
            const QString status = part.value("state").toObject().value("status").toString();
            if (status == QLatin1String("pending") || status == QLatin1String("running"))
                set(State::Working, part.value("tool").toString());
            else if (m_state == State::Working)
                set(State::Thinking);
        } else if (pt == QLatin1String("reasoning")) {
            if (m_state != State::Waiting)
                set(State::Thinking, QStringLiteral("reasoning"));
        } else if (pt == QLatin1String("text")) {
            if (free)
                set(State::Thinking, QStringLiteral("writing"));
        }
    }
}
