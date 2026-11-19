#!/bin/sh
# Qute Agent installer: checks the tools, builds the project and installs it.
#
#   ./install.sh                   per-user install into ~/.local (no root needed)
#   ./install.sh --system          system-wide install into /usr (asks for sudo if needed)
#   ./install.sh --prefix DIR      install into DIR
#   ./install.sh --uninstall       remove what the last install put on the system
#   ./install.sh --help
#
# Needs: cmake >= 3.21, a C++20 compiler, Qt 6.5 or newer (Core, Gui, Qml, Quick, QuickControls2,
# Network, Svg, DBus). Ninja is used when it is installed, otherwise make.
set -eu

HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BUILD="$HERE/build-install"
PREFIX="$HOME/.local"
MODE=user
UNINSTALL=0

usage() {
    sed -n '2,11p' "$0" | sed 's/^# \{0,1\}//'
}

die() { echo "error: $*" >&2; exit 1; }

while [ $# -gt 0 ]; do
    case "$1" in
        --system)    PREFIX=/usr; MODE=system ;;
        --prefix)    [ $# -ge 2 ] || die "--prefix needs a directory"; PREFIX="$2"; shift ;;
        --prefix=*)  PREFIX="${1#--prefix=}" ;;
        --uninstall) UNINSTALL=1 ;;
        -h|--help)   usage; exit 0 ;;
        *) die "unknown option: $1 (see --help)" ;;
    esac
    shift
done

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
        die "$target is not writable and sudo is not available"
    fi
}

if [ "$UNINSTALL" -eq 1 ]; then
    MANIFEST="$BUILD/install_manifest.txt"
    [ -f "$MANIFEST" ] || die "no $MANIFEST: nothing to uninstall from this source tree (use the same --prefix you installed with)"
    echo "Removing files listed in $MANIFEST"
    while IFS= read -r f; do
        [ -n "$f" ] || continue
        run_for_prefix "$(dirname -- "$f")" rm -f -- "$f"
    done < "$MANIFEST"
    echo "Qute Agent removed. Your settings in ~/.config/qute/ were kept."
    exit 0
fi

# ---- 1. tools -------------------------------------------------------------------------------------
command -v cmake >/dev/null 2>&1 || die "cmake is missing. Install it with your package manager (see README.md, 'Install dependencies')."
command -v c++ >/dev/null 2>&1 || command -v g++ >/dev/null 2>&1 || command -v clang++ >/dev/null 2>&1 \
    || die "no C++ compiler found (install g++ or clang++)."

GENERATOR="Unix Makefiles"
if command -v ninja >/dev/null 2>&1; then
    GENERATOR="Ninja"
fi
JOBS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)

# ---- 2. configure (fails early with a clear message when Qt is missing or too old) ------------------
echo "==> Configuring (prefix: $PREFIX, generator: $GENERATOR)"
if ! cmake -S "$HERE" -B "$BUILD" -G "$GENERATOR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$PREFIX" \
        -DQUTE_BUILD_TESTS=OFF; then
    cat >&2 <<MSG

error: configuration failed.
Most likely Qt 6.5+ is not installed or CMake cannot find it. Install the Qt 6 development packages
listed in README.md ("Install dependencies") for your distribution, then run ./install.sh again.
If Qt is installed in a non-standard place, set CMAKE_PREFIX_PATH, for example:
  CMAKE_PREFIX_PATH=/opt/Qt/6.8.1/gcc_64 ./install.sh
MSG
    exit 1
fi

# ---- 3. build -------------------------------------------------------------------------------------
echo "==> Building"
cmake --build "$BUILD" --parallel "$JOBS"

# ---- 4. install -----------------------------------------------------------------------------------
echo "==> Installing to $PREFIX"
run_for_prefix "$PREFIX" cmake --install "$BUILD"

# ---- 5. next steps --------------------------------------------------------------------------------
BIN="$PREFIX/bin"
cat <<MSG

Qute Agent is installed ($MODE install, binaries in $BIN).

Next steps:
  1. Make sure "$BIN" is in your PATH (open a new terminal after changing it).
  2. Set up opencode and the connection:       qute opencode setup
  3. Start the background daemon:               qute --daemon
  4. Bind a shortcut to the command:            qute toggle
     (see README.md, "Shortcut", for KDE, GNOME, XFCE, Windows and macOS)
  5. Open the settings app:                     qute-settings

To remove it later:  ./install.sh --uninstall $( [ "$MODE" = system ] && echo --system || echo "--prefix $PREFIX" )
MSG
