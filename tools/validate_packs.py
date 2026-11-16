#!/usr/bin/env python3
"""Validates every sprite pack in assets/spritepacks (dev/CI tool, needs Pillow). Exit code 1 on any problem.
Rules (docs/SPRITE_PACK.md): manifest parses, all 7 states exist (or are covered by `fallback`; built-in packs must define all),
every <variant>/<state>.png exists, sheet size == frames*frame.width x frame.height, frames are not empty,
all packs share the same variant names, at least 4 built-in packs and 5 variants."""
import json, os, sys
from PIL import Image

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "assets", "spritepacks")
STATES = ["idle", "thinking", "working", "waiting", "done", "error", "offline"]
errors, packs = [], {}

for pid in sorted(os.listdir(ROOT)):
    d = os.path.join(ROOT, pid)
    mf = os.path.join(d, "manifest.json")
    if not os.path.isdir(d) or not os.path.exists(mf):
        continue
    try:
        m = json.load(open(mf))
    except Exception as e:
        errors.append(f"{pid}: manifest.json invalid: {e}"); continue
    if m.get("id") != pid: errors.append(f"{pid}: manifest id '{m.get('id')}' != folder name")
    fw, fh = m["frame"]["width"], m["frame"]["height"]
    variants = m.get("variants", [])
    packs[pid] = variants
    if len(variants) < 5: errors.append(f"{pid}: needs >= 5 variants, has {len(variants)}")
    for s in STATES:
        if s not in m["states"]: errors.append(f"{pid}: state '{s}' missing in manifest"); continue
        info = m["states"][s]
        for v in variants:
            path = os.path.join(d, info["file"].replace("{variant}", v))
            if not os.path.exists(path): errors.append(f"{pid}/{v}/{s}: file missing"); continue
            im = Image.open(path)
            if im.size != (fw * info["frames"], fh):
                errors.append(f"{pid}/{v}/{s}: size {im.size}, expected {(fw * info['frames'], fh)}")
            if im.mode != "RGBA": errors.append(f"{pid}/{v}/{s}: mode {im.mode}, expected RGBA")
            alpha = im.getchannel("A") if im.mode == "RGBA" else None
            for i in range(info["frames"]):
                if alpha is not None and alpha.crop((i * fw, 0, (i + 1) * fw, fh)).getbbox() is None:
                    errors.append(f"{pid}/{v}/{s}: frame {i} is fully transparent")
            if alpha is not None:                         # corners must be transparent (no opaque background)
                for (x, y) in [(0, 0), (im.width - 1, 0), (0, im.height - 1), (im.width - 1, im.height - 1)]:
                    if alpha.getpixel((x, y)) != 0: errors.append(f"{pid}/{v}/{s}: opaque corner pixel at {(x, y)}")

if len(packs) < 4: errors.append(f"need >= 4 packs, found {len(packs)}: {list(packs)}")
ref = next(iter(packs.values()), [])
for pid, v in packs.items():
    if v != ref: errors.append(f"{pid}: variants {v} differ from the other packs {ref}")

print(f"packs: {list(packs)}  variants: {ref}")
for e in errors: print("ERROR", e)
print("OK" if not errors else f"{len(errors)} problem(s)")
sys.exit(1 if errors else 0)
