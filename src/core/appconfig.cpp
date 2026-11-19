#include "appconfig.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMetaProperty>
#include <QSaveFile>
#include <QStandardPaths>

AppConfig::AppConfig(QObject *parent) : QObject(parent), m_path(defaultPath())
{
    m_workDir = QDir::homePath();            // default working directory; a saved value (even empty) overrides it
    load();
}

QString AppConfig::defaultPath()
{
    // QCoreApplication::applicationName() must be "qute" in BOTH executables so they share this path.
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QStringLiteral("/config.json");
}

void AppConfig::migrate(QJsonObject &obj)
{
    const int version = qMax(1, obj.value(QStringLiteral("configVersion")).toInt(1));

    if (version < 2) {
        // v2 (F7): the appearance block is explicit now: overlay opacity, corner radius, density and
        // the animation switch. Older files get the defaults written once; their other keys stay.
        if (!obj.contains(QStringLiteral("overlayOpacity")))
            obj.insert(QStringLiteral("overlayOpacity"), 1.0);
        if (!obj.contains(QStringLiteral("cornerRadius")))
            obj.insert(QStringLiteral("cornerRadius"), 16);
        if (!obj.contains(QStringLiteral("density")))
            obj.insert(QStringLiteral("density"), QStringLiteral("compact"));
        if (!obj.contains(QStringLiteral("animations")))
            obj.insert(QStringLiteral("animations"), true);
        // Early files could contain an unversioned "auto" theme; the current value is "system".
        if (obj.value(QStringLiteral("themeMode")).toString() == QLatin1String("auto"))
            obj.insert(QStringLiteral("themeMode"), QStringLiteral("system"));
        // Values outside the slider range could come from hand-edited files.
        if (obj.contains(QStringLiteral("scale")))
            obj.insert(QStringLiteral("scale"), qBound(0.5, obj.value(QStringLiteral("scale")).toDouble(1.0), 3.0));
    }

    obj.insert(QStringLiteral("configVersion"), CurrentConfigVersion);
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
    QJsonObject obj = doc.object();
    migrate(obj);
    m_configVersion = obj.value(QStringLiteral("configVersion")).toInt(CurrentConfigVersion);

    const QMetaObject &mo = AppConfig::staticMetaObject;
    for (int i = mo.propertyOffset(); i < mo.propertyCount(); ++i) {
        const QMetaProperty p = mo.property(i);
        if (!p.isWritable())
            continue;                            // configVersion/configPath are derived
        if (obj.contains(QLatin1String(p.name())))
            setProperty(p.name(), obj.value(QLatin1String(p.name())).toVariant());
    }
}

bool AppConfig::save()
{
    // Keep keys this version does not know about (plugins, future options, hand edits).
    QJsonObject obj;
    QFile existing(m_path);
    if (existing.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(existing.readAll());
        if (doc.isObject())
            obj = doc.object();
    }

    const QMetaObject &mo = AppConfig::staticMetaObject;
    for (int i = mo.propertyOffset(); i < mo.propertyCount(); ++i) {
        const QMetaProperty p = mo.property(i);
        if (!p.isWritable())
            continue;
        obj.insert(QLatin1String(p.name()), QJsonValue::fromVariant(property(p.name())));
    }
    obj.insert(QStringLiteral("configVersion"), CurrentConfigVersion);

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
