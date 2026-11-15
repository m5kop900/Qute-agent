#include "macshell.h"

#include <QQuickWindow>

#import <Cocoa/Cocoa.h>

static NSWindow *nativeWindow(QQuickWindow *w)
{
    NSView *view = reinterpret_cast<NSView *>(w->winId());   // forces creation of the native window
    return view ? view.window : nil;
}

void MacShell::prepareWindow(QQuickWindow *window)
{
    // Also set by LSUIElement in the app bundle; this covers running the bare binary from the build tree.
    [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
    window->setFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    window->setColor(Qt::transparent);
}

void MacShell::showAtCursor(QQuickWindow *window, bool atCursor)
{
    GenericShell::showAtCursor(window, atCursor);            // position near cursor, show, raise

    NSWindow *nw = nativeWindow(window);
    if (!nw)
        return;
    [nw setOpaque:NO];
    [nw setBackgroundColor:[NSColor clearColor]];
    [nw setHasShadow:NO];
    [nw setLevel:NSStatusWindowLevel];
    [nw setCollectionBehavior:NSWindowCollectionBehaviorCanJoinAllSpaces
                              | NSWindowCollectionBehaviorFullScreenAuxiliary
                              | NSWindowCollectionBehaviorIgnoresCycle];

    if (@available(macOS 14.0, *))
        [NSApp activate];
    else
        [NSApp activateIgnoringOtherApps:YES];
    [nw makeKeyAndOrderFront:nil];
}
