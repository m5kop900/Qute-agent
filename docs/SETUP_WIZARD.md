# Setup wizard: `qute opencode setup`

A terminal program (no display needed, works over SSH). Code: `src/setup/`. Entry: `main()` in `src/overlay/main.cpp`
sends `qute opencode ...` and `qute setup` to `SetupCli::run()` *before* any GUI object is created.

```
qute opencode setup      full wizard (alias: qute setup)
qute opencode install    only find/install opencode
qute opencode mcp        only MCP servers      (mcp list | mcp remove <name>)
qute opencode provider   only the OpenAI-compatible provider
qute opencode doctor     diagnose everything
```
Add `-y/--yes` (no questions), `--dry-run` (print the resulting JSON, change nothing). `qute opencode setup --help` lists all flags.

## What `setup` does, step by step

1. **Detect opencode.** `OpencodeLocator` looks for `opencode` and `opencode2` in PATH *and* in places installers use but
   PATH often lacks (`~/.opencode/bin`, `~/.local/bin`, `~/.npm-global/bin`, `~/.bun/bin`, brew dirs). Each binary is run
   with `--version`; the major number says V1 or V2 (symlinks to the same file are de-duplicated).
2. **Install (only if missing, or `--force`).** Choose OpenCode 2 or 1, then one of the installers valid for this OS that
   have their tool present (pacman, brew, npm, paru/yay, curl, choco, scoop). The exact command is printed and needs a
   confirmation; `curl | bash` is never run silently. After it finishes the binary is detected again and a PATH hint is
   shown if it is not callable yet. The table lives in `installer.cpp` (docs for V1/V2 install conflict upstream, so it is
   data, not logic; update it there).
3. **Pick the config file.** Global (`~/.config/opencode/opencode.json[c]`, honors XDG_CONFIG_HOME) or the current project
   (`./opencode.json[c]`), or `--config <file>`. The file is parsed as JSONC (comments, trailing commas). If it cannot be
   parsed the wizard stops and touches nothing.
4. **Decide the dialect** (`resolveFormat`): `--format` > the style already in the file > (only V2 installed ? native V2 :
   V1 syntax). V1 syntax is read by both OpenCode 1 and 2; native V2 (`mcp.servers`, `providers`, `package`, `settings`)
   is NOT readable by V1. OpenCode 2 does not rewrite your file, so a V1-style file keeps working there.
5. **MCP servers** (`mcppresets.cpp`): multi-select from Context7 (remote, optional API key), Playwright
   (`npx @playwright/mcp@latest`, offers `npx playwright install chromium`), Sequential Thinking, GitHub (remote, token).
   Missing `npx` is reported with a link to Node.js. Existing servers are only overwritten after asking. Secrets are stored
   as a `{file:...}` reference to a 0600 file or as `{env:NAME}` - never as plain text in `opencode.json`.
6. **OpenCode-has-no-menu-for-this: OpenAI-compatible provider.** Asks base URL (default `http://127.0.0.1:8080/v1`, the
   llama.cpp default), id, name, key mode (none / stored file / env var), then `GET {base}/models` to list models,
   context/output limits, an optional tiny `POST /chat/completions` test, and offers it as the default model. Written as
   `@ai-sdk/openai-compatible` (V1 `provider` or V2 `providers`). Hints: llama.cpp needs `--jinja` for tool calls and the
   context must not exceed the server's `-c`.
7. **Connect Qute Agent.** Sets `serverCommand` in Qute Agent's own config to the detected binary (full path when it is not in PATH).
8. **Review, write, verify.** Lists the changes, (with `--dry-run` prints the JSON), asks before writing, backs the old file
   up as `<file>.qute-bak-YYYYMMDD-HHMMSS`, writes atomically, optionally runs `<bin> mcp list`, prints next steps.

macOS: Homebrew is the preferred installer (`brew install opencode` = V2); GUI-launched processes have a short PATH, so the
locator also searches `/opt/homebrew/bin`. The "Set up opencode" button opens Terminal.app.

Known limits: comments in `.jsonc` are lost when saving (backup exists); JSON keys are written alphabetically (Qt).

## Runtime link with the overlay
If the overlay cannot find opencode it shows "opencode is not installed. Run: qute opencode install" with a
**Set up opencode** button (`OverlayController::runSetup` -> `TerminalLauncher` opens a terminal with the wizard).

## Rules for changing this code
- No GUI classes in `src/setup/`. Only `Tty::` for terminal IO.
- A new MCP server = one entry in `mcppresets.cpp`. A new installer = one entry in `installer.cpp`.
- Config edits only through `OpencodeConfig` (keeps unknown keys, backs up). Add a test in `tests/tst_opencodeconfig.cpp`.
- Never write secrets into opencode.json in plain text.
