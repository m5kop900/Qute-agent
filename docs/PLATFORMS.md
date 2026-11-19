# Platforms

All platform code implements `PlatformShell` (`prepareWindow`, `showAtCursor`, `resizeContent`) in `src/platform/`.
Nothing outside that folder may mention a desktop environment, and the QML never does.

| Platform | Class | Status | Notes |
|---|---|---|---|
| KWin as window manager (Plasma, or LXQt/other + KWin; Wayland/X11) | `KWinShell` | written, untested | KWin script over D-Bus reads `workspace.cursorPos`, moves the window titled `qute-overlay`, sets keepAbove/skipTaskbar. |
| GNOME 45+ (Wayland and X11) | `GnomeShell` | written, untested | Placement through the GNOME Shell extension in `packaging/gnome-shell/` (D-Bus `io.github.qute.Agent`). Falls back to plain `GenericShell` behavior when the extension is not installed. |
| Windows | `GenericShell` | written, untested | `QCursor::pos()` + `setPosition()`; flags Frameless+OnTop+Tool. Packaging: `packaging/windows/`. |
| XFCE (X11), LXQt/Openbox, other X11 WMs | `GenericShell` | written, untested | Same as Windows. X11 lets clients read the cursor and place themselves, so no helper is needed. Needs the xfwm4 compositor for a transparent window. |
| Other Wayland compositors without a helper (sway, Hyprland, ...) | `GenericShell` | written, untested | Wayland ignores client-set positions; the overlay still shows, usually centered by the compositor. A layer-shell helper would be the fix (not implemented). |
| macOS | `MacShell` | written, untested | See docs/MACOS.md. Native NSWindow tweaks in `macshell.mm`, app bundles with LSUIElement, LaunchAgent for autostart. |

## Shortcuts per DE (user binds the command `qute toggle`)
KDE: System Settings → Keyboard → Shortcuts → Add Command · GNOME: Settings → Keyboard → Custom Shortcuts ·
XFCE: Settings → Keyboard → Application Shortcuts · Windows: `packaging/windows/install-shortcuts.ps1 -Hotkey`
or `packaging/windows/qute.ahk` · macOS: Shortcuts app (Run Shell Script) / skhd / Hammerspoon.

## Detection
`PlatformShell::create()` checks, in this order:
1. macOS → `MacShell`.
2. D-Bus service `org.kde.KWin` → `KWinShell` (NOT `XDG_CURRENT_DESKTOP`, so KWin also works under LXQt without
   plasmashell; KWin scripting does not need plasmashell).
3. D-Bus service `org.gnome.Shell` → `GnomeShell`.
4. Everything else → `GenericShell` (Windows, XFCE and other X11 WMs, unknown Wayland compositors).

## GNOME Shell extension (Wayland placement)
Mutter does not let a normal client read the global cursor position or move its own window, so the app talks to a
small GNOME Shell extension over D-Bus. Contract:

| | |
|---|---|
| Bus name / interface | `io.github.qute.Agent` |
| Object path | `/io/github/qute/Agent` |
| Methods | `PlaceWindow(width, contentHeight, dropRoom)`, `ResizeWindow(contentHeight, dropRoom)` |
| Window lookup | window title `qute-overlay` (same contract as the KWin script) |

Install for the current user:
```
packaging/gnome-shell/install.sh
# or, from an installed package:
/usr/share/qute/gnome-shell/install.sh
```
On Wayland, log out and back in (GNOME loads extensions at login), then check
`gnome-extensions info qute@qute-agent`. X11 sessions pick the extension up immediately.

The extension mirrors the KWin script: it finds the window, reads `global.get_pointer()`, clamps x to the work area
of the monitor under the cursor, puts the prompt box just below the cursor, calls `make_above()` and `stick()`, then
activates the window. Without the extension `GnomeShell` falls back to `GenericShell`, which positions correctly on
X11 and is ignored on Wayland (the overlay is still usable, just not placed at the cursor).

Tested shell versions: `metadata.json` lists 45–50. GNOME has no stable JS API, so a new major release may need a
small fix in `extension.js`; keep the manifest list in sync.

## Windows
See `packaging/windows/README.md` for build, portable zip (`package.ps1`), Start Menu / hotkey / autostart
(`install-shortcuts.ps1`, `qute.ahk`) and the GUI-subsystem console note. Nothing has been built or run on Windows
yet; `GenericShell` positioning, transparency and DPI scaling are the first things to verify.

## XFCE
XFCE is X11: `GenericShell` is enough. Requirements:
- Enable the compositor (Settings → Window Manager Tweaks → Compositor) or the transparent window will show a black
  border/background.
- Bind a key in Settings → Keyboard → Application Shortcuts to `qute toggle`.
- Autostart: Settings → Session and Startup → Application Autostart → `qute --daemon`.
- `skipTaskbar` comes from the `Qt::Tool` flag; if the panel still lists the overlay, add a WM rule for
  `qute-overlay` in `xfwm4` settings.

## KWin debugging
- Placement script is written to `/tmp/qute-kwin-place.js` and loaded as `qute-place`.
- Watch logs: `journalctl --user -f | grep -i kwin`. Test the script manually: `plasma-interactiveconsole --kwin`.
- If `/Scripting/Script<id>` does not exist on the user's Plasma version, the code falls back to `Scripting.start`;
  check with `qdbus org.kde.KWin /Scripting` which paths exist.
- Alternative without scripts: KWin window rule (Placement: Under Mouse, Keep Above, Skip Taskbar, No titlebar) for title `qute-overlay`.
