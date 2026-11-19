#include "opencodeclient.h"

#include "appconfig.h"
#include "opencodelocator.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <algorithm>
#include <QStandardPaths>
#include <QUrl>
#include <QUrlQuery>

OpenCodeClient::OpenCodeClient(AppConfig *config, QObject *parent)
    : QObject(parent), m_config(config), m_nam(new QNetworkAccessManager(this))
{
    m_retry.setSingleShot(true);
    connect(&m_retry, &QTimer::timeout, this, &OpenCodeClient::checkHealth);
    m_poll.setInterval(1500);
    connect(&m_poll, &QTimer::timeout, this, &OpenCodeClient::pollStatus);
}

OpenCodeClient::~OpenCodeClient()
{
    if (m_events) {
        m_events->disconnect(this);
        m_events->abort();
    }
    if (m_server) {                         // only set when WE started the server
        m_server->disconnect(this);         // the finished handler must not run during teardown
        m_server->terminate();
        m_server->waitForFinished(1500);
    }
}

QString OpenCodeClient::apiName() const
{
    return m_api == Api::V2 ? QStringLiteral("v2") : (m_api == Api::V1 ? QStringLiteral("v1") : QString());
}

QNetworkRequest OpenCodeClient::makeRequest(const QString &path, bool perDirectory) const
{
    QUrl url(m_base + path);
    if (perDirectory && !m_config->workDir().isEmpty()) {
        QUrlQuery q(url);
        if (!isV2()) {                                                    // V1 selects the project with ?directory=
            q.addQueryItem(QStringLiteral("directory"), m_config->workDir());
        } else if (path.startsWith(QLatin1String("/api/")) && !path.startsWith(QLatin1String("/api/session"))) {
            // V2 scopes project-level lists by location. Without it /api/command returns an empty list (verified).
            q.addQueryItem(QStringLiteral("location[directory]"), m_config->workDir());
        }
        url.setQuery(q);
    }
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    if (!m_config->serverPassword().isEmpty()) {
        // The username is "opencode" on OpenCode 2 (OPENCODE_SERVER_USERNAME was removed). It is only taken from
        // settings when the V1 flavor is forced: before the API version is known it must not be sent, or the
        // very first health check fails with a correct password.
        const QString user = (m_config->apiFlavor() == QLatin1String("v1")) ? m_config->serverUsername()
                                                                              : QStringLiteral("opencode");
        req.setRawHeader("Authorization", "Basic " + (user + QLatin1Char(':') + m_config->serverPassword()).toUtf8().toBase64());
    }
    return req;
}

void OpenCodeClient::start()
{
    m_base = m_config->serverUrl();
    while (m_base.endsWith(QLatin1Char('/')))
        m_base.chop(1);
    checkHealth();
}

// ---- API generation detection -------------------------------------------------------------------------
static bool authFailed(QNetworkReply *r)
{
    const int st = r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    return st == 401 || st == 403;
}

void OpenCodeClient::checkHealth()
{
    QNetworkReply *r = m_nam->get(makeRequest(QStringLiteral("/global/health"), false));
    connect(r, &QNetworkReply::finished, this, [this, r] {
        r->deleteLater();
        if (r->error() == QNetworkReply::NoError) {
            apiDecided(QJsonDocument::fromJson(r->readAll()).object().value("version").toString());
            return;
        }
        if (authFailed(r)) {
            authError();
            return;
        }
        probeV2Info();                                   // maybe a V2-only server
    });
}

void OpenCodeClient::probeV2Info()
{
    QNetworkReply *r = m_nam->get(makeRequest(QStringLiteral("/api/info"), false));
    connect(r, &QNetworkReply::finished, this, [this, r] {
        r->deleteLater();
        if (authFailed(r)) {
            authError();
            return;
        }
        if (r->error() == QNetworkReply::NoError) {
            const QString forced = m_config->apiFlavor();
            m_api = (forced == QLatin1String("v1")) ? Api::V1 : Api::V2;
            ensureSession();
            return;
        }
        notRunning();
    });
}

void OpenCodeClient::apiDecided(const QString &version)
{
    const QString forced = m_config->apiFlavor();
    if (forced == QLatin1String("v1")) { m_api = Api::V1; ensureSession(); return; }
    if (forced == QLatin1String("v2")) { m_api = Api::V2; ensureSession(); return; }
    if (!version.isEmpty()) {
        m_api = (OpencodeLocator::parseMajor(version) >= 2) ? Api::V2 : Api::V1;
        ensureSession();
        return;
    }
    // health answered without a version: ask the V2 info route to decide
    QNetworkReply *r = m_nam->get(makeRequest(QStringLiteral("/api/info"), false));
    connect(r, &QNetworkReply::finished, this, [this, r] {
        r->deleteLater();
        if (authFailed(r)) {
            authError();
            return;
        }
        m_api = (r->error() == QNetworkReply::NoError) ? Api::V2 : Api::V1;
        ensureSession();
    });
}

// A wrong/absent password is a user-fixable configuration problem: say so instead of retrying silently.
void OpenCodeClient::authError()
{
    setConnected(false);
    if (m_config->serverPassword().isEmpty())
        setError(tr("This opencode server requires a password. Enter it in qute settings (Server password)."));
    else if (m_server)
        setError(tr("The server started by qute rejected the configured password. Check the server password in settings."));
    else
        setError(tr("opencode rejected the password. The server on this address uses a different password, "
                    "or another opencode instance is running on it. Fix the password in settings, or stop the other server."));
    m_retry.start(4000);
}

void OpenCodeClient::notRunning()
{
    setConnected(false);

    // "Connecting…" forever is the worst outcome: say WHY there is no server and what to do about it.
    const QUrl u(m_base);
    const QString host = u.host();
    const bool local = host.isEmpty() || host == QLatin1String("127.0.0.1") || host == QLatin1String("localhost");
    if (!local) {
        setError(tr("opencode is not reachable at %1. Check the server URL and password in settings.").arg(m_base));
        m_retry.start(4000);
        return;
    }
    if (!m_config->autoStartServer()) {
        const QString program = m_config->serverCommand().isEmpty() ? QStringLiteral("opencode")
                                                                    : m_config->serverCommand();
        setError(tr("opencode is not running at %1. Enable \"Auto-start server\" in settings, or start it:  %2 serve --hostname %3 --port %4")
                     .arg(m_base, program, host, QString::number(u.port(4096))));
        m_retry.start(4000);
        return;
    }
    if (!m_server && !m_opencodeMissing)
        startServerProcess();
    m_retry.start(m_server ? 1000 : 4000);           // shorter retry while the server we spawned is booting
}

void OpenCodeClient::startServerProcess()
{
    const QUrl u(m_base);
    const QString host = u.host();
    if (host != QLatin1String("127.0.0.1") && host != QLatin1String("localhost"))
        return;                              // never spawn a server for a remote URL

    QString program = m_config->serverCommand();
    if (QStandardPaths::findExecutable(program).isEmpty()) {     // not in PATH (or a wrong path): search the usual places
        const OpencodeBinary b = OpencodeLocator::best();
        if (b.command.isEmpty()) {
            setMissing(true);
            setError(tr("opencode is not installed. Run:  qute opencode install"));
            return;
        }
        program = b.path;
    }

    m_server = new QProcess(this);
    if (!m_config->workDir().isEmpty())
        m_server->setWorkingDirectory(m_config->workDir());
    m_server->setProcessChannelMode(QProcess::ForwardedChannels);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!m_config->serverPassword().isEmpty()) {
        env.insert(QStringLiteral("OPENCODE_SERVER_PASSWORD"), m_config->serverPassword());   // V1 and V2
        env.insert(QStringLiteral("OPENCODE_PASSWORD"), m_config->serverPassword());          // V2
    }
    m_server->setProcessEnvironment(env);
    connect(m_server, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart) {
            setMissing(true);
            setError(tr("Could not start opencode. Run:  qute opencode doctor"));
        }
        if (m_server) {
            m_server->deleteLater();
            m_server = nullptr;
        }
    });
    // A spawned server that exits (bad port, crash, config error) must not leave us retrying silently forever.
    connect(m_server, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
        if (m_server) {
            m_server->deleteLater();
            m_server = nullptr;
        }
        if (m_connected)
            return;
        setError(status == QProcess::CrashExit
                     ? tr("opencode server crashed. Run:  qute opencode doctor")
                     : tr("opencode server exited (code %1). Run:  qute opencode doctor").arg(code));
        m_retry.start(4000);
    });
    m_server->start(program, {QStringLiteral("serve"), QStringLiteral("--hostname"), host,
                              QStringLiteral("--port"), QString::number(u.port(4096))});
}

// ---- session + events -------------------------------------------------------------------------------------
void OpenCodeClient::ensureSession()
{
    if (!m_sessionId.isEmpty()) {
        openEventStream();
        return;
    }
    if (m_sessionRequested)
        return;
    m_sessionRequested = true;

    // F4.2: resume the session of the last run when the server still has it, so a restart or a
    // /openinopencode round trip keeps the same conversation. Only "not found" creates a new session;
    // a network error must not silently fork the conversation.
    const QString last = m_config->lastSessionId();
    if (isV2() && !last.isEmpty()) {
        QNetworkReply *r = m_nam->get(makeRequest(QStringLiteral("/api/session/%1").arg(last)));
        connect(r, &QNetworkReply::finished, this, [this, r, last] {
            r->deleteLater();
            const int status = r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (r->error() == QNetworkReply::NoError && status == 200) {
                m_sessionRequested = false;
                adoptSession(last);
            } else if (status == 404) {
                m_sessionRequested = false;
                createSession();
            } else {
                m_sessionRequested = false;
                setError(tr("Could not resume session %1: %2").arg(last, r->errorString()));
                m_retry.start(4000);
            }
        });
        return;
    }
    createSession();
}

void OpenCodeClient::createSession()
{
    QJsonObject body;
    if (isV2()) {
        if (!m_config->workDir().isEmpty())
            body.insert("location", QJsonObject{{"directory", m_config->workDir()}});
    } else {
        body.insert("title", "Qute Agent");
    }
    QNetworkReply *r = m_nam->post(makeRequest(isV2() ? QStringLiteral("/api/session") : QStringLiteral("/session")),
                                   QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(r, &QNetworkReply::finished, this, [this, r] {
        r->deleteLater();
        m_sessionRequested = false;
        if (r->error() != QNetworkReply::NoError) {
            setError(tr("Creating session failed: %1").arg(r->errorString()));
            m_retry.start(4000);
            return;
        }
        const QJsonObject o = QJsonDocument::fromJson(r->readAll()).object();
        QString id = o.value("id").toString();
        if (id.isEmpty()) id = o.value("data").toObject().value("id").toString();
        if (id.isEmpty()) id = o.value("session").toObject().value("id").toString();
        if (id.isEmpty()) {
            setError(tr("Server returned no session id"));
            m_retry.start(4000);
            return;
        }
        adoptSession(id);
    });
}

void OpenCodeClient::adoptSession(const QString &id)
{
    m_sessionId = id;
    // Remember it for the next start and for /openinopencode.
    if (m_config->lastSessionId() != id) {
        m_config->setLastSessionId(id);
        m_config->save();
    }
    emit sessionChanged();
    openEventStream();
}

void OpenCodeClient::resetSessionState()
{
    m_partOrder.clear(); m_partText.clear(); m_partMessage.clear(); m_responseText.clear();
    m_roles.clear();
    m_childSessions.clear();
    m_requestSession.clear();
    m_turnActive = false;
    emit responseTextChanged();
}

void OpenCodeClient::newSession()
{
    // /new: a brand-new session. Deliberately NOT ensureSession(): that would resume the remembered one.
    if (m_sessionRequested)
        return;
    m_sessionRequested = true;
    m_sessionId.clear();
    resetSessionState();
    emit sessionChanged();
    emit serverEvent(QStringLiteral("session.switched"), QJsonObject{{"sessionID", QString()}});
    createSession();
}

void OpenCodeClient::switchSession(const QString &id)
{
    static const QRegularExpression idPattern(QStringLiteral("^[A-Za-z0-9_-]+$"));
    if (!idPattern.match(id).hasMatch()) {
        emit requestFailed(tr("Invalid session id."));
        return;
    }
    if (id == m_sessionId)
        return;
    resetSessionState();
    m_sessionId = id;
    m_config->setLastSessionId(id);
    m_config->save();
    emit sessionChanged();
    emit serverEvent(QStringLiteral("session.switched"), QJsonObject{{"sessionID", id}});
    openEventStream();
}

QVariantList OpenCodeClient::parseSessions(const QJsonDocument &document)
{
    QVariantList out;
    for (const QJsonValue &v : extractArray(document)) {
        const QJsonObject o = v.toObject();
        QString id = o.value(QStringLiteral("id")).toString();
        if (id.isEmpty())
            id = o.value(QStringLiteral("data")).toObject().value(QStringLiteral("id")).toString();
        if (id.isEmpty())
            continue;
        QString title = o.value(QStringLiteral("title")).toString();
        if (title.isEmpty())
            title = QStringLiteral("(untitled)");
        QString directory = o.value(QStringLiteral("directory")).toString();
        if (directory.isEmpty())
            directory = o.value(QStringLiteral("location")).toObject().value(QStringLiteral("directory")).toString();
        // Milliseconds since epoch, as opencode reports them; 0 when missing.
        double updated = o.value(QStringLiteral("time")).toObject().value(QStringLiteral("updated")).toDouble();
        if (updated == 0)
            updated = o.value(QStringLiteral("updated")).toDouble();
        out << QVariantMap{{QStringLiteral("key"), id}, {QStringLiteral("label"), title},
                           {QStringLiteral("sub"), directory}, {QStringLiteral("updated"), updated}};
    }
    // Most recently used first, like the opencode session list.
    std::stable_sort(out.begin(), out.end(), [](const QVariant &a, const QVariant &b) {
        return a.toMap().value(QStringLiteral("updated")).toDouble() > b.toMap().value(QStringLiteral("updated")).toDouble();
    });
    return out;
}

void OpenCodeClient::listSessions()
{
    // (verify) V2 list endpoint GET /api/session; V1 uses GET /session.
    const QString path = isV2() ? QStringLiteral("/api/session?limit=50") : QStringLiteral("/session");
    QNetworkReply *r = m_nam->get(makeRequest(path));
    connect(r, &QNetworkReply::finished, this, [this, r] {
        r->deleteLater();
        if (r->error() != QNetworkReply::NoError) {
            emit sessionsListFailed();
            emit requestFailed(tr("Listing sessions failed: %1").arg(r->errorString()));
            return;
        }
        emit sessionsListed(parseSessions(QJsonDocument::fromJson(r->readAll())));
    });
}

void OpenCodeClient::runServerCommand(const QString &name, const QString &arguments, const QVariantList &files,
                                      const QVariantList &agents)
{
    static const QRegularExpression namePattern(QStringLiteral("^[A-Za-z0-9_.:-]+$"));
    if (m_sessionId.isEmpty() || !namePattern.match(name).hasMatch()) {
        emit requestFailed(tr("No active session to run /%1 in.").arg(name));
        return;
    }
    const QString session = m_sessionId;
    if (isV2()) {
        // Verified against opencode 2: POST /api/session/{id}/command takes "command" (not "name") plus a required
        // "text". 204 on success.
        postCommand(QStringLiteral("/api/session/%1/command").arg(session),
                    QJsonObject{{"command", name}, {"text", arguments},
                                {"files", toJsonFiles(files)}, {"agents", toJsonAgents(agents)},
                                {"skills", QJsonArray()}},
                    tr("Running /%1").arg(name), {});
    } else {
        postCommand(QStringLiteral("/session/%1/command").arg(session),
                    QJsonObject{{"command", name}, {"arguments", arguments}}, tr("Running /%1").arg(name), {});
    }
}

void OpenCodeClient::compactSession()
{
    if (m_sessionId.isEmpty()) {
        emit requestFailed(tr("No active session to compact."));
        return;
    }
    if (isV2()) {
        // Verified against opencode 2: 200 and a "compaction" entry is queued for the session.
        postCommand(QStringLiteral("/api/session/%1/compact").arg(m_sessionId), QJsonObject{}, tr("Compacting"), {});
        return;
    }
    // V1 needs a model for the summary. Use the configured one; without it, say so instead of failing silently.
    if (m_config->modelProvider().isEmpty() || m_config->modelId().isEmpty()) {
        emit requestFailed(tr("Set a model in settings to compact on opencode 1."));
        return;
    }
    postCommand(QStringLiteral("/session/%1/summarize").arg(m_sessionId),
                QJsonObject{{"providerID", m_config->modelProvider()}, {"modelID", m_config->modelId()}},
                tr("Compacting"), {});
}

void OpenCodeClient::openEventStream()
{
    if (m_events) {
        m_events->disconnect(this);
        m_events->abort();
        m_events->deleteLater();
    }
    m_parser.reset();
    QNetworkRequest req = makeRequest(isV2() ? QStringLiteral("/api/event") : QStringLiteral("/event"));
    req.setRawHeader("Accept", "text/event-stream");
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    req.setTransferTimeout(0);                                      // SSE is long-lived: disable the default timeout
    m_events = m_nam->get(req);
    connect(m_events, &QNetworkReply::readyRead, this, &OpenCodeClient::onEventData);
    connect(m_events, &QNetworkReply::finished, this, [this] {
        if (!m_events)
            return;
        m_events->deleteLater();
        m_events = nullptr;
        // The server may legitimately close the stream (documented for V2). Stay "connected" if the server still
        // answers, just resubscribe; checkHealth() runs again via the retry timer.
        m_retry.start(1500);
    });
    // The stream is considered up once the server answered the request; server.connected confirms it.
}

void OpenCodeClient::onEventData()
{
    if (!m_events)
        return;
    for (const SseEvent &ev : m_parser.feed(m_events->readAll())) {
        QJsonObject o = QJsonDocument::fromJson(ev.data).object();
        if (o.contains(QStringLiteral("payload")))                  // /global/event wraps: {directory, payload}
            o = o.value("payload").toObject();
        QString type = o.value("type").toString();
        if (type.isEmpty())
            type = ev.event;
        QJsonObject props = o.value("properties").toObject();
        if (props.isEmpty())
            props = o.value("data").toObject();                     // V2 envelopes may use "data"
        handleBusEvent(type, props);
    }
}

static QString sessionOf(const QJsonObject &p)
{
    QString s = p.value("sessionID").toString();
    if (s.isEmpty()) s = p.value("info").toObject().value("sessionID").toString();
    if (s.isEmpty()) s = p.value("part").toObject().value("sessionID").toString();
    return s;
}

void OpenCodeClient::handleBusEvent(const QString &type, const QJsonObject &p)
{
    if (type == QLatin1String("server.connected")) {
        setConnected(true);
        emit serverEvent(type, p);            // SessionStore reconciles the history on every (re)connect
        return;
    }
    const QString sid = sessionOf(p);

    // Sub-agents run in CHILD sessions. Remember them, otherwise their tool activity and - worse - their permission
    // requests would be dropped and the agent would wait forever.
    if (type == QLatin1String("session.created") || type == QLatin1String("session.forked")) {
        QString parent = p.value("parentID").toString();
        if (parent.isEmpty()) parent = p.value("info").toObject().value("parentID").toString();
        if (parent.isEmpty()) parent = p.value("session").toObject().value("parentID").toString();
        QString child = sid;
        if (child.isEmpty()) child = p.value("info").toObject().value("id").toString();
        if (child.isEmpty()) child = p.value("session").toObject().value("id").toString();
        if (!parent.isEmpty() && (parent == m_sessionId || m_childSessions.contains(parent)) && !child.isEmpty())
            m_childSessions.insert(child);
    }
    if (!sid.isEmpty() && sid != m_sessionId && !m_childSessions.contains(sid))
        return;                                                     // another session's event

    if (type == QLatin1String("permission.asked") || type == QLatin1String("permission.updated")
        || type == QLatin1String("form.created")) {
        QString id = p.value("id").toString();
        if (id.isEmpty()) id = p.value("requestID").toString();
        if (id.isEmpty()) id = p.value("formID").toString();
        if (!id.isEmpty())
            m_requestSession.insert(id, sid.isEmpty() ? m_sessionId : sid);
    }
    setConnected(true);                                             // any event proves the stream is alive

    if (type == QLatin1String("message.updated")) {
        const QJsonObject info = p.value("info").toObject();
        m_roles.insert(info.value("id").toString(), info.value("role").toString());
        rebuildResponse();
    } else if (type == QLatin1String("message.part.updated")) {
        const QJsonObject part = p.value("part").toObject();
        if (part.value("type").toString() == QLatin1String("text") && !part.value("synthetic").toBool()) {
            const QString id = part.value("id").toString();
            if (!m_partOrder.contains(id)) m_partOrder << id;
            m_partMessage.insert(id, part.value("messageID").toString());
            m_partText.insert(id, part.value("text").toString());
            rebuildResponse();
        }
    } else if (type == QLatin1String("message.part.delta")) {
        if (p.value("field").toString() == QLatin1String("text")) {
            const QString id = p.value("partID").toString();
            if (!m_partOrder.contains(id)) m_partOrder << id;
            m_partMessage.insert(id, p.value("messageID").toString());
            m_partText[id] += p.value("delta").toString();
            rebuildResponse();
        }
    } else if (type == QLatin1String("session.text.started") || type == QLatin1String("session.text.delta")
               || type == QLatin1String("session.text.ended")) {
        // V2 streams the answer per text block: {assistantMessageID, ordinal, delta|text}
        // (verified against opencode 2.2.6 - V2 does NOT emit message.part.* events).
        const QString msgId = p.value("assistantMessageID").toString();
        const QString key = msgId + QLatin1Char('#') + QString::number(p.value("ordinal").toInt());
        if (!m_partOrder.contains(key)) m_partOrder << key;
        m_roles.insert(msgId, QStringLiteral("assistant"));
        m_partMessage.insert(key, msgId);
        if (type == QLatin1String("session.text.delta"))
            m_partText[key] += p.value("delta").toString();
        else if (type == QLatin1String("session.text.ended"))
            m_partText.insert(key, p.value("text").toString());   // authoritative full block
        rebuildResponse();
    }
    emit serverEvent(type, p);

    const QString status = p.value("status").toObject().value("type").toString();
    if (type == QLatin1String("session.idle") || type == QLatin1String("session.execution.succeeded")
        || (type == QLatin1String("session.status") && status == QLatin1String("idle")))
        turnEnded(false);
}

void OpenCodeClient::rebuildResponse()
{
    QStringList texts;
    for (const QString &id : std::as_const(m_partOrder)) {
        const QString role = m_roles.value(m_partMessage.value(id));
        const QString text = m_partText.value(id);
        if (role == QLatin1String("user"))
            continue;
        if (role.isEmpty() && text == m_lastPrompt)                 // user part seen before its message.updated
            continue;
        if (!text.isEmpty())
            texts << text;
    }
    const QString joined = texts.join(QStringLiteral("\n\n"));
    if (joined != m_responseText) {
        m_responseText = joined;
        emit responseTextChanged();
    }
}

// ---- polling (truth) + reconcile -----------------------------------------------------------------------
void OpenCodeClient::pollStatus()
{
    if (!m_turnActive || m_sessionId.isEmpty()) {
        m_poll.stop();
        return;
    }
    if (m_promptClock.isValid() && m_promptClock.elapsed() < 2500)
        return;                                                     // the session may not be "busy" yet
    QNetworkReply *r = m_nam->get(makeRequest(isV2() ? QStringLiteral("/api/session/active") : QStringLiteral("/session/status")));
    connect(r, &QNetworkReply::finished, this, [this, r] {
        r->deleteLater();
        if (!m_turnActive || r->error() != QNetworkReply::NoError)
            return;
        QJsonObject o = QJsonDocument::fromJson(r->readAll()).object();
        if (o.value("data").isObject())
            o = o.value("data").toObject();
        bool active;
        if (isV2()) {
            active = o.contains(m_sessionId);                       // absent = inactive
        } else {
            const QString st = o.value(m_sessionId).toObject().value("type").toString();
            active = (st == QLatin1String("busy") || st == QLatin1String("retry"));
        }
        if (active) { m_idleMisses = 0; return; }
        if (++m_idleMisses >= 2)                                    // two quiet polls in a row = finished
            turnEnded(true);
    });
}

void OpenCodeClient::turnEnded(bool synthesizeIdleEvent)
{
    if (!m_turnActive)
        return;
    m_turnActive = false;
    m_poll.stop();
    if (synthesizeIdleEvent)
        emit serverEvent(QStringLiteral("session.status"),
                         QJsonObject{{"sessionID", m_sessionId}, {"status", QJsonObject{{"type", "idle"}}}});
    reconcileMessages();
}

QJsonArray OpenCodeClient::extractArray(const QJsonDocument &doc)
{
    if (doc.isArray())
        return doc.array();
    const QJsonObject o = doc.object();
    for (const char *k : {"data", "items", "messages", "results"})
        if (o.value(QLatin1String(k)).isArray())
            return o.value(QLatin1String(k)).toArray();
    if (o.value("data").isObject())
        return extractArray(QJsonDocument(o.value("data").toObject()));
    return {};
}

static void collectText(const QJsonValue &v, QStringList &out, int depth = 0)
{
    if (depth > 6)
        return;
    if (v.isArray()) {
        for (const QJsonValue &e : v.toArray()) collectText(e, out, depth + 1);
    } else if (v.isObject()) {
        const QJsonObject o = v.toObject();
        if (o.value("type").toString() == QLatin1String("text") && o.value("text").isString() && !o.value("synthetic").toBool()) {
            out << o.value("text").toString();
            return;
        }
        for (const char *k : {"parts", "content", "data"})
            if (o.contains(QLatin1String(k))) collectText(o.value(QLatin1String(k)), out, depth + 1);
    }
}

QString OpenCodeClient::assistantTextAfterLastUser(const QJsonArray &asc)
{
    QStringList texts;
    for (const QJsonValue &item : asc) {
        const QJsonObject o = item.toObject();
        QString role = o.value("info").toObject().value("role").toString();      // V1: {info:{role}, parts:[]}
        if (role.isEmpty()) role = o.value("role").toString();
        if (role.isEmpty()) role = o.value("type").toString();                   // V2 projected: type = user|assistant|...
        if (role == QLatin1String("user")) {
            texts.clear();
        } else if (role == QLatin1String("assistant")) {
            QStringList part;
            collectText(o.contains("parts") ? o.value("parts") : o, part);
            texts << part;
        }
    }
    return texts.join(QStringLiteral("\n\n")).trimmed();
}

void OpenCodeClient::reconcileMessages()
{
    fetchMessages(100, true);          // F0: recent history; pagination follows when F3 needs it
}

void OpenCodeClient::requestMessageHistory(int limit)
{
    fetchMessages(qMax(1, limit), false);
}

void OpenCodeClient::fetchMessages(int limit, bool updateResponseText)
{
    if (m_sessionId.isEmpty())
        return;
    const QString path = isV2() ? QStringLiteral("/api/session/%1/message?order=desc&limit=%2").arg(m_sessionId).arg(limit)
                                : QStringLiteral("/session/%1/message?limit=%2").arg(m_sessionId).arg(limit);
    // The reply belongs to the session it was asked for. If the user has switched sessions meanwhile, the
    // answer is for the session they left: applying it would show that session's messages in this one.
    const QString requestedSession = m_sessionId;
    QNetworkReply *r = m_nam->get(makeRequest(path));
    connect(r, &QNetworkReply::finished, this, [this, r, updateResponseText, requestedSession] {
        r->deleteLater();
        if (requestedSession != m_sessionId)
            return;
        if (r->error() != QNetworkReply::NoError) {
            emit historyFailed();
            return;
        }
        QJsonArray arr = extractArray(QJsonDocument::fromJson(r->readAll()));
        if (isV2()) {                                               // requested newest-first: make it oldest-first
            QJsonArray rev;
            for (qsizetype i = arr.size() - 1; i >= 0; --i) rev.append(arr.at(i));
            arr = rev;
        }
        if (updateResponseText) {
            const QString text = assistantTextAfterLastUser(arr);
            if (!text.isEmpty() && text != m_responseText) {
                m_responseText = text;
                emit responseTextChanged();
            }
        }
        emit messageHistoryReceived(arr);
    });
}

void OpenCodeClient::fetchJson(const QString &path)
{
    QNetworkReply *r = m_nam->get(makeRequest(path));
    connect(r, &QNetworkReply::finished, this, [this, r, path] {
        r->deleteLater();
        const int status = r->error() == QNetworkReply::NoError
                               ? r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()
                               : 0;
        emit jsonFetched(path, QJsonDocument::fromJson(r->readAll()), status);
    });
}

// ---- commands -----------------------------------------------------------------------------------------------
void OpenCodeClient::postJson(const QString &path, const QJsonObject &body, const QString &what)
{
    QNetworkReply *r = m_nam->post(makeRequest(path), QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(r, &QNetworkReply::finished, this, [this, r, what] {
        r->deleteLater();
        if (r->error() != QNetworkReply::NoError)
            setError(tr("%1 failed: %2 %3").arg(what, r->errorString(), QString::fromUtf8(r->readAll().left(200))));
    });
}

void OpenCodeClient::postCommand(const QString &path, const QJsonObject &body, const QString &what,
                                 const std::function<void()> &onSuccess)
{
    QNetworkReply *r = m_nam->post(makeRequest(path), QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(r, &QNetworkReply::finished, this, [this, r, what, onSuccess] {
        r->deleteLater();
        if (r->error() != QNetworkReply::NoError) {
            setError(tr("%1 failed: %2 %3").arg(what, r->errorString(),
                                                QString::fromUtf8(r->readAll().left(200))));
            return;
        }
        if (onSuccess)
            onSuccess();
    });
}

void OpenCodeClient::postPrompt(const QJsonObject &body, const QJsonObject &fallbackBody)
{
    const QString path = isV2() ? QStringLiteral("/api/session/%1/prompt").arg(m_sessionId)
                                : QStringLiteral("/session/%1/prompt_async").arg(m_sessionId);
    QNetworkReply *r = m_nam->post(makeRequest(path), QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(r, &QNetworkReply::finished, this, [this, r, fallbackBody, path] {
        r->deleteLater();
        if (r->error() == QNetworkReply::NoError)
            return;
        const int st = r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (!fallbackBody.isEmpty() && (st == 400 || st == 422)) {  // V2 body shape differs between builds: retry once
            QNetworkReply *r2 = m_nam->post(makeRequest(path), QJsonDocument(fallbackBody).toJson(QJsonDocument::Compact));
            connect(r2, &QNetworkReply::finished, this, [this, r2] {
                r2->deleteLater();
                if (r2->error() != QNetworkReply::NoError)
                    setError(tr("Sending prompt failed: %1 %2").arg(r2->errorString(), QString::fromUtf8(r2->readAll().left(200))));
            });
            return;
        }
        setError(tr("Sending prompt failed: %1 %2").arg(r->errorString(), QString::fromUtf8(r->readAll().left(200))));
    });
}

void OpenCodeClient::sendPrompt(const QString &text)
{
    sendPromptWithFiles(text, {}, {});
}

bool OpenCodeClient::sendPromptWithFiles(const QString &text, const QVariantList &files, const QVariantList &agents)
{
    if (m_sessionId.isEmpty()) {
        setError(tr("Not connected to opencode yet"));
        return false;
    }
    if (!files.isEmpty() && !isV2()) {
        emit requestFailed(tr("Attachments need opencode 2; this server is opencode 1."));
        return false;
    }
    m_lastPrompt = text;
    m_partOrder.clear(); m_partText.clear(); m_partMessage.clear();
    m_responseText.clear();
    emit responseTextChanged();
    m_turnActive = true;
    m_idleMisses = 0;
    m_promptClock.start();
    m_poll.start();
    QStringList names;
    for (const QVariant &f : files)
        names << f.toMap().value(QStringLiteral("name")).toString();
    emit promptSent(text, names);

    if (isV2()) {
        // Body shape verified against opencode 2 with curl: flat {"text":...} plus optional "files" / "agents".
        // Each file is {"uri": "file:///abs/path" or "data:<mime>;base64,...", "name": ...}. Attachments are
        // only added when present, so a plain prompt keeps the minimal body.
        QJsonObject body{{"text", text}};
        if (!files.isEmpty())
            body.insert(QStringLiteral("files"), toJsonFiles(files));
        if (!agents.isEmpty())
            body.insert(QStringLiteral("agents"), toJsonAgents(agents));
        postPrompt(body, files.isEmpty() && agents.isEmpty() ? QJsonObject{{"prompt", QJsonObject{{"text", text}}}}
                                                             : QJsonObject{});
    } else {
        QJsonObject body{{"parts", QJsonArray{QJsonObject{{"type", "text"}, {"text", text}}}}};
        if (!m_config->modelProvider().isEmpty() && !m_config->modelId().isEmpty())
            body.insert("model", QJsonObject{{"providerID", m_config->modelProvider()}, {"modelID", m_config->modelId()}});
        postPrompt(body, {});
    }
    return true;
}

QJsonArray OpenCodeClient::toJsonAgents(const QVariantList &agents)
{
    QJsonArray out;
    for (const QVariant &v : agents) {
        const QString name = v.toMap().value(QStringLiteral("name")).toString();
        if (!name.isEmpty())
            out.append(QJsonObject{{"name", name}});
    }
    return out;
}

QJsonArray OpenCodeClient::toJsonFiles(const QVariantList &files)
{
    QJsonArray out;
    for (const QVariant &v : files) {
        const QVariantMap m = v.toMap();
        QJsonObject f{{"uri", m.value(QStringLiteral("uri")).toString()}};
        const QString name = m.value(QStringLiteral("name")).toString();
        if (!name.isEmpty())
            f.insert(QStringLiteral("name"), name);
        out.append(f);
    }
    return out;
}

void OpenCodeClient::abort()
{
    if (m_sessionId.isEmpty())
        return;
    postJson(isV2() ? QStringLiteral("/api/session/%1/interrupt").arg(m_sessionId)
                    : QStringLiteral("/session/%1/abort").arg(m_sessionId), {}, tr("Abort"));
}

void OpenCodeClient::answerPermission(const QString &permissionId, const QString &response)
{
    if (m_sessionId.isEmpty() || permissionId.isEmpty())
        return;
    // The request may come from a sub-agent's child session: answer in the session that asked.
    const QString session = m_requestSession.value(permissionId, m_sessionId);
    QString path;
    QJsonObject body;
    if (isV2()) {
        // OpenCode 2: POST /api/session/{sid}/permission/{requestID}/reply  {"decision": once|always|reject}
        path = QStringLiteral("/api/session/%1/permission/%2/reply").arg(session, permissionId);
        body.insert("decision", response);
    } else {
        path = QStringLiteral("/session/%1/permissions/%2").arg(session, permissionId);
        body.insert("response", response);
    }
    QNetworkReply *r = m_nam->post(makeRequest(path), QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(r, &QNetworkReply::finished, this, [this, r, permissionId] {
        r->deleteLater();
        if (r->error() != QNetworkReply::NoError) {
            setError(tr("Permission reply failed: %1 %2").arg(r->errorString(), QString::fromUtf8(r->readAll().left(200))));
            return;
        }
        // Do not rely on the stream alone: leave the "waiting" state as soon as the server accepted the answer.
        emit serverEvent(QStringLiteral("permission.replied"), QJsonObject{{"id", permissionId}, {"sessionID", m_sessionId}});
        m_requestSession.remove(permissionId);
    });
}

void OpenCodeClient::cancelForm(const QString &formId)
{
    if (!isV2() || m_sessionId.isEmpty() || formId.isEmpty())
        return;
    const QString session = m_requestSession.value(formId, m_sessionId);
    QNetworkReply *r = m_nam->deleteResource(makeRequest(QStringLiteral("/api/session/%1/form/%2").arg(session, formId)));
    connect(r, &QNetworkReply::finished, this, [this, r, formId] {
        r->deleteLater();
        if (r->error() != QNetworkReply::NoError) {
            setError(tr("Cancelling the question failed: %1").arg(r->errorString()));
            return;
        }
        emit serverEvent(QStringLiteral("form.cancelled"), QJsonObject{{"id", formId}, {"sessionID", m_sessionId}});
        m_requestSession.remove(formId);
    });
}

void OpenCodeClient::selectAgent(const QString &agent)
{
    if (!isV2() || m_sessionId.isEmpty() || agent.isEmpty())
        return;
    const QString session = m_sessionId;
    const QString path = QStringLiteral("/api/session/%1/agent").arg(session);
    QNetworkReply *r = m_nam->post(makeRequest(path), QJsonDocument(QJsonObject{{"agent", agent}}).toJson(QJsonDocument::Compact));
    connect(r, &QNetworkReply::finished, this, [this, r, session, agent] {
        r->deleteLater();
        if (r->error() == QNetworkReply::NoError) {
            emit serverEvent(QStringLiteral("session.agent.selected"),
                             QJsonObject{{"sessionID", session}, {"agent", agent}});
            return;
        }
        // The server did not take the switch: say so, and read the session again so the menu shows the agent that
        // is really active instead of the one we showed optimistically.
        emit requestFailed(tr("Could not switch to %1: %2").arg(agent, r->errorString()));
        if (session == m_sessionId)
            fetchJson(QStringLiteral("/api/session/%1").arg(session));
    });
}

void OpenCodeClient::selectModel(const QString &providerID, const QString &modelID, const QString &variant)
{
    if (!isV2() || m_sessionId.isEmpty() || modelID.isEmpty())
        return;
    QJsonObject model{{"id", modelID}, {"providerID", providerID}};
    if (!variant.isEmpty())
        model.insert("variant", variant);
    const QString session = m_sessionId;
    postCommand(QStringLiteral("/api/session/%1/model").arg(session), QJsonObject{{"model", model}},
                tr("Switching the model"), [this, session, model] {
                    emit serverEvent(QStringLiteral("session.model.selected"),
                                     QJsonObject{{"sessionID", session}, {"model", model}});
                });
}

void OpenCodeClient::setConnected(bool c)
{
    if (c == m_connected)
        return;
    m_connected = c;
    if (c) {
        setError(QString());
        setMissing(false);
    }
    emit connectedChanged(c);
}

void OpenCodeClient::setError(const QString &e)
{
    if (e == m_lastError)
        return;
    m_lastError = e;
    emit lastErrorChanged();
}

void OpenCodeClient::setMissing(bool m)
{
    if (m == m_opencodeMissing)
        return;
    m_opencodeMissing = m;
    emit opencodeMissingChanged();
}
