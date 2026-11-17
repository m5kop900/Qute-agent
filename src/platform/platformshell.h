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
    // The window properties "contentHeight" and "maxContentHeight" (ints, optional) describe the
    // visible card: it starts at the window's top edge and the prompt box sits contentHeight below
    // it. Implementations should put the prompt just below the cursor and keep room for
    // maxContentHeight so the card never grows off the screen.
    virtual void showAtCursor(QQuickWindow *window, bool atCursor) = 0;

    static PlatformShell *create(QObject *parent = nullptr);
};
