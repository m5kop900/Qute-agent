# opencode integration (V1 and V2)

Live specs: V1 `http://127.0.0.1:4096/doc`; V2 API reference https://opencode.ai/docs/api (OpenAPI at /docs/openapi.json).
Migration guide: https://opencode.ai/docs/migrate-v1.md . **Upstream moves fast and its docs disagree in places - trust
the running server's `/doc` and fix this file.**

## Generations
| | V1 | V2 |
|---|---|---|
| command | `opencode` | `opencode` (newer docs) / `opencode2` (beta docs) - the wizard handles both |
| config files | `~/.config/opencode/opencode.json(c)`, `<project>/opencode.json(c)` | same locations; V1 syntax still read |
| MCP | `mcp.<name>` + `enabled` | `mcp.servers.<name>` + `disabled`, `mcp.timeout.{startup,catalog,execution}` |
| providers | `provider.<id>` {npm, api/options.baseURL, models} | `providers.<id>` {package:"aisdk:...", settings.baseURL, models} |
| server auth | Basic, user `OPENCODE_SERVER_USERNAME`, pw `OPENCODE_SERVER_PASSWORD` | Basic, user always `opencode`, pw `OPENCODE_PASSWORD` (old var still works) |
| server API | `/global/health`, `/session`, `/event` | `/api/info`, `/api/session`, `/api/event` (new contracts) |

## Server API used by `OpenCodeClient` (src/core/opencodeclient.cpp)
Generation is detected from `GET /global/health` -> `version` (major >= 2 => V2), else by probing `/api/info`;
override with config `apiFlavor` = `v1` | `v2`.

| Purpose | V1 | V2 |
|---|---|---|
| create session | `POST /session` `{title}` | `POST /api/session` `{location:{directory}}` |
| send prompt | `POST /session/:id/prompt_async` `{parts:[{type:"text",text}], model?}` -> 204 | `POST /api/session/:id/prompt` `{prompt:{text}}` (retries `{text}` once on 400/422). Returns an *admitted input*, not the answer |
| abort | `POST /session/:id/abort` | `POST /api/session/:id/interrupt` |
| events | `GET /event` SSE | `GET /api/event` SSE (first event `server.connected`, heartbeats, **volatile: not replayed, may close**) |
| "is it still running?" | `GET /session/status` -> `{id:{type:busy|retry}}`, absent = idle | `GET /api/session/active` -> sessions currently executing, absent = idle |
| final text | `GET /session/:id/message` -> `[{info:{role},parts:[]}]` | `GET /api/session/:id/message?order=desc` -> projected messages (`type: user|assistant|...`) |

### Design rule: events are hints, polls are truth
Known upstream bugs made SSE streams close early or omit events. So after a prompt the client polls the status route every
1.5 s (after a 2.5 s grace period; 2 quiet polls = turn finished), emits a synthetic `session.status idle`, and then
reconciles the final answer from the message list. Events still give live streaming text and tool activity.

### Events handled (names verified from V1 docs / community reports; V2 names partly unverified)
`session.status`, `session.idle`, `session.error`, `message.updated`, `message.part.updated`, `message.part.delta`,
`permission.updated` | `permission.asked`, `permission.replied`, `session.execution.started|succeeded|failed` (V2).

## Verified against a real OpenCode 2.2.6 server (M4, commit c3e6e27)
- V2 prompt body is the FLAT form `{"text": "..."}`. The schema has `additionalProperties:false`, so `{"prompt":{"text":..}}` gets HTTP 400.
  (`OpenCodeClient` sends flat first and keeps the nested form only as a retry for older builds.)
- V2 does NOT emit `message.part.*`. The answer streams as `session.text.started|delta|ended` with
  `{assistantMessageID, ordinal, delta | text}`; `ended` carries the authoritative full block text.
- A wrong server password gives 401/403 on `/global/health` and `/api/info`; the client reports it instead of retrying silently.
- The running V2 server publishes its schema at `/openapi.json` - read it before changing any V2 code.

## V2 wire facts taken from upstream source/docs (M5; NOT yet exercised against a live server)
Sources: opencode API reference https://opencode.ai/docs/api and https://opencode.ai/v2/docs/api/* ; reducer tests in
PR anomalyco/opencode#40539 (`id` replaced `callID` in tool events); issue #35638 (tool `name` is only on `input.started`);
the V2 audit quoted in GetterSethya/Opencode-Android-Client#3.
- **SSE envelope:** frame data = `{id, type, created, data, location?, metadata?}`. Our `OpenCodeClient::onEventData` hands `data` on as the
  event properties (falls back to `properties` for V1).
- **Tool events** (`data` = `{sessionID, assistantMessageID, id, ...}`; `id` is the call id):
  `session.tool.input.started {name}` -> `input.delta {delta}` (ephemeral) -> `input.ended {text}` -> `session.tool.called {input, executed, state?}`
  -> `session.tool.progress` (ephemeral) -> `session.tool.success` | `session.tool.failed`. `AgentState` keeps a set of running call ids and
  stays in `working` until the last one finishes.
- **Other state events:** `session.execution.started|succeeded|failed|interrupted`, `session.step.started|ended|failed`,
  `session.reasoning.started|delta|ended`, `session.text.started|delta|ended`, `session.shell.started|ended`,
  `session.compaction.*`, `session.retry.scheduled`, ephemeral `session.status {status:{type}}` and `session.idle`.
- **Permissions:** event `permission.asked` with `Permission.Request {id, sessionID, action, resources[], save?, metadata?, source?{type:"tool",messageID,callID}, message?}`,
  `permission.replied`. Reply: `POST /api/session/{sessionID}/permission/{requestID}/reply` with `{"decision":"once|always|reject"}`.
  The request can belong to a CHILD (sub-agent) session: answer in the session that asked (`OpenCodeClient::m_requestSession`).
- **Questions are gone, replaced by Forms:** events `form.created|replied|cancelled`; reply `POST /api/session/{sid}/form/{fid}/reply {answer:{...}}`,
  cancel `DELETE /api/session/{sid}/form/{fid}`. The overlay shows the title and offers "Cancel question"; answering fields is NOT implemented.
- **Projected messages** (`GET /api/session/{id}/message`): `{data:[...], cursor}`; assistant message = `{type:"assistant", agent, providerID, content:[{type:"text"|"reasoning"|"tool", ...}], time}`;
  text items carry `text`, tool items `{id, name, state}`.

## Decision: OpenCode 2 is the target
The V1 code paths (`/session`, `/event`, `message.part.*`, `/session/:id/permissions/:pid`) still exist and have tests, but get no new
features. Removing them is an optional clean-up (see ROADMAP "M5c").

## F0 transport additions (catalog + history; endpoint shapes (verify))
`OpenCodeClient` gained two small transport primitives so the services never touch HTTP:
- `requestMessageHistory(limit)` -> `GET /api/session/{id}/message?order=desc&limit=N` (V2) and emits
  `messageHistoryReceived(QJsonArray)` oldest-first. `SessionStore` reconciles the conversation timeline from it.
- `fetchJson(path)` -> `GET <path>` and emits `jsonFetched(path, document, httpStatus)`.
  `OpenCodeCatalog` uses it for `GET /api/command`, `/api/model`, `/api/agent` and `/api/fs/find?query=...`.
  These four paths come from the V2 docs, not from a live server yet: read `/openapi.json` and fix the parsers in
  `src/core/opencodecatalog.cpp` if a shape differs. The parsers tolerate array, `{data:[...]}` and `{providers:[...]}`.

## Connection failures are reported, not retried silently
`OpenCodeClient::notRunning()` distinguishes the cases where waiting can never help and sets `lastError`
(with a `Settings` button in the overlay, `OverlayController::runSettings()`):
- remote `serverUrl` (not 127.0.0.1/localhost) -> "not reachable at <url>, check URL/password"
- local but `autoStartServer = false` -> "not running at <url>, enable Auto-start server, or start it: <cmd> serve --hostname <h> --port <p>"
- autostart on, server spawned -> no error while it boots; `QProcess::finished` reports "server exited (code N)"/"crashed"
  and re-arms the retry instead of retrying forever behind a "Connecting…" placeholder.
Errors clear automatically on `setConnected(true)`. Tests: `tst_clientparse` (unreachable without autostart, spawned
server exits).

## Still open (verify against the running server, then fix code + this table)
1. Run the checklist in ROADMAP "M5 live check" against a real V2 server. Record any event whose payload differs from the list above.
2. Sub-agent child sessions: the `parentID` field name on `session.created` is a guess (`parentID` / `info.parentID` / `session.parentID`).
3. V2 server start flags: assumed `opencode serve --hostname H --port P`; docs also mention `opencode serve --service`.
4. Form answering (fields: string/number/boolean/multiselect/external) is not implemented; only cancel.
