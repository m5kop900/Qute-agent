// F7.3: config versioning, migration and unknown-key preservation.
#include "appconfig.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QtTest>

class TstAppConfig : public QObject {
    Q_OBJECT
    QString m_path;

    void writeConfig(const QJsonObject &obj)
    {
        QDir().mkpath(QFileInfo(m_path).absolutePath());
        QFile f(m_path);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
        f.write(QJsonDocument(obj).toJson());
    }
    QJsonObject readConfig()
    {
        QFile f(m_path);
        if (!f.open(QIODevice::ReadOnly))
            return {};
        return QJsonDocument::fromJson(f.readAll()).object();
    }

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        m_path = AppConfig::defaultPath();
    }
    void init()
    {
        QFile::remove(m_path);
    }
    void cleanup()
    {
        QFile::remove(m_path);
    }

    void defaultsHaveTheCurrentVersion()
    {
        AppConfig c;
        QCOMPARE(c.configVersion(), AppConfig::CurrentConfigVersion);
        QCOMPARE(c.overlayOpacity(), 1.0);
        QCOMPARE(c.cornerRadius(), 16);
        QCOMPARE(c.density(), QStringLiteral("compact"));
        QCOMPARE(c.animations(), true);
        QCOMPARE(c.themeMode(), QStringLiteral("system"));
    }

    void appearanceRoundTrips()
    {
        {
            AppConfig c;
            c.setScale(1.5);
            c.setOverlayOpacity(0.7);
            c.setCornerRadius(22);
            c.setDensity(QStringLiteral("spacious"));
            c.setAnimations(false);
            c.setThemeMode(QStringLiteral("dark"));
            c.setAccentColor(QStringLiteral("#123456"));
            c.setMaxAttachmentSizeMB(64);
            QVERIFY(c.save());
        }
        AppConfig c;
        QCOMPARE(c.scale(), 1.5);
        QCOMPARE(c.overlayOpacity(), 0.7);
        QCOMPARE(c.cornerRadius(), 22);
        QCOMPARE(c.density(), QStringLiteral("spacious"));
        QCOMPARE(c.animations(), false);
        QCOMPARE(c.themeMode(), QStringLiteral("dark"));
        QCOMPARE(c.accentColor(), QStringLiteral("#123456"));
        QCOMPARE(c.maxAttachmentSizeMB(), 64);
        QCOMPARE(c.configVersion(), AppConfig::CurrentConfigVersion);
    }

    void invalidAppearanceValuesAreClamped()
    {
        AppConfig c;
        c.setOverlayOpacity(0.0);
        c.setCornerRadius(999);
        c.setScale(99.0);
        c.setDensity(QStringLiteral("microscopic"));
        QCOMPARE(c.overlayOpacity(), 0.3);
        QCOMPARE(c.cornerRadius(), 40);
        QCOMPARE(c.scale(), 3.0);
        QCOMPARE(c.density(), QStringLiteral("compact"));
    }

    void unknownKeysSurviveASave()
    {
        writeConfig({
            {"serverUrl", "http://old"},
            {"pluginFoo", QJsonObject{{"enabled", true}, {"level", 3}}},
            {"futureKey", "keep me"},
        });
        AppConfig c;
        QCOMPARE(c.serverUrl(), QStringLiteral("http://old"));
        c.setWorkDir(QStringLiteral("/tmp/qute-work"));
        QVERIFY(c.save());

        const QJsonObject after = readConfig();
        QCOMPARE(after.value("futureKey").toString(), QStringLiteral("keep me"));
        QCOMPARE(after.value("pluginFoo").toObject().value("level").toInt(), 3);
        QCOMPARE(after.value("serverUrl").toString(), QStringLiteral("http://old"));
        QCOMPARE(after.value("workDir").toString(), QStringLiteral("/tmp/qute-work"));
        QCOMPARE(after.value("configVersion").toInt(), AppConfig::CurrentConfigVersion);
    }

    void versionOneFileIsMigrated()
    {
        writeConfig({
            {"themeMode", "auto"},
            {"scale", 9.0},
            {"serverUrl", "http://legacy"},
            {"legacyPlugin", true},
        });                                  // no configVersion, no F7 appearance keys
        AppConfig c;
        QCOMPARE(c.configVersion(), AppConfig::CurrentConfigVersion);
        QCOMPARE(c.themeMode(), QStringLiteral("system"));
        QCOMPARE(c.scale(), 3.0);
        QCOMPARE(c.overlayOpacity(), 1.0);
        QCOMPARE(c.cornerRadius(), 16);
        QCOMPARE(c.density(), QStringLiteral("compact"));
        QCOMPARE(c.animations(), true);

        QVERIFY(c.save());
        const QJsonObject after = readConfig();
        QCOMPARE(after.value("configVersion").toInt(), AppConfig::CurrentConfigVersion);
        QCOMPARE(after.value("themeMode").toString(), QStringLiteral("system"));
        QCOMPARE(after.value("legacyPlugin").toBool(), true);
        QVERIFY(after.contains("density"));
        QVERIFY(after.contains("animations"));
    }

    void migrationIsIdempotentForVersionTwo()
    {
        QJsonObject obj{
            {"configVersion", AppConfig::CurrentConfigVersion},
            {"scale", 9.0},
            {"themeMode", "dark"},
            {"custom", 1},
        };
        AppConfig::migrate(obj);
        QCOMPARE(obj.value("scale").toDouble(), 9.0);        // already v2: values are not rewritten
        QCOMPARE(obj.value("themeMode").toString(), QStringLiteral("dark"));
        QCOMPARE(obj.value("custom").toInt(), 1);
        QCOMPARE(obj.value("configVersion").toInt(), AppConfig::CurrentConfigVersion);
    }

    void brokenFileKeepsTheDefaults()
    {
        QDir().mkpath(QFileInfo(m_path).absolutePath());
        QFile f(m_path);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
        f.write("{ not json");
        f.close();

        AppConfig c;
        QCOMPARE(c.serverUrl(), QStringLiteral("http://127.0.0.1:4096"));
        QCOMPARE(c.configVersion(), AppConfig::CurrentConfigVersion);
    }
};

QTEST_APPLESS_MAIN(TstAppConfig)
#include "tst_appconfig.moc"
