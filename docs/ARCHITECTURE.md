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
The OS window has 3 discrete heights (prompt only / reply / long reply). The panel inside animates smoothly.
Reason: continuous native resize is janky on Wayland. The window is only as big as its content, so no click-through hacks are needed.

## Settings split
`AppConfig` is a static-lib class used by both executables. Both MUST use `setApplicationName("qute")` so the config path matches.

## Extending
- New sprite character: new folder in `assets/spritepacks/` (docs/SPRITE_PACK.md). No code.
- New platform: new `PlatformShell` subclass (docs/PLATFORMS.md).
- New setting: Q_PROPERTY in `AppConfig` + member/default + control in settings `Main.qml`.
