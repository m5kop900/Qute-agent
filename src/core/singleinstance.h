#pragma once
#include <QLocalServer>
#include <QObject>

// Tiny cross-platform "talk to the running instance" channel (QLocalServer / QLocalSocket).
// Used so that `qute toggle` (bound to a desktop shortcut) can reach the daemon.
class SingleInstance : public QObject {
    Q_OBJECT
public:
    explicit SingleInstance(const QString &name, QObject *parent = nullptr);

    // Start listening. Call only after send() failed (= no instance is running).
    bool listen();

    // Send a command to a running instance. Returns false if none answered.
    static bool send(const QString &name, const QString &command, int timeoutMs = 400);

signals:
    void commandReceived(const QString &command);

private:
    QString m_name;
    QLocalServer m_server;
};
