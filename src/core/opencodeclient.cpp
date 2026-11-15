#include "opencodeclient.h"

#include "appconfig.h"
#include "opencodelocator.h"

#include <QJsonDocument>
#include <QNetworkReply>
#include <QProcessEnvironment>
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
    if (perDirectory && !isV2() && !m_config->workDir().isEmpty()) {     // V1 selects the project with ?directory=
        QUrlQuery q(url);
        q.addQueryItem(QStringLiteral("directory"), m_config->workDir());
        url.setQuery(q);
    }
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    if (!m_config->serverPassword().isEmpty()) {
        // V2: the username is always "opencode" (OPENCODE_SERVER_USERNAME was removed)
        const QString user = isV2() ? QStringLiteral("opencode") : m_config->serverUsername();
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
    setError(tr("opencode rejected the password. Check the server password in settings "
                "(OPENCODE_SERVER_PASSWORD) and restart the overlay."));
    m_retry.start(4000);
}

void OpenCodeClient::notRunning()
{
    setConnected(false);
    if (m_config->autoStartServer() && !m_server && !m_opencodeMissing)
        startServerProcess();
    m_retry.start(m_server ? 1000 : 4000);
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
        m_server->deleteLater();
        m_server = nullptr;
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
        m_sessionId = o.value("id").toString();
        if (m_sessionId.isEmpty()) m_sessionId = o.value("data").toObject().value("id").toString();
        if (m_sessionId.isEmpty()) m_sessionId = o.value("session").toObject().value("id").toString();
        emit sessionChanged();
        if (m_sessionId.isEmpty()) {
            setError(tr("Server returned no session id"));
            m_retry.start(4000);
            return;
        }
        openEventStream();
    });
}

void OpenCodeClient::newSession()
{
    m_sessionId.clear();
    emit sessionChanged();
    m_partOrder.clear(); m_partText.clear(); m_partMessage.clear(); m_responseText.clear();
    emit responseTextChanged();
    ensureSession();
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
        return;
    }
    const QString sid = sessionOf(p);
    if (!sid.isEmpty() && sid != m_sessionId)
        return;                                                     // another session's event
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
    if (m_sessionId.isEmpty())
        return;
    const QString path = isV2() ? QStringLiteral("/api/session/%1/message?order=desc&limit=10").arg(m_sessionId)
                                : QStringLiteral("/session/%1/message?limit=10").arg(m_sessionId);
    QNetworkReply *r = m_nam->get(makeRequest(path));
    connect(r, &QNetworkReply::finished, this, [this, r] {
        r->deleteLater();
        if (r->error() != QNetworkReply::NoError)
            return;
        QJsonArray arr = extractArray(QJsonDocument::fromJson(r->readAll()));
        if (isV2()) {                                               // requested newest-first: make it oldest-first
            QJsonArray rev;
            for (qsizetype i = arr.size() - 1; i >= 0; --i) rev.append(arr.at(i));
            arr = rev;
        }
        const QString text = assistantTextAfterLastUser(arr);
        if (!text.isEmpty() && text != m_responseText) {
            m_responseText = text;
            emit responseTextChanged();
        }
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
    if (m_sessionId.isEmpty()) {
        setError(tr("Not connected to opencode yet"));
        return;
    }
    m_lastPrompt = text;
    m_partOrder.clear(); m_partText.clear(); m_partMessage.clear();
    m_responseText.clear();
    emit responseTextChanged();
    m_turnActive = true;
    m_idleMisses = 0;
    m_promptClock.start();
    m_poll.start();
    emit promptSent();

    if (isV2()) {
        // Body shape (verified against opencode 2.2.6 /openapi.json): flat {"text":...}; the schema has
        // additionalProperties:false, so {"prompt":{...}} is rejected with 400. Older builds used the nested form.
        postPrompt(QJsonObject{{"text", text}}, QJsonObject{{"prompt", QJsonObject{{"text", text}}}});
    } else {
        QJsonObject body{{"parts", QJsonArray{QJsonObject{{"type", "text"}, {"text", text}}}}};
        if (!m_config->modelProvider().isEmpty() && !m_config->modelId().isEmpty())
            body.insert("model", QJsonObject{{"providerID", m_config->modelProvider()}, {"modelID", m_config->modelId()}});
        postPrompt(body, {});
    }
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
    if (isV2()) {
        // TODO(M5, verify in /doc): V2 moved permissions to /api/permission/* and "forms"; until verified, report it.
        setError(tr("Answering permission requests is not wired for OpenCode 2 yet - answer in the opencode TUI"));
        return;
    }
    postJson(QStringLiteral("/session/%1/permissions/%2").arg(m_sessionId, permissionId),
             QJsonObject{{"response", response}}, tr("Permission reply"));
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
