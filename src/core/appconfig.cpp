#include "appconfig.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaProperty>
#include <QSaveFile>
#include <QStandardPaths>

AppConfig::AppConfig(QObject *parent) : QObject(parent), m_path(defaultPath())
{
    load();
}

QString AppConfig::defaultPath()
{
    // QCoreApplication::applicationName() must be "qute" in BOTH executables so they share this path.
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QStringLiteral("/config.json");
}

void AppConfig::load()
{
    QFile f(m_path);
    if (!f.open(QIODevice::ReadOnly))
        return;                                  // no file yet => defaults
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return;                                  // keep current values on a broken file
    const QJsonObject obj = doc.object();

    const QMetaObject &mo = AppConfig::staticMetaObject;
    for (int i = mo.propertyOffset(); i < mo.propertyCount(); ++i) {
        const QMetaProperty p = mo.property(i);
        if (obj.contains(QLatin1String(p.name())))
            setProperty(p.name(), obj.value(QLatin1String(p.name())).toVariant());
    }
}

bool AppConfig::save()
{
    QJsonObject obj;
    const QMetaObject &mo = AppConfig::staticMetaObject;
    for (int i = mo.propertyOffset(); i < mo.propertyCount(); ++i) {
        const QMetaProperty p = mo.property(i);
        obj.insert(QLatin1String(p.name()), QJsonValue::fromVariant(property(p.name())));
    }
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QSaveFile f(m_path);                         // atomic write (temp file + rename)
    if (!f.open(QIODevice::WriteOnly))
        return false;
    f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    return f.commit();
}

void AppConfig::enableWatching()
{
    if (m_watcher)
        return;
    m_watcher = new QFileSystemWatcher(this);
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    m_watcher->addPath(QFileInfo(m_path).absolutePath());
    if (QFileInfo::exists(m_path))
        m_watcher->addPath(m_path);

    m_debounce.setSingleShot(true);
    m_debounce.setInterval(150);
    connect(&m_debounce, &QTimer::timeout, this, [this] {
        // QSaveFile replaces the inode, so the watcher loses the file: re-add it.
        if (QFileInfo::exists(m_path) && !m_watcher->files().contains(m_path))
            m_watcher->addPath(m_path);
        load();
    });
    connect(m_watcher, &QFileSystemWatcher::fileChanged, &m_debounce, qOverload<>(&QTimer::start));
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, &m_debounce, qOverload<>(&QTimer::start));
}
