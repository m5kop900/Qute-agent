#pragma once
#include <QObject>
#include <QPointer>
#include <QTimer>

class AppConfig;
class PlatformShell;
class QQuickWindow;

// Bridge between "commands" (IPC, QML buttons) and the overlay window.
class OverlayController : public QObject {
    Q_OBJECT
public:
    OverlayController(PlatformShell *shell, AppConfig *config, QObject *parent = nullptr);

    // The window only exists after the QML engine loaded; QML needs this object BEFORE that.
    void setWindow(QQuickWindow *window);

    Q_INVOKABLE void show();
    Q_INVOKABLE void hide();
    Q_INVOKABLE void toggle();
    Q_INVOKABLE void quit();
    Q_INVOKABLE void runSetup();     // opens a terminal running `qute opencode setup`
    Q_INVOKABLE void runSettings();  // opens the separate settings app
    // Limits mouse input to a rectangle of the window (the window keeps its size, the rest of it
    // is transparent and lets clicks pass through). Empty w/h clears the limit.
    Q_INVOKABLE void setInputRegion(int x, int y, int w, int h);
    // Locates and starts qute-settings; also used by `qute settings`.
    static bool launchSettingsApp();

signals:
    void shown();     // QML focuses the prompt box and resets its size on this

private:
    QPointer<QQuickWindow> m_window;
    PlatformShell *m_shell;
    AppConfig *m_config;
    QTimer m_focusGrace;   // ignore focus-loss for a moment right after show()
};
