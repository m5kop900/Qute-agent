#include "appconfig.h"
#include "attachmentmodel.h"

#include <QDir>
#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <QUrl>
#include <QtTest>

// F1.1 / F1.2: attachment list, validation and the prompt payload.
class TstAttachments : public QObject {
    Q_OBJECT
private:
    static void writeFile(const QString &path, const QByteArray &bytes)
    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(bytes);
    }

private slots:
    void settingDefaultsAndClamps()
    {
        AppConfig config;
        QCOMPARE(config.maxAttachmentSizeMB(), 0);     // 0 = no limit
        config.setMaxAttachmentSizeMB(25);
        QCOMPARE(config.maxAttachmentSizeMB(), 25);
        config.setMaxAttachmentSizeMB(-5);
        QCOMPARE(config.maxAttachmentSizeMB(), 0);
    }

    void addTextFileAsFileUri()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("notes.txt"));
        writeFile(path, "hello\n");
        AppConfig config;
        AttachmentModel model(&config);

        QCOMPARE(model.addFile(QUrl::fromLocalFile(path).toString()), QString());
        QCOMPARE(model.count(), 1);
        const QVariantList files = model.promptFiles();
        QCOMPARE(files.size(), 1);
        QVERIFY(files.first().toMap().value("uri").toString().startsWith(QStringLiteral("file://")));
        QCOMPARE(files.first().toMap().value("name").toString(), QStringLiteral("notes.txt"));
        QCOMPARE(model.data(model.index(0), AttachmentModel::IsImageRole).toBool(), false);
    }

    void sameFileTwiceIsANoOp()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("a.md"));
        writeFile(path, "# a\n");
        AppConfig config;
        AttachmentModel model(&config);
        QCOMPARE(model.addFile(path), QString());
        QCOMPARE(model.addFile(path), QString());
        QCOMPARE(model.count(), 1);
    }

    void missingFileIsReported()
    {
        AppConfig config;
        AttachmentModel model(&config);
        const QString err = model.addFile(QStringLiteral("/nonexistent/dir/missing.txt"));
        QVERIFY(err.contains(QLatin1String("not found")));
        QCOMPARE(model.count(), 0);
    }

    void folderIsRefused()
    {
        QTemporaryDir dir;
        AppConfig config;
        AttachmentModel model(&config);
        QVERIFY(model.addFile(dir.path()).contains(QLatin1String("Folders")));
        QCOMPARE(model.count(), 0);
    }

    void unsupportedTypeIsRefused()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("blob.bin"));
        writeFile(path, QByteArray("\x00\x01\x02\xff\xfe\x00\x10", 7));
        AppConfig config;
        AttachmentModel model(&config);
        QVERIFY(model.addFile(path).contains(QLatin1String("Unsupported file type")));
        QCOMPARE(model.count(), 0);
    }

    void sizeLimitIsEnforced()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("big.txt"));
        writeFile(path, QByteArray(2 * 1024 * 1024, 'x'));   // 2 MiB
        AppConfig config;
        config.setMaxAttachmentSizeMB(1);
        AttachmentModel model(&config);
        QVERIFY(model.addFile(path).contains(QLatin1String("too large")));
        QCOMPARE(model.count(), 0);

        config.setMaxAttachmentSizeMB(0);        // no limit
        QCOMPARE(model.addFile(path), QString());
        QCOMPARE(model.count(), 1);
    }

    void mentionResolvesAgainstBaseDir()
    {
        QTemporaryDir dir;
        QDir(dir.path()).mkdir(QStringLiteral("src"));
        writeFile(dir.filePath(QStringLiteral("src/main.cpp")), "int main() {}\n");
        AppConfig config;
        AttachmentModel model(&config);
        QCOMPARE(model.addMention(QStringLiteral("src/main.cpp"), dir.path()), QString());
        QCOMPARE(model.count(), 1);
        QVERIFY(model.promptFiles().first().toMap().value("uri").toString().endsWith(QStringLiteral("/src/main.cpp")));
        QCOMPARE(model.promptFiles().first().toMap().value("name").toString(), QStringLiteral("src/main.cpp"));
    }

    void pastedImageIsInlineDataUri()
    {
        QImage image(4, 4, QImage::Format_RGB32);
        image.fill(Qt::red);
        AppConfig config;
        AttachmentModel model(&config);
        QCOMPARE(model.attachImage(image), QString());
        QCOMPARE(model.count(), 1);
        QCOMPARE(model.data(model.index(0), AttachmentModel::SourceTypeRole).toString(), QStringLiteral("inline"));
        QVERIFY(model.data(model.index(0), AttachmentModel::UriRole).toString()
                    .startsWith(QStringLiteral("data:image/png;base64,")));
        QVERIFY(model.data(model.index(0), AttachmentModel::IsImageRole).toBool());
        QVERIFY(model.promptFiles().first().toMap().value("name").toString().endsWith(QStringLiteral(".png")));
    }

    void emptyImageIsRefused()
    {
        AppConfig config;
        AttachmentModel model(&config);
        QVERIFY(!model.attachImage(QImage()).isEmpty());
        QCOMPARE(model.count(), 0);
    }

    void agentRefsAreNameObjects()
    {
        AppConfig config;
        AttachmentModel model(&config);
        const QVariantList refs = model.agentRefs({QStringLiteral("build"), QString(), QStringLiteral("plan")});
        QCOMPARE(refs.size(), 2);
        QCOMPARE(refs.at(1).toMap().value("name").toString(), QStringLiteral("plan"));
    }

    void removeAndClear()
    {
        QTemporaryDir dir;
        const QString a = dir.filePath(QStringLiteral("a.txt"));
        const QString b = dir.filePath(QStringLiteral("b.txt"));
        writeFile(a, "a"); writeFile(b, "b");
        AppConfig config;
        AttachmentModel model(&config);
        model.addFile(a);
        model.addFile(b);
        QCOMPARE(model.count(), 2);
        model.remove(0);
        QCOMPARE(model.count(), 1);
        QCOMPARE(model.data(model.index(0), AttachmentModel::NameRole).toString(), QStringLiteral("b.txt"));
        model.clear();
        QCOMPARE(model.count(), 0);
    }

    void supportedMimeTypes()
    {
        QVERIFY(AttachmentModel::isSupportedMime(QStringLiteral("text/plain")));
        QVERIFY(AttachmentModel::isSupportedMime(QStringLiteral("image/png")));
        QVERIFY(AttachmentModel::isSupportedMime(QStringLiteral("application/pdf")));
        QVERIFY(!AttachmentModel::isSupportedMime(QStringLiteral("application/x-executable")));
        QVERIFY(!AttachmentModel::isSupportedMime(QStringLiteral("application/octet-stream")));
    }

    void fileBrowserListsOnlySupportedFiles()
    {
        // The in-app "+" browser: folders first, then files Qute can send; hidden and unsupported files are left out.
        QTemporaryDir dir;
        QDir(dir.path()).mkdir(QStringLiteral("sub"));
        writeFile(dir.filePath(QStringLiteral("notes.txt")), "hello\n");
        writeFile(dir.filePath(QStringLiteral("picture.png")), QByteArray("\x89PNG\r\n\x1a\n", 8));
        writeFile(dir.filePath(QStringLiteral("program.exe")), QByteArray("MZ\0\0", 4));
        writeFile(dir.filePath(QStringLiteral(".secret.txt")), "hidden\n");
        AppConfig config;
        AttachmentModel model(&config);

        const QVariantList items = model.listDirectory(dir.path());
        QStringList labels;
        for (const QVariant &v : items)
            labels << v.toMap().value("label").toString();
        QVERIFY(labels.contains(QStringLiteral("sub/")));
        QVERIFY(labels.contains(QStringLiteral("notes.txt")));
        QVERIFY(!labels.contains(QStringLiteral("program.exe")));   // unsupported type: not offered
        QVERIFY(!labels.contains(QStringLiteral(".secret.txt")));   // hidden
        // Folders come before files.
        QCOMPARE(items.first().toMap().value("kind").toString(), QStringLiteral("dir"));
    }

    void fileBrowserOffersParentFolderAndPlaceholders()
    {
        QTemporaryDir dir;
        AppConfig config;
        AttachmentModel model(&config);
        const QVariantList empty = model.listDirectory(dir.path());
        QCOMPARE(empty.last().toMap().value("kind").toString(), QStringLiteral("none"));   // "no supported files here"
        const QVariantList missing = model.listDirectory(dir.filePath(QStringLiteral("does-not-exist")));
        QCOMPARE(missing.size(), 1);
        QCOMPARE(missing.first().toMap().value("kind").toString(), QStringLiteral("none"));
    }
};

QTEST_MAIN(TstAttachments)
#include "tst_attachments.moc"
