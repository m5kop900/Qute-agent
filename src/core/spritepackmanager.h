#pragma once
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

// Finds and loads sprite packs (see docs/SPRITE_PACK.md for the on-disk format).
// QML asks it: packs.stateInfo("thinking") -> { source: url, frames: int, fps: int }.
class SpritePackManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(int frameWidth READ frameWidth NOTIFY loaded)
    Q_PROPERTY(int frameHeight READ frameHeight NOTIFY loaded)
    Q_PROPERTY(QString packName READ packName NOTIFY loaded)
    Q_PROPERTY(int revision READ revision NOTIFY loaded)   // bump => QML re-evaluates stateInfo()
    Q_PROPERTY(QVariantList availablePacks READ availablePacks NOTIFY packsChanged) // [{id,name,variants:[...]}]
public:
    explicit SpritePackManager(QObject *parent = nullptr);

    static QStringList searchPaths();

    int frameWidth() const { return m_frameW; }
    int frameHeight() const { return m_frameH; }
    QString packName() const { return m_name; }
    int revision() const { return m_revision; }
    QVariantList availablePacks() const { return m_packs; }

    Q_INVOKABLE void rescan();
    Q_INVOKABLE bool load(const QString &packId, const QString &variant);
    Q_INVOKABLE QStringList variantsOf(const QString &packId) const;
    Q_INVOKABLE QVariantMap stateInfo(const QString &stateName) const;

    // F7.2: user pack folder + validation. validatePack accepts a pack id or a directory path and
    // returns { ok, path, errors: [...], warnings: [...] }.
    Q_INVOKABLE QString packsFolder() const;
    Q_INVOKABLE bool openPacksFolder();
    Q_INVOKABLE QVariantMap validatePack(const QString &packIdOrPath) const;
    Q_INVOKABLE QVariantList validateUserPacks() const;   // every folder in packsFolder(), broken ones included

signals:
    void loaded();
    void packsChanged();

private:
    QString packDir(const QString &packId) const;

    QVariantList m_packs;
    QString m_dir;
    QString m_variant;
    QString m_name;
    QJsonObject m_manifest;
    int m_frameW = 256;
    int m_frameH = 256;
    int m_revision = 0;
};
