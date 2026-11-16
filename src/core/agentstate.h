#pragma once
#include <QJsonObject>
#include <QObject>
#include <QSet>
#include <QString>
#include <QTimer>

// Pure logic: turns opencode bus events into ONE sprite/UI state.
// No QML, no network, no platform code in here (keeps it unit-testable).
//
// State names (strings, shared with sprite packs and QML):
//   offline  - no connection to the opencode server
//   idle     - connected, nothing happening
//   thinking - model is busy (reasoning / writing text / waiting for the model)
//   working  - a tool is running (detail = tool name, e.g. "bash", "edit")
//   waiting  - blocked on the user: a permission request (requestKind "permission") or a question form
//              (requestKind "form"); permissionId holds the request/form id, detail a human readable text
//   done     - just finished a turn (auto-returns to idle after a short time)
//   error    - session error (auto-returns to idle)
class AgentState : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString stateName READ stateName NOTIFY stateChanged)
    Q_PROPERTY(QString detail READ detail NOTIFY stateChanged)
    Q_PROPERTY(QString permissionId READ permissionId NOTIFY stateChanged)   // id of the pending permission OR form
    Q_PROPERTY(QString requestKind READ requestKind NOTIFY stateChanged)     // "permission" | "form" | ""
public:
    enum class State { Offline, Idle, Thinking, Working, Waiting, Done, Error };

    explicit AgentState(QObject *parent = nullptr);

    State state() const { return m_state; }
    QString stateName() const { return nameOf(m_state); }
    QString detail() const { return m_detail; }
    QString permissionId() const { return m_permissionId; }
    QString requestKind() const { return m_requestKind; }
    static QString nameOf(State s);

public slots:
    void handleEvent(const QString &type, const QJsonObject &properties);
    void setConnected(bool connected);
    void promptSent();

signals:
    void stateChanged();

private:
    void set(State s, const QString &detail = {});
    void finish();
    void setWaiting(const QString &kind, const QString &id, const QString &detail);
    void handleToolEvent(const QString &type, const QJsonObject &p);

    static QString firstString(const QJsonObject &o, std::initializer_list<const char *> keys);
    static QString toolName(const QJsonObject &p);

    State m_state = State::Offline;
    QString m_detail;
    QString m_permissionId;
    QString m_requestKind;
    QSet<QString> m_tools;     // tool calls currently running (OpenCode 2 reports them one by one)
    QTimer m_settle;     // Done/Error -> Idle
};
