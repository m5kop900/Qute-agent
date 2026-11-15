#pragma once
#include <QByteArray>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <optional>

// Reads/edits/writes opencode's own config (opencode.json / opencode.jsonc).
//
// Two on-disk dialects (see docs/OPENCODE.md):
//   V1 : "mcp": { "<name>": {type, command|url, enabled, ...} },  "provider": { "<id>": {npm, options, models} }
//   V2 : "mcp": { "servers": { "<name>": {...} } },                "providers": { "<id>": {package, settings, models} }
// OpenCode 2 reads V1 files too, OpenCode 1 can NOT read native V2 -> V1 is the safe default when unsure.
// Everything we do not touch is preserved. Comments in .jsonc are lost on save (a backup is always written).
class OpencodeConfig {
public:
    enum class Format { V1, V2 };

    struct McpSpec {
        QString name;
        bool remote = false;
        QStringList command;                 // local
        QString url;                         // remote
        QMap<QString, QString> environment;  // local
        QMap<QString, QString> headers;      // remote
    };
    struct ModelSpec {
        QString id;
        QString name;
        int context = 0;
        int output = 0;
    };
    struct ProviderSpec {                    // an OpenAI-compatible endpoint
        QString id;
        QString name;
        QString baseUrl;
        QString apiKeyRef;                   // e.g. "{file:/home/u/.config/qute/secrets/x.key}" or "{env:X_KEY}"; empty = none
        QList<ModelSpec> models;
    };

    static QString globalDir();                          // ~/.config/opencode (honors XDG_CONFIG_HOME)
    static QString globalPath();                         // existing opencode.jsonc / opencode.json, else .../opencode.json
    static QString projectPath(const QString &projectDir);

    bool load(const QString &path, QString *error = nullptr);   // missing file => empty config, returns true
    bool loadFromJson(const QByteArray &jsonc, QString *error = nullptr);

    QString path() const { return m_path; }
    bool existed() const { return m_existed; }
    bool hadComments() const { return m_hadComments; }
    const QJsonObject &root() const { return m_root; }

    std::optional<Format> detectFormat() const;          // from the keys present; nullopt for an empty/neutral file

    QStringList mcpNames() const;
    bool hasMcp(const QString &name) const;
    void setMcp(const McpSpec &spec, Format f);
    bool removeMcp(const QString &name);

    QStringList providerIds() const;
    bool hasProvider(const QString &id) const;
    void setProvider(const ProviderSpec &p, Format f);

    void setDefaultModel(const QString &providerSlashModel);

    QByteArray toJson() const;
    bool save(QString *error = nullptr, QString *backupPath = nullptr);

private:
    void ensureSchema();

    QString m_path;
    QJsonObject m_root;
    bool m_existed = false;
    bool m_hadComments = false;
};
