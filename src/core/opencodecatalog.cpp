#include "opencodecatalog.h"

#include "opencodeclient.h"

#include <QJsonArray>
#include <QUrl>

static QString firstString(const QJsonObject &o, std::initializer_list<const char *> keys)
{
    for (const char *k : keys) {
        const QString v = o.value(QLatin1String(k)).toString();
        if (!v.isEmpty())
            return v;
    }
    return {};
}

// Pull a JSON list out of the common wrappers without assuming one exact shape.
static QJsonArray listOf(const QJsonDocument &doc, std::initializer_list<const char *> keys)
{
    if (doc.isArray())
        return doc.array();
    const QJsonObject root = doc.object();
    for (const char *k : keys)
        if (root.value(QLatin1String(k)).isArray())
            return root.value(QLatin1String(k)).toArray();
    const QJsonObject data = root.value("data").toObject();
    for (const char *k : keys)
        if (data.value(QLatin1String(k)).isArray())
            return data.value(QLatin1String(k)).toArray();
    if (root.value("data").isArray())
        return root.value("data").toArray();
    return {};
}

static QString variantId(const QJsonValue &v)
{
    return v.isString() ? v.toString() : firstString(v.toObject(), {"id", "name", "variant"});
}

static QVariantMap modelMap(const QJsonObject &m, const QString &providerDefault)
{
    QString provider = firstString(m, {"providerID", "providerId", "provider"});
    if (provider.isEmpty())
        provider = providerDefault;
    QString id = firstString(m, {"id", "modelID", "modelId", "model"});
    if (id.isEmpty())
        id = firstString(m, {"name"});
    if (id.isEmpty())
        return {};
    QString name = firstString(m, {"name"});
    if (name.isEmpty())
        name = id;

    QVariantList variants;
    for (const QJsonValue &v : m.value("variants").toArray()) {
        const QString vid = variantId(v);
        if (vid.isEmpty())
            continue;
        QString label = vid;
        label[0] = label.at(0).toUpper();
        variants << QVariantMap{{"id", vid}, {"label", label}};
    }
    const QJsonObject limit = m.value("limit").toObject();
    return QVariantMap{{"id", provider.isEmpty() ? id : provider + QLatin1Char('/') + id},
                       {"providerID", provider},
                       {"modelID", id},
                       {"name", name},
                       {"variants", variants},
                       {"contextLimit", qint64(limit.value("context").toDouble())},
                       {"outputLimit", qint64(limit.value("output").toDouble())},
                       {"enabled", m.value("enabled").toBool(true)},
                       {"type", firstString(m, {"type"})},
                       {"family", firstString(m, {"family"})}};
}

OpenCodeCatalog::OpenCodeCatalog(OpenCodeClient *client, QObject *parent)
    : QObject(parent), m_client(client)
{
    m_refreshTimer.setSingleShot(true);
    m_refreshTimer.setInterval(150);
    connect(&m_refreshTimer, &QTimer::timeout, this, &OpenCodeCatalog::refresh);

    if (m_client) {
        connect(m_client, &OpenCodeClient::jsonFetched, this, &OpenCodeCatalog::onJson);
        connect(m_client, &OpenCodeClient::connectedChanged, this, &OpenCodeCatalog::onConnectedChanged);
        connect(m_client, &OpenCodeClient::serverEvent, this, &OpenCodeCatalog::onEvent);
    }
}

void OpenCodeCatalog::onConnectedChanged(bool connected)
{
    if (connected)
        m_refreshTimer.start();      // debounced with the server.connected event
}

void OpenCodeCatalog::onEvent(const QString &type, const QJsonObject &)
{
    if (type == QLatin1String("server.connected") || type == QLatin1String("command.updated")
        || type == QLatin1String("model.updated") || type == QLatin1String("agent.updated")
        || type == QLatin1String("config.updated"))
        m_refreshTimer.start();
}

void OpenCodeCatalog::refresh()
{
    if (!m_client || !m_client->isV2Server())
        return;
    m_client->fetchJson(QStringLiteral("/api/command"));
    m_client->fetchJson(QStringLiteral("/api/model"));
    m_client->fetchJson(QStringLiteral("/api/model/default"));
    m_client->fetchJson(QStringLiteral("/api/agent"));
    if (!m_fileQuery.isEmpty())
        m_client->fetchJson(QStringLiteral("/api/fs/find?query=%1")
                                .arg(QString::fromUtf8(QUrl::toPercentEncoding(m_fileQuery))));
}

void OpenCodeCatalog::searchFiles(const QString &query)
{
    m_fileQuery = query;
    if (query.isEmpty()) {
        if (!m_files.isEmpty()) {
            m_files.clear();
            emit filesChanged();
        }
        return;
    }
    if (!m_client || !m_client->isV2Server())
        return;
    m_client->fetchJson(QStringLiteral("/api/fs/find?query=%1").arg(QString::fromUtf8(QUrl::toPercentEncoding(query))));
}

void OpenCodeCatalog::onJson(const QString &path, const QJsonDocument &document, int status)
{
    if (status < 200 || status >= 300)
        return;
    const QString bare = path.left(path.indexOf(QLatin1Char('?')));
    if (bare == QLatin1String("/api/command")) {
        m_commands = parseCommands(document);
        emit commandsChanged();
    } else if (bare == QLatin1String("/api/model")) {
        m_models = parseModels(document);
        emit modelsChanged();
    } else if (bare == QLatin1String("/api/model/default")) {
        const QVariantList list = parseModels(document);
        const QVariantMap model = list.isEmpty() ? QVariantMap() : list.first().toMap();
        if (model != m_defaultModel) {
            m_defaultModel = model;
            emit defaultModelChanged();
        }
    } else if (bare == QLatin1String("/api/agent")) {
        m_agents = parseAgents(document);
        emit agentsChanged();
    } else if (bare == QLatin1String("/api/fs/find")) {
        m_files = parseFiles(document);
        emit filesChanged();
    }
}

QVariantList OpenCodeCatalog::parseCommands(const QJsonDocument &doc)
{
    QVariantList out;
    const QJsonArray arr = listOf(doc, {"commands", "data", "items"});
    for (const QJsonValue &v : arr) {
        if (v.isString()) {
            out << QVariantMap{{"name", v.toString()}, {"description", QString()}};
            continue;
        }
        const QJsonObject o = v.toObject();
        const QString name = firstString(o, {"name", "id", "command"});
        if (name.isEmpty())
            continue;
        out << QVariantMap{{"name", name},
                           {"description", firstString(o, {"description", "detail"})},
                           {"agent", firstString(o, {"agent"})}};
    }
    if (!out.isEmpty())
        return out;

    // Some builds return a map: {"init": {"description": ...}}
    const QJsonObject root = doc.object();
    for (auto it = root.begin(); it != root.end(); ++it) {
        if (!it.value().isObject() || it.key() == QLatin1String("data"))
            continue;
        out << QVariantMap{{"name", it.key()},
                           {"description", firstString(it.value().toObject(), {"description", "detail"})}};
    }
    return out;
}

QVariantList OpenCodeCatalog::parseModels(const QJsonDocument &doc)
{
    QVariantList out;
    auto addModel = [&out](const QJsonObject &m, const QString &providerDefault) {
        const QVariantMap model = modelMap(m, providerDefault);
        if (!model.isEmpty())
            out << model;
    };
    auto addProvider = [&addModel](const QJsonObject &p) {
        const QString pid = firstString(p, {"id", "providerID", "provider"});
        for (const QJsonValue &m : p.value("models").toArray())
            addModel(m.toObject(), pid);
    };

    const QJsonObject root = doc.object();
    QJsonArray providers;
    if (root.value("providers").isArray())
        providers = root.value("providers").toArray();
    else if (root.value("data").toObject().value("providers").isArray())
        providers = root.value("data").toObject().value("providers").toArray();
    if (!providers.isEmpty()) {
        for (const QJsonValue &p : providers)
            addProvider(p.toObject());
        return out;
    }
    if (root.value("models").isArray()
        && (root.contains("id") || root.contains("providerID") || root.contains("provider"))) {
        addProvider(root);
        return out;
    }

    // /api/model/default returns one Model.Info inside "data" instead of a list.
    const QJsonObject single = root.value("data").toObject();
    if (!single.isEmpty() && !firstString(single, {"id", "modelID"}).isEmpty()) {
        addModel(single, {});
        return out;
    }
    if (!firstString(root, {"id", "modelID"}).isEmpty()) {
        addModel(root, {});
        return out;
    }

    for (const QJsonValue &v : listOf(doc, {"models", "data", "items"})) {
        const QJsonObject o = v.toObject();
        if (o.value("models").isArray())
            addProvider(o);
        else
            addModel(o, {});
    }
    return out;
}

QVariantList OpenCodeCatalog::parseAgents(const QJsonDocument &doc)
{
    QVariantList out;
    for (const QJsonValue &v : listOf(doc, {"agents", "data", "items"})) {
        const QJsonObject o = v.toObject();
        const QString id = firstString(o, {"id", "agent", "name"});
        if (id.isEmpty())
            continue;
        QString name = firstString(o, {"name"});
        if (name.isEmpty())
            name = id;
        out << QVariantMap{{"id", id},
                           {"name", name},
                           {"description", firstString(o, {"description", "detail"})},
                           {"mode", firstString(o, {"mode"})},
                           {"hidden", o.value("hidden").toBool(false)}};
    }
    return out;
}

QVariantList OpenCodeCatalog::primaryAgents() const
{
    QVariantList out;
    for (const QVariant &v : m_agents) {
        const QVariantMap agent = v.toMap();
        if (agent.value("hidden").toBool())
            continue;
        if (agent.value("mode").toString() != QLatin1String("primary"))
            continue;
        out << agent;
    }
    return out;
}

QVariantMap OpenCodeCatalog::modelByRef(const QString &providerID, const QString &modelID) const
{
    if (modelID.isEmpty())
        return {};
    for (const QVariant &v : m_models) {
        const QVariantMap model = v.toMap();
        if (model.value("modelID").toString() != modelID)
            continue;
        if (!providerID.isEmpty() && model.value("providerID").toString() != providerID)
            continue;
        return model;
    }
    return {};
}

QVariantMap OpenCodeCatalog::agentById(const QString &id) const
{
    for (const QVariant &v : m_agents) {
        const QVariantMap agent = v.toMap();
        if (agent.value("id").toString() == id)
            return agent;
    }
    return {};
}

QVariantList OpenCodeCatalog::parseFiles(const QJsonDocument &doc)
{
    QVariantList out;
    for (const QJsonValue &v : listOf(doc, {"files", "data", "results", "items"})) {
        QString path;
        QString name;
        if (v.isString()) {
            path = v.toString();
        } else {
            const QJsonObject o = v.toObject();
            path = firstString(o, {"path", "file", "absolute", "filename"});
            name = firstString(o, {"name"});
        }
        if (path.isEmpty())
            continue;
        if (name.isEmpty())
            name = path.mid(path.lastIndexOf(QLatin1Char('/')) + 1);
        out << QVariantMap{{"path", path}, {"name", name}};
    }
    return out;
}
