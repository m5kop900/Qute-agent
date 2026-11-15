#include "singleinstance.h"

#include <QLocalSocket>

SingleInstance::SingleInstance(const QString &name, QObject *parent)
    : QObject(parent), m_name(name)
{
    connect(&m_server, &QLocalServer::newConnection, this, [this] {
        while (QLocalSocket *sock = m_server.nextPendingConnection()) {
            connect(sock, &QLocalSocket::readyRead, this, [this, sock] {
                while (sock->canReadLine()) {
                    const QString cmd = QString::fromUtf8(sock->readLine()).trimmed();
                    if (!cmd.isEmpty() && cmd != QLatin1String("ping"))
                        emit commandReceived(cmd);
                    sock->write("ok\n");
                    sock->flush();
                }
            });
            connect(sock, &QLocalSocket::disconnected, sock, &QObject::deleteLater);
        }
    });
}

bool SingleInstance::listen()
{
    QLocalServer::removeServer(m_name);   // clean a stale socket file
    m_server.setSocketOptions(QLocalServer::UserAccessOption);
    return m_server.listen(m_name);
}

bool SingleInstance::send(const QString &name, const QString &command, int timeoutMs)
{
    QLocalSocket sock;
    sock.connectToServer(name);
    if (!sock.waitForConnected(timeoutMs))
        return false;
    sock.write(command.toUtf8() + '\n');
    sock.flush();
    sock.waitForBytesWritten(timeoutMs);
    sock.waitForReadyRead(timeoutMs);     // wait for "ok" ack (best effort)
    sock.disconnectFromServer();
    return true;
}
