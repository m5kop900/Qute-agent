#include "spritepackmanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QUrl>

SpritePackManager::SpritePackManager(QObject *parent) : QObject(parent)
{
    rescan();
}

QStringList SpritePackManager::searchPaths()
{
    QStringList paths;
    const QByteArray env = qgetenv("QUTE_PACKS");           // dev override
    if (!env.isEmpty())
        paths << QString::fromLocal8Bit(env);
    paths << QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/spritepacks");
    paths << QCoreApplication::applicationDirPath() + QStringLiteral("/../Resources/spritepacks");   // macOS app bundle
    paths << QCoreApplication::applicationDirPath() + QStringLiteral("/../share/qute/spritepacks");
#ifdef QUTE_INSTALL_PACKS_DIR
    paths << QStringLiteral(QUTE_INSTALL_PACKS_DIR);
#endif
#ifdef QUTE_DEV_PACKS_DIR
    paths << QStringLiteral(QUTE_DEV_PACKS_DIR);            // running from the build tree
#endif
    return paths;                                           // earlier entries win
}

static QJsonObject readManifest(const QString &dir)
{
    QFile f(dir + QStringLiteral("/manifest.json"));
    if (!f.open(QIODevice::ReadOnly))
        return {};
    const QJsonDocument d = QJsonDocument::fromJson(f.readAll());
    return d.isObject() ? d.object() : QJsonObject{};
}

void SpritePackManager::rescan()
{
    m_packs.clear();
    QStringList seen;
    for (const QString &root : searchPaths()) {
        const QDir rd(root);
        for (const QString &sub : rd.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            const QJsonObject m = readManifest(rd.filePath(sub));
            const QString id = m.value("id").toString();
            if (id.isEmpty() || seen.contains(id))
                continue;
            seen << id;
            QStringList variants;
            for (const QJsonValue &v : m.value("variants").toArray())
                variants << v.toString();
            m_packs.append(QVariantMap{{"id", id}, {"name", m.value("name").toString(id)}, {"variants", variants}});
        }
    }
    emit packsChanged();
}

QString SpritePackManager::packDir(const QString &packId) const
{
    for (const QString &root : searchPaths()) {
        const QDir rd(root);
        for (const QString &sub : rd.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
            if (readManifest(rd.filePath(sub)).value("id").toString() == packId)
                return rd.filePath(sub);
    }
    return {};
}

QStringList SpritePackManager::variantsOf(const QString &packId) const
{
    for (const QVariant &v : m_packs) {
        const QVariantMap m = v.toMap();
        if (m.value("id").toString() == packId)
            return m.value("variants").toStringList();
    }
    return {};
}

bool SpritePackManager::load(const QString &packId, const QString &variant)
{
    QString dir = packDir(packId);
    if (dir.isEmpty() && !m_packs.isEmpty())                 // unknown pack id: fall back to the first pack
        dir = packDir(m_packs.first().toMap().value("id").toString());
    if (dir.isEmpty())
        return false;

    const QJsonObject m = readManifest(dir);
    QStringList variants;
    for (const QJsonValue &v : m.value("variants").toArray())
        variants << v.toString();

    m_dir = dir;
    m_manifest = m;
    m_name = m.value("name").toString();
    m_variant = variants.contains(variant) ? variant : (variants.isEmpty() ? QString() : variants.first());
    m_frameW = m.value("frame").toObject().value("width").toInt(256);
    m_frameH = m.value("frame").toObject().value("height").toInt(256);
    ++m_revision;
    emit loaded();
    return true;
}

QVariantMap SpritePackManager::stateInfo(const QString &stateName) const
{
    if (m_manifest.isEmpty())
        return {{"source", QUrl()}, {"frames", 1}, {"fps", 1}};

    const QJsonObject states = m_manifest.value("states").toObject();
    const QJsonObject fallback = m_manifest.value("fallback").toObject();
    QString name = stateName;
    for (int hops = 0; hops < 5 && !states.contains(name); ++hops) {
        if (!fallback.contains(name)) { name = QStringLiteral("idle"); break; }
        name = fallback.value(name).toString();
    }
    const QJsonObject s = states.value(name).toObject();
    QString file = s.value("file").toString();
    file.replace(QStringLiteral("{variant}"), m_variant);
    return {
        {"source", QUrl::fromLocalFile(QDir(m_dir).filePath(file))},
        {"frames", s.value("frames").toInt(1)},
        {"fps", s.value("fps").toInt(8)},
    };
}
