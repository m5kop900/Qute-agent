#!/bin/sh
# Installs the Qute Agent GNOME Shell extension into the current user.
# Usage: packaging/gnome-shell/install.sh
# After installing on Wayland: log out and back in (GNOME only loads new extensions at login),
# then check with `gnome-extensions info qute@qute-agent`.
set -eu
UUID="qute@qute-agent"
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
SRC="$HERE/$UUID"
DATA="${XDG_DATA_HOME:-$HOME/.local/share}"
DEST="$DATA/gnome-shell/extensions/$UUID"

[ -d "$SRC" ] || { echo "missing extension folder: $SRC" >&2; exit 1; }
mkdir -p "$DATA/gnome-shell/extensions"
rm -rf "$DEST"
cp -R "$SRC" "$DEST"

if command -v gnome-extensions >/dev/null 2>&1; then
    gnome-extensions enable "$UUID" || true
fi

echo "Installed $UUID to $DEST"
echo "On Wayland, log out and back in, then run: gnome-extensions info $UUID"
