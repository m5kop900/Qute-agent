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
