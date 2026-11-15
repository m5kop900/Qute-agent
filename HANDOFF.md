# Qute Agent — Handoff State

Copy this file into your brief for the next AI. It captures the full project state,
running processes, and the exact commands needed to continue.

---

## Project

- **Repo:** `/home/mkop/Downloads/wisp/wisp/wisp-project` (branch `main`, clean tree)
- **Binary name:** `qute` (overlay/daemon + CLI) and `qute-settings` (settings app).
  No trace of "wisp" except in `UNIX`/`Unix`/`Q_OS_UNIX`.
- **Stack:** C++20 + Qt 6.5+ / QML. Build: `cmake -S . -B build -G Ninja`, `cmake --build build`, `ctest --test-dir build --output-on-failure`.
- **Docs:** `docs/ROADMAP.md` (M0–M8, acceptance tests), `docs/OPENCODE.md` (V1/V2 API notes), `docs/PLATFORMS.md`, `docs/SPRITE_PACK.md`.
- **Current git head:** `ed653ed` — M3 (cursor placement, multi-monitor safe).

## Roadmap progress (M0–M4)

- **M0** initial commit (wisp→qute rename). **M1** window+sprite. **M2** daemon+toggle. **M3** cursor placement. → all **DONE, committed**.
- **M4 (opencode connection):** ask "say hi" → text streams into panel; wrong password shows error.
  - **Status: IN PROGRESS / unverified.** The prompt box works (text reaches it), but Enter has not been confirmed to submit.
  - **Action for next AI:** complete + verify M4, then commit before M4b/M5.

## Running processes (keep these alive)

- **qute daemon:** PID 375841, `./build/src/overlay/qute --daemon`. Starts **hidden**; run `./build/src/overlay/qute show` (or `toggle`) to display the window.
- **Test opencode serve:** `OPENCODE_CONFIG_PATH=/tmp/octest/opencode.json OPENCODE_DIR=/tmp/octest/data opencode serve --hostname 127.0.0.1 --port 4098` (model `local2/default` → 8082). Logs: `/tmp/octest/serve.log`.

## Config

- `~/.config/qute/config.json`: `serverUrl` `http://127.0.0.1:4098`, username `opencode`, password `b13auUoL_4IpCL_Bewm4_B1AoxawZnRfOAHcnGRgNgI`, `apiFlavor` `v2`, `packId` `qute`, `variant` `ocean`.

## Model backends

- **8081 = Groot** runs with `-np 1`. Sending a concurrent request interrupts it. **Avoid 8081.**
- **8082 = Secondary Model (LFM3-1.7B)**. Up and answering. Use this for isolated tests.
- User's real opencode config uses `local2/default` → 8082 on the main 4096 server (auth required).

## Testing / GUI input notes (Wayland / KWin)

- `wtype` fails ("Compositor does not support the virtual keyboard protocol").
- **`ydotool` works** but needs `ydotool daemon` running (socket `/run/user/1000/.ydotool_socket`).
  Commands: `mousemove x y`, `click x y`, `type "text"`, `key enter`.
- qute overlay window title is `qute-overlay`. KWin D-Bus scripting `print()` output is not captured reliably; verify placement via screenshots (`spectacle -b 0 -o /tmp/octest/x.png`).
- **PromptBox submit logic** (`src/overlay/PromptBox.qml`): `onAccepted` trims text and only submits if non-empty, then clears the field. `onSubmitted` → `client.sendPrompt(text)`.

## Next milestones (per ROADMAP.md)

- **M4b:** OpenCode 2 server integration refinements.
- **M5:** state mapping (opencode events → AgentState machine: offline/idle/thinking/working/waiting/done/error).
