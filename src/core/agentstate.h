#pragma once
#include <QJsonObject>
#include <QObject>
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
//   waiting  - blocked on a permission request (permissionId + detail are set)
//   done     - just finished a turn (auto-returns to idle after a short time)
//   error    - session error (auto-returns to idle)
class AgentState : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString stateName READ stateName NOTIFY stateChanged)
    Q_PROPERTY(QString detail READ detail NOTIFY stateChanged)
    Q_PROPERTY(QString permissionId READ permissionId NOTIFY stateChanged)
public:
    enum class State { Offline, Idle, Thinking, Working, Waiting, Done, Error };

    explicit AgentState(QObject *parent = nullptr);

    State state() const { return m_state; }
    QString stateName() const { return nameOf(m_state); }
    QString detail() const { return m_detail; }
    QString permissionId() const { return m_permissionId; }
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

    State m_state = State::Offline;
    QString m_detail;
    QString m_permissionId;
    QTimer m_settle;     // Done/Error -> Idle
};
