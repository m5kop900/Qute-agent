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

private:
    void runPlacementScript(int maxHeight);
};
