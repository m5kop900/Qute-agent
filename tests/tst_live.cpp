#include "appconfig.h"
#include "opencodeclient.h"
#include "opencodecatalog.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

// Live check against a REAL opencode 2 server. Skipped unless these are set:
//   QUTE_LIVE_URL=http://127.0.0.1:4096   QUTE_LIVE_PASSWORD=...   QUTE_LIVE_DIR=/path/to/a/project
// Start the server with: OPENCODE_PASSWORD=... opencode serve --port 4096
// Checks what the mock server cannot: the real endpoints accept the bodies the client sends.
class TstLive : public QObject {
    Q_OBJECT
private slots:
    void initTestCase()
    {
        if (qEnvironmentVariableIsEmpty("QUTE_LIVE_URL"))
            QSKIP("set QUTE_LIVE_URL (and QUTE_LIVE_PASSWORD) to run against a real opencode 2 server");
    }

    void compactAndPromptWithFileAreAccepted()
    {
        QTemporaryDir fallback;
        const QString dir = qEnvironmentVariable("QUTE_LIVE_DIR", fallback.path());
        const QString file = dir + QStringLiteral("/qute-live-check.txt");
        {
            QFile f(file);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write("qute live check\n");
        }

        AppConfig config;
        config.setServerUrl(qEnvironmentVariable("QUTE_LIVE_URL"));
        config.setServerPassword(qEnvironmentVariable("QUTE_LIVE_PASSWORD"));
        config.setApiFlavor(QStringLiteral("v2"));
        config.setAutoStartServer(false);
        config.setWorkDir(dir);

        OpenCodeClient client(&config);
        OpenCodeCatalog catalog(&client);
        QSignalSpy failed(&client, &OpenCodeClient::requestFailed);
        client.start();
        QTRY_VERIFY_WITH_TIMEOUT(!client.sessionId().isEmpty(), 20000);

        // The "/" list must show the server's project commands (regression: the catalog used to send no location,
        // so the list was always empty) and opencode's built-in /compact.
        const QString command = qEnvironmentVariable("QUTE_LIVE_COMMAND");
        if (!command.isEmpty()) {
            QTRY_VERIFY_WITH_TIMEOUT([&] {
                for (const QVariant &v : catalog.slashItems())
                    if (v.toMap().value("key").toString() == command && v.toMap().value("kind").toString() == "server")
                        return true;
                return false;
            }(), 15000);
        }
        bool hasCompact = false;
        for (const QVariant &v : catalog.slashItems())
            hasCompact = hasCompact || v.toMap().value("key").toString() == QLatin1String("compact");
        QVERIFY(hasCompact);

        // /compact: native endpoint, must not report a failure.
        client.compactSession();
        QTest::qWait(1500);
        QCOMPARE(failed.count(), 0);

        // A prompt with a file: the server reads the file itself (file:// uri). The model may fail later for lack of
        // a provider; only the admission of the prompt is checked here.
        QVariantList files;
        files << QVariantMap{{"uri", QUrl::fromLocalFile(file).toString()}, {"name", "qute-live-check.txt"}};
        QVERIFY(client.sendPromptWithFiles(QStringLiteral("read the attached file"), files, QVariantList{}));
        QTest::qWait(1500);
        QCOMPARE(failed.count(), 0);

        // Optional: a project command, e.g. QUTE_LIVE_COMMAND=hello
        if (!command.isEmpty()) {
            client.runServerCommand(command, QStringLiteral("live"), QVariantList{}, QVariantList{});
            QTest::qWait(1500);
            QCOMPARE(failed.count(), 0);
        }
    }
};

QTEST_GUILESS_MAIN(TstLive)
#include "tst_live.moc"
