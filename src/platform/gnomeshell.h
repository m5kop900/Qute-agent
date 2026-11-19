#pragma once
#include "genericshell.h"

// GNOME (Wayland and X11). Mutter does not let a normal client read the global cursor position
// or place its own window, so a small GNOME Shell extension (packaging/gnome-shell) owns the
// D-Bus name io.github.qute.Agent and does the placement for us.
//
// Without the extension this falls back to GenericShell: positioning works on X11 and is ignored
// by Wayland, which is the best a plain client can do there. The window is still usable.
class GnomeShell : public GenericShell {
    Q_OBJECT
public:
    using GenericShell::GenericShell;
    QString name() const override { return QStringLiteral("gnome"); }
    void prepareWindow(QQuickWindow *window) override;
    void showAtCursor(QQuickWindow *window, bool atCursor) override;
    void resizeContent(QQuickWindow *window, int contentHeight, int dropRoom) override;
};
