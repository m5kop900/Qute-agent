#include "agentstate.h"

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
    if (s != State::Waiting)
        m_permissionId.clear();
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
    set(State::Thinking);
}

void AgentState::finish()
{
    set(State::Done);
    m_settle.start(2500);
}

void AgentState::handleEvent(const QString &type, const QJsonObject &p)
{
    if (m_state == State::Offline)
        return;   // stream says it is alive again via setConnected(true)

    // NOTE: event names follow opencode's /event stream. Verify against <server>/doc
    // and docs/OPENCODE.md when upgrading opencode; unknown events are ignored on purpose.
    if (type == QLatin1String("session.status")) {
        const QString st = p.value("status").toObject().value("type").toString();
        if (st == QLatin1String("busy")) {
            if (m_state != State::Working && m_state != State::Waiting)
                set(State::Thinking);
        } else if (st == QLatin1String("retry")) {
            set(State::Thinking, QStringLiteral("retrying"));
        } else if (st == QLatin1String("idle")) {
            finish();
        }
    } else if (type == QLatin1String("session.idle") || type == QLatin1String("session.execution.succeeded")) {
        finish();                                   // OpenCode 2 reports turn end as session.execution.*
    } else if (type == QLatin1String("session.execution.started")) {
        set(State::Thinking);
    } else if (type == QLatin1String("session.execution.failed")) {
        set(State::Error, p.value("error").toObject().value("message").toString());
        m_settle.start(4000);
    } else if (type == QLatin1String("session.error")) {
        const QJsonObject err = p.value("error").toObject();
        set(State::Error, err.value("data").toObject().value("message").toString(err.value("name").toString()));
        m_settle.start(4000);
    } else if (type == QLatin1String("permission.updated") || type == QLatin1String("permission.asked")) {
        m_permissionId = p.value("id").toString();
        const QString title = p.value("title").toString(p.value("permission").toString());
        m_state = State::Waiting;           // set() would clear permissionId for non-waiting states
        m_detail = title;
        emit stateChanged();
    } else if (type == QLatin1String("permission.replied")) {
        set(State::Thinking);
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
            if (m_state != State::Waiting && m_state != State::Working)
                set(State::Thinking, QStringLiteral("writing"));
        }
    }
}
