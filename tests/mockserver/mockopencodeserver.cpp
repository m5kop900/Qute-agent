#include "mockopencodeserver.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>

MockOpenCodeServer::MockOpenCodeServer(QObject *parent)
    : QObject(parent), m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection, this, &MockOpenCodeServer::onNewConnection);

    // Defaults that make the generation handshake succeed (OpenCode 2 target).
    setJson(QStringLiteral("/global/health"), QByteArrayLiteral("{\"healthy\":true,\"version\":\"2.2.6\"}"));
    setJson(QStringLiteral("/api/info"), QByteArrayLiteral("{\"version\":\"2.2.6\"}"));
    setJson(QStringLiteral("/api/session"), QByteArrayLiteral("{\"id\":\"ses_mock\"}"));
    setJson(QStringLiteral("/api/session/active"), QByteArrayLiteral("{}"));
    setJson(QStringLiteral("/api/session/ses_mock/message"), QByteArrayLiteral("{\"data\":[],\"cursor\":{\"next\":null}}"));
}

MockOpenCodeServer::~MockOpenCodeServer()
{
    stop();
}

bool MockOpenCodeServer::start(quint16 port)
{
    return m_server->listen(QHostAddress::LocalHost, port);
}

void MockOpenCodeServer::stop()
{
    for (Connection *c : std::as_const(m_connections)) {
        c->socket->disconnect(this);
        c->socket->abort();
        c->socket->deleteLater();
        delete c;
    }
    m_connections.clear();
    m_server->close();
}

quint16 MockOpenCodeServer::port() const
{
    return m_server->serverPort();
}

QString MockOpenCodeServer::baseUrl() const
{
    return QStringLiteral("http://127.0.0.1:%1").arg(port());
}

void MockOpenCodeServer::setJson(const QString &path, const QByteArray &body, int status)
{
    m_responses.insert(path, {status, body});
}

void MockOpenCodeServer::setJson(const QString &path, const QJsonDocument &document, int status)
{
    setJson(path, document.toJson(QJsonDocument::Compact), status);
}

void MockOpenCodeServer::setEventFrames(const QStringList &frames)
{
    m_frames = frames;
}

void MockOpenCodeServer::pushEvent(const QString &frame)
{
    const QByteArray payload = "data: " + frame.toUtf8() + "\n\n";
    for (Connection *c : std::as_const(m_connections)) {
        if (c->sse)
            c->socket->write(payload);
    }
}

void MockOpenCodeServer::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *socket = m_server->nextPendingConnection();
        auto *c = new Connection;
        c->socket = socket;
        m_connections << c;
        connect(socket, &QTcpSocket::readyRead, this, [this, c] { onReadyRead(c); });
        connect(socket, &QTcpSocket::disconnected, this, [this, c] { onDisconnected(c); });
    }
}

void MockOpenCodeServer::onDisconnected(Connection *c)
{
    m_connections.removeAll(c);
    c->socket->deleteLater();
    delete c;
}

void MockOpenCodeServer::onReadyRead(Connection *c)
{
    c->buffer += c->socket->readAll();
    for (;;) {
        const int headerEnd = c->buffer.indexOf("\r\n\r\n");
        if (headerEnd < 0)
            return;
        const QByteArray head = c->buffer.left(headerEnd);
        int contentLength = 0;
        const QList<QByteArray> lines = head.split('\n');
        for (const QByteArray &raw : lines) {
            const QByteArray line = raw.trimmed();
            if (line.toLower().startsWith("content-length:"))
                contentLength = line.mid(15).trimmed().toInt();
        }
        if (c->buffer.size() < headerEnd + 4 + contentLength)
            return;                                    // body not complete yet

        const QList<QByteArray> first = lines.value(0).trimmed().split(' ');
        const QByteArray method = first.value(0);
        const QString path = QString::fromUtf8(first.value(1));
        const QByteArray body = c->buffer.mid(headerEnd + 4, contentLength);
        c->buffer.remove(0, headerEnd + 4 + contentLength);

        handleRequest(c, method, path, body);
    }
}

void MockOpenCodeServer::handleRequest(Connection *c, const QByteArray &method, const QString &path,
                                       const QByteArray &body)
{
    Request req;
    req.method = QString::fromUtf8(method);
    req.path = path;
    req.body = body;
    m_requests << req;

    const QString bare = path.left(path.indexOf(QLatin1Char('?')));
    if (bare == QLatin1String("/api/event") || bare == QLatin1String("/event")) {
        startSse(c);
        return;
    }

    const auto it = m_responses.constFind(bare);
    if (it == m_responses.constEnd()) {
        respondJson(c, 404, QByteArrayLiteral("{\"error\":\"not found\"}"));
        return;
    }
    respondJson(c, it.value().first, it.value().second);
}

void MockOpenCodeServer::respondJson(Connection *c, int status, const QByteArray &body)
{
    QByteArray head = "HTTP/1.1 " + QByteArray::number(status) + " OK\r\n";
    head += "Content-Type: application/json\r\n";
    head += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    head += "Connection: close\r\n\r\n";
    c->socket->write(head + body);
    c->socket->disconnectFromHost();
}

void MockOpenCodeServer::startSse(Connection *c)
{
    c->sse = true;
    QByteArray head = "HTTP/1.1 200 OK\r\n";
    head += "Content-Type: text/event-stream\r\n";
    head += "Cache-Control: no-cache\r\n";
    head += "Connection: keep-alive\r\n\r\n";
    c->socket->write(head);
    for (const QString &frame : std::as_const(m_frames))
        c->socket->write("data: " + frame.toUtf8() + "\n\n");
}
