#!/bin/sh
# Makes `qute` available in the terminal on macOS by linking the binary inside the app bundle.
# Usage: packaging/macos/install-cli.sh /Applications/qute.app
set -e
APP="${1:-/Applications/qute.app}"
BIN="$APP/Contents/MacOS/qute"
[ -x "$BIN" ] || { echo "not found: $BIN" >&2; exit 1; }
DEST="/usr/local/bin"
[ -d /opt/homebrew/bin ] && DEST="/opt/homebrew/bin"
echo "Linking $BIN -> $DEST/qute (may ask for your password)"
sudo ln -sf "$BIN" "$DEST/qute"
echo "Done. Try: qute --version"
