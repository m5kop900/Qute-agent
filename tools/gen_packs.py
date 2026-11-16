#!/usr/bin/env python3
"""Generates ALL built-in sprite packs: qute (ghost), blob, bot, cat.
Dev tool only (needs Pillow), not part of the build:   python3 tools/gen_packs.py [pack ...]

Per pack:  assets/spritepacks/<id>/manifest.json
           assets/spritepacks/<id>/<variant>/<state>.png   (6 frames of 256x256 in ONE row)
States (contract with AgentState): idle thinking working waiting done error offline
Variants (same names in every pack so the user's choice survives switching packs): ocean mint grape rose amber

Rendering: every frame is drawn as two layers.
  body layer = shapes that carry the color (hue-shifted per variant)
  top  layer = eyes, mouth, props, badges (fixed colors, never shifted: a red error badge stays red)
"""
import json, math, os, sys
from functools import lru_cache
from PIL import Image, ImageChops, ImageDraw, ImageFilter

W, S, FR = 256, 3, 6
N = W * S
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "assets", "spritepacks")
STATES = ["idle", "thinking", "working", "waiting", "done", "error", "offline"]
FPS = {"idle": 6, "thinking": 8, "working": 10, "waiting": 6, "done": 8, "error": 8, "offline": 4}
VARIANTS = {"ocean": 0, "mint": -60, "grape": 75, "rose": 130, "amber": -165}   # hue offsets from the blue base (205 deg)
INK = (38, 42, 78, 255)
WHITE = (255, 255, 255, 255)

def sc(v): return int(round(v * S))

# ---------------------------------------------------------------- drawing helpers (coordinates are in 256px space)
def layer():
    im = Image.new("RGBA", (N, N), (0, 0, 0, 0))
    return im, ImageDraw.Draw(im)

def ell(d, cx, cy, rx, ry, fill):
    d.ellipse([sc(cx - rx), sc(cy - ry), sc(cx + rx), sc(cy + ry)], fill=fill)

def poly(d, pts, fill):
    d.polygon([(sc(x), sc(y)) for x, y in pts], fill=fill)

def line(d, pts, fill, w):
    d.line([(sc(x), sc(y)) for x, y in pts], fill=fill, width=max(1, sc(w)), joint="curve")

def rrect(d, x0, y0, x1, y1, r, fill):
    d.rounded_rectangle([sc(x0), sc(y0), sc(x1), sc(y1)], radius=sc(r), fill=fill)

def arc(d, cx, cy, rx, ry, a0, a1, fill, w):
    d.arc([sc(cx - rx), sc(cy - ry), sc(cx + rx), sc(cy + ry)], a0, a1, fill=fill, width=max(1, sc(w)))

def new_mask(): return Image.new("L", (N, N), 0)

def mask_ell(m, cx, cy, rx, ry): ImageDraw.Draw(m).ellipse([sc(cx - rx), sc(cy - ry), sc(cx + rx), sc(cy + ry)], fill=255)
def mask_poly(m, pts): ImageDraw.Draw(m).polygon([(sc(x), sc(y)) for x, y in pts], fill=255)
def mask_rrect(m, x0, y0, x1, y1, r): ImageDraw.Draw(m).rounded_rectangle([sc(x0), sc(y0), sc(x1), sc(y1)], radius=sc(r), fill=255)

@lru_cache(maxsize=None)
def vgrad(top, bot, y0, y1):
    g = Image.new("RGB", (1, N))
    for y in range(N):
        t = max(0.0, min(1.0, (y / S - y0) / float(y1 - y0)))
        g.putpixel((0, y), tuple(int(top[i] + (bot[i] - top[i]) * t) for i in range(3)))
    return g.resize((N, N))

def dim(c):
    g = sum(c) / 3.0
    return tuple(int((g + (v - g) * 0.45) * 0.82) for v in c)

def body_layer(mask, top, bot, glow, y0=50, y1=210, highlight=(86, 62, 126, 92), glow_strength=0.7):
    body = vgrad(tuple(top), tuple(bot), y0, y1).convert("RGBA")
    body.putalpha(mask)
    g = Image.new("RGBA", (N, N), tuple(glow) + (0,))
    g.putalpha(mask.filter(ImageFilter.GaussianBlur(sc(16))).point(lambda v: int(v * glow_strength)))
    out = Image.alpha_composite(g, body)
    if highlight:
        hl = Image.new("RGBA", (N, N), (255, 255, 255, 0))
        ImageDraw.Draw(hl).ellipse([sc(highlight[0]), sc(highlight[1]), sc(highlight[2]), sc(highlight[3])], fill=(255, 255, 255, 115))
        hl = hl.filter(ImageFilter.GaussianBlur(sc(5)))
        hl.putalpha(ImageChops.multiply(hl.getchannel("A"), mask))
        out = Image.alpha_composite(out, hl)
    return out

# ---------------------------------------------------------------- shared face parts
def eyes(d, kind, cx, cy, dx, rx=11, ry=15, look=(0, 0)):
    for s in (-1, 1):
        x = cx + s * dx
        if kind == "open":
            ell(d, x, cy, rx, ry, INK)
            ell(d, x - 5 + look[0], cy - 6 + look[1], 4, 4.5, WHITE)
            ell(d, x + 4 + look[0], cy + 5 + look[1], 2.5, 2.5, (255, 255, 255, 200))
        elif kind == "blink":
            line(d, [(x - rx + 1, cy), (x + rx - 1, cy)], INK, 4)
        elif kind == "happy":
            arc(d, x, cy, 12, 12, 200, 340, INK, 5)
        elif kind == "x":
            line(d, [(x - 8, cy - 8), (x + 8, cy + 8)], INK, 4)
            line(d, [(x - 8, cy + 8), (x + 8, cy - 8)], INK, 4)
        elif kind == "focus":
            ell(d, x, cy, rx, ry * 0.55, INK)
            ell(d, x - 4, cy - 3, 3.5, 3, WHITE)
        elif kind == "sleep":
            arc(d, x, cy - 2, 11, 8, 20, 160, INK, 4)
        elif kind == "wide":
            ell(d, x, cy, rx + 1, ry + 2, INK)
            ell(d, x - 4, cy - 6, 4.5, 5, WHITE)
            ell(d, x + 4, cy + 6, 2.5, 2.5, (255, 255, 255, 200))

def cheeks(d, cx, cy, dx, alpha=120):
    for s in (-1, 1):
        ell(d, cx + s * dx, cy, 9, 5.5, (255, 130, 160, alpha))

def mouth(d, kind, x, y):
    if kind == "smile":
        arc(d, x, y, 10, 8, 20, 160, INK, 3)
    elif kind == "o":
        ell(d, x, y, 5, 4.5, INK)
    elif kind == "open":
        d.pieslice([sc(x - 14), sc(y - 9), sc(x + 14), sc(y + 19)], 0, 180, fill=INK)
        d.pieslice([sc(x - 7), sc(y + 5), sc(x + 7), sc(y + 19)], 0, 180, fill=(255, 120, 140, 255))
    elif kind == "flat":
        line(d, [(x - 7, y), (x + 7, y)], INK, 3)
    elif kind == "wavy":
        line(d, [(x - 10 + i, y + 2.5 * math.sin(i * 0.9)) for i in range(0, 21, 2)], INK, 3)
    elif kind == "tiny":
        ell(d, x, y, 3, 2, INK)

def star(d, cx, cy, r, col):
    pts = []
    for k in range(8):
        rr = r if k % 2 == 0 else r * 0.35
        a = math.pi / 4 * k - math.pi / 2
        pts.append((cx + rr * math.cos(a), cy + rr * math.sin(a)))
    poly(d, pts, col)

def cog(d, cx, cy, r, angle_deg, col):
    pts = []
    for k in range(16):
        rr = r if k % 2 == 0 else r * 0.74
        a = math.radians(angle_deg + k * 22.5)
        pts.append((cx + rr * math.cos(a), cy + rr * math.sin(a)))
    poly(d, pts, col)
    ell(d, cx, cy, r * 0.42, r * 0.42, (0, 0, 0, 0))      # punch the hole (RGBA draw replaces pixels)

def qmark(d, cx, cy, s, col):
    pts = [(cx + 0.5 * s * math.cos(math.radians(a)), cy - 0.35 * s + 0.5 * s * math.sin(math.radians(a))) for a in range(200, 451, 15)]
    line(d, pts, col, 0.22 * s)
    line(d, [(cx, cy + 0.15 * s), (cx, cy + 0.32 * s)], col, 0.22 * s)
    ell(d, cx, cy + 0.62 * s, 0.13 * s, 0.13 * s, col)

def exclaim(d, cx, cy, s, col):
    rrect(d, cx - 0.12 * s, cy - 0.5 * s, cx + 0.12 * s, cy + 0.15 * s, 0.1 * s, col)
    ell(d, cx, cy + 0.42 * s, 0.14 * s, 0.14 * s, col)

def zzz(d, x, y, s, col):
    line(d, [(x, y), (x + s, y), (x, y + s), (x + s, y + s)], col, max(2, s * 0.2))

def props(d, state, t, sway=0):
    """Badges/bubbles/sparkles in the upper right (fixed colors)."""
    ph = 2 * math.pi * t / FR
    if state == "thinking":
        for (x, y, r) in [(200, 74, 5), (214, 54, 8), (232, 28, 11)][:[1, 2, 3, 3, 2, 1][t]]:
            ell(d, x, y, r, r, (255, 255, 255, 235))
    elif state == "working":
        cog(d, 214, 50, 20, t * 7.5, (222, 230, 246, 255))
        cog(d, 190, 82, 11, -t * 7.5 + 11, (190, 205, 235, 255))
    elif state == "waiting":
        r = 21 + 2 * math.sin(ph)
        poly(d, [(196, 62), (204, 72), (190, 82)], (255, 255, 255, 240))
        ell(d, 214, 48, r, r, (255, 255, 255, 245))
        qmark(d, 214, 48, 24, (240, 150, 40, 255))
    elif state == "done":
        for i, (x, y, r, col) in enumerate([(40, 60, 14, (255, 240, 150, 255)), (216, 78, 11, WHITE),
                                            (206, 30, 8, (255, 220, 120, 255)), (52, 180, 8, (255, 255, 255, 230))]):
            star(d, x, y, r * (1 + 0.3 * math.sin(ph + i * 1.3)), col)
    elif state == "error":
        ell(d, 214, 48, 20, 20, (236, 84, 84, 255))
        exclaim(d, 214, 47, 26, WHITE)
        ell(d, 176 + sway, 86 + (t % 3) * 3, 5, 7, (150, 205, 255, 235))      # sweat drop
    elif state == "offline":
        for (x, y, s) in [(196, 76, 10), (212, 52, 14), (230, 24, 18)][:[1, 2, 3, 3, 2, 1][t]]:
            zzz(d, x, y, s, (210, 220, 245, 235))

def motion(state, t):
    ph = 2 * math.pi * t / FR
    if state == "idle":     return 0, round(3 * math.sin(ph))
    if state == "thinking": return 0, round(2 * math.sin(ph))
    if state == "working":  return round(2 * math.sin(2 * ph)), round(1.5 * math.sin(2 * ph))
    if state == "waiting":  return 0, -round(5 * abs(math.sin(ph)))
    if state == "done":     return 0, -round(10 * math.sin(ph / 2))
    if state == "error":    return [-4, 4, -3, 3, -2, 2][t], 0
    return 0, round(2 * math.sin(ph))                      # offline: slow breathing

# ---------------------------------------------------------------- characters
# Each build(state, t) -> (body_img, top_img, ground_y or None)

def eye_kind(state, t):
    return {"idle": "blink" if t == 4 else "open", "thinking": "open", "working": "focus", "waiting": "wide",
            "done": "happy", "error": "x", "offline": "sleep"}[state]

def mouth_kind(state):
    return {"idle": "smile", "thinking": "o", "working": "flat", "waiting": "tiny", "done": "open", "error": "wavy", "offline": "tiny"}[state]

# --- qute: the ghost (original design; per-state palettes kept, new states added)
GHOST_PAL = {
    "idle": ((190, 245, 255), (110, 170, 255), (120, 210, 255)),
    "thinking": ((225, 205, 255), (140, 120, 245), (190, 150, 255)),
    "working": ((190, 250, 230), (90, 190, 190), (120, 230, 210)),
    "waiting": ((255, 240, 200), (255, 190, 120), (255, 215, 130)),
    "done": ((255, 245, 200), (255, 170, 120), (255, 220, 130)),
    "error": ((255, 215, 215), (230, 120, 130), (255, 140, 140)),
    "offline": ((190, 245, 255), (110, 170, 255), (120, 210, 255)),
}

def build_qute(state, t):
    ph = 2 * math.pi * t / FR
    tilt = {"thinking": -7 + 2 * math.sin(ph), "error": [-3, 3, -3, 3, -2, 2][t], "waiting": 3 * math.sin(ph)}.get(state, 0)
    squash = {"done": 1 + 0.04 * math.sin(ph), "offline": 0.9}.get(state, 1.0)
    wave = ph * (2 if state in ("working", "error") else 1)
    cx, cy, R = 128, 112, 62
    bottom = 200 if state == "offline" else 196
    pts = [(cx + R * math.cos(math.radians(a)), cy + R * math.sin(math.radians(a)) * squash) for a in range(180, 361, 3)]
    n = 40
    for i in range(n + 1):
        pts.append((cx + R - 2 * R * i / n, bottom + 7 * math.sin(i / n * 6 * math.pi + wave)))
    c, r = (cx, 150), math.radians(tilt)
    rot = [(c[0] + (x - c[0]) * math.cos(r) - (y - c[1]) * math.sin(r), c[1] + (x - c[0]) * math.sin(r) + (y - c[1]) * math.cos(r)) for x, y in pts]
    m = new_mask(); mask_poly(m, rot)
    top, bot, glow = GHOST_PAL[state]
    if state == "offline": top, bot, glow = dim(top), dim(bot), dim(glow)
    body = body_layer(m, top, bot, glow)

    ti, d = layer()
    lx = 128 - tilt * 0.6
    poly(d, [(lx, 44), (lx - 9, 24), (lx + 2, 8), (lx + 13, 26)], (255, 255, 255, 235))
    poly(d, [(lx, 44), (lx + 13, 26), (lx + 2, 8), (lx + 6, 28)], (255, 255, 255, 140))
    look = (5, -6) if state == "thinking" else (0, 0)
    eyes(d, eye_kind(state, t), 128, 118, 26, look=look)
    cheeks(d, 128, 135, 44)
    mouth(d, mouth_kind(state), 128, 137 if state != "thinking" else 142)
    props(d, state, t)
    return body, ti, None

# --- blob: squishy slime that sits on the ground
def build_blob(state, t):
    ph = 2 * math.pi * t / FR
    sy = {"idle": 1 + 0.07 * math.sin(ph), "thinking": 1 + 0.03 * math.sin(ph), "working": 1 + 0.05 * math.sin(2 * ph),
          "waiting": 1.08 + 0.03 * math.sin(ph), "done": 1 + 0.1 * math.cos(ph / 2 * 2) * (1 if t < 3 else -1) * 0.6,
          "error": 0.9 + 0.02 * math.sin(2 * ph), "offline": 0.78 + 0.02 * math.sin(ph)}[state]
    sx = 1 / sy
    ground = 206
    rx, ry = 72 * sx, 60 * sy
    cy = ground + 10 - ry
    m = new_mask()
    mask_ell(m, 128, cy, rx, ry)
    mask_ell(m, 138, cy - ry - 5, 7, 9)                       # little drip on top
    ImageDraw.Draw(m).rectangle([0, sc(ground), N, N], fill=0)  # flat bottom
    top, bot, glow = (190, 245, 255), (110, 170, 255), (120, 210, 255)
    if state == "offline": top, bot, glow = dim(top), dim(bot), dim(glow)
    body = body_layer(m, top, bot, glow, y0=cy - ry, y1=ground, highlight=(94, cy - ry + 10, 140, cy - ry + 38))

    ti, d = layer()
    ey = cy - 2
    look = (5, -6) if state == "thinking" else (0, 0)
    eyes(d, eye_kind(state, t), 128, ey, 27, rx=12, ry=15, look=look)
    cheeks(d, 128, ey + 20, 46)
    mouth(d, mouth_kind(state), 128, ey + 22 if state != "thinking" else ey + 26)
    props(d, state, t)
    return body, ti, ground

# --- bot: boxy robot with a screen face, antenna light and swinging arms
BULB = {"idle": (120, 235, 150), "thinking": (255, 215, 90), "working": (110, 170, 255), "waiting": (255, 160, 60),
        "done": (120, 235, 150), "error": (240, 80, 80), "offline": (110, 115, 135)}

def build_bot(state, t):
    ph = 2 * math.pi * t / FR
    arm = {"working": 9 * math.sin(2 * ph), "waiting": -14 - 6 * math.sin(ph), "done": -18, "error": 5 * math.sin(2 * ph)}.get(state, 2 * math.sin(ph))
    m = new_mask()
    mask_rrect(m, 76, 96, 180, 198, 24)                       # body
    mask_rrect(m, 90, 196, 112, 210, 6); mask_rrect(m, 144, 196, 166, 210, 6)   # feet
    ImageDraw.Draw(m).rectangle([sc(125), sc(66), sc(131), sc(98)], fill=255)  # antenna stem
    mask_ell(m, 70, 150 + arm, 10, 12); mask_ell(m, 186, 150 - arm * (1 if state != "working" else -1), 10, 12)   # arms
    top, bot, glow = (170, 225, 255), (80, 140, 235), (110, 190, 255)
    if state == "offline": top, bot, glow = dim(top), dim(bot), dim(glow)
    body = body_layer(m, top, bot, glow, y0=70, y1=208, highlight=(90, 100, 140, 120))

    ti, d = layer()
    bulb = BULB[state] + (255,)
    ell(d, 128, 60, 9, 9, bulb)
    ell(d, 125, 57, 3, 3, (255, 255, 255, 200))
    rrect(d, 88, 108, 168, 168, 14, (24, 28, 48, 255))           # screen
    led = (150, 235, 255, 255) if state != "error" else (255, 130, 130, 255)
    if state == "offline": led = (90, 110, 140, 255)
    kind = eye_kind(state, t)
    # LED eyes: rounded rectangles instead of round eyes
    for s in (-1, 1):
        x = 128 + s * 20
        if kind in ("open", "wide"):
            rrect(d, x - 8, 122, x + 8, 146 + (4 if kind == "wide" else 0), 5, led)
        elif kind == "blink" or kind == "sleep":
            line(d, [(x - 8, 136), (x + 8, 136)], led, 4)
        elif kind == "focus":
            rrect(d, x - 9, 130, x + 9, 142, 4, led)
        elif kind == "happy":
            arc(d, x, 138, 10, 10, 200, 340, led, 5)
        elif kind == "x":
            line(d, [(x - 7, 129), (x + 7, 145)], led, 4); line(d, [(x - 7, 145), (x + 7, 129)], led, 4)
    if state == "thinking":
        for i in range(3):
            ell(d, 116 + i * 12, 156, 2.6 + (1.5 if (t % 3) == i else 0), 2.6 + (1.5 if (t % 3) == i else 0), led)
    elif state == "working":
        for i in range(5):
            rrect(d, 104 + i * 10, 154, 110 + i * 10, 154 + 4 + 5 * ((i + t) % 3), 1.5, led)   # equalizer bars
    elif state == "done":
        arc(d, 128, 150, 12, 8, 20, 160, led, 3)
    elif state == "error":
        line(d, [(112 + i, 156 + 2.5 * math.sin(i * 0.9)) for i in range(0, 33, 2)], led, 3)
    elif state == "waiting":
        ell(d, 128, 158, 3, 3, led)
    elif state == "idle":
        arc(d, 128, 150, 10, 7, 20, 160, led, 3)
    props(d, state, t)
    return body, ti, 210

# --- cat: round head, ears, whiskers; a paw comes up when it needs you
def build_cat(state, t):
    ph = 2 * math.pi * t / FR
    ear = {"idle": 3 * math.sin(ph), "waiting": 6 * math.sin(2 * ph), "error": -8, "offline": -5, "thinking": 4, "working": 2 * math.sin(2 * ph)}.get(state, 0)
    tilt = {"thinking": -6, "waiting": 4 * math.sin(ph), "error": [-3, 3, -3, 3, -2, 2][t]}.get(state, 0)
    cx, cy, R = 128, 134, 60
    m = new_mask()
    mask_ell(m, cx, 192, 42, 20)                               # chest / body bump
    mask_ell(m, cx, cy, R, R * (0.9 if state == "offline" else 1.0))
    for s in (-1, 1):                                          # ears
        mask_poly(m, [(cx + s * 52, 104), (cx + s * (58 - ear * 0.5 * s * -0.0), 52 + (ear if s > 0 else -ear) * 0.5), (cx + s * 14, 80)])
    if state == "waiting":
        mask_ell(m, 78, 172 - 14 * abs(math.sin(ph)), 11, 14)  # raised paw
    top, bot, glow = (200, 235, 255), (120, 175, 245), (130, 200, 255)
    if state == "offline": top, bot, glow = dim(top), dim(bot), dim(glow)
    # tilt the whole body mask a little around the chest
    r = math.radians(tilt)
    m = m.rotate(-tilt, resample=Image.BICUBIC, center=(sc(128), sc(190)))
    body = body_layer(m, top, bot, glow, y0=50, y1=214, highlight=(84, 84, 128, 112))

    ti, d = layer()
    for s in (-1, 1):                                          # inner ears
        poly(d, [(cx + s * 46, 100), (cx + s * (50), 64 + (ear if s > 0 else -ear) * 0.4), (cx + s * 22, 86)], (255, 170, 190, 200))
    ey = cy - 4
    look = (5, -6) if state == "thinking" else (0, 0)
    eyes(d, eye_kind(state, t), cx, ey, 27, rx=11, ry=14, look=look)
    poly(d, [(cx - 5, ey + 16), (cx + 5, ey + 16), (cx, ey + 22)], (255, 130, 160, 255))          # nose
    if state in ("done", "idle", "thinking", "working", "waiting"):
        arc(d, cx - 6, ey + 24, 6, 5, 0, 180, INK, 2.5); arc(d, cx + 6, ey + 24, 6, 5, 0, 180, INK, 2.5)
    elif state == "error":
        line(d, [(cx - 10 + i, ey + 30 + 2.5 * math.sin(i * 0.9)) for i in range(0, 21, 2)], INK, 3)
    else:
        ell(d, cx, ey + 28, 3, 2, INK)
    for s in (-1, 1):                                          # whiskers
        for k, dy in enumerate((-4, 4)):
            line(d, [(cx + s * 40, ey + 18 + dy), (cx + s * 66, ey + 14 + dy * 2.2)], (255, 255, 255, 190), 2)
    cheeks(d, cx, ey + 18, 46, alpha=90)
    props(d, state, t)
    return body, ti, 212

CHARACTERS = {
    "qute": {"name": "Qute Agent", "build": build_qute, "desc": "The classic little ghost."},
    "blob": {"name": "Blob", "build": build_blob, "desc": "A squishy slime that bounces with the mood."},
    "bot":  {"name": "Bot", "build": build_bot, "desc": "A tiny robot with a screen face and an antenna light."},
    "cat":  {"name": "Cat", "build": build_cat, "desc": "A round cat that raises a paw when it needs you."},
}

# ---------------------------------------------------------------- compose + save
def hue_shift(img, deg):
    if deg == 0: return img
    a = img.getchannel("A")
    h, s, v = img.convert("RGB").convert("HSV").split()
    off = int(round(deg / 360 * 256))
    out = Image.merge("HSV", (h.point(lambda x: (x + off) % 256), s, v)).convert("RGB")
    out.putalpha(a)
    return out

def render_pack(pid):
    spec = CHARACTERS[pid]
    sheets = {v: {} for v in VARIANTS}
    for state in STATES:
        frames = {v: [] for v in VARIANTS}
        for t in range(FR):
            body, top, ground = spec["build"](state, t)
            body_s, top_s = body.resize((W, W), Image.LANCZOS), top.resize((W, W), Image.LANCZOS)
            dx, dy = motion(state, t)
            for v, deg in VARIANTS.items():
                canvas = Image.new("RGBA", (W, W), (0, 0, 0, 0))
                if ground is not None:                                    # ground shadow (does not bob)
                    sh = Image.new("RGBA", (W, W), (0, 0, 0, 0))
                    k = max(0.7, 1 + dy / 40.0)
                    ImageDraw.Draw(sh).ellipse([128 - 58 * k, ground - 5, 128 + 58 * k, ground + 7], fill=(0, 0, 0, 55))
                    canvas = Image.alpha_composite(canvas, sh.filter(ImageFilter.GaussianBlur(2)))
                fr = Image.alpha_composite(hue_shift(body_s, deg), top_s)
                moved = Image.new("RGBA", (W, W), (0, 0, 0, 0))
                moved.paste(fr, (dx, dy))                                  # raw copy: shifts without touching alpha
                canvas = Image.alpha_composite(canvas, moved)
                frames[v].append(canvas)
        for v in VARIANTS:
            sheet = Image.new("RGBA", (W * FR, W), (0, 0, 0, 0))
            for t, f in enumerate(frames[v]):
                sheet.paste(f, (t * W, 0))
            sheets[v][state] = sheet
    out = os.path.join(ROOT, pid)
    for v in VARIANTS:
        os.makedirs(os.path.join(out, v), exist_ok=True)
        for state, sheet in sheets[v].items():
            sheet.save(os.path.join(out, v, f"{state}.png"), optimize=True)
    manifest = {
        "id": pid, "name": spec["name"], "description": spec["desc"], "author": "Qute Agent project", "license": "CC0-1.0",
        "version": 2, "frame": {"width": W, "height": W}, "variants": list(VARIANTS),
        "states": {s: {"file": "{variant}/%s.png" % s, "frames": FR, "fps": FPS[s]} for s in STATES},
        "fallback": {"working": "thinking", "waiting": "thinking", "error": "idle", "offline": "idle", "done": "idle"},
    }
    with open(os.path.join(out, "manifest.json"), "w") as f:
        json.dump(manifest, f, indent=2); f.write("\n")
    # contact sheet: rows = variants, columns = states (first frame)
    prev = Image.new("RGBA", (W * len(STATES), W * len(VARIANTS)), (24, 26, 40, 255))
    for r, v in enumerate(VARIANTS):
        for c, s in enumerate(STATES):
            prev.alpha_composite(sheets[v][s].crop((0, 0, W, W)), (c * W, r * W))
    prev.convert("RGB").resize((prev.width // 2, prev.height // 2), Image.LANCZOS).save(os.path.join(out, "preview.png"), optimize=True)
    print("ok", pid)

if __name__ == "__main__":
    for pid in (sys.argv[1:] or CHARACTERS):
        render_pack(pid)
