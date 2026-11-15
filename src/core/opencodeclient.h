#pragma once
#include <QElapsedTimer>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QStringList>
#include <QTimer>

#include "sseparser.h"

class AppConfig;
class QNetworkReply;

// Talks to `opencode serve`. Supports BOTH server generations (see docs/OPENCODE.md):
//   V1: /global/health, /session, /session/:id/prompt_async, /event, /session/status
//   V2: /api/info,      /api/session, /api/session/:id/prompt, /api/event, /api/session/active
// The generation is detected from the server version (or forced with config apiFlavor = v1|v2).
// Events are treated as HINTS: because the SSE stream is documented to be volatile, a status poll decides when a turn
// ended and the final answer text is reconciled from the message list.
class OpenCodeClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString responseText READ responseText NOTIFY responseTextChanged)
    Q_PROPERTY(QString sessionId READ sessionId NOTIFY sessionChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(bool opencodeMissing READ opencodeMissing NOTIFY opencodeMissingChanged)
    Q_PROPERTY(QString apiName READ apiName NOTIFY connectedChanged)
public:
    enum class Api { Unknown, V1, V2 };

    explicit OpenCodeClient(AppConfig *config, QObject *parent = nullptr);
    ~OpenCodeClient() override;

    bool connected() const { return m_connected; }
    QString responseText() const { return m_responseText; }
    QString sessionId() const { return m_sessionId; }
    QString lastError() const { return m_lastError; }
    bool opencodeMissing() const { return m_opencodeMissing; }
    QString apiName() const;

    Q_INVOKABLE void start();
    Q_INVOKABLE void sendPrompt(const QString &text);
    Q_INVOKABLE void abort();
    Q_INVOKABLE void answerPermission(const QString &permissionId, const QString &response); // once|always|reject
    Q_INVOKABLE void newSession();

    // pure helpers (unit-tested)
    static QString assistantTextAfterLastUser(const QJsonArray &messagesAscending);
    static QJsonArray extractArray(const QJsonDocument &doc);

signals:
    void connectedChanged(bool connected);
    void responseTextChanged();
    void sessionChanged();
    void lastErrorChanged();
    void opencodeMissingChanged();
    void promptSent();
    // Every bus event for OUR session (unwrapped to type + properties), plus synthetic "session.status idle".
    void serverEvent(const QString &type, const QJsonObject &properties);

private:
    bool isV2() const { return m_api == Api::V2; }
    QNetworkRequest makeRequest(const QString &path, bool perDirectory = true) const;
    void checkHealth();
    void probeV2Info();
    void apiDecided(const QString &version);
    void notRunning();
    void ensureSession();
    void openEventStream();
    void onEventData();
    void handleBusEvent(const QString &type, const QJsonObject &props);
    void rebuildResponse();
    void startServerProcess();
    void setConnected(bool c);
    void setError(const QString &e);
    void setMissing(bool m);
    void postJson(const QString &path, const QJsonObject &body, const QString &what);
    void postPrompt(const QJsonObject &body, const QJsonObject &fallbackBody);
    void pollStatus();
    void turnEnded(bool synthesizeIdleEvent);
    void reconcileMessages();

    AppConfig *m_config;
    QNetworkAccessManager *m_nam;
    QPointer<QNetworkReply> m_events;
    SseParser m_parser;
    QProcess *m_server = nullptr;
    QTimer m_retry;
    QTimer m_poll;
    QElapsedTimer m_promptClock;
    QString m_base;
    QString m_sessionId;
    QString m_lastPrompt;
    QString m_responseText;
    QString m_lastError;
    Api m_api = Api::Unknown;
    bool m_connected = false;
    bool m_sessionRequested = false;
    bool m_opencodeMissing = false;
    bool m_turnActive = false;
    int m_idleMisses = 0;

    QStringList m_partOrder;
    QHash<QString, QString> m_partText;
    QHash<QString, QString> m_partMessage;
    QHash<QString, QString> m_roles;
};
