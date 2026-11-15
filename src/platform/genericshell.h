#pragma once
#include "platformshell.h"

// Plain Qt behavior: frameless, transparent, always-on-top tool window.
// Positioning with QCursor::pos() + QWindow::setPosition(), which works on Windows and X11
// (XFCE etc.). On pure Wayland compositors without layer-shell/extensions the position is ignored.
class GenericShell : public PlatformShell {
    Q_OBJECT
public:
    using PlatformShell::PlatformShell;
    QString name() const override { return QStringLiteral("generic"); }
    void prepareWindow(QQuickWindow *window) override;
    void showAtCursor(QQuickWindow *window, bool atCursor) override;
};
