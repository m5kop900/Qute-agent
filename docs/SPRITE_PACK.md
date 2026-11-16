# Sprite pack format

A pack is a folder (searched in: `$QUTE_PACKS`, `~/.local/share/qute/spritepacks`, `<prefix>/share/qute/spritepacks`,
and the repo `assets/spritepacks` when running from the build tree). Earlier paths win on duplicate ids.

```
mypack/
  manifest.json
  <variant>/idle.png  thinking.png  done.png ...   (or a flat layout without {variant})
```

## manifest.json
```json
{
  "id": "qute", "name": "Qute Agent", "author": "...", "version": 1,
  "frame": { "width": 256, "height": 256 },
  "variants": ["ocean", "mint", "grape", "rose", "amber"],
  "states": {
    "idle":     { "file": "{variant}/idle.png",     "frames": 6, "fps": 6 },
    "thinking": { "file": "{variant}/thinking.png", "frames": 6, "fps": 8 }
  },
  "fallback": { "working": "thinking", "waiting": "thinking", "error": "idle", "offline": "idle", "done": "idle" }
}
```
- Sheet = ONE horizontal row of `frames` frames, each `frame.width × frame.height`, transparent PNG.
- State names: `offline idle thinking working waiting done error`. Missing states follow `fallback` (max 5 hops, then `idle`).
- `{variant}` in `file` is replaced by the selected variant; if the variant is unknown the first one is used.
- Color variants are pre-rendered files (hue-shifted). Keep the sprite readable on light AND dark backgrounds.

## Built-in packs (M7) - all 4 are complete
| id | character | notes |
|---|---|---|
| `qute` | little ghost (original design) | per-state body colors (blue/purple/teal/yellow/orange/red/gray) |
| `blob` | squishy slime | squash & stretch, flat bottom, ground shadow |
| `bot` | tiny robot | screen face, antenna light that changes color per state, swinging arms |
| `cat` | round cat | ears twitch, paw raised when waiting |

Every built-in pack defines ALL 7 states (`idle thinking working waiting done error offline`), 6 frames each, 256x256 frames,
and the SAME 5 variant names (`ocean mint grape rose amber`) so the user's color choice survives switching characters.
Only the body carries the variant color; eyes, props and badges are drawn on a separate layer and are never hue-shifted
(a red error badge stays red in every variant).

| state | what the art shows |
|---|---|
| idle | breathing bob, blinking |
| thinking | eyes look up, growing "..." bubble |
| working | focused eyes, two spinning cogs |
| waiting | wide eyes, hopping, "?" bubble (needs your permission/answer) |
| done | happy eyes, jump, sparkles |
| error | X eyes, shaking, red "!" badge, sweat drop |
| offline | dimmed, asleep, floating "Z"s |

## Tools (dev only, need Pillow; the app itself never needs Python)
- `python3 tools/gen_packs.py [pack ...]` regenerates the built-in packs (draws every frame in code; ~30 s for all four).
  To add a character: write `build_<name>(state, t) -> (body_layer, top_layer, ground_y|None)` and register it in `CHARACTERS`.
- `python3 tools/validate_packs.py` checks manifests, all states/variants, sheet sizes, alpha, corners. Exit code 1 on problems.
- `tests/tst_spritepacks.cpp` runs the same rules inside `ctest`.

## Rules for a hand-drawn / third-party pack
Same manifest. Missing states fall back via `fallback`, but built-in packs must be complete (the tests enforce it).
Sheets: transparent PNG, one row, no background, transparent corner pixels, readable on light and dark backgrounds.
