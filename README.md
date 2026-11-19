# Qute Agent

A small animated desktop companion for [opencode](https://opencode.ai). Press a shortcut and a sprite
appears next to your cursor with a prompt box. The sprite reacts to what the agent is doing: working,
thinking, waiting for your permission, done, or failed.

- **Two apps:** `qute` (the overlay, a background daemon and CLI) and `qute-settings` (the settings window).
- **Platforms:** Linux (KDE first; GNOME and XFCE supported), macOS 12+ and Windows. See `docs/PLATFORMS.md`.
- **Look:** colors, fonts, accent and Qt style come from your desktop by default, and you can override each one.
- **Sprites:** characters and color variants are plain folders with a `manifest.json` (`docs/SPRITE_PACK.md`).

---

## 1. Install dependencies

You need **Qt 6.5 or newer**, CMake 3.21+, a C++20 compiler and (optionally) Ninja.
You also need [opencode](https://opencode.ai) itself; `qute opencode setup` can install it for you.

> **Ubuntu 24.04 / Linux Mint 22 ship Qt 6.4, which is too old.** Use Debian 13 (trixie), Fedora,
> Arch-based systems, openSUSE Tumbleweed, or install Qt 6.5+ with the official installer
> ([aqtinstall](https://github.com/miurla/aqtinstall) or qt.io) and point CMake to it (see §3).

| Distribution | Command |
|---|---|
| **Arch / CachyOS / Manjaro** | `sudo pacman -S --needed base-devel cmake ninja qt6-base qt6-declarative qt6-svg` |
| **Fedora** | `sudo dnf install gcc-c++ cmake ninja-build qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtsvg-devel` |
| **Debian 13 / newer Ubuntu** | `sudo apt install g++ cmake ninja-build qt6-base-dev qt6-declarative-dev qt6-svg-dev` |
| **openSUSE Tumbleweed** | `sudo zypper install gcc-c++ cmake ninja qt6-base-devel qt6-declarative-devel qt6-svg-devel` |
| **macOS** | `brew install qt cmake ninja` (details: `docs/MACOS.md`) |
| **Windows** | see `packaging/windows/README.md` |

Check the Qt version before you build: `qmake6 -query QT_VERSION` must print 6.5 or higher.
(On some distributions the binary is called `qtpaths6` or `qmake`; any Qt 6 tool works.)

## 2. Get the source

Either use the release archive `qute-agent-0.1.0.tar.xz`:

```sh
tar -xf qute-agent-0.1.0.tar.xz
cd qute-agent-0.1.0
```

or clone the repository:

```sh
git clone https://github.com/m5kop900/Qute-agent.git
cd Qute-agent
```

## 3. Install (recommended: one command)

```sh
./install.sh                 # installs to ~/.local, no root needed
```

This checks the tools, builds a release binary and installs it. If Qt is missing or too old, the script
stops with a message that explains what to install.

| Option | What it does |
|---|---|
| `./install.sh --system` | Installs to `/usr` for all users (asks for `sudo` if needed) |
| `./install.sh --prefix DIR` | Installs to any directory |
| `./install.sh --uninstall` | Removes the files of the last install (your settings in `~/.config/qute` stay) |

If Qt is not in a standard location, tell CMake where it is:

```sh
CMAKE_PREFIX_PATH=/opt/Qt/6.8.1/gcc_64 ./install.sh
```

Make sure the install directory's `bin` folder is in your `PATH` (for `~/.local`, that is `~/.local/bin`).
Open a new terminal afterwards.

### Manual build (without the script)

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$HOME/.local
cmake --build build
cmake --install build
```

Run the tests with `ctest --test-dir build --output-on-failure` (configure with `-DQUTE_BUILD_TESTS=ON`, the default).

### Arch Linux package

The `packaging/arch/PKGBUILD` builds a proper pacman package from the release archive:

```sh
cp qute-agent-0.1.0.tar.xz packaging/arch/
cd packaging/arch
makepkg -si
```

The AUR git package (`packaging/aur/qute-agent-git`) always builds the latest commit.

## 4. First run

```sh
qute opencode setup          # finds or installs opencode, adds MCP servers, optional local model
qute --daemon                # starts the background process (keep it running)
qute toggle                  # shows or hides the overlay
qute-settings                # opens the settings window
```

`qute opencode doctor` diagnoses problems if something does not connect.
The full guide to the setup wizard is in `docs/SETUP_WIZARD.md`.

## 5. Keyboard shortcut

Qute Agent does not register a global shortcut itself. Your desktop runs the command `qute toggle`:

- **KDE Plasma:** System Settings → Keyboard → Shortcuts → Add Command → `qute toggle`
- **GNOME:** Settings → Keyboard → View and Customize Shortcuts → Custom Shortcuts → `qute toggle`
- **XFCE:** Settings → Keyboard → Application Shortcuts → `qute toggle`
- **macOS:** Shortcuts app → new shortcut → Run Shell Script → `qute toggle`
- **Windows:** `packaging/windows/install-shortcuts.ps1 -Hotkey "CTRL+ALT+Q"` or `packaging/windows/qute.ahk`

**Autostart (optional):** run `qute --daemon` at login. On KDE add it under Autostart; on GNOME and XFCE
use Startup Applications. On GNOME with Wayland, the placement extension also needs
`packaging/gnome-shell/install.sh` and a log-out/log-in.

## 6. Appearance

Everything below is in `qute-settings` → **Appearance**. Changes show in the live preview at once; press
**Save changes** to write them. The running overlay then reloads automatically.

| Setting | Default | Notes |
|---|---|---|
| Color scheme | Follow system | Light, dark, or follow the desktop |
| Accent color | System accent | A hex color such as `#6ea8ff`, or a swatch. "Use system" resets it |
| Interface font | System default | Any font installed on your system. Follows the desktop font when empty |
| Code font | System monospace | Used for code blocks |
| Controls style | Custom | `System` uses your desktop's Qt style (Breeze on KDE). Restart the overlay after changing it: `qute quit`, then `qute --daemon` |
| Scale, opacity, corner radius, density | 1.0 / 100 % / 16 px / compact | Overlay size and layout |

The settings are stored in `~/.config/qute/config.json`. You can also edit that file by hand; the overlay
picks up the change without a restart.

## 7. Troubleshooting

- **`qute: command not found`** — the install directory is not in `PATH`. Add `~/.local/bin` to `PATH` and open a new terminal.
- **`configuration failed` during `./install.sh`** — Qt 6.5+ is missing. Install the packages from §1 or pass `CMAKE_PREFIX_PATH`.
- **The overlay does not appear** — is the daemon running? `qute --daemon`, then `qute toggle`.
- **The sprite stays in a wrong state or nothing connects** — run `qute opencode doctor`.
- **Fonts did not change** — pick a font from the list in Appearance and press Save. Fonts that are not installed fall back to the system font.

## Docs

`docs/MACOS.md` · `docs/PLATFORMS.md` · `docs/SETUP_WIZARD.md` · `docs/ARCHITECTURE.md` · `docs/OPENCODE.md` · `docs/SPRITE_PACK.md` · `docs/ROADMAP.md` · `AGENTS.md` (rules for contributors)

## Status

Milestones M0–M4 and M7 are done and were verified on KDE Wayland with OpenCode 2.2.6. The sprite packs (4 characters
× 7 states × 5 colors) are checked by `ctest`. Packaging for Linux is done; macOS, Windows and GNOME packaging is
written but not yet tested on those systems. Open items are listed in `docs/ROADMAP.md` ("Review status").

## License

No license file has been added yet. Until one is added, all rights are reserved by the authors.
