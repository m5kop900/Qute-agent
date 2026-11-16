#include "overlaycontroller.h"

#include "appconfig.h"
#include "platformshell.h"
#include "terminallauncher.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QProcess>
#include <QQuickWindow>

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
