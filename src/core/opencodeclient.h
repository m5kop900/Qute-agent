#pragma once
#include <QElapsedTimer>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QSet>
#include <QStringList>
#include <QTimer>

#include <functional>

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
    Q_INVOKABLE bool isV2Server() const { return isV2(); }

    Q_INVOKABLE void start();
    Q_INVOKABLE void sendPrompt(const QString &text);
    // Sends text plus attachments ({uri, name} entries, see AttachmentModel::promptFiles). Returns false when the
    // prompt was not sent (no session, or attachments on a V1 server); the reason is shown via requestFailed.
    // agents: names of agents mentioned with "@name" ([{name}] in the body).
    Q_INVOKABLE bool sendPromptWithFiles(const QString &text, const QVariantList &files, const QVariantList &agents);
    Q_INVOKABLE void abort();
    Q_INVOKABLE void answerPermission(const QString &permissionId, const QString &response); // once|always|reject
    Q_INVOKABLE void cancelForm(const QString &formId);   // OpenCode 2: dismiss a question the agent asked
    // OpenCode 2 selection endpoints. Both answer 204 and persist on the session; on success a synthetic
    // event is emitted so the UI updates immediately (same pattern as answerPermission).
    Q_INVOKABLE void selectAgent(const QString &agent);
    Q_INVOKABLE void selectModel(const QString &providerID, const QString &modelID, const QString &variant);
    Q_INVOKABLE void newSession();                          // always a NEW session (never resumes the last one)
    // /session: list the server's sessions (includes sessions created in the opencode TUI) and switch to one.
    Q_INVOKABLE void listSessions();
    Q_INVOKABLE void switchSession(const QString &id);
    // Slash commands defined on the server (GET /api/command): run in the current session with optional arguments.
    Q_INVOKABLE void runServerCommand(const QString &name, const QString &arguments, const QVariantList &files,
                                      const QVariantList &agents);
    // The native /compact: POST /api/session/{id}/compact (V2). Shows up as a compaction entry in the conversation.
    Q_INVOKABLE void compactSession();
    static QVariantList parseSessions(const QJsonDocument &document);
    // Ask for the projected message history (oldest first). SessionStore uses this to reconcile.
    Q_INVOKABLE void requestMessageHistory(int limit = 100);
    // Transport for the catalog service: fetch any API path and hand the parsed body back.
    void fetchJson(const QString &path);

    // pure helpers (unit-tested)
    static QString assistantTextAfterLastUser(const QJsonArray &messagesAscending);
    static QJsonArray extractArray(const QJsonDocument &doc);

signals:
    void sessionsListed(const QVariantList &sessions);
    void requestFailed(const QString &message);     // visible error for a user action (shown as a notice)
    void connectedChanged(bool connected);
    void responseTextChanged();
    void sessionChanged();
    void lastErrorChanged();
    void opencodeMissingChanged();
    void promptSent(const QString &text, const QStringList &attachmentNames);
    // Every bus event for OUR session (unwrapped to type + properties), plus synthetic "session.status idle".
    void serverEvent(const QString &type, const QJsonObject &properties);
    // Projected message list (oldest first), fetched after a turn / on (re)connect.
    void messageHistoryReceived(const QJsonArray &messages);
    // The history request for the current session failed (so a loading state can end).
    void historyFailed();
    // The session list request failed (so the picker shown while it loads can close).
    void sessionsListFailed();
    // Result of fetchJson() for the catalog service. status is the HTTP status code (0 = network error).
    void jsonFetched(const QString &path, const QJsonDocument &document, int status);

private:
    bool isV2() const { return m_api == Api::V2; }
    QNetworkRequest makeRequest(const QString &path, bool perDirectory = true) const;
    static QJsonArray toJsonFiles(const QVariantList &files);
    static QJsonArray toJsonAgents(const QVariantList &agents);
    void checkHealth();
    void heartbeat();                 // periodic check of an open connection (see start())
    bool m_heartbeatInFlight = false;
    bool m_lostConnection = false;    // the connection dropped and has not come back yet
    QTimer m_heartbeat;
    qint64 m_lastEventMs = 0;         // when the last event arrived on the stream (stall detection)
    void probeV2Info();
    void apiDecided(const QString &version);
    void notRunning();
    void authError();
    void ensureSession();
    void createSession();
    void adoptSession(const QString &id);
    void resetSessionState();
    void openEventStream();
    void onEventData();
    void handleBusEvent(const QString &type, const QJsonObject &props);
    void rebuildResponse();
    void startServerProcess();
    void setConnected(bool c);
    void setError(const QString &e);
    void setMissing(bool m);
    void postJson(const QString &path, const QJsonObject &body, const QString &what);
    void postCommand(const QString &path, const QJsonObject &body, const QString &what,
                     const std::function<void()> &onSuccess);
    void postPrompt(const QJsonObject &body, const QJsonObject &fallbackBody);
    void pollStatus();
    void turnEnded(bool synthesizeIdleEvent);
    void reconcileMessages();
    void fetchMessages(int limit, bool updateResponseText);

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
    QSet<QString> m_childSessions;                 // sub-agent sessions spawned by our session
    QHash<QString, QString> m_requestSession;      // permission/form id -> session that asked (may be a child)
};
