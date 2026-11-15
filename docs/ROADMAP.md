# Roadmap for the coding agent (do in order; each milestone has an acceptance test)

**M0 – Builds.** `cmake` configures, everything compiles, `ctest` passes (SseParser + AgentState tests).
Fix compile errors only; no redesign.

**M1 – Window + sprite.** `qute` shows a transparent frameless window with the animated Qute Agent sprite and the
prompt box. Accept: no black/white background, sprite animates, typing works, Esc hides.
If the prompt cannot get keyboard focus on KWin, investigate before continuing (largest known risk).

✅ **DONE** — verified on Wayland: transparent frameless window renders (ghost sprite + prompt box visible floating over
the desktop, no black/white background), sprite animates (6 frames/state across all 5 variants), prompt box renders,
`Escape` hides via `controller.hide()`. Build/tests green (M0). Confirmed with `spectacle`.

**M2 – Command + daemon.** `qute --daemon`, then `qute toggle` shows/hides the same instance (no second process).
`qute quit` stops it. Accept: `pgrep qute` shows exactly one process after 5 toggles.

**M2b – Setup wizard** (docs/SETUP_WIZARD.md). `qute opencode doctor` runs; `qute opencode setup --dry-run -y --mcp recommended
--base-url http://127.0.0.1:8080/v1 --models test` prints valid JSON; the unit tests `tst_jsonc` and `tst_opencodeconfig` pass.
Then test the real thing: install on a clean user (or with `--method`), add Playwright + Context7, run `opencode mcp list`.

**M3 – Cursor placement (KDE).** `qute toggle` opens near the mouse cursor on multi-monitor, stays fully on screen,
keeps room for the tallest size. Accept: test at all 4 screen corners. If the KWin script path fails, fall back to the
window-rule approach in docs/PLATFORMS.md.

**M4 – opencode connection.** With `opencode serve` running (or auto-started), the prompt is sent and the answer
streams into the panel. Accept: ask "say hi" → text appears incrementally; wrong password shows an error.

**M4b – OpenCode 2 server.** Run against a V2 server. Walk through the TODO list in docs/OPENCODE.md (prompt body,
event names, message shape) and fix `OpenCodeClient` with evidence. Accept: same behavior as M4 on V1 and V2.

**M5 – State mapping.** thinking → working (tool name shown) → waiting (Allow/Always/Deny works) → done → idle;
error state on failure; offline when the server dies and recovery when it returns.
Add: reconcile final text via `GET /session/:id/message` after idle.

**M6 – Settings app.** All AppConfig fields editable; Save applies LIVE to a running overlay (no restart).

**M7 – Sprite packs.** Add ≥3 more characters (docs/SPRITE_PACK.md) with all states and ≥5 variants each;
pack/variant switching in settings works live.

**M1m / M3m – macOS check** (docs/MACOS.md, 'Known unknowns'): repeat M1-M3 on a Mac: transparent window, keyboard focus after
`qute toggle` from another app, cursor placement on multi-monitor, appears over full-screen apps.

**M8 – Packaging & ports.** AUR PKGBUILD + install rules check; macOS signing/notarization + Homebrew cask; then Windows (`GenericShell` + hotkey docs), XFCE,
GNOME extension (docs/PLATFORMS.md).
