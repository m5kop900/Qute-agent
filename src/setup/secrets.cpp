#include "secrets.h"

#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>

QString Secrets::dir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QStringLiteral("/secrets");
}

QString Secrets::writeKey(const QString &name, const QString &value)
{
    QString safe = name;
    safe.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9._-]")), QStringLiteral("_"));
    QDir().mkpath(dir());
    QFile::setPermissions(dir(), QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    const QString path = dir() + QLatin1Char('/') + safe + QStringLiteral(".key");
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return {};
    f.write(value.toUtf8());
    if (!f.commit())
        return {};
    QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    return path;
}

QString Secrets::fileRef(const QString &path) { return QStringLiteral("{file:%1}").arg(path); }
QString Secrets::envRef(const QString &envName) { return QStringLiteral("{env:%1}").arg(envName); }
