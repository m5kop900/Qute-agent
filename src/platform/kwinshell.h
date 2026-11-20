#pragma once
#include "platformshell.h"

#include <QPointer>
#include <QQuickWindow>

// KDE Plasma. Wayland clients cannot read the global cursor position or place their own
// windows, so we load a tiny KWin script over D-Bus (org.kde.KWin /Scripting) that reads
// workspace.cursorPos and moves OUR window (matched by its title "qute-overlay").
// If D-Bus support is not compiled in, this falls back to GenericShell behavior.
class KWinShell : public PlatformShell {
    Q_OBJECT
    // Smart chatbox positioning: the KWin script decides inside the compositor (a client cannot read its own
    // global position or the work area on Wayland) and reports the layout back with callDBus.
    Q_CLASSINFO("D-Bus Interface", "io.github.qute.Overlay")
public:
    using PlatformShell::PlatformShell;
    QString name() const override { return QStringLiteral("kwin"); }
    void prepareWindow(QQuickWindow *window) override;
    void showAtCursor(QQuickWindow *window, bool atCursor) override;
    void resizeContent(QQuickWindow *window, int contentHeight, int dropRoom) override;

public slots:
    // Called by the KWin script through D-Bus: the layout it chose (ChatboxPlacement::toWire).
    Q_SCRIPTABLE void LayoutReady(const QString &wire);

private:
    void dispatchSmart(bool place, int contentHeight, int dropRoom);
    void smartFinished();
    QPointer<QQuickWindow> m_window;
    bool m_smartBusy = false;
    bool m_revealOnLayout = false;
    int m_smartSeq = 0;
    struct PendingSmart {
        bool valid = false;
        bool place = false;
        int contentHeight = 0;
        int dropRoom = 0;
    } m_smartPending;
    void runPlacementScript(QQuickWindow *window, int width, int contentHeight, int dropRoom);
    // Writes `js` to a temp file, loads + runs it as KWin script `name`, then unloads it.
    void runScript(const QString &name, QString js);
    // Empty room below the card that the window is currently sized with (-1 until the first placement).
    int m_dropRoom = -1;
};
