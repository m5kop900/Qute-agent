#include "overlaycontrol.h"
#include "singleinstance.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QStringList>
#include <QProcess>
#include <QThread>

OverlayControl::OverlayControl(QObject *parent) : QObject(parent) {}

// Must match the overlay's ipcName() in src/overlay/main.cpp.
QString OverlayControl::ipcName()
{
    const QString user = qEnvironmentVariable("USER", qEnvironmentVariable("USERNAME", QStringLiteral("user")));
    return QStringLiteral("qute-overlay-") + user;
}

// The `qute` executable: next to this app (installed or in the same build folder), in the build tree
// (build/src/overlay/qute), or the one from PATH.
QString OverlayControl::quteExecutable()
{
#ifdef Q_OS_WIN
    const QString name = QStringLiteral("qute.exe");
#else
    const QString name = QStringLiteral("qute");
#endif
    const QString dir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        dir + QLatin1Char('/') + name,
        dir + QStringLiteral("/../overlay/") + name,
        dir + QStringLiteral("/../../overlay/") + name,
    };
    for (const QString &c : candidates) {
        if (QFileInfo::exists(c))
            return QFileInfo(c).canonicalFilePath();
    }
    return name;
}

bool OverlayControl::isRunning() const
{
    return SingleInstance::send(ipcName(), QStringLiteral("ping"));
}

bool OverlayControl::restart()
{
    const QString exe = quteExecutable();
    QProcess::execute(exe, {QStringLiteral("quit")});

    // Wait for the old daemon to really exit. If it is still answering, a new daemon would just hand
    // over to it and quit again, which looked like "closed and never came back".
    QElapsedTimer waited;
    waited.start();
    while (isRunning()) {
        if (waited.elapsed() > 8000)
            return false;
        QThread::msleep(100);
    }

    return QProcess::startDetached(exe, {QStringLiteral("--daemon")});
}
