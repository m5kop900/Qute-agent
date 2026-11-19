#include "sessionstore.h"
#include "compactionpolicy.h"

#include <QDateTime>

#include "appconfig.h"
#include "conversationmodel.h"
#include "opencodecatalog.h"
#include "opencodeclient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QVariant>
#include <QtMath>

static QString firstString(const QJsonObject &o, std::initializer_list<const char *> keys)
{
    for (const char *k : keys) {
        const QString v = o.value(QLatin1String(k)).toString();
        if (!v.isEmpty())
            return v;
    }
    return {};
}

static QString messageRole(const QJsonObject &o)
{
    QString r = o.value("info").toObject().value("role").toString();
    if (r.isEmpty())
        r = o.value("role").toString();
    if (r.isEmpty())
        r = o.value("type").toString();
    return r;
}

// "provider/id#variant" or {providerID, id, variant}.
static bool readModelRef(const QJsonValue &value, QString &provider, QString &id, QString &variant)
{
    provider.clear();
    id.clear();
    variant.clear();
    if (value.isObject()) {
        const QJsonObject o = value.toObject();
        id = firstString(o, {"id", "modelID"});
        provider = firstString(o, {"providerID", "provider"});
        variant = firstString(o, {"variant"});
        return !id.isEmpty();
    }
    if (value.isString()) {
        QString ref = value.toString();
        const int hash = ref.indexOf(QLatin1Char('#'));
        if (hash >= 0) {
            variant = ref.mid(hash + 1);
            ref.truncate(hash);
        }
        const int slash = ref.indexOf(QLatin1Char('/'));
        if (slash >= 0) {
            provider = ref.left(slash);
            ref = ref.mid(slash + 1);
        }
        id = ref;
        return !id.isEmpty();
    }
    return false;
}

// Message-shaped object: either a nested model object or the flat providerID/modelID pair.
static bool readModelOf(const QJsonObject &o, QString &provider, QString &id, QString &variant)
{
    const QJsonValue model = o.value("model");
    if (model.isObject() || (model.isString() && model.toString().contains(QLatin1Char('/'))))
        return readModelRef(model, provider, id, variant);

    id = firstString(o, {"modelID"});
    if (id.isEmpty() && model.isString())
        id = model.toString();
    provider = firstString(o, {"providerID"});
    variant = firstString(o, {"variant"});
    return !id.isEmpty();
}

SessionStore::SessionStore(OpenCodeClient *client, AppConfig *config, OpenCodeCatalog *catalog, QObject *parent)
    : QObject(parent), m_client(client), m_config(config), m_catalog(catalog),
      m_conversation(new ConversationModel(this))
{
    m_workDir = config ? config->workDir() : QString();

    if (m_client) {
        connect(m_client, &OpenCodeClient::serverEvent, this, &SessionStore::onEvent);
        connect(m_client, &OpenCodeClient::messageHistoryReceived, this, &SessionStore::onMessages);
        connect(m_client, &OpenCodeClient::historyFailed, this, [this] { setLoadingHistory(false); });
        connect(m_client, &OpenCodeClient::requestFailed, this, [this] { m_modelPushPending = false; });
        connect(m_client, &OpenCodeClient::promptSent, this, &SessionStore::onPromptSent);
        connect(m_client, &OpenCodeClient::connectedChanged, this, &SessionStore::onConnectedChanged);
        connect(m_client, &OpenCodeClient::sessionChanged, this, &SessionStore::sessionChanged);
        connect(m_client, &OpenCodeClient::jsonFetched, this, &SessionStore::onJsonFetched);
    }
    // F5: check the automatic compaction rules whenever the context or the turn state changes.
    connect(this, &SessionStore::busyChanged, this, &SessionStore::maybeAutoCompact);
    connect(this, &SessionStore::contextChanged, this, &SessionStore::maybeAutoCompact);
    m_compactWatchdog.setSingleShot(true);
    m_compactWatchdog.setInterval(45 * 1000);
    connect(&m_compactWatchdog, &QTimer::timeout, this, [this] {
        if (!m_compactInFlight)
            return;
        m_conversation->addNotice(tr("The compaction did not finish: opencode gave no answer."));
        finishCompaction(false);
    });
    if (m_catalog) {
        connect(m_catalog, &OpenCodeCatalog::modelsChanged, this, [this] {
            recomputeContextLimit();
        });
    }
    if (m_config) {
        connect(m_config, &AppConfig::changed, this, [this] {
            if (m_workDir != m_config->workDir()) {
                m_workDir = m_config->workDir();
                emit workDirChanged();
            }
        });
    }
}

QString SessionStore::sessionId() const
{
    return m_client ? m_client->sessionId() : QString();
}

QString SessionStore::activeModel() const
{
    if (m_modelId.isEmpty())
        return {};
    QString ref = m_modelProvider.isEmpty() ? m_modelId : m_modelProvider + QLatin1Char('/') + m_modelId;
    if (!m_variant.isEmpty())
        ref += QLatin1Char('#') + m_variant;
    return ref;
}

void SessionStore::setBusy(bool busy)
{
    if (busy == m_busy)
        return;
    m_busy = busy;
    if (!busy)
        m_conversation->clearRetryNotice();       // a finished or stopped turn is not retrying any more
    emit busyChanged();
}

void SessionStore::setActiveAgent(const QString &agent)
{
    if (agent.isEmpty() || agent == m_activeAgent)
        return;
    m_activeAgent = agent;
    emit activeAgentChanged();
}

void SessionStore::setActiveModel(const QString &providerID, const QString &modelID, const QString &variant)
{
    if (modelID.isEmpty())
        return;
    if (providerID == m_modelProvider && modelID == m_modelId && variant == m_variant)
        return;
    m_modelProvider = providerID;
    m_modelId = modelID;
    m_variant = variant;
    recomputeContextLimit();
    emit activeModelChanged();
}

qint64 SessionStore::tokenTotal(const QJsonObject &tokens)
{
    if (tokens.isEmpty())
        return 0;
    const QJsonObject cache = tokens.value("cache").toObject();
    const double total = tokens.value("input").toDouble() + tokens.value("output").toDouble()
                         + tokens.value("reasoning").toDouble() + cache.value("read").toDouble()
                         + cache.value("write").toDouble();
    return qint64(total);
}

int SessionStore::percentFor(qint64 tokens, qint64 limit)
{
    if (limit <= 0)
        return -1;
    if (tokens <= 0)
        return 0;
    return int(qRound(double(tokens) * 100.0 / double(limit)));
}

void SessionStore::setContextTokens(qint64 tokens)
{
    if (tokens < 0)
        tokens = 0;
    const int percent = percentFor(tokens, m_contextLimit);
    if (tokens == m_contextTokens && percent == m_contextPercent)
        return;
    m_contextTokens = tokens;
    m_contextPercent = percent;
    emit contextChanged();
}

void SessionStore::setContextLimit(qint64 limit)
{
    if (limit < 0)
        limit = 0;
    const int percent = percentFor(m_contextTokens, limit);
    if (limit == m_contextLimit && percent == m_contextPercent)
        return;
    m_contextLimit = limit;
    m_contextPercent = percent;
    emit contextChanged();
}

void SessionStore::recomputeContextLimit()
{
    qint64 limit = 0;
    if (m_catalog && !m_modelId.isEmpty())
        limit = m_catalog->modelByRef(m_modelProvider, m_modelId).value("contextLimit").toLongLong();
    setContextLimit(limit);
}

void SessionStore::onPromptSent(const QString &text, const QStringList &attachmentNames)
{
    ++m_promptCount;
    m_compactFailures = 0;     // the user acted: a new attempt is allowed after failures
    setBusy(true);
    m_conversation->applyUserPrompt(text, attachmentNames);
}

void SessionStore::onConnectedChanged(bool connected)
{
    if (!connected)
        setBusy(false);
    // On connect the client forwards "server.connected" as a normal event; history is requested there,
    // so a stream reconnect also triggers exactly one reconcile.
}

void SessionStore::onJsonFetched(const QString &path, const QJsonDocument &document, int status)
{
    if (!m_client || status < 200 || status >= 300)
        return;
    const QString sid = m_client->sessionId();
    if (sid.isEmpty() || path != QStringLiteral("/api/session/%1").arg(sid))
        return;
    applySessionInfo(document.object());          // {data: Session.Info}
}

void SessionStore::selectAgent(const QString &agent)
{
    if (agent.isEmpty())
        return;
    setActiveAgent(agent);
    if (m_client)
        m_client->selectAgent(agent);
}

QString SessionStore::nextAgent(const QString &current, const QStringList &ids)
{
    if (ids.size() < 2)
        return QString();
    // An unknown or empty current agent starts the cycle at the first one ("build" when unset).
    const QString cur = current.isEmpty() ? QStringLiteral("build") : current;
    const int idx = ids.indexOf(cur);
    return ids.at((idx + 1) % ids.size());
}

QString SessionStore::cycleAgent()
{
    if (!m_catalog)
        return {};
    QStringList ids;
    for (const QVariant &v : m_catalog->primaryAgents())
        ids << v.toMap().value(QStringLiteral("id")).toString();
    const QString next = nextAgent(m_activeAgent, ids);
    if (!next.isEmpty())
        selectAgent(next);
    return next;
}

void SessionStore::selectModel(const QString &providerID, const QString &modelID, const QString &variant)
{
    if (modelID.isEmpty())
        return;
    if (m_client && !m_client->isV2Server()) {
        m_conversation->addNotice(tr("This opencode version keeps its own model: change it in opencode."));
        return;
    }
    setActiveModel(providerID, modelID, variant);
    // The user's choice is remembered for the next start and for other sessions.
    if (m_config) {
        m_config->setModelProvider(providerID);
        m_config->setModelId(modelID);
        m_config->save();
    }
    if (m_client)
        m_client->selectModel(providerID, modelID, variant);
}

void SessionStore::onEvent(const QString &type, const QJsonObject &p)
{
    if (type == QLatin1String("session.switched")) {
        // /new or /session: the view belongs to the previous session. Start clean, then load the new one.
        m_conversation->clear();
        setBusy(false);
        // A new session starts with a clean compaction history.
        m_compactWatchdog.stop();
        m_compactInFlight = false;
        m_serverCompacts = false;
        m_compactFailures = 0;
        m_lastCompactRequestMs = -1;
        m_promptCount = 0;
        m_promptsAtLastCompaction = -1;
        if (!m_sessionDirectory.isEmpty()) {
            m_sessionDirectory.clear();
            emit sessionDirectoryChanged();
        }
        if (!m_usage.isEmpty()) {
            m_usage.clear();
            emit usageChanged();
        }
        setContextTokens(0);
        const QString sid = p.value(QStringLiteral("sessionID")).toString();
        // The switched-to session shows its OWN model, read from the server: the chip stays empty until it is known.
        m_modelPushPending = false;
        clearActiveModel();
        if (m_client && !sid.isEmpty()) {
            setLoadingHistory(true);
            m_client->requestMessageHistory();
            if (m_client->isV2Server())
                m_client->fetchJson(QStringLiteral("/api/session/%1").arg(sid));
        }
        return;
    }
    // F5: track compaction. A "started" we did not request means the server compacts by itself.
    if (type == QLatin1String("session.compaction.started")) {
        if (!m_compactInFlight)
            m_serverCompacts = true;
        m_compactInFlight = true;
    } else if (type == QLatin1String("session.compaction.ended")) {
        finishCompaction(true);
    } else if (type == QLatin1String("session.compaction.failed")) {
        finishCompaction(false);
    }

    m_conversation->applyEvent(type, p);

    // The compaction summary is written by the server: reload the history so it shows up as its own entry.
    if (type == QLatin1String("session.compaction.ended") && m_client)
        m_client->requestMessageHistory();

    if (type == QLatin1String("server.connected")) {
        if (m_client) {
            m_client->requestMessageHistory();
            // The projected history does not reveal the session's agent/model before the first turn.
            if (m_client->isV2Server() && !m_client->sessionId().isEmpty())
                m_client->fetchJson(QStringLiteral("/api/session/%1").arg(m_client->sessionId()));
        }
        return;
    }

    if (type == QLatin1String("session.status")) {
        const QString st = p.value("status").toObject().value("type").toString();
        if (st == QLatin1String("busy") || st == QLatin1String("retry"))
            setBusy(true);
        else if (st == QLatin1String("idle"))
            setBusy(false);
    } else if (type == QLatin1String("session.execution.started") || type == QLatin1String("session.step.started")) {
        setBusy(true);
    } else if (type == QLatin1String("session.idle") || type == QLatin1String("session.execution.succeeded")
               || type == QLatin1String("session.execution.failed")
               || type == QLatin1String("session.execution.interrupted")) {
        setBusy(false);
    }

    if (type == QLatin1String("session.agent.selected")) {
        setActiveAgent(firstString(p, {"agent", "name", "id"}));
    } else if (type == QLatin1String("session.model.selected")) {
        m_modelPushPending = false;
        QString provider;
        QString id;
        QString variant;
        if (readModelRef(p.value("model"), provider, id, variant)
            || readModelOf(p, provider, id, variant))
            setActiveModel(provider, id, variant);
    } else if (type == QLatin1String("message.updated")) {
        const QJsonObject info = p.value("info").toObject();
        const QJsonObject message = info.isEmpty() ? p : info;
        setActiveAgent(firstString(message, {"agent"}));
        QString provider;
        QString id;
        QString variant;
        if (readModelOf(message, provider, id, variant))
            setActiveModel(provider, id, variant);
        const QJsonObject tokens = message.value("tokens").toObject();
        if (tokenTotal(tokens) > 0)
            setContextTokens(tokenTotal(tokens));
    } else if (type == QLatin1String("session.step.ended")) {
        const QJsonObject tokens = p.value("tokens").toObject();
        if (tokenTotal(tokens) > 0)
            setContextTokens(tokenTotal(tokens));
        QString provider;
        QString id;
        QString variant;
        if (readModelOf(p, provider, id, variant))
            setActiveModel(provider, id, variant);
    }

    if (type == QLatin1String("session.updated") || type == QLatin1String("session.usage.updated")
        || type == QLatin1String("session.usage"))
        applySessionInfo(p);
}

void SessionStore::applySessionInfo(const QJsonObject &object)
{
    QJsonObject o = object;
    if (o.value("info").isObject())
        o = o.value("info").toObject();
    if (o.value("data").isObject())
        o = o.value("data").toObject();

    // Session folder: the server's own answer, never the config folder (which may be empty).
    QString dir = o.value("directory").toString();
    if (dir.isEmpty())
        dir = o.value("location").toObject().value("directory").toString();
    if (!dir.isEmpty() && dir != m_sessionDirectory) {
        m_sessionDirectory = dir;
        emit sessionDirectoryChanged();
    }

    const QJsonObject usage = o.value("usage").toObject();
    QJsonObject tokens = o.value("tokens").toObject();
    if (tokens.isEmpty())
        tokens = usage.value("tokens").toObject();

    QVariantMap m = m_usage;
    if (!tokens.isEmpty()) {
        const double in = tokens.value("input").toDouble();
        const double out = tokens.value("output").toDouble();
        if (tokens.contains("input")) m.insert("input", in);
        if (tokens.contains("output")) m.insert("output", out);
        const double total = tokens.value("total").toDouble(in + out);
        m.insert("total", total);
    }

    if (o.contains("cost") && o.value("cost").isDouble())
        m.insert("cost", o.value("cost").toDouble());
    else if (usage.contains("cost") && usage.value("cost").isDouble())
        m.insert("cost", usage.value("cost").toDouble());

    setActiveAgent(firstString(o, {"agent"}));
    QString infoProvider;
    QString infoId;
    QString infoVariant;
    if (!m_modelPushPending && (readModelOf(o, infoProvider, infoId, infoVariant)
                                || readModelRef(o.value("model"), infoProvider, infoId, infoVariant)))
        setActiveModel(infoProvider, infoId, infoVariant);

    // Session tokens are cumulative, so they are NOT the context window usage; only agent/model are taken here.
    if (o.contains("model")) {
        QString provider;
        QString id;
        QString variant;
        if (readModelOf(o, provider, id, variant))
            setActiveModel(provider, id, variant);
    }

    if (m != m_usage) {
        m_usage = m;
        emit usageChanged();
    }
}

void SessionStore::deriveFromMessages(const QJsonArray &asc)
{
    bool haveModel = false;
    bool haveTokens = false;
    for (qsizetype i = asc.size() - 1; i >= 0; --i) {
        const QJsonObject o = asc.at(i).toObject();
        if (messageRole(o) != QLatin1String("assistant"))
            continue;
        const QJsonObject info = o.value("info").toObject();
        const QJsonObject message = info.isEmpty() ? o : info;

        if (!haveModel) {
            const QString agent = firstString(message, {"agent"});
            if (!agent.isEmpty())
                setActiveAgent(agent);
            QString provider;
            QString id;
            QString variant;
            if (readModelOf(message, provider, id, variant))
                haveModel = true;
            setActiveModel(provider, id, variant);
        }
        if (!haveTokens) {
            const qint64 total = tokenTotal(message.value("tokens").toObject());
            if (total > 0) {
                setContextTokens(total);
                haveTokens = true;
            }
        }
        if (haveModel && haveTokens)
            break;
    }
}

void SessionStore::onMessages(const QJsonArray &messagesAscending)
{
    setLoadingHistory(false);
    m_conversation->applyMessages(messagesAscending);
    deriveFromMessages(messagesAscending);
    // An empty session has no messages to read its model from: show opencode's default, which is what it uses.
    if (m_modelId.isEmpty() && m_catalog && !m_catalog->defaultModel().isEmpty()) {
        const QVariantMap def = m_catalog->defaultModel();
        setActiveModel(def.value("providerID").toString(), def.value("modelID").toString(), QString());
    }
}

void SessionStore::clearActiveModel()
{
    if (m_modelProvider.isEmpty() && m_modelId.isEmpty() && m_variant.isEmpty())
        return;
    m_modelProvider.clear();
    m_modelId.clear();
    m_variant.clear();
    recomputeContextLimit();
    emit activeModelChanged();
}

void SessionStore::setLoadingHistory(bool loading)
{
    if (m_loadingHistory == loading)
        return;
    m_loadingHistory = loading;
    emit loadingHistoryChanged();
}

void SessionStore::requestCompaction()
{
    startCompaction(false);
}

void SessionStore::startCompaction(bool automatic)
{
    if (!m_client || m_client->sessionId().isEmpty())
        return;
    m_compactInFlight = true;
    m_lastCompactRequestMs = QDateTime::currentMSecsSinceEpoch();
    m_compactWatchdog.start();
    if (automatic)
        m_conversation->addNotice(tr("Context is at %1 %%: compacting the session…").arg(m_contextPercent));
    m_client->compactSession();
}

void SessionStore::finishCompaction(bool ok)
{
    m_compactWatchdog.stop();
    m_compactInFlight = false;
    if (ok) {
        m_compactFailures = 0;
        m_promptsAtLastCompaction = m_promptCount;   // the next automatic one needs a new prompt first
    } else {
        ++m_compactFailures;
    }
}

void SessionStore::maybeAutoCompact()
{
    if (!m_config || !m_client)
        return;
    CompactionState s;
    s.enabled = m_config->autoCompactEnabled();
    s.contextPercent = m_contextPercent;
    s.thresholdPercent = m_config->autoCompactThreshold();
    s.busy = m_busy || m_loadingHistory;
    s.compactionInFlight = m_compactInFlight;
    s.serverCompactsItself = m_serverCompacts;
    s.failures = m_compactFailures;
    s.msSinceLastRequest = m_lastCompactRequestMs < 0 ? -1 : QDateTime::currentMSecsSinceEpoch() - m_lastCompactRequestMs;
    s.turnsSinceLastCompaction = m_promptsAtLastCompaction < 0 ? -1 : m_promptCount - m_promptsAtLastCompaction;
    if (CompactionPolicy::shouldCompact(s))
        startCompaction(true);
}
