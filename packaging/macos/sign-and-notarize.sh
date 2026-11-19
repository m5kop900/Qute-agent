#!/bin/bash
# Sign (and optionally notarize + staple) the Qute Agent macOS app bundles.
#
# Prerequisites (once):
#   xcrun notarytool store-credentials qute-notary \
#       --apple-id "you@example.com" --team-id TEAMID --password "app-specific-password"
#
# Usage:
#   QUTE_SIGN_IDENTITY="Developer ID Application: Your Name (TEAMID)" \
#   QUTE_NOTARY_PROFILE=qute-notary \
#   packaging/macos/sign-and-notarize.sh build/src/overlay/qute.app build/src/settings/qute-settings.app
#
# Env:
#   QUTE_SIGN_IDENTITY   required, "Developer ID Application: ..."
#   QUTE_NOTARY_PROFILE  optional, keychain profile for notarytool; skip notarization when empty
#   QUTE_DIST_ZIP        optional, where to write the stapled release zip
#   QUTE_ENTITLEMENTS    optional, defaults to packaging/macos/entitlements.plist
#
# The signing order is inside-out: every Mach-O file, then framework bundles, then the main
# executable, then the .app. Do not use `codesign --deep` for distribution.
set -euo pipefail

if [ "$#" -lt 1 ]; then
    echo "usage: $0 <app.app> [<app.app> ...]" >&2
    exit 64
fi

IDENTITY="${QUTE_SIGN_IDENTITY:-}"
PROFILE="${QUTE_NOTARY_PROFILE:-}"
HERE="$(cd "$(dirname "$0")" && pwd)"
ENTITLEMENTS="${QUTE_ENTITLEMENTS:-$HERE/entitlements.plist}"

if [ -z "$IDENTITY" ]; then
    echo "QUTE_SIGN_IDENTITY is not set (e.g. 'Developer ID Application: Name (TEAMID)')" >&2
    exit 1
fi
if [ ! -f "$ENTITLEMENTS" ]; then
    echo "entitlements file not found: $ENTITLEMENTS" >&2
    exit 1
fi

sign_lib() {
    # Frameworks/plugins get the hardened runtime but no app entitlements.
    /usr/bin/codesign --force --timestamp --options runtime --sign "$IDENTITY" "$1"
}

sign_app() {
    local app="$1"
    [ -d "$app/Contents" ] || { echo "not an app bundle: $app" >&2; return 1; }

    local dirs=()
    for d in "$app/Contents/Frameworks" "$app/Contents/PlugIns" "$app/Contents/MacOS"; do
        [ -d "$d" ] && dirs+=("$d")
    done

    # 1. Every Mach-O file (frameworks, plugins, helper binaries).
    if [ "${#dirs[@]}" -gt 0 ]; then
        while IFS= read -r -d '' f; do
            if file "$f" | grep -q "Mach-O"; then
                sign_lib "$f"
            fi
        done < <(find "${dirs[@]}" -type f -print0)
    fi

    # 2. Nested bundles (deepest first), usually Qt .framework folders.
    while IFS= read -r -d '' bundle; do
        /usr/bin/codesign --force --timestamp --options runtime --sign "$IDENTITY" "$bundle"
    done < <(find "$app/Contents" -type d \( -name "*.framework" -o -name "*.app" \) -depth -print0)

    # 3. Main executable with the app entitlements, then the bundle itself.
    local exe
    exe=$(/usr/libexec/PlistBuddy -c "Print :CFBundleExecutable" "$app/Contents/Info.plist")
    /usr/bin/codesign --force --timestamp --options runtime \
        --entitlements "$ENTITLEMENTS" --sign "$IDENTITY" "$app/Contents/MacOS/$exe"
    /usr/bin/codesign --force --timestamp --options runtime \
        --entitlements "$ENTITLEMENTS" --sign "$IDENTITY" "$app"

    /usr/bin/codesign --verify --deep --strict --verbose=2 "$app"
    echo "signed: $app"
}

for app in "$@"; do
    sign_app "$app"
done

if [ -z "$PROFILE" ]; then
    echo "QUTE_NOTARY_PROFILE is empty; skipping notarization."
    exit 0
fi

STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT
for app in "$@"; do
    /usr/bin/ditto "$app" "$STAGE/$(basename "$app")"
done
NOTARIZE_ZIP="$STAGE/review.zip"
/usr/bin/ditto -c -k --keepParent "$STAGE" "$NOTARIZE_ZIP"

xcrun notarytool submit "$NOTARIZE_ZIP" --keychain-profile "$PROFILE" --wait
for app in "$@"; do
    xcrun stapler staple "$app"
    /usr/sbin/spctl --assess --type execute --verbose=2 "$app"
    echo "notarized + stapled: $app"
done

if [ -n "${QUTE_DIST_ZIP:-}" ]; then
    FINAL="$(mktemp -d)"
    for app in "$@"; do
        /usr/bin/ditto "$app" "$FINAL/$(basename "$app")"
    done
    /usr/bin/ditto -c -k --keepParent "$FINAL" "$QUTE_DIST_ZIP"
    rm -rf "$FINAL"
    echo "release zip: $QUTE_DIST_ZIP"
fi
