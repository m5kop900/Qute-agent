# Architecture

```
 DE shortcut ──► `qute toggle` ──► QLocalSocket ──► [ qute daemon ]
                                                      │
        ┌─────────────────────────────────────────────┼──────────────────────────────┐
        │ QML (look only)                             │ C++ core                      │
        │  Main / SpriteView / PromptBox / Response   │  OverlayController            │
        │        ▲ context properties                 │  PlatformShell (per OS/DE)    │
        │        │ config, packs, client, agent,      │  OpenCodeClient ─ HTTP + SSE ─┼─► opencode serve
        │        │ controller                         │  AgentState (events → state)  │
        └────────┴────────────────────────────────────┤  SpritePackManager            │
                                                      │  AppConfig (config.json) ◄────┼── qute-settings
                                                      └───────────────────────────────┘
```

## Data flow
1. `qute toggle` → daemon `OverlayController::toggle()` → QML gets `shown` (sizes itself) → `PlatformShell::showAtCursor()`.
2. Prompt typed → `client.sendPrompt()` → `POST /session/:id/prompt_async`; `agent.promptSent()` → state `thinking`.
3. SSE `/event` → `OpenCodeClient::handleBusEvent` → (a) accumulates assistant text → `responseText`, (b) emits `serverEvent` → `AgentState::handleEvent` → state name.
4. QML: `SpriteView.stateName = agent.stateName` → `packs.stateInfo(state)` → `AnimatedSprite`.
5. Settings app writes `config.json` → overlay `QFileSystemWatcher` → `AppConfig::load()` → `changed()` → bindings update, pack reloads.

## Session data (F0)
`OpenCodeClient` is the only transport. On top of it, three services live in `src/core/`:
- `ConversationModel` (`QAbstractListModel`) — ordered items `user | assistant-text | reasoning | tool | request | notice`.
  Pure: `applyEvent(type, properties)`, `applyUserPrompt(text)`, `applyMessages(projection)`. Unit-tested by replaying a
  recorded event sequence and comparing it with the projection of the same turn (`tests/fixtures/`).
- `SessionStore` — owns the model and tracks `busy`, `sessionId`, `workDir`, `activeAgent`, `activeModel`, token `usage`.
  It consumes `serverEvent`, `promptSent` and `messageHistoryReceived`; it never parses HTTP itself.
- `OpenCodeCatalog` — commands, models (+variants), agents and file search. Refreshes from
  `OpenCodeClient::fetchJson()` on connect and on `command.updated` / `model.updated` / `agent.updated` / `config.updated`.

Reconcile: after a turn (and on reconnect) `OpenCodeClient::requestMessageHistory()` fetches the projected message list and
emits `messageHistoryReceived` (oldest first); `SessionStore` replaces the timeline with it. The QML overlay uses the
context properties `store`, `conversation` and `catalog` and never sees JSON.


## State → sprite (names are the contract between AgentState and packs)
`offline idle thinking working waiting done error`. Packs may omit states; the manifest `fallback` map decides.

## Window sizing
The OS window has one fixed maximum size (it never resizes: continuous native resize is janky on Wayland).
Inside it the card is anchored to the **top edge** and its height animates between 3 levels (prompt only /
reply / long reply), growing downward. Everything below the card is transparent, and the window's input
region is limited to the card (plus an open dropdown), so clicks there pass through to the app underneath.
The card must stay flush with the window top: the compositor stops an interactive window move once the
window's top edge reaches the screen top, so invisible space above the card would be a hard limit for
dragging the overlay up. Placement (`contentHeight` / `maxContentHeight` window properties) puts the prompt
box just below the cursor and keeps room for the tallest card.

## Settings split
`AppConfig` is a static-lib class used by both executables. Both MUST use `setApplicationName("qute")` so the config path matches.

## Extending
- New sprite character: new folder in `assets/spritepacks/` (docs/SPRITE_PACK.md). No code.
- New platform: new `PlatformShell` subclass (docs/PLATFORMS.md).
- New setting: Q_PROPERTY in `AppConfig` + member/default + control in settings `Main.qml`.


## Smart chatbox positioning

Setting `smartChatboxPositioning` (qute-settings > General > Window, default **on**). The overlay is one OS window that
holds the sprite, the chatbox (card) and invisible room below the card for dropdowns. The old arrangement always put the
card to the right of the sprite and re-clamped the *window* to the screen when the card did not fit, which moved the
sprite (on open, and on every restore of a minimized card). With the setting on, the sprite's place on screen is an
input and only the card adapts:

- `src/core/chatboxplacement.{h,cpp}`: pure geometry, no Qt types. Candidates: right/left of the sprite (card bottom or
  top level with the sprite's, or centred), above/below it (card right or left edge level with the sprite's, or
  centred). **Top-right** (right of the sprite, growing upward: the classic look) wins whenever the whole card fits on
  the monitor's work area without touching the sprite. Otherwise the valid candidate with the most room on screen and
  the least distance to the sprite wins. If nothing fits, the card is kept on screen and overlaps the sprite as little as
  possible. A card size of 0 means minimized: the window is the sprite alone.
- Who decides: `GenericShell` (Windows, macOS, X11) calls it in C++ with `QScreen::availableGeometry()`. A Wayland
  client cannot read its own global position or the work area, so on KDE (`KWinShell`) and GNOME (`GnomeShell` +
  extension) the same algorithm runs **inside the compositor** as a JavaScript port (`src/platform/placementscript.h`,
  copied between markers into the extension) and reports the result: KWin through `callDBus` to the daemon's
  `io.github.qute.Overlay /Layout LayoutReady(s)`, GNOME as the D-Bus method reply. Plain Wayland without an
  extension cannot place windows at all: the classic arrangement is kept.
- The sprite is placed where the classic code put it (at the cursor, or where it is now, only the sprite itself is kept on
  screen), and the window is the bounding box of sprite + card + drop room. The layout string is
  `dir,align,wx,wy,ww,wh,sx,sy,cx,cy,cw,ch,anchorBottom,valid`; `PlatformShell::applyLayoutWire` publishes it as the
  `layout*` properties of the QML window, and `Main.qml` draws the sprite and card at those offsets.
- Requests are serialized (KWin, GNOME): the sprite's offset inside the window is only known once the previous answer arrived.
- Minimizing/restoring and card growth go through the same path (`resizeContent`) as opening, so there is one positioning
  logic. The summon animation's sideways "bleed" is switched off in this mode (it slid the sprite 80 px).
- Off: every one of these code paths is skipped; the classic placement scripts and QML bindings run unchanged.
- Tests: `tests/tst_chatboxplacement.cpp` (geometry) and `tests/placement/parity.js` (C++ vs JavaScript on 20,000 random
  inputs, invariants, and the extension's copy). Run `sh tests/placement/parity.sh` without a Qt build.
- Known limit: the direction is re-evaluated when the card opens, restores or changes size, not while the sprite is being
  dragged (on Wayland the app gets no event when a compositor-driven move ends).
