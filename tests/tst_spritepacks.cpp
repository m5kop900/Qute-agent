// M7 acceptance, automated: every built-in sprite pack is complete and consistent.
// (tools/validate_packs.py checks the same rules without Qt.)
#include "spritepackmanager.h"

#include <QDir>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest>

static const QStringList kStates = {"idle", "thinking", "working", "waiting", "done", "error", "offline"};

static QJsonObject manifestOf(const QString &packDir)
{
    QFile f(packDir + "/manifest.json");
    return f.open(QIODevice::ReadOnly) ? QJsonDocument::fromJson(f.readAll()).object() : QJsonObject();
}

class TstSpritePacks : public QObject {
    Q_OBJECT
    QStringList packDirs() const
    {
        QStringList out;
        const QDir root(QUTE_DEV_PACKS_DIR);
        for (const QString &sub : root.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
            if (QFile::exists(root.filePath(sub) + "/manifest.json"))
                out << root.filePath(sub);
        return out;
    }
private slots:
    void atLeastFourPacksWithFiveVariants()
    {
        const QStringList dirs = packDirs();
        QVERIFY2(dirs.size() >= 4, qPrintable(QString("found %1 packs").arg(dirs.size())));
        for (const QString &d : dirs)
            QVERIFY2(manifestOf(d).value("variants").toArray().size() >= 5, qPrintable(d));
    }
    void everyPackIsCompleteAndSheetsHaveTheRightSize()
    {
        for (const QString &dir : packDirs()) {
            const QJsonObject m = manifestOf(dir);
            const int fw = m.value("frame").toObject().value("width").toInt();
            const int fh = m.value("frame").toObject().value("height").toInt();
            QVERIFY2(fw > 0 && fh > 0, qPrintable(dir));
            const QJsonObject states = m.value("states").toObject();
            for (const QString &state : kStates) {
                QVERIFY2(states.contains(state), qPrintable(dir + ": state missing: " + state));
                const QJsonObject info = states.value(state).toObject();
                const int frames = info.value("frames").toInt();
                QVERIFY2(frames >= 1 && info.value("fps").toInt() >= 1, qPrintable(dir + " " + state));
                for (const QJsonValue &v : m.value("variants").toArray()) {
                    QString file = info.value("file").toString();
                    file.replace("{variant}", v.toString());
                    const QImage img(dir + "/" + file);
                    const QString what = dir + "/" + file;
                    QVERIFY2(!img.isNull(), qPrintable("cannot load " + what));
                    QVERIFY2(img.size() == QSize(fw * frames, fh), qPrintable("wrong size: " + what));
                    QVERIFY2(img.hasAlphaChannel(), qPrintable("no alpha: " + what));
                    QCOMPARE(qAlpha(img.pixel(0, 0)), 0);                   // transparent corner = no background box
                }
            }
        }
    }
    void allPacksShareTheSameVariantNames()
    {
        QJsonArray ref;
        for (const QString &dir : packDirs()) {
            const QJsonArray v = manifestOf(dir).value("variants").toArray();
            if (ref.isEmpty()) ref = v;
            QVERIFY2(v == ref, qPrintable(dir + ": variants differ from the other packs"));
        }
    }
    void managerResolvesVariantAndFallback()
    {
        SpritePackManager mgr;
        QVERIFY(mgr.availablePacks().size() >= 4);
        QVERIFY(mgr.load("cat", "mint"));
        const QVariantMap working = mgr.stateInfo("working");
        QVERIFY(working.value("source").toUrl().toLocalFile().endsWith("mint/working.png"));
        QCOMPARE(working.value("frames").toInt(), 6);
        // unknown variant -> first variant; unknown state -> idle
        QVERIFY(mgr.load("bot", "does-not-exist"));
        QVERIFY(mgr.stateInfo("no-such-state").value("source").toUrl().toLocalFile().endsWith("ocean/idle.png"));
    }
};

QTEST_GUILESS_MAIN(TstSpritePacks)
#include "tst_spritepacks.moc"
