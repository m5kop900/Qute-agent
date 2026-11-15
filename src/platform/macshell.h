#pragma once
#include "genericshell.h"

// macOS: GenericShell placement (QCursor::pos works on macOS) + native NSWindow tweaks done in macshell.mm:
//  - accessory app (no Dock icon / menu bar) so `qute toggle` never steals a Dock slot
//  - floats above normal windows and over full-screen apps, on every Space
//  - really becomes the active app + key window so the prompt box gets keyboard focus
// Note: NOT Qt::Tool - Qt turns that into a panel that hides itself whenever the app is inactive.
class MacShell : public GenericShell {
    Q_OBJECT
public:
    using GenericShell::GenericShell;
    QString name() const override { return QStringLiteral("macos"); }
    void prepareWindow(QQuickWindow *window) override;
    void showAtCursor(QQuickWindow *window, bool atCursor) override;
};
