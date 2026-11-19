# Changelog

## 0.1.0 + appearance update

### Appearance
- New theme tokens for every color the overlay and settings use: `danger`, `warning`, `success`,
  `accentAlt`, `codeBackground`, `syntaxString`, `syntaxNumber`. No hard-coded colors remain in the QML.
- Interface font and code font can be chosen (`fontFamily`, `codeFontFamily`). Empty means the system font.
  The interface font follows the desktop font when it changes.
- Qt Quick Controls style is selectable (`controlStyle`): `custom` (default, unchanged look), `system`
  (the desktop's Qt style, e.g. Breeze on KDE) or `Fusion`.

### Fixes
- Overlay opacity now changes the card background (it was saved but never used).
- Corner radius now applies to every rounded surface in the overlay (it was saved but never used).
- Density now changes the spacing and margins of the card and the conversation (it was saved but never used).
- The "Animations" switch now stops the card, sprite and summon animations.

- Changing the UI scale while the overlay is open no longer leaves the card and its click area out of
  step: the card position is computed from the requested geometry, not from the window size that the OS
  applies later, and the click mask is updated at the same time as the window is resized.

- The summon animation no longer gets cut off at the window edge: the window opens a little wider on the
  left and the content takes that room back once the animation has finished.
- The overlay no longer logs "Cannot read property 'count' of null" when it quits (the level bindings are
  null-safe now).
- Restarting the overlay from the settings app waits until the old daemon has really exited, and finds
  the `qute` binary in the build tree too.
- Sprite animations run faster (1.6x the pack's fps; idle up to 12 fps).

### Settings app
- After saving, the settings app asks whether to restart the overlay (the overlay reads its settings at
  start-up). A "Restart overlay" button is also in the footer.
- Appearance page with a live preview, color presets, font lists and Qt style.
- Footer shows unsaved changes; Revert is only active when there are changes.

### Installation
- `install.sh`: dependency check with a clear message, build, install to `~/.local` or `/usr`, and `--uninstall`.
- `packaging/arch/PKGBUILD` for a package from the release archive.
- README rewritten: per-distribution dependencies, Qt 6.5+ requirement, first-run steps, shortcut and troubleshooting.

### Tests
- `FontFamilyPrecedence` in `tests/tst_theme.cpp`.
