#pragma once
#include <QObject>
#include <QString>

#include "chatboxplacement.h"

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

    // Called once the window is at its final position (or cannot be moved at all): shows its content.
    // The window stays content-hidden from the moment it is first shown until this runs.
    static void reveal(QQuickWindow *window);

    static PlatformShell *create(QObject *parent = nullptr);

    // ---- Smart chatbox positioning (setting "smartChatboxPositioning", see docs/ARCHITECTURE.md) -----------
    // The window holds the sprite, the card and the drop room. With the setting on, the shell decides where the
    // card goes AROUND the sprite (src/core/chatboxplacement) and never moves the sprite to make it fit. The
    // window property "smartPlacement" says whether it is on; the QML window publishes the sizes below.
    struct SmartInputs {
        int spriteSize = 0;
        int cardW = 0;          // 0 (with cardH 0): minimized, the window is the sprite alone
        int cardH = 0;
        int dropRoom = 0;
        int gap = 4;
        int spriteOffX = 0;     // where the sprite is inside the window right now
        int spriteOffY = 0;
    };
    static bool smartEnabled(QQuickWindow *window);
    // contentHeight/dropRoom are what the QML card asked for (shownHeight/shownDrop).
    static SmartInputs smartInputs(QQuickWindow *window, int contentHeight, int dropRoom);
    // Publishes the chosen layout to the QML window (properties layout*). Returns false for a bad wire string.
    static bool applyLayoutWire(QQuickWindow *window, const QString &wire);
    static void applyLayout(QQuickWindow *window, const ChatboxPlacement::Result &r);
};
