#include "spritepackmanager.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
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

QString SpritePackManager::packsFolder() const
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                        + QStringLiteral("/spritepacks");
    QDir().mkpath(dir);
    return dir;
}

bool SpritePackManager::openPacksFolder()
{
    return QDesktopServices::openUrl(QUrl::fromLocalFile(packsFolder()));
}

QVariantMap SpritePackManager::validatePack(const QString &packIdOrPath) const
{
    QStringList errors;
    QStringList warnings;

    const QFileInfo given(packIdOrPath);
    const QString dir = given.isDir() ? given.absoluteFilePath() : packDir(packIdOrPath);
    if (dir.isEmpty() || !QFileInfo(dir).isDir()) {
        errors << QStringLiteral("pack folder not found: %1").arg(packIdOrPath);
        return {{"ok", false}, {"path", dir}, {"errors", errors}, {"warnings", warnings}};
    }

    const QJsonObject m = readManifest(dir);
    if (m.isEmpty()) {
        errors << QStringLiteral("%1/manifest.json is missing or not valid JSON").arg(dir);
        return {{"ok", false}, {"path", dir}, {"errors", errors}, {"warnings", warnings}};
    }

    const QString id = m.value("id").toString();
    if (id.isEmpty())
        errors << QStringLiteral("manifest: \"id\" is empty");
    const int fw = m.value("frame").toObject().value("width").toInt();
    const int fh = m.value("frame").toObject().value("height").toInt();
    if (fw <= 0 || fh <= 0)
        errors << QStringLiteral("manifest: frame.width/frame.height must be positive");

    QStringList variants;
    for (const QJsonValue &v : m.value("variants").toArray()) {
        const QString name = v.toString();
        if (name.isEmpty())
            errors << QStringLiteral("manifest: empty variant name");
        else if (variants.contains(name))
            errors << QStringLiteral("manifest: duplicate variant \"%1\"").arg(name);
        else
            variants << name;
    }
    if (variants.isEmpty())
        warnings << QStringLiteral("no variants: the pack uses a flat layout and {variant} becomes empty");

    const QJsonObject states = m.value("states").toObject();
    const QJsonObject fallback = m.value("fallback").toObject();
    if (states.isEmpty()) {
        errors << QStringLiteral("manifest: \"states\" is empty");
    } else if (!states.contains(QStringLiteral("idle"))) {
        errors << QStringLiteral("manifest: the \"idle\" state is required (it is the fallback target)");
    }

    const QStringList known = {"idle", "thinking", "working", "waiting", "done", "error", "offline"};
    for (const QString &state : known) {
        if (states.contains(state))
            continue;
        QString resolved = state;
        for (int hops = 0; hops < 5 && !states.contains(resolved); ++hops) {
            if (!fallback.contains(resolved)) { resolved = QStringLiteral("idle"); break; }
            resolved = fallback.value(resolved).toString();
        }
        if (!states.contains(resolved))
            warnings << QStringLiteral("state \"%1\" has no sheet and no usable fallback").arg(state);
    }

    for (auto it = states.begin(); it != states.end(); ++it) {
        const QString state = it.key();
        QString file = it.value().toObject().value("file").toString();
        const int frames = it.value().toObject().value("frames").toInt();
        const int fps = it.value().toObject().value("fps").toInt();
        if (file.isEmpty())
            errors << QStringLiteral("state \"%1\": file is empty").arg(state);
        if (frames < 1)
            errors << QStringLiteral("state \"%1\": frames must be at least 1").arg(state);
        if (fps < 1)
            errors << QStringLiteral("state \"%1\": fps must be at least 1").arg(state);
        if (file.isEmpty() || frames < 1)
            continue;

        const QStringList variantNames = variants.isEmpty() ? QStringList{ QString() } : variants;
        for (const QString &variant : variantNames) {
            QString sheet = file;
            sheet.replace(QStringLiteral("{variant}"), variant);
            const QString path = QDir(dir).filePath(sheet);
            const QImage img(path);
            const QString what = QStringLiteral("%1/%2 (%3)").arg(state, sheet, variant.isEmpty() ? QStringLiteral("flat") : variant);
            if (img.isNull()) {
                errors << QStringLiteral("%1: cannot load the sheet").arg(what);
                continue;
            }
            if (img.size() != QSize(fw * frames, fh))
                errors << QStringLiteral("%1: sheet is %2x%3, expected %4x%5")
                              .arg(what).arg(img.width()).arg(img.height()).arg(fw * frames).arg(fh);
            if (!img.hasAlphaChannel())
                warnings << QStringLiteral("%1: no alpha channel (the pack needs transparency)").arg(what);
            else if (qAlpha(img.pixel(0, 0)) != 0)
                warnings << QStringLiteral("%1: top-left pixel is not transparent").arg(what);
        }
    }

    return {{"ok", errors.isEmpty()}, {"path", dir}, {"errors", errors}, {"warnings", warnings}};
}

QVariantList SpritePackManager::validateUserPacks() const
{
    QVariantList out;
    const QDir root(packsFolder());
    const QStringList subs = root.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &sub : subs) {
        QVariantMap result = validatePack(root.filePath(sub));
        result.insert(QStringLiteral("name"), sub);
        out.append(result);
    }
    return out;
}
