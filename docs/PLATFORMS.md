# Platforms

All platform code implements `PlatformShell` (`prepareWindow`, `showAtCursor`) in `src/platform/`.

| Platform | Class | Status | Notes |
|---|---|---|---|
| KWin as window manager (Plasma, or LXQt/other + KWin; Wayland/X11) | `KWinShell` | written, untested | KWin script over D-Bus reads `workspace.cursorPos`, moves window titled `qute-overlay`, sets keepAbove/skipTaskbar. |
| Windows / XFCE(X11) / unknown | `GenericShell` | written, untested | `QCursor::pos()` + `setPosition()`; flags Frameless+OnTop+Tool. |
| GNOME (Wayland) | — | TODO | Mutter denies cursor position & placement to clients. Plan: tiny GNOME Shell extension (JS) that places the window and exposes cursor pos over D-Bus; app calls it. Always-on-top also needs the extension. |
| macOS | `MacShell` | written, untested | See docs/MACOS.md. Native NSWindow tweaks in `macshell.mm`, app bundles with LSUIElement, LaunchAgent for autostart. |
| Windows extras | — | TODO | Hotkey: ship a `.lnk` with hotkey or doc AutoHotkey. Optional tray icon. |

## Shortcuts per DE (user binds the command `qute toggle`)
KDE: System Settings → Keyboard → Shortcuts → Add Command · GNOME: Settings → Keyboard → Custom Shortcuts ·
XFCE: Settings → Keyboard → Application Shortcuts · Windows: `.lnk` hotkey / AutoHotkey · macOS: Shortcuts app (Run Shell Script) / skhd / Hammerspoon.

## KWin debugging
- Placement script is written to `/tmp/qute-kwin-place.js` and loaded as `qute-place`.
- Watch logs: `journalctl --user -f | grep -i kwin`. Test the script manually: `plasma-interactiveconsole --kwin`.
- If `/Scripting/Script<id>` does not exist on the user's Plasma version, the code falls back to `Scripting.start`;
  check with `qdbus org.kde.KWin /Scripting` which paths exist.
- Alternative without scripts: KWin window rule (Placement: Under Mouse, Keep Above, Skip Taskbar, No titlebar) for title `qute-overlay`.

## Detection
`PlatformShell::create()` picks `KWinShell` when the D-Bus service `org.kde.KWin` exists (NOT by XDG_CURRENT_DESKTOP),
so it also works with KWin under LXQt without plasmashell. KWin scripting does not need plasmashell.
