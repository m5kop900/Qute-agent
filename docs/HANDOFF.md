# HANDOFF: full context of the Qute Agent project (written Nov 15, 2026, for another AI taking over)

Read this first. It summarizes a long conversation between the user and Claude (Anthropic). Everything below is either
something the user said, a decision made together, or work Claude produced. Items marked **UNVERIFIED** were never compiled,
run, or checked against a live system.

## 0. How to work with this user
- The user writes **Persian (Farsi)**, casual, often terse. Reply in Persian unless asked otherwise. Keep answers short and concrete (they read on a phone).
- Code, comments and docs in the repo are English.
- They are NOT a C++ programmer and do not want to learn it. A **local AI coding agent** (they call it "cyber tiel coder"; its quality is exactly what they are testing) will write/fix the code. So: precise docs, small milestones, explicit acceptance tests, no guessing.
- They dislike needless complexity ("don't make the project heavier for nothing"). They explicitly rejected Python + a C++ helper for layer-shell.
- They run the finished product while using **LXQt with KWin as the window manager** (not full Plasma). Primary dev/target: Linux + KWin. They want macOS compatibility from day one, and later Windows, GNOME, XFCE ("everywhere").
- Be honest about uncertainty. The previous assistant could not compile anything (no Qt/CMake in its sandbox).

## 1. The idea (user's words, condensed)
A Linux, KWin-first desktop AI-agent companion for **opencode**:
- A command opens it. A **cute animated sprite creature** appears on screen with a **prompt box next to it**.
- When a message is sent the prompt box **grows a bit** and the **sprite's behavior changes** according to what the agent is doing (idle, thinking, tool work, waiting for permission, done, error).
- Must open **at the mouse cursor**.
- At least **4 premade sprite models in every color variant**; follows the **system Qt theme** but the user can choose their own.
- **Settings are a separate app** (installing installs two apps).
- A **terminal setup wizard** (`qute opencode install` / `setup`): installs opencode if missing, tells the user to run it if opencode is not detected, **supports important MCP servers (Playwright, Context7)**, is **opencode V2 compatible**, and can **add OpenAI-compatible model endpoints** (opencode has no easy built-in way).
- No full-screen/desktop client (decided: a small "mascot next to you" is the identity; offer "expand" mode + "Open in opencode" button later).
- Possibly published on GitHub later (advice given: demo GIF in README, easy install, cute name/sprite, post on r/kde, r/unixporn, opencode community, Show HN).

## 2. Key decisions and the reasoning
| Topic | Decision | Why |
|---|---|---|
| Framework | **Qt 6 (C++20) + QML**. One language. | Best KDE/KWin integration; native, light; single CMake project. |
| Rejected | Tauri (WebKitGTK transparency/Wayland issues, global shortcut plugin is X11-centric), Flutter/Dart (GTK embedder, layer-shell/click-through need FFI), PySide6+C++ helper (two languages), Rust cxx-qt (too hard for agent) | see above |
| Shortcut | **App registers NO global shortcut.** The DE runs the command `qute toggle`; the daemon is reached over `QLocalSocket` | Works on every DE/OS; avoids KGlobalAccel/portal code |
| Window | Plain frameless transparent top-level window (NOT layer-shell). Window size = content size (3 discrete heights), so no click-through hacks | Layer-shell + click-through/focus were the riskiest parts |
| Cursor placement | KDE/KWin: load a tiny **KWin script over D-Bus** that reads `workspace.cursorPos` and moves the window titled `qute-overlay`. Alternative: KWin window rule "Placement: Under Mouse". Detect KWin by D-Bus service `org.kde.KWin` (works under LXQt+KWin, no plasmashell needed). Windows/X11: `QCursor::pos()`. GNOME Wayland: needs a GNOME Shell extension (TODO). macOS: `MacShell` | Wayland clients cannot read global cursor pos |
| Platform abstraction | `PlatformShell` interface; all OS code only in `src/platform/` | Port later without rewrites |
| Animation | QML `AnimatedSprite` + PNG sprite sheets from data-driven **sprite packs** (`manifest.json`) | Add characters with zero C++ changes |
| Color variants | pre-rendered hue-shifted sheets (5 variants for Qute Agent) | simple, robust |
| Theme | QML `SystemPalette` + `Qt.styleHints.colorScheme`; modes system/light/dark + accent override | follows Qt platform theme (needs qt6 lxqt platform theme under LXQt) |
| Settings | separate executable `qute-settings` sharing `AppConfig` JSON; overlay watches the file (live reload) | user request |
| opencode config dialect | Write **V1-compatible syntax by default** (read by V1 and V2); native V2 only if file already uses it or only V2 installed | V2 reads V1 files, V1 cannot read native V2 |
| Secrets | never plain text in opencode.json; 0600 file + `{file:...}` or `{env:NAME}` | safety |
| IPC | `QLocalServer/QLocalSocket` (not D-Bus) | cross-platform |
| Events vs truth | SSE events are hints; **status polling decides turn end**, final text reconciled from message list | V1/V2 SSE streams documented as volatile/buggy |
| Name | working name **qute** (Familiar, Kodama, Plasmite, Pip also suggested). Must check GitHub/AUR for clashes | |

## 3. What exists (deliverable: `qute-project.zip`, ~4.5k lines + assets)
**Status: written, NEVER compiled or run (no Qt/CMake in the author's sandbox). Expect compile errors. Fix root causes, don't redesign.**
```
AGENTS.md  README.md  CMakeLists.txt  .gitignore
docs/  ARCHITECTURE OPENCODE SPRITE_PACK PLATFORMS ROADMAP SETUP_WIZARD MACOS HANDOFF(.md)
assets/spritepacks/qute/manifest.json + {ocean,mint,grape,rose,amber}/{idle,thinking,done}.png (6 frames, 256x256, one row) + preview.png
tools/gen_qute_pack.py        (Pillow art generator; optional dev tool, not part of build)
packaging/  qute.desktop qute-settings.desktop  macos/{Info-qute.plist.in, Info-settings.plist.in, install-cli.sh, io.github.qute.daemon.plist}
src/core/      AppConfig, SingleInstance(IPC), SseParser, OpenCodeClient (V1+V2), AgentState, SpritePackManager, OpencodeLocator
src/platform/  PlatformShell, KWinShell, GenericShell, MacShell(.mm), TerminalLauncher
src/overlay/   main.cpp (CLI + daemon + dispatch to wizard), OverlayController, QML: Main, SpriteView, PromptBox, ResponseView, Theme
src/settings/  main.cpp, Main.qml
src/setup/     jsonc, tty, installer, opencodeconfig, mcppresets, providerprobe, secrets, setupcli   (terminal wizard, no GUI)
tests/         tst_sseparser tst_agentstate tst_jsonc tst_opencodeconfig tst_clientparse   (Qt Test)
```
Executables: `qute` (overlay daemon + CLI; bundle `qute.app` on macOS, LSUIElement) and `qute-settings`.
Commands: `qute [toggle|show|hide|quit|reload|settings]`, `qute --daemon`, `qute opencode <setup|install|doctor|mcp|provider>`, `qute setup`.
`qute opencode ...` is handled in `main()` BEFORE `QGuiApplication` (plain `QCoreApplication`, works over SSH).

### State machine (src/core/agentstate.*): `offline idle thinking working waiting done error`
Mapped from opencode events (`session.status busy/idle/retry`, `session.idle`, `session.error`, `permission.updated|asked|replied`,
`message.part.updated` tool/reasoning/text, V2 `session.execution.started|succeeded|failed`). Pack manifests have a `fallback` map; only idle/thinking/done art exists.

### Overlay UI
Window title must stay `qute-overlay` (KWin script matches it). Levels 0/1/2 = prompt only / reply / long reply; panel animates, OS window resizes in discrete steps. Esc hides. Permission bar (Allow/Always/Deny), Stop button, connection-error row with "Set up opencode" button (opens a terminal running the wizard).

### Setup wizard (docs/SETUP_WIZARD.md) — steps
1 detect opencode/opencode2 (PATH + ~/.opencode/bin, ~/.local/bin, npm/bun/brew dirs; `--version` -> major) ·
2 install if missing (data table in `installer.cpp`; always shows the exact command and asks; no silent `curl|bash`) ·
3 choose config file (global `~/.config/opencode/opencode.json[c]`, or project; JSONC parsed; broken file => abort untouched) ·
4 choose dialect (flag > existing style > V2-only-installed ? V2 : V1) ·
5 MCP presets: Context7 (remote `https://mcp.context7.com/mcp`, optional key header), Playwright (`npx @playwright/mcp@latest`, offers `npx playwright install chromium`), Sequential Thinking, GitHub (remote, token) ·
6 OpenAI-compatible provider (`@ai-sdk/openai-compatible`; default URL `http://127.0.0.1:8080/v1`; `GET /models`; context/output limits; tiny chat test; optional default model; llama.cpp tip: needs `--jinja`, context <= server `-c`) ·
7 set Qute Agent's `serverCommand` ·
8 review/diff, backup `<file>.qute-bak-<timestamp>`, atomic write, optional `<bin> mcp list`.
Flags: `--yes --dry-run --channel --method --no-install --force --scope --project-dir --config --format --mcp --no-mcp --no-provider --no-qute --base-url --provider-id --provider-name --api-key-env --models --context --output --default-model`.
Known limits: comments in `.jsonc` are lost on save (backup exists); JSON keys written alphabetically.

## 4. opencode facts gathered (researched Nov 2026; docs conflict in places — trust the live server `/doc`)
Sources: https://opencode.ai/docs/migrate-v1.md · https://opencode.ai/docs/api · https://opencode.ai/docs/build/client/ · https://opencode.ai/v2/docs/ · https://v2.opencode.ai/mcp-servers.md · https://opencode.ai/docs/ (V1 install) · GitHub issues #38458, #27391, #26697, #55701, #62681 (SSE problems) · PR #41930 (V2 lifecycle).
- **V1 vs V2 install/command is inconsistent upstream:** newest migration doc says both use `opencode` (V2 curl installer replaces V1); older V2-beta docs say V2 installs as `opencode2` (`npm i -g @opencode/cli@beta`, `curl -fsSL https://opencode.ai/v2/install | bash`, `paru -S opencode-beta`). V1: `curl -fsSL https://opencode.ai/install | bash`, `npm i -g opencode-ai`, `brew install anomalyco/tap/opencode`, `paru -S opencode-bin`. Official Arch package `opencode` = V2; Homebrew formula `opencode` = V2. The wizard probes both binary names.
- **Config files (same for V1/V2):** `~/.config/opencode/opencode.json(c)`, `<project>/opencode.json(c)`, `<project>/.opencode/opencode.json(c)`. V2 normalizes V1 syntax in memory without rewriting.
- **V2 native config changes:** `mcp.servers.<name>`, `disabled` instead of `enabled`, `mcp.timeout.{startup,catalog,execution}`; `provider` -> `providers` with `package: "aisdk:@ai-sdk/openai-compatible"`, `settings.baseURL/apiKey`; `agent` -> `agents` (`system`, `disabled`, `model: "p/m#variant"`); `permission` -> ordered `permissions` array (`bash` -> `shell`); `command` -> `commands`; `snapshot` -> `snapshots`; `attachment` -> `media`; V2 allows external_directory by default; TUI config moves to `~/.config/opencode/cli.json`; plugins API is new; LSP is accepted but not run.
- **V2 MCP CLI:** `opencode mcp add NAME --url URL --header 'K={env:V}'` / `-- npx ...`, `mcp list`, `mcp auth`. Remote servers use MCP Streamable HTTP. Context7 header example: `CONTEXT7_API_KEY: {env:CONTEXT7_API_KEY}`.
- **Server auth:** V1 `OPENCODE_SERVER_USERNAME/PASSWORD`; V2 `OPENCODE_PASSWORD` (old var still works), username always `opencode`.
- **V1 API:** `GET /global/health` -> `{healthy,version}`, `POST /session`, `POST /session/:id/prompt_async` (204), `POST /session/:id/abort`, `POST /session/:id/permissions/:pid {response}`, `GET /event` (SSE, first event `server.connected`), `GET /session/status`, `GET /session/:id/message`, OpenAPI at `/doc`.
- **V2 API (`/api/*`, "experimental HttpApi", 170 operations):** `GET /api/info`, `POST /api/session {location:{directory}}`, `POST /api/session/:id/prompt` (admits an input; returns before the answer; body shape UNVERIFIED: `{prompt:{text}}` vs `{text}`), `POST /api/session/:id/interrupt`, `GET /api/session/active`, `GET /api/session/:id/message?order&limit&type`, `GET /api/event` (SSE: `server.connected`, heartbeats; **volatile, not replayed, may close**), permissions/forms/questions under `/api/permission/*`, `/api/session/:id/form/*`. Official JS client `@opencode/client`; background service `opencode serve --service`.
- **Known bugs reported upstream (reason for the poll-based design):** SSE `/event` closing after `server.connected` (several 1.14.x–1.18.x issues); V2 `/api/event` closing mid-turn.

## 5. Risk list (nothing below was tested)
1. Whole project: compile errors expected (CMake `qt_add_qml_module`/`loadFromModule("Qute.Overlay","Main")`, MOC in static libs with target_sources from subdirs, QML `Qt.styleHints.colorScheme`).
2. `kwinshell.cpp`: D-Bus `org.kde.KWin /Scripting loadScript` -> `/Scripting/Script<id>.run` (fallback `Scripting.start`); script API: `workspace.cursorPos`, `windowList()`, `clientArea(KWin.PlacementArea, w)`, assign full `frameGeometry {x,y,width,height}`.
3. Keyboard focus of the prompt box on KWin (frameless window, no layer-shell) and on macOS (`macshell.mm` activation logic).
4. QML: `AnimatedSprite` re-binding when source/frameCount change; transparent windows (`QQuickWindow::setDefaultAlphaBuffer(true)`).
5. `OpenCodeClient` V2 paths, event names, message shape, permission answering (intentionally disabled on V2).
6. `installer.cpp` commands (upstream docs disagree), `TerminalLauncher`, macOS bundle/CMake logic.
7. Art: only ONE character (Qute Agent) with 3 states (idle/thinking/done); user wants >= 4 characters, all states, all colors. Cheeks look greenish in some hue-shifted variants.

## 6. Hard rules for whoever continues (also in AGENTS.md)
Don't guess APIs (check Qt/KWin/opencode docs and installed headers). Platform code only in `src/platform/`. QML = looks only. Sprites = data. No in-app global shortcut. Settings via `AppConfig` Q_PROPERTY + `NOTIFY changed`. `AgentState` stays pure + tested. Edit opencode config only through `OpencodeConfig` (backup, keep unknown keys, support both dialects). Never store secrets in opencode.json. No GUI classes in `src/setup/`. Both executables must use `setApplicationName("qute")` (shared config path).

## 7. Next steps (see docs/ROADMAP.md for acceptance tests)
M0 build + `ctest` · M1 transparent window + sprite + typing/Esc · M2 daemon/`qute toggle` single instance · M2b setup wizard (`doctor`, `--dry-run`, real install) · M3 cursor placement (all 4 corners, multi-monitor; fallback to window rule) · M4 opencode V1 end-to-end · M4b V2 (walk the TODO list in docs/OPENCODE.md) · M5 full state mapping + permissions · M6 settings app live-apply · M7 three more characters, more states · M1m/M3m repeat on macOS · M8 packaging (AUR, brew cask, signing/notarization), Windows, XFCE, GNOME extension.
Ask the user to paste exact compiler errors / KWin logs; fix with evidence.

## 8. Open questions / not decided
Final project name; remaining 3+ character designs; GNOME extension approach; Windows hotkey story (.lnk/AutoHotkey); whether to add a tray icon; whether to publish on GitHub (user asked, no decision).
