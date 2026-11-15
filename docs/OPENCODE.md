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

## TODO (verify against the running server, then fix code + this table)
1. V2 prompt body: `{prompt:{text}}` vs flat `{text}` (client retries both; confirm and drop one).
2. V2 permission / question handling (`/api/permission/*`, "forms"): `answerPermission` is intentionally disabled for V2.
3. V2 projected message shape used by `assistantTextAfterLastUser` (defensive parser; add real samples to the test).
4. V2 server start flags: assumed `opencode serve --hostname H --port P`; docs also mention `opencode serve --service`.
5. V1 permission reply endpoint may have changed to `/permission/:id/reply` in newer 1.x builds.
