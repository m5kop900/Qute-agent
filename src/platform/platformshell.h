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
    // The window properties "contentHeight" and "dropRoom" (ints, optional) describe the visible card:
    // the prompt box is the card's bottom edge, contentHeight px below the window top, and dropRoom px
    // of empty room are kept below it (dropdowns). Implementations should put the prompt just below the
    // cursor.
    virtual void showAtCursor(QQuickWindow *window, bool atCursor) = 0;

    // Size the window for a card of `contentHeight` px, keeping the card's bottom edge where it is on
    // screen, so the card grows upward from the prompt box. Called when the card level changes.
    // Default: a plain client-side resize (position-anchored platforms override it if they can).
    virtual void resizeContent(QQuickWindow *window, int contentHeight, int dropRoom);

    static PlatformShell *create(QObject *parent = nullptr);
};
