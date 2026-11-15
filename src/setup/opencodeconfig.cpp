#include "opencodeconfig.h"

#include "jsonc.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>

static const char kSchema[] = "https://opencode.ai/config.json";

QString OpencodeConfig::globalDir()
{
    const QString xdg = qEnvironmentVariable("XDG_CONFIG_HOME");
    // opencode uses ~/.config/opencode on every OS (also Windows), not %APPDATA%.
    return (xdg.isEmpty() ? QDir::homePath() + QStringLiteral("/.config") : xdg) + QStringLiteral("/opencode");
}

QString OpencodeConfig::globalPath()
{
    const QString dir = globalDir();
    for (const char *name : {"opencode.jsonc", "opencode.json"})
        if (QFileInfo::exists(dir + QLatin1Char('/') + QLatin1String(name)))
            return dir + QLatin1Char('/') + QLatin1String(name);
    return dir + QStringLiteral("/opencode.json");
}

QString OpencodeConfig::projectPath(const QString &projectDir)
{
    for (const char *name : {"opencode.jsonc", "opencode.json"})
        if (QFileInfo::exists(projectDir + QLatin1Char('/') + QLatin1String(name)))
            return projectDir + QLatin1Char('/') + QLatin1String(name);
    return projectDir + QStringLiteral("/opencode.json");
}

bool OpencodeConfig::loadFromJson(const QByteArray &jsonc, QString *error)
{
    const QByteArray strict = Jsonc::strip(jsonc, &m_hadComments);
    if (strict.trimmed().isEmpty()) {
        m_root = {};
        return true;
    }
    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(strict, &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error)
            *error = QStringLiteral("not valid JSON/JSONC (%1 at offset %2)").arg(pe.errorString()).arg(pe.offset);
        return false;
    }
    m_root = doc.object();
    return true;
}

bool OpencodeConfig::load(const QString &path, QString *error)
{
    m_path = path;
    m_root = {};
    m_hadComments = false;
    m_existed = QFileInfo::exists(path);
    if (!m_existed)
        return true;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QStringLiteral("cannot read %1").arg(path);
        return false;
    }
    return loadFromJson(f.readAll(), error);
}

std::optional<OpencodeConfig::Format> OpencodeConfig::detectFormat() const
{
    const QJsonObject mcp = m_root.value("mcp").toObject();
    bool v2 = m_root.contains("providers") || mcp.contains("servers") || m_root.contains("agents")
              || m_root.value("permissions").isArray() || m_root.contains("commands");
    bool v1 = m_root.contains("provider") || m_root.contains("agent") || m_root.contains("permission")
              || m_root.contains("command");
    for (auto it = mcp.begin(); it != mcp.end(); ++it)
        if (it.key() != QLatin1String("servers") && it.key() != QLatin1String("timeout") && it.value().isObject())
            v1 = true;
    if (v1)
        return Format::V1;          // mixed files: V1 syntax is valid for both versions
    if (v2)
        return Format::V2;
    return std::nullopt;
}

QStringList OpencodeConfig::mcpNames() const
{
    QStringList names;
    const QJsonObject mcp = m_root.value("mcp").toObject();
    for (auto it = mcp.begin(); it != mcp.end(); ++it)
        if (it.key() != QLatin1String("servers") && it.key() != QLatin1String("timeout") && it.value().isObject())
            names << it.key();
    const QJsonObject servers = mcp.value("servers").toObject();
    for (auto it = servers.begin(); it != servers.end(); ++it)
        if (!names.contains(it.key()))
            names << it.key();
    return names;
}

bool OpencodeConfig::hasMcp(const QString &name) const { return mcpNames().contains(name); }

void OpencodeConfig::ensureSchema()
{
    if (!m_root.contains("$schema"))
        m_root.insert(QStringLiteral("$schema"), QLatin1String(kSchema));
}

static QJsonObject mapToObject(const QMap<QString, QString> &m)
{
    QJsonObject o;
    for (auto it = m.begin(); it != m.end(); ++it)
        o.insert(it.key(), it.value());
    return o;
}

void OpencodeConfig::setMcp(const McpSpec &s, Format f)
{
    QJsonObject entry;
    if (s.remote) {
        entry.insert("type", "remote");
        entry.insert("url", s.url);
        if (!s.headers.isEmpty())
            entry.insert("headers", mapToObject(s.headers));
    } else {
        entry.insert("type", "local");
        entry.insert("command", QJsonArray::fromStringList(s.command));
        if (!s.environment.isEmpty())
            entry.insert("environment", mapToObject(s.environment));
    }
    if (f == Format::V1)
        entry.insert("enabled", true);          // V2 has no "enabled" (servers connect unless disabled:true)

    removeMcp(s.name);                           // never leave the same server in both dialects
    QJsonObject mcp = m_root.value("mcp").toObject();
    if (f == Format::V1) {
        mcp.insert(s.name, entry);
    } else {
        QJsonObject servers = mcp.value("servers").toObject();
        servers.insert(s.name, entry);
        mcp.insert("servers", servers);
    }
    m_root.insert("mcp", mcp);
    ensureSchema();
}

bool OpencodeConfig::removeMcp(const QString &name)
{
    bool removed = false;
    QJsonObject mcp = m_root.value("mcp").toObject();
    if (name != QLatin1String("servers") && name != QLatin1String("timeout") && mcp.contains(name)) {
        mcp.remove(name);
        removed = true;
    }
    QJsonObject servers = mcp.value("servers").toObject();
    if (servers.contains(name)) {
        servers.remove(name);
        mcp.insert("servers", servers);
        removed = true;
    }
    if (removed)
        m_root.insert("mcp", mcp);
    return removed;
}

QStringList OpencodeConfig::providerIds() const
{
    QStringList ids = m_root.value("provider").toObject().keys();
    for (const QString &k : m_root.value("providers").toObject().keys())
        if (!ids.contains(k))
            ids << k;
    return ids;
}

bool OpencodeConfig::hasProvider(const QString &id) const { return providerIds().contains(id); }

void OpencodeConfig::setProvider(const ProviderSpec &p, Format f)
{
    QJsonObject models;
    for (const ModelSpec &m : p.models) {
        QJsonObject mo;
        mo.insert("name", m.name.isEmpty() ? m.id : m.name);
        if (m.context > 0 || m.output > 0) {
            QJsonObject limit;
            if (m.context > 0) limit.insert("context", m.context);
            if (m.output > 0) limit.insert("output", m.output);
            mo.insert("limit", limit);
        }
        models.insert(m.id, mo);
    }

    QJsonObject entry;
    entry.insert("name", p.name.isEmpty() ? p.id : p.name);
    entry.insert("models", models);
    if (f == Format::V1) {
        entry.insert("npm", "@ai-sdk/openai-compatible");
        QJsonObject options;
        options.insert("baseURL", p.baseUrl);
        if (!p.apiKeyRef.isEmpty())
            options.insert("apiKey", p.apiKeyRef);
        entry.insert("options", options);
    } else {
        entry.insert("package", "aisdk:@ai-sdk/openai-compatible");
        QJsonObject settings;
        settings.insert("baseURL", p.baseUrl);
        if (!p.apiKeyRef.isEmpty())
            settings.insert("apiKey", p.apiKeyRef);
        entry.insert("settings", settings);
    }

    const QString key = (f == Format::V1) ? QStringLiteral("provider") : QStringLiteral("providers");
    const QString other = (f == Format::V1) ? QStringLiteral("providers") : QStringLiteral("provider");
    QJsonObject otherMap = m_root.value(other).toObject();
    if (otherMap.contains(p.id)) {               // same id in the other dialect would conflict
        otherMap.remove(p.id);
        m_root.insert(other, otherMap);
    }
    QJsonObject map = m_root.value(key).toObject();
    map.insert(p.id, entry);
    m_root.insert(key, map);
    ensureSchema();
}

void OpencodeConfig::setDefaultModel(const QString &ref)
{
    m_root.insert("model", ref);
    ensureSchema();
}

QByteArray OpencodeConfig::toJson() const
{
    return QJsonDocument(m_root).toJson(QJsonDocument::Indented);
}

bool OpencodeConfig::save(QString *error, QString *backupPath)
{
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    if (m_existed) {
        const QString bak = m_path + QStringLiteral(".qute-bak-") + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
        if (!QFile::copy(m_path, bak)) {
            if (error) *error = QStringLiteral("could not create backup %1").arg(bak);
            return false;
        }
        if (backupPath) *backupPath = bak;
    }
    QSaveFile f(m_path);
    if (!f.open(QIODevice::WriteOnly)) {
        if (error) *error = QStringLiteral("cannot write %1").arg(m_path);
        return false;
    }
    f.write(toJson());
    if (!f.commit()) {
        if (error) *error = QStringLiteral("commit failed for %1").arg(m_path);
        return false;
    }
    m_existed = true;
    return true;
}
