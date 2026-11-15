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

## Requirements for the 4+ built-in characters (TODO: only `qute` exists)
Each pack needs idle, thinking, working, waiting, done, error (+ optional offline), ≥5 variants.
`tools/gen_qute_pack.py` shows how the Qute Agent pack is generated; other packs can be hand-drawn (Aseprite/Krita).
