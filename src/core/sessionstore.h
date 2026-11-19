#pragma once
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVariantMap>

class AppConfig;
class ConversationModel;
class OpenCodeCatalog;
class QJsonArray;
class QJsonDocument;
class OpenCodeClient;

// F0.1: session-level state on top of the transport. It owns the ConversationModel, tracks the turn
// (busy), the session id / working directory, the active agent & model and token usage.
// OpenCodeClient stays the transport: SessionStore only consumes its events + fetched message list.
// Everything here is deliberately independent of the sprite state (AgentState) and of QML.
class SessionStore : public QObject {
    Q_OBJECT
    Q_PROPERTY(ConversationModel *conversation READ conversation CONSTANT)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool loadingHistory READ loadingHistory NOTIFY loadingHistoryChanged)   // a session is being loaded
    Q_PROPERTY(QString sessionId READ sessionId NOTIFY sessionChanged)
    Q_PROPERTY(QString workDir READ workDir NOTIFY workDirChanged)
    // The folder the CURRENT session runs in, as the server reports it (may differ from the config folder).
    Q_PROPERTY(QString sessionDirectory READ sessionDirectory NOTIFY sessionDirectoryChanged)
    Q_PROPERTY(QString activeAgent READ activeAgent NOTIFY activeAgentChanged)
    // Display string "provider/id#variant" plus the parts the composer needs to switch model/variant.
    Q_PROPERTY(QString activeModel READ activeModel NOTIFY activeModelChanged)
    Q_PROPERTY(QString activeModelProvider READ activeModelProvider NOTIFY activeModelChanged)
    Q_PROPERTY(QString activeModelId READ activeModelId NOTIFY activeModelChanged)
    Q_PROPERTY(QString activeVariant READ activeVariant NOTIFY activeModelChanged)
    Q_PROPERTY(QVariantMap usage READ usage NOTIFY usageChanged)
    // Context window usage of the last assistant message, matching opencode's own calculation.
    Q_PROPERTY(qint64 contextTokens READ contextTokens NOTIFY contextChanged)
    Q_PROPERTY(qint64 contextLimit READ contextLimit NOTIFY contextChanged)
    Q_PROPERTY(int contextPercent READ contextPercent NOTIFY contextChanged)   // -1 = unknown
public:
    explicit SessionStore(OpenCodeClient *client = nullptr, AppConfig *config = nullptr,
                          OpenCodeCatalog *catalog = nullptr, QObject *parent = nullptr);

    ConversationModel *conversation() const { return m_conversation; }
    bool busy() const { return m_busy; }
    bool loadingHistory() const { return m_loadingHistory; }
    QString sessionId() const;
    QString workDir() const { return m_workDir; }
    QString sessionDirectory() const { return m_sessionDirectory; }
    // Pure helper (unit-tested): the id after `current` in `ids`, wrapping; empty when there is nothing to switch to.
    static QString nextAgent(const QString &current, const QStringList &ids);

    QString activeAgent() const { return m_activeAgent; }
    QString activeModel() const;
    QString activeModelProvider() const { return m_modelProvider; }
    QString activeModelId() const { return m_modelId; }
    QString activeVariant() const { return m_variant; }
    QVariantMap usage() const { return m_usage; }
    qint64 contextTokens() const { return m_contextTokens; }
    qint64 contextLimit() const { return m_contextLimit; }
    int contextPercent() const { return m_contextPercent; }

    // Tolerant extraction from session info / usage events (V2 shapes are still partly unverified).
    void applySessionInfo(const QJsonObject &object);

    // Pure helpers (unit-tested): sum of an assistant message's token block, and its share of the limit.
    static qint64 tokenTotal(const QJsonObject &tokens);
    static int percentFor(qint64 tokens, qint64 limit);

public slots:
    void onEvent(const QString &type, const QJsonObject &properties);
    void onMessages(const QJsonArray &messagesAscending);
    void onPromptSent(const QString &text, const QStringList &attachmentNames = {});
    void onConnectedChanged(bool connected);
    // Selections go through the server; the local state is updated optimistically so the chip reacts at once.
    // Q_INVOKABLE: the QML (menu strip, Shift+Tab, /agents, /models) calls these. Without it the call fails in
    // QML silently, which is why Shift+Tab did nothing.
    Q_INVOKABLE void selectAgent(const QString &agent);
    // Shift+Tab: switch to the next primary agent (mode) the server offers. Returns the agent chosen, or "" when
    // there is nothing to switch to.
    Q_INVOKABLE QString cycleAgent();
    Q_INVOKABLE void selectModel(const QString &providerID, const QString &modelID, const QString &variant);

signals:
    void busyChanged();
    void loadingHistoryChanged();
    void sessionChanged();
    void workDirChanged();
    void sessionDirectoryChanged();
    void activeAgentChanged();
    void activeModelChanged();
    void usageChanged();
    void contextChanged();

private:
    void setBusy(bool busy);
    void setLoadingHistory(bool loading);
    bool m_loadingHistory = false;
    void setActiveAgent(const QString &agent);
    void setActiveModel(const QString &providerID, const QString &modelID, const QString &variant);
    void setContextTokens(qint64 tokens);
    void setContextLimit(qint64 limit);
    void recomputeContextLimit();
    void adoptDefaultModel();
    void onJsonFetched(const QString &path, const QJsonDocument &document, int status);
    void deriveFromMessages(const QJsonArray &messagesAscending);

    OpenCodeClient *m_client = nullptr;
    AppConfig *m_config = nullptr;
    OpenCodeCatalog *m_catalog = nullptr;
    ConversationModel *m_conversation;
    bool m_busy = false;
    QString m_workDir;
    QString m_sessionDirectory;
    QString m_activeAgent;
    QString m_modelProvider;
    QString m_modelId;
    QString m_variant;
    qint64 m_contextTokens = 0;
    qint64 m_contextLimit = 0;
    int m_contextPercent = -1;
    QVariantMap m_usage;
};
