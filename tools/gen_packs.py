#!/usr/bin/env python3
"""Generates ALL built-in sprite packs: qute (ghost), blob, bot, cat + 7 anime girls (sakura aoi midori yuki luna akane hikari).
The anime girls are single-color "flat" packs (no color variants).
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

# --- anime girls: chibi characters, ONE fixed color scheme each.
# They are "flat" packs: no color variants, the sheets sit directly in the pack folder (manifest "variants": []).
# Style: sticker look (dark outline + thin light halo) so they read on light AND dark backgrounds.
SKIN, SKIN_SH = (255, 232, 218), (240, 196, 182)
OUT_INK = (58, 40, 74)
GOLD = (255, 214, 96)

def dk(c, f=0.6): return (int(c[0] * f), int(c[1] * f), int(c[2] * f), 255)
def lt(c, f=0.4): return (int(c[0] + (255 - c[0]) * f), int(c[1] + (255 - c[1]) * f), int(c[2] + (255 - c[2]) * f), 255)

def shape(d, pts, fill, edge=None, w=1.8):
    poly(d, pts, fill)
    if edge: line(d, list(pts) + [pts[0]], edge, w)

def blob(d, cx, cy, rx, ry, fill, edge=None, w=1.8):
    ell(d, cx, cy, rx, ry, fill)
    if edge: d.ellipse([sc(cx - rx), sc(cy - ry), sc(cx + rx), sc(cy + ry)], outline=edge, width=max(1, sc(w)))

def rr(d, x0, y0, x1, y1, r, fill, edge=None, w=1.6):
    kw = {"outline": edge, "width": max(1, sc(w))} if edge else {}
    d.rounded_rectangle([sc(x0), sc(y0), sc(x1), sc(y1)], radius=sc(r), fill=fill, **kw)

def bez(p0, p1, p2, n=18):
    pts = []
    for i in range(n + 1):
        s = i / n
        pts.append(((1 - s) ** 2 * p0[0] + 2 * (1 - s) * s * p1[0] + s * s * p2[0],
                    (1 - s) ** 2 * p0[1] + 2 * (1 - s) * s * p1[1] + s * s * p2[1]))
    return pts

def wavy(p0, p1, amp, k, n=28):
    return [(p0[0] + (p1[0] - p0[0]) * (i / n) + amp * math.sin(2 * math.pi * k * i / n) * (i / n),
             p0[1] + (p1[1] - p0[1]) * (i / n)) for i in range(n + 1)]

def strand(d, path, w0, w1, fill, edge=None):
    """Tapered ribbon along a path (hair tails, sleeves)."""
    L, R, n = [], [], len(path)
    for i, (x, y) in enumerate(path):
        xa, ya = path[max(i - 1, 0)]; xb, yb = path[min(i + 1, n - 1)]
        tx, ty = xb - xa, yb - ya
        ln = math.hypot(tx, ty) or 1.0
        nx, ny = -ty / ln, tx / ln
        hw = (w0 + (w1 - w0) * i / (n - 1)) / 2
        L.append((x + nx * hw, y + ny * hw)); R.append((x - nx * hw, y - ny * hw))
    shape(d, L + R[::-1], fill, edge)
    blob(d, path[-1][0], path[-1][1], w1 / 2, w1 / 2, fill, edge)

def bow(d, cx, cy, s, fill, edge, knot):
    for k in (-1, 1):
        shape(d, [(cx, cy), (cx + k * 18 * s, cy - 11 * s), (cx + k * 20 * s, cy + 11 * s)], fill, edge, 1.5)
    blob(d, cx, cy, 5 * s, 5 * s, knot, edge, 1.3)

def grow(a, r):
    return a.filter(ImageFilter.GaussianBlur(max(1.0, r / 2.0))).point(lambda v: 255 if v > 6 else 0)

def stickerize(img):
    a = img.getchannel("A")
    out = Image.new("RGBA", img.size, (0, 0, 0, 0))
    for ring, col, al in ((grow(a, sc(4.2)), (255, 255, 255), 190), (grow(a, sc(2.4)), OUT_INK, 255)):
        lay = Image.new("RGBA", img.size, col + (0,))
        lay.putalpha(ring.point(lambda v, al=al: al if v > 100 else 0))
        out = Image.alpha_composite(out, lay)
    return Image.alpha_composite(out, img)

# ---- face
def girl_eye(d, kind, x, y, side, iris, look=(0, 0)):
    lx, ly = look
    ird, irl = dk(iris, 0.55), lt(iris, 0.4)
    if kind in ("open", "wide"):
        big = kind == "wide"
        rx, ry = (12.5, 16.5) if big else (11.5, 15)
        ell(d, x, y, rx, ry, ird)
        ell(d, x, y + 2.5, rx - 2, ry - 4, iris)
        ell(d, x, y + 6.5, rx - 4.5, ry - 9.5, irl)
        ell(d, x + lx * 0.6, y + ly * 0.6 + 1, 3.6 if big else 4.4, 5 if big else 6.6, INK)
        ell(d, x - 4 + lx * 0.4, y - 6 + ly * 0.4, 4, 4, WHITE)
        ell(d, x + 4.5, y + 7, 2, 2, WHITE)
        arc(d, x, y + 1, rx + 0.5, ry + 0.5, 200, 340, INK, 3.6)
        line(d, [(x + side * (rx - 1), y - ry * 0.5), (x + side * (rx + 5), y - ry * 0.8)], INK, 2.6)
    elif kind == "blink" or kind == "sleep":
        arc(d, x, y + 3, 10, 6, 20, 160, INK, 3.2)
        line(d, [(x + side * 9, y + 7), (x + side * 14, y + 10)], INK, 2.4)
    elif kind == "happy":
        arc(d, x, y + 4, 11, 10, 200, 340, INK, 3.8)
    elif kind == "x":
        line(d, [(x - 8, y - 8), (x + 8, y + 8)], INK, 3.6)
        line(d, [(x - 8, y + 8), (x + 8, y - 8)], INK, 3.6)
    elif kind == "focus":                                       # half-lidded, determined
        d.pieslice([sc(x - 11.5), sc(y - 9), sc(x + 11.5), sc(y + 13)], 0, 180, fill=ird)
        d.pieslice([sc(x - 9.5), sc(y - 6), sc(x + 9.5), sc(y + 12.5)], 0, 180, fill=iris)
        ell(d, x, y + 4, 3.8, 4.8, INK)
        ell(d, x - 3.5, y + 2, 2.6, 2.6, WHITE)
        line(d, [(x - 12.5, y + 2), (x + 12.5, y + 2)], INK, 3.6)
        line(d, [(x + side * 11, y + 1), (x + side * 16, y - 3)], INK, 2.4)

def girl_mouth(d, kind, x, y):
    if kind == "smile":
        arc(d, x, y - 2, 6, 5, 20, 160, INK, 2.4)
    elif kind == "o":
        ell(d, x, y + 1, 3.4, 4, INK)
    elif kind == "open":
        d.pieslice([sc(x - 8), sc(y - 4), sc(x + 8), sc(y + 11)], 0, 180, fill=INK)
        d.pieslice([sc(x - 4.5), sc(y + 3), sc(x + 4.5), sc(y + 11)], 0, 180, fill=(255, 120, 140, 255))
    elif kind == "flat":
        line(d, [(x - 5, y), (x + 5, y)], INK, 2.4)
    elif kind == "wavy":
        line(d, [(x - 8 + i, y + 2 * math.sin(i * 0.9)) for i in range(0, 17, 2)], INK, 2.4)
    else:
        ell(d, x, y, 2.4, 1.8, INK)

# ---- hair
FRINGES = {
    "default": [(180, 128), (170, 108), (160, 99), (150, 101), (140, 99), (134, 101), (128, 112), (122, 101), (116, 99),
                (106, 101), (96, 99), (86, 108), (76, 128)],
    "hime":    [(180, 142), (175, 100), (160, 100), (96, 100), (81, 100), (76, 142)],
    "swept":   [(180, 130), (172, 106), (160, 96), (148, 104), (136, 94), (120, 100), (104, 98), (92, 106), (84, 102), (76, 130)],
}

def back_hair(d, style, hair, hd, sw):
    blob(d, 128, 106, 56, 52, hair, hd, 1.8)
    if style == "twintail":
        for s in (-1, 1):
            strand(d, bez((128 + s * 44, 90), (128 + s * 92, 140), (128 + s * 78 + sw * 0.8, 206)), 28, 8, hair, hd)
    elif style == "bob":
        rr(d, 70, 62, 186, 150, 34, hair, hd, 1.8)
    elif style == "ponytail":
        strand(d, bez((100, 70), (26, 50), (46 + sw, 166)), 24, 8, hair, hd)
    elif style == "long":
        rr(d, 90, 100, 166, 206, 18, hair, hd, 1.8)
        for s in (-1, 1):
            strand(d, bez((128 + s * 42, 96), (128 + s * 68, 150), (128 + s * 52 + sw * 0.5, 208)), 30, 20, hair, hd)
    elif style == "waves":
        for s in (-1, 1):
            strand(d, bez((128 + s * 42, 98), (128 + s * 72, 138), (128 + s * 58 + sw * 0.5, 178)), 28, 12, hair, hd)
    elif style == "buns":
        for s in (-1, 1):
            strand(d, bez((128 + s * 44, 86), (128 + s * 68, 112), (128 + s * 58 + sw * 0.5, 144)), 14, 6, hair, hd)
            blob(d, 128 + s * 40, 68, 18, 18, hair, hd, 1.8)
            ell(d, 128 + s * 40 - 5, 62, 7, 4.5, lt(hair, 0.55))
    elif style == "drills":
        for s in (-1, 1):
            strand(d, wavy((128 + s * 44, 94), (128 + s * 78 + sw * 0.5, 204), 11, 2.4), 25, 9, hair, hd)

def front_hair(d, g, hair, hd, hl):
    top = [(128 + 54 * math.cos(math.radians(a)), 104 + 52 * math.sin(math.radians(a))) for a in range(180, 361, 6)]
    shape(d, top + FRINGES[g.get("fringe", "default")], hair, hd, 1.8)
    arc(d, 128, 98, 38, 32, 205, 295, hl, 4.5)

# ---- outfit (legs, skirt, torso) and arms
SKIRTS = {"hoodie": [(106, 184), (150, 184), (154, 206), (102, 206)], "qipao": [(104, 184), (152, 184), (162, 208), (94, 208)],
          "robe": [(102, 184), (154, 184), (172, 210), (84, 210)]}

def draw_outfit(d, g, C):
    top, trim, skirt, sock, shoe = C(g["top"]), C(g["trim"]), C(g["skirt"]), C(g["sock"]), C(g["shoe"])
    kind = g["outfit"]
    for x in (118, 138):
        rr(d, x - 6, 202, x + 6, 223, 4, sock, dk(sock, 0.7), 1.4)
        if g.get("sock2"):
            for y in (208, 216):
                d.rectangle([sc(x - 5), sc(y), sc(x + 5), sc(y + 3)], fill=C(g["sock2"]))
    for x in (115, 141):
        blob(d, x, 224, 10, 5.5, shoe, dk(shoe, 0.7), 1.4)
    if kind == "hoodie":
        blob(d, 128, 152, 27, 11, dk(top, 0.8), dk(top, 0.5), 1.4)
    if kind in ("magical", "ojou", "dress"):                    # lace frill under the skirt
        fc = lt(skirt, 0.75)
        for x in range(88, 170, 11):
            blob(d, x, 210, 7, 6, fc, dk(fc, 0.75), 1.2)
    shape(d, SKIRTS.get(kind, [(102, 184), (154, 184), (170, 208), (86, 208)]), skirt, dk(skirt, 0.65), 1.6)
    shape(d, [(106, 150), (150, 150), (155, 190), (101, 190)], top, dk(top, 0.65), 1.6)
    red = (236, 70, 90)
    if kind == "sailor":
        shape(d, [(104, 150), (152, 150), (128, 182)], trim, dk(trim, 0.7), 1.4)
        shape(d, [(115, 150), (141, 150), (128, 171)], top)
        bow(d, 128, 175, 0.75, C(red), dk(red, 0.7), C(red))
        for x in (108, 118, 128, 138, 148):
            line(d, [(x, 190), (128 + (x - 128) * 1.25, 207)], dk(skirt, 0.8), 1.6)
    elif kind == "magical":
        shape(d, [(108, 150), (148, 150), (140, 163), (116, 163)], C((255, 255, 255)), dk(top, 0.65), 1.3)
        bow(d, 128, 168, 1.05, C((255, 104, 156)), dk((255, 104, 156), 0.65), C(GOLD))
        rr(d, 102, 182, 154, 189, 3, C((255, 255, 255)), dk(top, 0.65), 1.2)
    elif kind == "hoodie":
        line(d, [(128, 156), (128, 190)], trim, 2.2)
        line(d, [(120, 156), (118, 176)], trim, 2); line(d, [(136, 156), (138, 176)], trim, 2)
        line(d, [(102, 204), (154, 204)], trim, 2)
    elif kind == "dress":
        bow(d, 128, 162, 0.6, trim, dk(trim, 0.65), lt(trim, 0.5))
        rr(d, 102, 180, 154, 188, 3, trim, dk(trim, 0.65), 1.2)
    elif kind == "robe":
        shape(d, [(112, 150), (144, 150), (136, 161), (120, 161)], C((255, 255, 255)), dk(top, 0.6), 1.2)
        star(d, 128, 174, 8, trim)
        rr(d, 102, 183, 154, 189, 3, trim, dk(trim, 0.6), 1.2)
        for (x, y) in ((100, 198), (156, 198), (128, 202)): star(d, x, y, 4, trim)
    elif kind == "qipao":
        rr(d, 116, 146, 140, 157, 4, trim, dk(trim, 0.6), 1.2)
        for (x, y) in ((136, 162), (134, 171), (131, 180)): blob(d, x, y, 2.6, 2.6, trim, dk(trim, 0.6), 1)
        line(d, [(104, 188), (101, 190)], trim, 2); line(d, [(94, 207), (162, 207)], trim, 3)
    elif kind == "ojou":
        bow(d, 128, 162, 1.1, C((255, 150, 190)), dk((255, 150, 190), 0.65), C((255, 226, 120)))
        rr(d, 102, 181, 154, 189, 3, trim, dk(trim, 0.65), 1.2)

def draw_arms(d, g, C, state, t, ph):
    top, skin = C(g["top"]), C(SKIN)
    hL, hR, bend = {
        "idle":     ((96, 188 + 2 * math.sin(ph)), (160, 188 - 2 * math.sin(ph)), 8),
        "thinking": ((100, 186), (140, 153), 8),
        "working":  ((116, 184 + 4 * math.sin(2 * ph)), (140, 184 - 4 * math.sin(2 * ph)), 8),
        "waiting":  ((98, 188), (186, 126 + 8 * math.sin(2 * ph)), 14),
        "done":     ((80, 118 - 4 * math.sin(ph)), (176, 118 - 4 * math.sin(ph)), 20),
        "error":    ((84, 104), (172, 104), 22),
        "offline":  ((112, 188), (144, 188), 4),
    }[state]
    for s, sh, hand in ((-1, (106, 160), hL), (1, (150, 160), hR)):
        mid = ((sh[0] + hand[0]) / 2 + s * bend, (sh[1] + hand[1]) / 2 + 6)
        strand(d, bez(sh, mid, hand), 12, 10, top, dk(top, 0.65))
        blob(d, hand[0], hand[1], 6.5, 6.5, skin, dk(skin, 0.8), 1.4)

def draw_acc(d, g, C):
    a, trim = g["acc"], C(g["trim"])
    if a == "bows":
        for s in (-1, 1): bow(d, 128 + s * 46, 88, 0.9, C((255, 92, 150)), dk((255, 92, 150), 0.65), C((255, 222, 232)))
        star(d, 152, 86, 9, C(GOLD))
    elif a == "clip":
        for y in (90, 99): rr(d, 148, y, 172, y + 5, 2, C((250, 250, 255)), dk((120, 140, 190), 0.8), 1)
    elif a == "headphones":
        dark = C((40, 46, 66))
        arc(d, 128, 102, 57, 54, 196, 344, dark, 7)
        for x0 in (68, 172):
            rr(d, x0, 98, x0 + 16, 130, 6, dark, dk(dark, 0.5), 1.2)
            rr(d, x0 + 4, 104, x0 + 12, 124, 3, trim)
        blob(d, 98, 72, 6, 6, trim, dk(trim, 0.6), 1.2)                     # ponytail tie
    elif a == "snow":
        for ang in (0, 60, 120):
            r = 9
            line(d, [(156 - r * math.cos(math.radians(ang)), 88 - r * math.sin(math.radians(ang))),
                     (156 + r * math.cos(math.radians(ang)), 88 + r * math.sin(math.radians(ang)))], C((130, 185, 250)), 2.6)
        blob(d, 156, 88, 2.8, 2.8, C((255, 255, 255)), C((130, 185, 250)), 1)
    elif a == "witch":
        hat = dk(C(g["top"]), 0.9)
        blob(d, 128, 78, 68, 14, hat, dk(hat, 0.55), 1.6)
        left, right = bez((96, 78), (112, 38), (174, 22)), bez((160, 78), (158, 46), (174, 22))
        shape(d, left + right[::-1], hat, dk(hat, 0.55), 1.6)
        rr(d, 98, 64, 158, 75, 3, trim, dk(trim, 0.6), 1.2)
        star(d, 128, 52, 8, trim)
    elif a == "rings":
        for s in (-1, 1):
            blob(d, 128 + s * 40, 86, 10, 4.5, C(GOLD), dk(GOLD, 0.6), 1.2)
            blob(d, 128 + s * 40, 70, 3.5, 3.5, C((255, 240, 160)), dk(GOLD, 0.6), 1)
    elif a == "tiara":
        shape(d, [(106, 66), (111, 46), (120, 58), (128, 38), (136, 58), (145, 46), (150, 66)], C(GOLD), dk(GOLD, 0.6), 1.5)
        blob(d, 128, 57, 3.6, 3.6, C((255, 120, 170)), dk((255, 120, 170), 0.6), 1)

GIRLS = {
    "sakura": dict(name="Sakura", desc="Pink twin-tail magical girl.", hair=(255, 152, 196), iris=(255, 92, 150),
                   style="twintail", outfit="magical", top=(255, 178, 208), trim=(255, 255, 255), skirt=(255, 140, 184),
                   sock=(255, 255, 255), shoe=(255, 120, 170), acc="bows"),
    "aoi":    dict(name="Aoi", desc="Calm blue bob-cut girl in a sailor uniform.", hair=(92, 158, 255), iris=(54, 118, 235),
                   style="bob", fringe="swept", outfit="sailor", top=(244, 247, 255), trim=(52, 76, 140), skirt=(60, 92, 170),
                   sock=(52, 76, 140), shoe=(110, 72, 60), acc="clip"),
    "midori": dict(name="Midori", desc="Green ponytail gamer girl with headphones.", hair=(92, 205, 130), iris=(36, 160, 120),
                   style="ponytail", outfit="hoodie", top=(74, 88, 118), trim=(120, 255, 170), skirt=(52, 60, 84),
                   sock=(52, 60, 84), shoe=(240, 240, 250), acc="headphones"),
    "yuki":   dict(name="Yuki", desc="Silver-haired, cool and quiet.", hair=(236, 242, 255), iris=(110, 195, 255),
                   style="long", fringe="hime", outfit="dress", top=(214, 228, 255), trim=(120, 170, 240), skirt=(196, 214, 250),
                   sock=(255, 255, 255), shoe=(120, 170, 240), acc="snow"),
    "luna":   dict(name="Luna", desc="Violet-haired little witch.", hair=(160, 118, 240), iris=(255, 200, 80),
                   style="waves", outfit="robe", top=(92, 66, 150), trim=GOLD, skirt=(78, 56, 134),
                   sock=(255, 255, 255), sock2=(140, 110, 210), shoe=(48, 36, 80), acc="witch"),
    "akane":  dict(name="Akane", desc="Fiery red-orange twin-bun girl.", hair=(255, 118, 58), iris=(255, 190, 50),
                   style="buns", outfit="qipao", top=(224, 58, 64), trim=GOLD, skirt=(200, 44, 56),
                   sock=(60, 44, 60), shoe=(224, 58, 64), acc="rings"),
    "hikari": dict(name="Hikari", desc="Blonde twin-drill rich girl with a tiara.", hair=(255, 224, 110), iris=(110, 150, 255),
                   style="drills", outfit="ojou", top=(255, 238, 245), trim=(255, 150, 190), skirt=(255, 214, 232),
                   sock=(255, 255, 255), shoe=(255, 150, 190), acc="tiara"),
}

def build_girl(g):
    def build(state, t):
        ph = 2 * math.pi * t / FR
        off = state == "offline"
        C = (lambda c: tuple(dim(c)) + (255,)) if off else (lambda c: (c[0], c[1], c[2], 255))
        hair = C(g["hair"]); hd = dk(hair, 0.66); hl = lt(hair, 0.5)
        skin = C(SKIN)
        sway = {"idle": 3 * math.sin(ph), "thinking": 2 * math.sin(ph), "working": 2.5 * math.sin(2 * ph), "waiting": 6 * math.sin(ph),
                "done": 7 * math.sin(ph), "error": [-4, 4, -3, 3, -2, 2][t], "offline": 1.5 * math.sin(ph)}[state]
        body, d = layer()
        back_hair(d, g["style"], hair, hd, sway)
        draw_outfit(d, g, C)
        blob(d, 128, 104, 49, 45, skin, dk(skin, 0.8), 1.6)             # head
        front_hair(d, g, hair, hd, hl)
        draw_acc(d, g, C)
        draw_arms(d, g, C, state, t, ph)
        body = stickerize(body)

        ti, dt = layer()
        kind = eye_kind(state, t)
        look = (3, -4) if state == "thinking" else (0, 0)
        for s in (-1, 1):
            girl_eye(dt, kind, 128 + s * 22, 119, s, g["iris"], look)
            ell(dt, 128 + s * 33, 135, 7, 4, (255, 130, 150, 70 if off else 125))          # blush
        girl_mouth(dt, mouth_kind(state), 128, 140 if state != "thinking" else 142)
        props(dt, state, t)
        return body, ti, 226
    return build

CHARACTERS = {
    "qute": {"name": "Qute Agent", "build": build_qute, "desc": "The classic little ghost."},
    "blob": {"name": "Blob", "build": build_blob, "desc": "A squishy slime that bounces with the mood."},
    "bot":  {"name": "Bot", "build": build_bot, "desc": "A tiny robot with a screen face and an antenna light."},
    "cat":  {"name": "Cat", "build": build_cat, "desc": "A round cat that raises a paw when it needs you."},
}

for _id, _g in GIRLS.items():          # anime girls: flat single-color packs (variants=None)
    CHARACTERS[_id] = {"name": _g["name"], "build": build_girl(_g), "desc": _g["desc"], "variants": None}

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
    flat = "variants" in spec and spec["variants"] is None      # single-color pack: files directly in the pack folder
    vmap = {"": 0} if flat else VARIANTS
    sheets = {v: {} for v in vmap}
    for state in STATES:
        frames = {v: [] for v in vmap}
        for t in range(FR):
            body, top, ground = spec["build"](state, t)
            body_s, top_s = body.resize((W, W), Image.LANCZOS), top.resize((W, W), Image.LANCZOS)
            dx, dy = motion(state, t)
            for v, deg in vmap.items():
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
        for v in vmap:
            sheet = Image.new("RGBA", (W * FR, W), (0, 0, 0, 0))
            for t, f in enumerate(frames[v]):
                sheet.paste(f, (t * W, 0))
            sheets[v][state] = sheet
    out = os.path.join(ROOT, pid)
    for v in vmap:
        vdir = os.path.join(out, v) if v else out
        os.makedirs(vdir, exist_ok=True)
        for state, sheet in sheets[v].items():
            sheet.save(os.path.join(vdir, f"{state}.png"), optimize=True)
    tmpl = "%s.png" if flat else "{variant}/%s.png"
    manifest = {
        "id": pid, "name": spec["name"], "description": spec["desc"], "author": "Qute Agent project", "license": "CC0-1.0",
        "version": 2, "frame": {"width": W, "height": W}, "variants": [] if flat else list(VARIANTS),
        "states": {s: {"file": tmpl % s, "frames": FR, "fps": FPS[s]} for s in STATES},
        "fallback": {"working": "thinking", "waiting": "thinking", "error": "idle", "offline": "idle", "done": "idle"},
    }
    with open(os.path.join(out, "manifest.json"), "w") as f:
        json.dump(manifest, f, indent=2); f.write("\n")
    # contact sheet: rows = variants, columns = states (first frame)
    prev = Image.new("RGBA", (W * len(STATES), W * len(vmap)), (24, 26, 40, 255))
    for r, v in enumerate(vmap):
        for c, s in enumerate(STATES):
            prev.alpha_composite(sheets[v][s].crop((0, 0, W, W)), (c * W, r * W))
    prev.convert("RGB").resize((prev.width // 2, prev.height // 2), Image.LANCZOS).save(os.path.join(out, "preview.png"), optimize=True)
    print("ok", pid)

if __name__ == "__main__":
    for pid in (sys.argv[1:] or CHARACTERS):
        render_pack(pid)
