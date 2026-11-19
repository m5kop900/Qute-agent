#pragma once
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

class OpenCodeClient;

// F0.2: cached, refreshable lists that come from the server - never hardcoded.
//   commands (GET /api/command), models + variants (GET /api/model), agents (GET /api/agent),
//   file search (GET /api/fs/find?query=).
// OpenCode 2 only for now (the V1 endpoints differ); the lists stay empty against a V1 server.
// OpenCodeClient remains the transport: the catalog only parses the JSON it is handed.
class OpenCodeCatalog : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList commands READ commands NOTIFY commandsChanged)
    Q_PROPERTY(QVariantList slashItems READ slashItems NOTIFY commandsChanged)   // "/" list: built-ins + server (F1.4)
    Q_PROPERTY(QVariantList models READ models NOTIFY modelsChanged)
    Q_PROPERTY(QVariantList agents READ agents NOTIFY agentsChanged)
    Q_PROPERTY(QVariantList primaryAgents READ primaryAgents NOTIFY agentsChanged)
    Q_PROPERTY(QVariantList files READ files NOTIFY filesChanged)
    Q_PROPERTY(QVariantMap defaultModel READ defaultModel NOTIFY defaultModelChanged)
public:
    explicit OpenCodeCatalog(OpenCodeClient *client = nullptr, QObject *parent = nullptr);

    QVariantList commands() const { return m_commands; }
    QVariantList slashItems() const;
    QVariantList models() const { return m_models; }
    QVariantList agents() const { return m_agents; }
    QVariantList primaryAgents() const;             // mode=="primary" && !hidden - the mode chip
    QVariantList files() const { return m_files; }
    QVariantMap defaultModel() const { return m_defaultModel; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void searchFiles(const QString &query);
    // Lookups used by SessionStore/QML for display names and the context limit.
    Q_INVOKABLE QVariantMap modelByRef(const QString &providerID, const QString &modelID) const;
    Q_INVOKABLE QVariantMap agentById(const QString &id) const;

    // Pure parsers (unit-tested). Tolerant of array / {data:[...]} / {providers:[...]} shapes.
    static QVariantList parseCommands(const QJsonDocument &document);
    static QVariantList parseModels(const QJsonDocument &document);
    static QVariantList parseAgents(const QJsonDocument &document);
    static QVariantList parseFiles(const QJsonDocument &document);

public slots:
    void onEvent(const QString &type, const QJsonObject &properties);

signals:
    void commandsChanged();
    void modelsChanged();
    void agentsChanged();
    void filesChanged();
    void defaultModelChanged();

private:
    void onJson(const QString &path, const QJsonDocument &document, int status);
    void onConnectedChanged(bool connected);

    OpenCodeClient *m_client = nullptr;
    QVariantList m_commands;
    QVariantList m_models;
    QVariantList m_agents;
    QVariantList m_files;
    QVariantMap m_defaultModel;
    QString m_fileQuery;
    QTimer m_refreshTimer;   // debounce for *.updated events
};
