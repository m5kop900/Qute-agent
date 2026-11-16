# Qute Agent — Handoff State

Copy this file into your brief for the next AI. Longer background: `docs/HANDOFF.md` (written before the rename from "wisp",
read "wisp" as "qute") and `AGENTS.md` (rules). Per-milestone evidence: `docs/ROADMAP.md` -> "Review status".

## Project
- **Repo:** `/home/mkop/Downloads/wisp/wisp/wisp-project`, branch `main`.
- **Binaries:** `qute` (overlay/daemon + CLI) and `qute-settings`. Stack: C++20 + Qt 6.5+ / QML.
- **Build:** `cmake -S . -B build -G Ninja`, `cmake --build build`, `ctest --test-dir build --output-on-failure` (6 tests).
- **Sprite packs:** `qute`, `blob`, `bot`, `cat` (7 states x 6 frames x 5 variants each). Regenerate: `python3 tools/gen_packs.py`;
  validate: `python3 tools/validate_packs.py`.

## Milestone state
- **Target:** OpenCode **2** only. V1 code still exists but gets no new work (optional clean-up = M5c in docs/ROADMAP.md).
- **Done:** M0, M1, M2, M3 (KDE Wayland), M4 (opencode 2.2.6), M7 (packs; GUI live-switch check still to do by hand).
- **Partly:** M4b (open items in docs/OPENCODE.md), M6 (settings: all fields now editable; live-apply unverified).
- **Written, needs live check:** M5 on V2 (tool/shell/step events, permission reply, forms-cancel, child sessions). Checklist: docs/ROADMAP.md "M5 live check".
- **Not done:** M2b (setup wizard never run for real), M1m/M3m (macOS), M8.
- **Feature work (Part 2, F0–F7):** the product feature spec is in docs/ROADMAP.md ("Part 2"). Order: M5 live check -> F0 foundations -> F4.1 -> F1 -> F2 -> F3 -> F5 -> F6 -> F7.
- **Next best step:** run the M5 live check; fix whatever event payload differs (usually one line in `AgentState::handleEvent`).

## Config (do NOT paste secrets into this file)
- `~/.config/qute/config.json`: `serverUrl`, `serverUsername`, `serverPassword`, `apiFlavor` (`auto|v1|v2`), `packId`, `variant`, ...
- **Security:** an earlier version of this file contained the test server password; it is in git history (commits c3e6e27 and
  2aedfbb, not pushed yet as of this writing). Rotate that password and remove it from history BEFORE pushing
  (`git rebase -i` to edit those two commits, or `git filter-repo --replace-text`).

## Model backends (user's machine)
- **8081 = Groot** runs with `-np 1`. A concurrent request interrupts it. **Avoid 8081 for tests.**
- **8082 = Secondary Model (LFM3-1.7B).** Use this for isolated tests.
- The user's real opencode config uses `local2/default` -> 8082 on the main 4096 server (auth required).
- Test server used so far: `OPENCODE_CONFIG_PATH=/tmp/octest/opencode.json OPENCODE_DIR=/tmp/octest/data opencode serve --hostname 127.0.0.1 --port 4098`.

## Testing / GUI input notes (Wayland / KWin)
- `wtype` fails (compositor lacks the virtual keyboard protocol). **`ydotool` works** but needs `ydotool daemon`
  (socket `/run/user/1000/.ydotool_socket`): `mousemove x y`, `click x y`, `type "text"`, `key enter`.
- Overlay window title is `qute-overlay` (the KWin placement script matches it). KWin script `print()` output is not captured
  reliably; verify placement with screenshots: `spectacle -b 0 -o /tmp/octest/x.png`.
- `PromptBox.qml`: `onAccepted` trims, submits only if non-empty, clears the field; `onSubmitted` -> `client.sendPrompt(text)`.

## Gotchas learned
- **moc and raw string literals:** inside a `Q_OBJECT` test class, `R"(...)"` strings that contain `//` make moc treat the rest as a
  comment and silently drop the class. Use normal escaped strings or split the literal.
- **V2 wire facts** (verified on 2.2.6): flat prompt body `{"text":...}`, answer streams via `session.text.*` events.
