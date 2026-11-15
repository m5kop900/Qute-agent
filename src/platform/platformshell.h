#pragma once
#include <QObject>
#include <QString>

class QQuickWindow;

// Everything that differs per OS / desktop environment goes behind this interface.
// QML and the rest of the app must NEVER contain KDE / Windows / GNOME specific code.
//
// To add a platform: subclass, implement the 3 virtuals, register it in PlatformShell::create().
class PlatformShell : public QObject {
    Q_OBJECT
public:
    explicit PlatformShell(QObject *parent = nullptr) : QObject(parent) {}

    virtual QString name() const = 0;

    // Called ONCE before the window is first shown: window flags, transparency, skip-taskbar...
    virtual void prepareWindow(QQuickWindow *window) = 0;

    // Show + focus the window. If atCursor, place it near the mouse cursor (best effort).
    // The window property "maxHeight" (int, optional) is the tallest size the window can grow to;
    // implementations should keep that much room on screen.
    virtual void showAtCursor(QQuickWindow *window, bool atCursor) = 0;

    static PlatformShell *create(QObject *parent = nullptr);
};
