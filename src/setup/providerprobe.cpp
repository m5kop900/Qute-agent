#include "providerprobe.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QUrl>

QString ProviderProbe::joinUrl(const QString &baseUrl, const QString &path)
{
    QString b = baseUrl.trimmed();
    while (b.endsWith(QLatin1Char('/')))
        b.chop(1);
    return b + path;
}

static QNetworkRequest makeReq(const QString &url, const QString &apiKey)
{
    QNetworkRequest r{QUrl(url)};
    r.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    if (!apiKey.isEmpty())
        r.setRawHeader("Authorization", "Bearer " + apiKey.toUtf8());
    return r;
}

// Waits for the reply or the timeout. Returns false on timeout (reply aborted).
static bool wait(QNetworkReply *reply, int timeoutMs)
{
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    bool timedOut = false;
    QObject::connect(&timer, &QTimer::timeout, &loop, [&] { timedOut = true; reply->abort(); });
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();
    return !timedOut;
}

ProbeResult ProviderProbe::listModels(const QString &baseUrl, const QString &apiKey, int timeoutMs)
{
    ProbeResult res;
    QNetworkAccessManager nam;
    QNetworkReply *r = nam.get(makeReq(joinUrl(baseUrl, QStringLiteral("/models")), apiKey));
    const bool inTime = wait(r, timeoutMs);
    res.httpStatus = r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (!inTime) {
        res.error = QStringLiteral("timeout");
    } else if (r->error() != QNetworkReply::NoError) {
        res.error = r->errorString();
    } else {
        const QJsonObject o = QJsonDocument::fromJson(r->readAll()).object();
        for (const QJsonValue &v : o.value("data").toArray())
            res.models << v.toObject().value("id").toString();
        res.models.removeAll(QString());
        res.ok = true;
    }
    r->deleteLater();
    return res;
}

ChatTestResult ProviderProbe::testChat(const QString &baseUrl, const QString &apiKey, const QString &model, int timeoutMs)
{
    ChatTestResult res;
    QNetworkAccessManager nam;
    const QJsonObject body{
        {"model", model},
        {"max_tokens", 16},
        {"messages", QJsonArray{QJsonObject{{"role", "user"}, {"content", "Reply with the single word: ok"}}}},
    };
    QNetworkReply *r = nam.post(makeReq(joinUrl(baseUrl, QStringLiteral("/chat/completions")), apiKey),
                                QJsonDocument(body).toJson(QJsonDocument::Compact));
    const bool inTime = wait(r, timeoutMs);
    if (!inTime) {
        res.error = QStringLiteral("timeout (the model may still be loading)");
    } else if (r->error() != QNetworkReply::NoError) {
        res.error = r->errorString() + QLatin1Char(' ') + QString::fromUtf8(r->readAll().left(200));
    } else {
        const QJsonObject o = QJsonDocument::fromJson(r->readAll()).object();
        res.reply = o.value("choices").toArray().first().toObject().value("message").toObject().value("content").toString().trimmed();
        res.ok = true;
    }
    r->deleteLater();
    return res;
}
