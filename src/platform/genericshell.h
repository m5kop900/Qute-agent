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
    void resizeContent(QQuickWindow *window, int contentHeight, int dropRoom) override;

protected:
    // Smart chatbox positioning decided in C++ (Windows, macOS, X11): places the sprite (at the cursor, or where it
    // is now) and the card around it, then publishes the layout to the QML window.
    static void smartPlace(QQuickWindow *window, bool atCursor, int contentHeight, int dropRoom);
};
