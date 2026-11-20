#pragma once
#include "genericshell.h"

#include <QPointer>
#include <QQuickWindow>

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

private:
    // Smart chatbox positioning through the extension (PlaceWindowSmart / ResizeWindowSmart). One request at a
    // time: the sprite's place inside the window is only known once the previous answer has arrived, so a
    // request that comes in meanwhile waits (the latest one wins) and runs with fresh numbers.
    void dispatchSmart(QQuickWindow *window, bool place, int contentHeight, int dropRoom);
    bool m_smartBusy = false;
    struct PendingSmart {
        QPointer<QQuickWindow> window;
        bool valid = false;
        bool place = false;
        int contentHeight = 0;
        int dropRoom = 0;
    } m_smartPending;
};
