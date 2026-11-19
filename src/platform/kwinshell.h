#pragma once
#include "platformshell.h"

// KDE Plasma. Wayland clients cannot read the global cursor position or place their own
// windows, so we load a tiny KWin script over D-Bus (org.kde.KWin /Scripting) that reads
// workspace.cursorPos and moves OUR window (matched by its title "qute-overlay").
// If D-Bus support is not compiled in, this falls back to GenericShell behavior.
class KWinShell : public PlatformShell {
    Q_OBJECT
public:
    using PlatformShell::PlatformShell;
    QString name() const override { return QStringLiteral("kwin"); }
    void prepareWindow(QQuickWindow *window) override;
    void showAtCursor(QQuickWindow *window, bool atCursor) override;
    void resizeContent(QQuickWindow *window, int contentHeight, int dropRoom) override;

private:
    void runPlacementScript(QQuickWindow *window, int width, int contentHeight, int dropRoom);
    // Writes `js` to a temp file, loads + runs it as KWin script `name`, then unloads it.
    void runScript(const QString &name, QString js);
    // Empty room below the card that the window is currently sized with (-1 until the first placement).
    int m_dropRoom = -1;
};
