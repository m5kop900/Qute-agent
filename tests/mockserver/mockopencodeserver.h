#pragma once
#include <QByteArray>
#include <QHash>
#include <QJsonDocument>
#include <QList>
#include <QObject>
#include <QPair>
#include <QStringList>

class QTcpServer;
class QTcpSocket;

// Minimal in-process OpenCode server for tests (F0.4). Qt-only, no model, no real state:
//  - canned JSON responses per path (setJson)
//  - an SSE stream for /api/event (and /event) with queued + live frames
//  - a request log so a test can assert what the client actually sent
// Supports only what OpenCodeClient uses: request line, Content-Length bodies, keep-alive SSE.
class MockOpenCodeServer : public QObject {
    Q_OBJECT
public:
    explicit MockOpenCodeServer(QObject *parent = nullptr);
    ~MockOpenCodeServer() override;

    bool start(quint16 port = 0);          // 0 = pick a free port
    void stop();
    quint16 port() const;
    QString baseUrl() const;

    // Response for a path (query string ignored). Keep it JSON unless you know otherwise.
    void setJson(const QString &path, const QByteArray &body, int status = 200);
    void setJson(const QString &path, const QJsonDocument &document, int status = 200);
    // Frames sent (as `data: <frame>`) as soon as an SSE client connects.
    void setEventFrames(const QStringList &frames);
    // Push one live SSE frame to every connected event stream.
    void pushEvent(const QString &frame);

    struct Request {
        QString method;
        QString path;                      // includes the query string
        QByteArray body;
    };
    QList<Request> requests() const { return m_requests; }
    void clearRequests() { m_requests.clear(); }

private:
    struct Connection {
        QTcpSocket *socket = nullptr;
        QByteArray buffer;
        bool sse = false;
    };

    void onNewConnection();
    void onReadyRead(Connection *c);
    void onDisconnected(Connection *c);
    void handleRequest(Connection *c, const QByteArray &method, const QString &path, const QByteArray &body);
    void respondJson(Connection *c, int status, const QByteArray &body);
    void startSse(Connection *c);

    QTcpServer *m_server;
    QList<Connection *> m_connections;
    QHash<QString, QPair<int, QByteArray>> m_responses;
    QStringList m_frames;
    QList<Request> m_requests;
};
