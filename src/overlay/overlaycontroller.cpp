#include "overlaycontroller.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>

#include "appconfig.h"
#include "platformshell.h"
#include "terminallauncher.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QProcess>
#include <QQuickWindow>
#include <QRegion>

OverlayController::OverlayController(PlatformShell *shell, AppConfig *config, QObject *parent)
    : QObject(parent), m_shell(shell), m_config(config)
{
    m_focusGrace.setSingleShot(true);
    m_focusGrace.setInterval(700);

    connect(qApp, &QGuiApplication::focusWindowChanged, this, [this](QWindow *w) {
        if (m_window && m_window->isVisible() && w != m_window.data()
            && m_config->hideOnFocusLoss() && !m_focusGrace.isActive())
            hide();
    });
}

void OverlayController::setWindow(QQuickWindow *window)
{
    m_window = window;
}

void OverlayController::show()
{
    if (!m_window)
        return;
    m_focusGrace.start();
    emit shown();                                     // QML resizes for the current state BEFORE placement
    m_shell->showAtCursor(m_window, m_config->placeAtCursor());
}

void OverlayController::setInputRegion(int x, int y, int w, int h)
{
    if (!m_window)
        return;
    m_window->setMask(w > 0 && h > 0 ? QRegion(x, y, w, h) : QRegion());
}

void OverlayController::resizeToContent(int contentHeight, int dropRoom)
{
    if (!m_window || contentHeight <= 0)
        return;
    m_shell->resizeContent(m_window, contentHeight, dropRoom);
}

void OverlayController::hide()
{
    if (m_window)
        m_window->hide();
}

void OverlayController::toggle()
{
    if (m_window && m_window->isVisible())
        hide();
    else
        show();
}

void OverlayController::quit()
{
    QGuiApplication::quit();
}

void OverlayController::runSetup()
{
    hide();
    if (!TerminalLauncher::launch(QCoreApplication::applicationFilePath(), {QStringLiteral("opencode"), QStringLiteral("setup")}))
        qWarning("qute: no terminal emulator found. Run `qute opencode setup` in a terminal yourself.");
}

bool OverlayController::launchSettingsApp()
{
    const QString local = QCoreApplication::applicationDirPath() + QStringLiteral("/qute-settings");
    return QProcess::startDetached(local, {}) || QProcess::startDetached(QStringLiteral("qute-settings"), {});
}

void OverlayController::runSettings()
{
    hide();
    if (!launchSettingsApp())
        qWarning("qute: could not start qute-settings");
}

QString OverlayController::setWorkDirectory(const QString &path)
{
    QString p = path.trimmed();
    if (p == QLatin1String("~") || p.startsWith(QLatin1String("~/")))
        p = QDir::homePath() + p.mid(1);
    const QFileInfo fi(p);
    if (p.isEmpty() || !fi.exists() || !fi.isDir())
        return tr("Directory not found: %1").arg(path.trimmed());
    m_config->setWorkDir(fi.canonicalFilePath());
    if (!m_config->save())
        return tr("Could not save the working directory to the settings file.");
    return QString();
}

QString OverlayController::openInOpencode(const QString &sessionId, const QString &directory)
{
    // Session ids come from the server; anything else must never reach a command line.
    static const QRegularExpression idPattern(QStringLiteral("^[A-Za-z0-9_-]+$"));
    if (!idPattern.match(sessionId).hasMatch())
        return tr("There is no active session to open in opencode yet.");
    const QString dir = directory.isEmpty() ? m_config->workDir() : directory;
    if (dir.isEmpty() || !QFileInfo(dir).isDir())
        return tr("Working directory of this session not found: %1").arg(dir);
    if (QStandardPaths::findExecutable(QStringLiteral("opencode")).isEmpty())
        return tr("The opencode CLI was not found. Install it, or run `qute opencode install`.");
    // The real TUI, attached to this exact session, started in the session's directory. No shell string is built.
    if (!TerminalLauncher::launch(QStringLiteral("opencode"), {QStringLiteral("--session"), sessionId}, dir))
        return tr("No terminal emulator was found to open opencode in.");
    return QString();
}
