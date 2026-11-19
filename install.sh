#!/bin/sh
# Qute Agent installer: checks the tools, builds the project and installs it.
#
#   ./install.sh                     per-user install into ~/.local (no root needed)
#   ./install.sh --system            system-wide install into /usr (uses sudo when needed)
#   ./install.sh --prefix DIR        install into DIR
#   ./install.sh --qt-prefix DIR     look for Qt 6 in DIR (e.g. /opt/Qt/6.8.1/gcc_64)
#   ./install.sh --jobs N            parallel build jobs (default: number of CPUs)
#   ./install.sh --clean             delete the build directory first (after a failed or changed configure)
#   ./install.sh --uninstall         remove the files the last install put on the system
#   ./install.sh --help              this text
#
# Needs: cmake >= 3.21, a C++20 compiler, Qt 6.5 or newer (Core, Gui, Qml, Quick, QuickControls2,
# Network, Svg, DBus). Ninja is used when it is installed, otherwise make.
set -eu

HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BUILD="$HERE/build-install"
PREFIX="$HOME/.local"
MODE=user
UNINSTALL=0
CLEAN=0
JOBS=""
QT_PREFIX=""
MIN_CMAKE="3.21"

usage() {
    cat <<'USAGE'
Qute Agent installer.

  ./install.sh                     per-user install into ~/.local (no root needed)
  ./install.sh --system            system-wide install into /usr (uses sudo when needed)
  ./install.sh --prefix DIR        install into DIR
  ./install.sh --qt-prefix DIR     look for Qt 6 in DIR (e.g. /opt/Qt/6.8.1/gcc_64)
  ./install.sh --jobs N            parallel build jobs (default: number of CPUs)
  ./install.sh --clean             delete the build directory first
  ./install.sh --uninstall         remove the files the last install put on the system
  ./install.sh --help              show this text

The build directory is build-install/ next to this script. Settings in ~/.config/qute/ are never touched.
USAGE
}

die()  { echo "error: $*" >&2; exit 1; }
warn() { echo "warning: $*" >&2; }
step() { echo "==> $*"; }

while [ $# -gt 0 ]; do
    case "$1" in
        --system)      PREFIX=/usr; MODE=system ;;
        --prefix)      [ $# -ge 2 ] || die "--prefix needs a directory"; PREFIX="$2"; shift ;;
        --prefix=*)    PREFIX="${1#--prefix=}" ;;
        --qt-prefix)   [ $# -ge 2 ] || die "--qt-prefix needs a directory"; QT_PREFIX="$2"; shift ;;
        --qt-prefix=*) QT_PREFIX="${1#--qt-prefix=}" ;;
        --jobs)        [ $# -ge 2 ] || die "--jobs needs a number"; JOBS="$2"; shift ;;
        --jobs=*)      JOBS="${1#--jobs=}" ;;
        --clean)       CLEAN=1 ;;
        --uninstall)   UNINSTALL=1 ;;
        -h|--help)     usage; exit 0 ;;
        *)             die "unknown option: $1 (see --help)" ;;
    esac
    shift
done

[ -n "$PREFIX" ] || die "the install prefix is empty"
if [ -n "$JOBS" ]; then
    case "$JOBS" in
        ''|*[!0-9]*) die "--jobs needs a whole number, got '$JOBS'" ;;
    esac
    [ "$JOBS" -ge 1 ] || die "--jobs must be at least 1"
fi

# Runs a command as root only when the target directory is not writable by us.
run_for_prefix() {
    target="$1"; shift
    probe="$target"
    while [ ! -d "$probe" ]; do probe=$(dirname -- "$probe"); done
    if [ -w "$probe" ]; then
        "$@"
    elif command -v sudo >/dev/null 2>&1; then
        echo "-> using sudo for $target"
        sudo "$@"
    else
        die "$target is not writable and sudo is not available (try --prefix DIR)"
    fi
}

# True when version $1 is at least version $2 (both "major.minor[.patch]").
version_at_least() {
    [ "$(printf '%s\n%s\n' "$2" "$1" | sort -V | head -n 1)" = "$2" ]
}

# ---- uninstall ------------------------------------------------------------------------------------
if [ "$UNINSTALL" -eq 1 ]; then
    MANIFEST="$HERE/build-install/install_manifest.txt"
    [ -f "$MANIFEST" ] || die "no $MANIFEST: nothing to uninstall from this source tree (run ./install.sh first)"
    step "Removing the files listed in $MANIFEST"
    removed=0
    while IFS= read -r f; do
        [ -n "$f" ] || continue
        run_for_prefix "$(dirname -- "$f")" rm -f -- "$f"
        removed=$((removed + 1))
    done < "$MANIFEST"
    echo "Removed $removed files. Your settings in ~/.config/qute/ were kept."
    exit 0
fi

# ---- 1. tools -------------------------------------------------------------------------------------
step "Checking the tools"
command -v cmake >/dev/null 2>&1 \
    || die "cmake is missing. Install it with your package manager (see README.md, 'Install dependencies')."

cmake_version=$(cmake --version 2>/dev/null | sed -n 's/^cmake version //p' | head -n 1)
[ -n "$cmake_version" ] || die "could not read the cmake version (cmake --version printed nothing)."
version_at_least "$cmake_version" "$MIN_CMAKE" \
    || die "cmake $cmake_version is too old: Qute needs $MIN_CMAKE or newer. Update cmake and run ./install.sh again."

command -v c++ >/dev/null 2>&1 || command -v g++ >/dev/null 2>&1 || command -v clang++ >/dev/null 2>&1 \
    || die "no C++ compiler found (install g++ or clang++)."

GENERATOR="Unix Makefiles"
if command -v ninja >/dev/null 2>&1; then
    GENERATOR="Ninja"
fi
[ -n "$JOBS" ] || JOBS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)

# ---- 2. configure ---------------------------------------------------------------------------------
if [ "$CLEAN" -eq 1 ] && [ -d "$BUILD" ]; then
    step "Removing the old build directory"
    rm -rf -- "$BUILD"
fi

step "Configuring (cmake $cmake_version, prefix: $PREFIX, generator: $GENERATOR)"
set -- -S "$HERE" -B "$BUILD" -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DQUTE_BUILD_TESTS=OFF
[ -z "$QT_PREFIX" ] || set -- "$@" -DCMAKE_PREFIX_PATH="$QT_PREFIX"

if ! cmake "$@"; then
    cat >&2 <<MSG

error: configuration failed.
The most likely causes, in order:
  1. Qt 6.5 or newer is not installed. Install the Qt 6 development packages listed in README.md
     ("Install dependencies") for your distribution.
  2. Qt is installed somewhere CMake does not look. Point the installer at it:
       ./install.sh --qt-prefix /opt/Qt/6.8.1/gcc_64
  3. The build directory was configured earlier with other settings. Start over with:
       ./install.sh --clean
MSG
    exit 1
fi

# ---- 3. build -------------------------------------------------------------------------------------
step "Building ($JOBS jobs)"
if ! cmake --build "$BUILD" --parallel "$JOBS"; then
    echo "error: the build failed. The first 'error:' line above is the cause; fix it and run ./install.sh again." >&2
    exit 1
fi

# ---- 4. install -----------------------------------------------------------------------------------
step "Installing to $PREFIX"
run_for_prefix "$PREFIX" cmake --install "$BUILD"

BIN="$PREFIX/bin"
if [ "$MODE" = user ] && [ -d "$PREFIX/share/applications" ] && command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$PREFIX/share/applications" >/dev/null 2>&1 || true
fi

# ---- 5. checks and next steps ---------------------------------------------------------------------
if [ ! -x "$BIN/qute" ]; then
    warn "$BIN/qute was not found after the install. Check the output above."
fi
case ":$PATH:" in
    *":$BIN:"*) ;;
    *) warn "$BIN is not in your PATH. Add it to your shell profile, then open a new terminal." ;;
esac

REMOVE="./install.sh --uninstall"

cat <<MSG

Qute Agent is installed ($MODE install, binaries in $BIN).

Next steps:
  1. Set up opencode and the connection:       qute opencode setup
  2. Start the background daemon:               qute --daemon
  3. Bind a shortcut to the command:            qute toggle
     (see README.md, "Shortcut", for KDE, GNOME, XFCE, Windows and macOS)
  4. Open the settings app:                     qute-settings

To remove it later:  $REMOVE
MSG
