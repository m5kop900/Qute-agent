# AGENTS.md — instructions for the coding agent working on Qute Agent

Qute Agent is a desktop assistant: a global command (`qute toggle`, bound to a DE shortcut) shows an animated
sprite with a prompt box next to the mouse cursor. Prompts go to a local `opencode serve`; the sprite's
behavior follows what opencode is doing. Two executables: `qute` (overlay/daemon + CLI) and
`qute-settings` (separate settings app). Platforms: Linux (KDE first), macOS 12+ (supported from the start, docs/MACOS.md), later Windows/GNOME/XFCE. Language: **C++20 + Qt 6.5+ (QML)**. No Python in the product.

## Hard rules
1. **Do not guess APIs.** Check installed headers (`/usr/include/qt6/...`), Qt docs, KWin docs
   (https://develop.kde.org/docs/plasma/kwin/api) and the opencode OpenAPI spec (`http://127.0.0.1:4096/doc`).
2. **Platform code only in `src/platform/`** (Objective-C++ `.mm` files only there too, guarded by `if(APPLE)` in CMake). QML and `src/core/` must not mention KDE, KWin, Windows,
   GNOME, D-Bus or X11. New platform = new `PlatformShell` subclass (see docs/PLATFORMS.md).
3. **QML = look and animation only.** Network, files, processes, parsing live in C++ (`src/core`).
4. **Sprites are data, not code.** Never hardcode a character. Everything comes from a sprite pack
   (docs/SPRITE_PACK.md). Adding a character must need zero C++ changes.
5. **No global shortcut code in the app.** The DE launches `qute toggle`; the daemon is reached via `QLocalSocket`.
6. **All settings go through `AppConfig`** (flat JSON, Q_PROPERTY + `NOTIFY changed`). Overlay watches the file.
7. Keep `AgentState` pure (no QML/network). Every new event mapping gets a test in `tests/tst_agentstate.cpp`.
8. Do not change the overlay window title `qute-overlay`: the KWin script finds the window by it.
9. Small steps. After each step: build, run tests, and report the exact compiler / runtime errors.

## Setup wizard (src/setup/, docs/SETUP_WIZARD.md)
- `qute opencode ...` / `qute setup` are handled in `main()` BEFORE QGuiApplication. No GUI classes in `src/setup/`.
- Edit opencode's config only through `OpencodeConfig` (keeps unknown keys, writes a backup). Support BOTH dialects (V1 and native V2).
- Never write API keys into opencode.json in plain text: `Secrets::` (0600 file + `{file:..}`) or `{env:..}`.
- New MCP server = entry in `mcppresets.cpp`; new installer = entry in `installer.cpp`.
- opencode V1 and V2 differ (config + server API). See docs/OPENCODE.md; `OpenCodeClient` supports both and polls instead of trusting SSE.

## Build & test
```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/src/overlay/qute --daemon &     # start hidden
./build/src/overlay/qute toggle         # what the DE shortcut runs
./build/src/settings/qute-settings
```
macOS: `brew install qt cmake ninja`, configure with `-DCMAKE_PREFIX_PATH="$(brew --prefix qt)"`; the targets are `.app` bundles.
Dependencies (Arch): `qt6-base qt6-declarative qt6-svg cmake ninja` (+ `opencode`). Qt DBus ships with qt6-base.

## Layout
```
src/setup/     terminal wizard: qute opencode setup|install|doctor|mcp|provider
src/core/      AppConfig, SingleInstance(IPC), SseParser, OpenCodeClient, AgentState, SpritePackManager
src/platform/  PlatformShell (interface), KWinShell, MacShell (.mm), GenericShell, TerminalLauncher   <- ONLY place with OS-specific code
src/overlay/   main.cpp (CLI + daemon), OverlayController, QML (Main, SpriteView, PromptBox, ResponseView, Theme)
src/settings/  settings app (main.cpp + Main.qml)
assets/spritepacks/<id>/   manifest.json + PNG sheets
tests/         Qt Test unit tests
docs/          ARCHITECTURE, OPENCODE, SPRITE_PACK, PLATFORMS, ROADMAP
tools/         gen_qute_pack.py (art generator, optional, not part of the build)
```

## Work order — follow docs/ROADMAP.md (M0 → M8). Do NOT start a milestone before the previous one passes.

## Known risk areas (written WITHOUT being compiled or run — verify first, fix with evidence)
- `src/platform/kwinshell.cpp`: KWin script placement via D-Bus (`loadScript` / `/Scripting/Script<id>` / `run`).
- `src/core/opencodeclient.cpp`: event names / payload shapes of opencode (`/event`), permission endpoints.
- QML: `Qt.styleHints.colorScheme`, `AnimatedSprite` re-binding when the source changes, window resize steps.
- `src/setup/*`, `OpenCodeClient` V2 paths, `TerminalLauncher`: never compiled or run. V2 API details are partly unverified (docs/OPENCODE.md TODO).
- macOS: `src/platform/macshell.mm`, bundle/Info.plist CMake logic, Resources/spritepacks copy - nothing was run on a Mac.
- CMake: `qt_add_qml_module` + `loadFromModule("Qute.Overlay", "Main")` naming.
If something does not compile, fix the root cause; do not rewrite the architecture.
