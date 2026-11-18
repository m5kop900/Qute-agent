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
    // Hidden until the shell has moved the window to the cursor. Only a window that was not on screen
    // needs this: an already visible one is simply moved, and nothing flashes.
    if (!m_window->isVisible())
        m_window->setProperty("placed", false);
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
    // The terminal starts in the session's folder. attach uses the server's session, so when the folder is
    // unknown we fall back to the home folder instead of refusing.
    QString dir = directory;
    if (dir.isEmpty())
        dir = m_config->workDir();
    if (dir.isEmpty())
        dir = QDir::homePath();
    if (!QFileInfo(dir).isDir())
        return tr("This session's folder (%1) is not on this computer, so opencode cannot open it here.").arg(dir);
    if (QStandardPaths::findExecutable(QStringLiteral("opencode")).isEmpty())
        return tr("The opencode CLI was not found. Install it, or run `qute opencode install`.");
    const QString url = m_config->serverUrl();
    if (url.isEmpty())
        return tr("The opencode server address is not set in the settings.");
    // The real TUI, attached to the server Qute already uses and to this exact session. A plain
    // `opencode --session` would start a second server inside the TUI and not see Qute's session.
    // Credentials go through the environment, not argv: argv is visible to every user in the process list,
    // and the documented variables are OPENCODE_SERVER_USERNAME / OPENCODE_SERVER_PASSWORD.
    // V2 reads the password from OPENCODE_PASSWORD (the variable the V2 client documents for --server);
    // the other name is set too so either edition finds it.
    if (!m_config->serverPassword().isEmpty()) {
        qputenv("OPENCODE_PASSWORD", m_config->serverPassword().toUtf8());
        qputenv("OPENCODE_SERVER_PASSWORD", m_config->serverPassword().toUtf8());
    }
    // Run through sh so that when attach fails the terminal stays open and shows the exit code,
    // instead of closing at once and leaving nothing to read.
    auto quote = [](const QString &v) {
        QString out = v;
        out.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
        return QLatin1Char('\'') + out + QLatin1Char('\'');
    };
    // V2 client: `opencode --server <url> --session <id>`. The V1 client is the one that has `attach`.
    const bool v1 = m_config->apiFlavor() == QLatin1String("v1");
    const QString command = v1
        ? QStringLiteral("opencode attach %1 --session %2").arg(quote(url), quote(sessionId))
        : QStringLiteral("opencode --server %1 --session %2").arg(quote(url), quote(sessionId));
    const QString script = QStringLiteral(
        "%1; code=$?; "
        "if [ $code -ne 0 ]; then echo; echo \"opencode exited with code $code. Press Enter to close.\"; read _; fi")
        .arg(command);
    if (!TerminalLauncher::launch(QStringLiteral("sh"), {QStringLiteral("-c"), script}, dir))
        return tr("No terminal emulator was found to open opencode in.");
    return QString();
}
