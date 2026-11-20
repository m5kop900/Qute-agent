# Changelog

## Unreleased

### Overlay
- Smart Chatbox Positioning (qute-settings > General > Window, on by default): opening the overlay or restoring the
  minimized card no longer moves the sprite. The chatbox goes around the sprite instead: top-right when it fits,
  otherwise above, below or to the left, whichever has the most room, using the real card size, the monitor's work
  area and the sprite's rectangle. Turning the setting off restores the previous placement exactly.
- Dragging across a reply to select text no longer scrolls the chat list at the same time. Dragging past the top or
  bottom edge of the chat scrolls it, and the selection keeps growing as the text scrolls into view.
- The prompt box grows to two lines; longer text scrolls inside the box with the mouse wheel and a scrollbar.
- Right-clicking the prompt box opens a themed menu (Undo, Redo, Cut, Copy, Paste, Select all) with shortcut hints.
- The "+" button opens a file manager window, centred on the screen: places (Home, working folder, Desktop, Documents,
  Downloads, Computer), a breadcrumb path, search, and the files Qute can send. Click to add or remove a file from the selection,
  Shift+click for a range, "Select all" for the folder, double-click a folder to open it, Enter or Attach to add the selection. "Other location…" opens the system dialog.

### Overlay
- Minimize: the "–" at the top right of the card folds the card into the sprite, so only the sprite stays on screen
  and clicks pass through everywhere else. Double-click the sprite to open the card again (animated both ways).
  A permission request or question from opencode opens a folded card automatically.
- The context ring moved to the right of the reasoning-effort chip.
- Minimizing now plays a collapse animation: the sprite squashes, the card twists and is pulled into the sprite while it
  fades, and a sparkle burst follows. Opening springs the card back out of the sprite.
- A minimized overlay is only as big as the sprite: the OS window shrinks to it (KWin), so nothing transparent is left
  around the sprite and it can be dragged anywhere. Opening grows the window back, keeping the prompt box in place.
- Level 2 card height is editable in qute-settings (Overlay > Long reply height, 300–1000 px, default 430).
- Dragging the sprite starts only when the pointer moves, so a plain click or double-click is not turned into a move.
- Overlay width is editable in qute-settings (Overlay > Overlay width, 380–1000 px, default 560), scaled with the UI scale.
- The "+" button next to the prompt opens the system file dialog (the desktop's own file manager), filtered to the file
  types Qute can send, instead of the in-app list. Files refused by the dialog are reported in the chat.
- Right-click on a message bubble (anywhere on it, padding included) offers "Copy message".
- Text selection in a reply runs across the whole message: a drag can start in text above a code block, pass through
  the block and end in the text below it. Right-click copies the selection, or the whole message when nothing is selected.
- The prompt box grows with multi-line text (up to about six lines), then the text scrolls inside it.
- Right-click menus stay inside the overlay window instead of running past its right or bottom edge.

### Installer
- install.sh checks the cmake version (3.21 or newer) before configuring, and says what to do when Qt is missing.
- New options: --qt-prefix DIR, --jobs N, --clean. The build directory is reported and the install is verified
  (binary present, PATH checked).
- The usage text is now written out in the script, not read from fixed line numbers.

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
