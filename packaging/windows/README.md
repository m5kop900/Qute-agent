# Qute Agent on Windows

Status: **written, never built or run on Windows** (the project is developed on Linux/KDE). Treat everything
here as the intended procedure; report/fix defects with evidence when a Windows machine is available.

The Windows build uses the plain `GenericShell` platform: a frameless, always-on-top tool window placed with
`QCursor::pos()` + `QWindow::setPosition()`, which works on Windows because normal windows may position
themselves. No extra process, service, or extension is needed.

## 1. Prerequisites
- **Qt 6.5+** — MSVC 2022 64-bit or MinGW 64-bit build (Qt Online Installer).
- **CMake 3.21+** and **Ninja** (or the Visual Studio generator).
- **opencode** on `PATH` (`npm i -g opencode-ai`, or the installer) — only needed at runtime.
- Optional: **Inno Setup** if you want an installer instead of the portable zip.

## 2. Build
From a *x64 Native Tools Command Prompt for VS 2022* (or a shell where `cmake` and Qt are on `PATH`):

```bat
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release ^
      -DCMAKE_PREFIX_PATH=C:\Qt\6.8.0\msvc2022_64
cmake --build build
ctest --test-dir build --output-on-failure
```

The executables land in `build\src\overlay\qute.exe` and `build\src\settings\qute-settings.exe`
(multi-config generators append `\Release`).

Both are built as GUI applications (`WIN32_EXECUTABLE`), so no console window flashes when they are started
from Explorer or autostart. `qute.exe` re-attaches to the parent console at startup, so the CLI still prints:

```bat
qute --version
qute opencode doctor
start /wait qute opencode setup     :: interactive wizard: `start /wait` keeps the prompt clean
qute toggle
```

### Running from the build tree
Sprite packs are found via `QUTE_DEV_PACKS_DIR` (set by CMake) or `<exe>\..\share\qute\spritepacks`, so a
deployed copy must keep that layout:

```
qute-agent\
  bin\qute.exe
  bin\qute-settings.exe
  bin\Qt6*.dll, platforms\, styles\, qml\, ...
  share\qute\spritepacks\<pack>\...
```

## 3. Package a portable zip
```powershell
powershell -ExecutionPolicy Bypass -File packaging\windows\package.ps1 -BuildDir build -OutDir dist
```
It copies the executables, runs `windeployqt` (Qt DLLs + QML plugins), copies the sprite packs, writes the zip
and prints its SHA-256. Pass `-QtBin C:\Qt\6.8.0\msvc2022_64\bin` if `windeployqt.exe` is not on `PATH`, and
`-Config Release` for multi-config builds.

## 4. Shortcut, hotkey, autostart
`qute` has no built-in global shortcut on any platform; the desktop launches `qute toggle`.

```powershell
# Start Menu entries (+ optional hotkey on the overlay shortcut)
powershell -ExecutionPolicy Bypass -File packaging\windows\install-shortcuts.ps1 -BinDir .\dist\qute-agent\bin -Hotkey "CTRL+ALT+Q"

# Also start the daemon at login (hidden, so the first toggle is instant)
powershell -ExecutionPolicy Bypass -File packaging\windows\install-shortcuts.ps1 -BinDir .\dist\qute-agent\bin -Autostart
```
Use `-BinDir` with the folder that contains `qute.exe` (omit it to look on `PATH`). The packaged zip ships both
scripts at its root, so on an end-user machine the same line is `.\install-shortcuts.ps1 -BinDir .\bin -Hotkey "CTRL+ALT+Q"`.
Alternatively use the AutoHotkey v2 script `qute.ahk` (edit `quteExe` if needed) — it binds `Ctrl+Alt+Q` the same way.

`install-shortcuts.ps1` writes only to the current user's Start Menu and Startup folder; it needs no admin
rights and can be undone by deleting the two shortcuts it lists at the end.

## 5. Known limitations
- **Not validated on Windows.** Window placement, transparency, DPI scaling and the console re-attach need a
  live check (`qute toggle`, `qute --daemon`, `qute opencode doctor`).
- The overlay is a tool window: it does not appear in the taskbar (intended) and Alt+Tab behavior depends on
  the Windows version.
- `cmd.exe` does not wait for GUI-subsystem processes. A command finishes and the prompt returns immediately;
  output may appear after the next prompt. For the interactive wizard use `start /wait qute opencode setup`.
- "Always on top" follows Qt's `WindowStaysOnTopHint`; full-screen exclusive games still cover it.
- No installer, tray icon, or code signing yet. SmartScreen will warn about an unsigned `qute.exe`.
