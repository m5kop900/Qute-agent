# Qute Agent

A tiny animated desktop companion for [opencode](https://opencode.ai). Press a shortcut → a sprite
appears next to your cursor with a prompt box; the sprite reacts to what the agent is doing.

- C++20 / Qt 6 / QML, one codebase: Linux (KDE first), macOS (docs/MACOS.md), Windows (docs/PLATFORMS.md), GNOME and XFCE
- Two apps: `qute` (overlay) and `qute-settings`
- Sprite packs (multiple characters × color variants) are plain folders with a `manifest.json`

## Build
```
cmake -S . -B build -G Ninja && cmake --build build && ctest --test-dir build
```
Run: `./build/src/overlay/qute --daemon`, then `./build/src/overlay/qute toggle`.
macOS: `brew install qt cmake ninja` and add `-DCMAKE_PREFIX_PATH="$(brew --prefix qt)"` (details: docs/MACOS.md).
Windows: `packaging/windows/README.md`. Arch: `packaging/aur/qute-agent-git` (`makepkg -si`).

## Install
```
sudo cmake --install build --prefix /usr     # binaries, sprite packs, desktop entries, icons, docs
packaging/gnome-shell/install.sh             # GNOME only: placement extension (log out/in on Wayland)
```
The install step never touches global shortcuts or autostart; those stay a user choice (see Shortcut).

## Setup wizard
`qute opencode setup` finds or installs opencode (V1 or V2), adds MCP servers (Playwright, Context7, ...), and lets you add any
OpenAI-compatible endpoint (llama.cpp, vLLM, LM Studio, ...). `qute opencode doctor` diagnoses problems. See `docs/SETUP_WIZARD.md`.

## Shortcut
Qute Agent has no built-in global shortcut. Bind the command `qute toggle` in your desktop:
KDE: System Settings → Keyboard → Shortcuts → Add Command · GNOME: Settings → Keyboard → Custom Shortcuts ·
XFCE: Settings → Keyboard → Application Shortcuts · Windows: `packaging/windows/install-shortcuts.ps1 -Hotkey "CTRL+ALT+Q"`
or `packaging/windows/qute.ahk` · macOS: Shortcuts app → Run Shell Script.

Autostart (optional): run `qute --daemon` at login (KDE: Autostart; GNOME/XFCE: Startup Applications;
Windows: `install-shortcuts.ps1 -Autostart`; macOS: the LaunchAgent in `packaging/macos/`).

## Docs
`docs/MACOS.md` · `docs/SETUP_WIZARD.md` · `docs/ARCHITECTURE.md` · `docs/OPENCODE.md` · `docs/SPRITE_PACK.md` · `docs/PLATFORMS.md` · `docs/ROADMAP.md` · `AGENTS.md`

## Status
M0–M4 done (verified on KDE Wayland and OpenCode 2.2.6), M7 sprite packs done (4 characters x 7 states x 5 colors, validated by
`ctest`), M8 packaging done on Linux and written for macOS/Windows/GNOME. Open: M5 (V2 tool events, permissions), M2b (wizard
never run for real), M6/M7 live-switch check in the GUI, and live runs on macOS/Windows/GNOME.
Details and evidence per milestone: `docs/ROADMAP.md` ("Review status").
