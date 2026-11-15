# macOS support

Supported from the start: macOS 12+ (Apple Silicon and Intel), Qt 6.5+. **Written but not yet compiled or run on a Mac.**

## Build
```
brew install qt cmake ninja
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build
ctest --test-dir build --output-on-failure
```
Result: `build/src/overlay/qute.app` (accessory app, no Dock icon) and `build/src/settings/qute-settings.app`.
Sprite packs are copied into `<app>/Contents/Resources/spritepacks` by a post-build step.
Install: `cmake --install build --prefix ~/Applications` then `packaging/macos/install-cli.sh ~/Applications/qute.app`
to get `qute` in the terminal (the CLI and the GUI are the same binary: `qute.app/Contents/MacOS/qute`).

## How it differs from Linux (all code in `src/platform/`)
- **`MacShell`** (`macshell.mm`, Objective-C++): accessory activation policy, window level above normal windows and
  full-screen apps, joins all Spaces, becomes key window so the prompt box receives keys.
  Does NOT use `Qt::Tool` (Qt would make a panel that hides when the app is inactive).
- **Cursor position:** `QCursor::pos()` works on macOS; placement logic is inherited from `GenericShell`.
- **No global shortcut inside the app** (same rule as everywhere). Bind `qute toggle`:
  - Shortcuts app: new shortcut -> action "Run Shell Script" -> `/opt/homebrew/bin/qute toggle` -> in the shortcut's
    details add a Keyboard Shortcut; or
  - skhd / Hammerspoon / Raycast / Alfred: run the same command.
  No Accessibility permission is needed because the app never listens to keys globally.
- **Autostart:** `packaging/macos/io.github.qute.daemon.plist` (LaunchAgent running `qute --daemon`).
- **Paths:** Qute Agent's own config lives in `~/Library/Preferences/qute/` (Qt `AppConfigLocation`); opencode's config stays
  `~/.config/opencode/` (opencode uses that on every OS; `OpencodeConfig::globalDir()` hardcodes it on purpose).
- **PATH:** apps started from Finder/launchd have a short PATH. `OpencodeLocator::searchPath()` adds
  `/opt/homebrew/bin`, `/usr/local/bin`, `~/.opencode/bin`, `~/.bun/bin`, `~/.npm-global/bin`.
- **Installers offered by `qute opencode install`:** Homebrew (`brew install opencode` = V2, or the V1 tap), npm, the
  official curl scripts.
- **Terminal for the "Set up opencode" button:** Terminal.app via `osascript`.

## Known unknowns (verify on a real Mac first - milestone M1/M3)
1. Keyboard focus in the prompt box after `qute toggle` from another app (activation logic in `MacShell::showAtCursor`).
2. Transparent window background and no shadow on Metal (`setOpaque:NO` + `clearColor`).
3. Window above full-screen apps / on all Spaces (`NSStatusWindowLevel` + collection behavior).
4. Unsigned local builds: if Gatekeeper complains, run `xattr -dr com.apple.quarantine qute.app`. Distribution needs
   signing + notarization (later, M8).
