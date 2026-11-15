#!/usr/bin/env python3
"""Generates the built-in 'qute' sprite pack (dev tool, needs Pillow).
Run from the repo root:  python3 tools/gen_qute_pack.py
Output: assets/spritepacks/qute/<variant>/<state>.png  (6 frames, 256x256 each, one row)
The app itself does NOT need Python; this only regenerates art."""
import math, os, sys
from PIL import Image, ImageDraw, ImageFilter, ImageChops

S, W = 4, 256
N = W * S
FRAMES = 6
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "assets", "spritepacks", "qute")
VARIANTS = {"ocean": 0, "mint": -60, "grape": 75, "rose": 130, "amber": -165}
STATES = ["idle", "thinking", "done"]

def sc(v): return int(v * S)

def body_mask(tilt, squash, wave):
    cx, cy, R = 128, 112, 62
    pts = [(cx + R*math.cos(math.radians(a)), cy + R*math.sin(math.radians(a))*squash) for a in range(180, 361, 3)]
    n = 40
    for i in range(n + 1):
        x = cx + R - 2*R*i/n
        pts.append((x, 196 + 7*math.sin(i/n*6*math.pi + wave)))
    c, r = (cx, 150), math.radians(tilt)
    out = [(sc(c[0] + (x-c[0])*math.cos(r) - (y-c[1])*math.sin(r)),
            sc(c[1] + (x-c[0])*math.sin(r) + (y-c[1])*math.cos(r))) for x, y in pts]
    m = Image.new("L", (N, N), 0)
    ImageDraw.Draw(m).polygon(out, fill=255)
    return m

def gradient(top, bot):
    g = Image.new("RGB", (1, N))
    for y in range(N):
        t = max(0, min(1, (y / N - 0.2) / 0.55))
        g.putpixel((0, y), tuple(int(top[i] + (bot[i]-top[i])*t) for i in range(3)))
    return g.resize((N, N))

def frame(state, t):
    ph = 2*math.pi*t/FRAMES
    tilt = {"idle": 0, "thinking": -7 + 2*math.sin(ph), "done": 0}[state]
    squash = {"idle": 1.0, "thinking": 1.0, "done": 1.0 + 0.04*math.sin(ph)}[state]
    glow_col = {"idle": (120, 210, 255), "thinking": (190, 150, 255), "done": (255, 220, 130)}[state]
    top, bot = {"idle": ((190, 245, 255), (110, 170, 255)),
                "thinking": ((225, 205, 255), (140, 120, 245)),
                "done": ((255, 245, 200), (255, 170, 120))}[state]
    m = body_mask(tilt, squash, ph)
    img = Image.new("RGBA", (N, N), (0, 0, 0, 0))
    glow = Image.new("RGBA", (N, N), glow_col + (0,))
    glow.putalpha(m.filter(ImageFilter.GaussianBlur(sc(22))).point(lambda v: int(v*0.75)))
    img = Image.alpha_composite(img, glow)
    body = gradient(top, bot).convert("RGBA"); body.putalpha(m)
    img = Image.alpha_composite(img, body)
    hl = Image.new("RGBA", (N, N), (255, 255, 255, 0))
    ImageDraw.Draw(hl).ellipse([sc(86), sc(62), sc(126), sc(92)], fill=(255, 255, 255, 120))
    hl = hl.filter(ImageFilter.GaussianBlur(sc(5)))
    hl.putalpha(ImageChops.multiply(hl.getchannel("A"), m))
    img = Image.alpha_composite(img, hl)
    d = ImageDraw.Draw(img)
    ink = (38, 42, 78, 255)
    lx = 128 - tilt*0.6
    d.polygon([(sc(lx), sc(44)), (sc(lx-9), sc(24)), (sc(lx+2), sc(8)), (sc(lx+13), sc(26))], fill=(255, 255, 255, 235))
    d.polygon([(sc(lx), sc(44)), (sc(lx+13), sc(26)), (sc(lx+2), sc(8)), (sc(lx+6), sc(28))], fill=(255, 255, 255, 140))
    ex, ey = 128, 118
    blink = (state == "idle" and t == 4)
    if state in ("idle", "thinking") and not blink:
        look = (0, 0) if state == "idle" else (5, -6)
        for sx in (-26, 26):
            cx = ex + sx
            d.ellipse([sc(cx-11), sc(ey-15), sc(cx+11), sc(ey+15)], fill=ink)
            d.ellipse([sc(cx-6+look[0]), sc(ey-10+look[1]), sc(cx+2+look[0]), sc(ey-2+look[1])], fill=(255, 255, 255, 255))
            d.ellipse([sc(cx+2+look[0]), sc(ey+3+look[1]), sc(cx+7+look[0]), sc(ey+8+look[1])], fill=(255, 255, 255, 200))
    elif blink:
        for sx in (-26, 26):
            cx = ex + sx
            d.line([(sc(cx-10), sc(ey)), (sc(cx+10), sc(ey))], fill=ink, width=sc(4))
    else:
        for sx in (-26, 26):
            cx = ex + sx
            d.arc([sc(cx-12), sc(ey-12), sc(cx+12), sc(ey+12)], 200, 340, fill=ink, width=sc(5))
    for sx in (-44, 44):
        ch = Image.new("RGBA", (N, N), (0, 0, 0, 0))
        ImageDraw.Draw(ch).ellipse([sc(ex+sx-9), sc(ey+17), sc(ex+sx+9), sc(ey+27)], fill=(255, 130, 160, 150))
        img = Image.alpha_composite(img, ch.filter(ImageFilter.GaussianBlur(sc(2))))
    d = ImageDraw.Draw(img)
    if state == "idle":
        d.arc([sc(118), sc(128), sc(138), sc(146)], 20, 160, fill=ink, width=sc(3))
    elif state == "thinking":
        d.ellipse([sc(122), sc(138), sc(132), sc(146)], fill=ink)
    else:
        d.pieslice([sc(114), sc(128), sc(142), sc(156)], 0, 180, fill=ink)
        d.pieslice([sc(121), sc(142), sc(135), sc(156)], 0, 180, fill=(255, 120, 140, 255))
    if state == "thinking":
        dots = [(200, 70, 5), (214, 50, 8), (232, 26, 11)]
        for (x, y, r) in dots[:[1, 2, 3, 3, 2, 1][t]]:
            d.ellipse([sc(x-r), sc(y-r), sc(x+r), sc(y+r)], fill=(255, 255, 255, 230))
    if state == "done":
        def star(cx, cy, r, col):
            pts = []
            for k in range(8):
                rr = r if k % 2 == 0 else r*0.35
                a = math.pi/4*k - math.pi/2
                pts.append((sc(cx + rr*math.cos(a)), sc(cy + rr*math.sin(a))))
            d.polygon(pts, fill=col)
        for i, (x, y, r, col) in enumerate([(40, 60, 14, (255, 240, 150, 255)), (216, 78, 11, (255, 255, 255, 255)),
                                            (206, 30, 8, (255, 220, 120, 255)), (52, 180, 8, (255, 255, 255, 230))]):
            star(x, y, r*(1 + 0.3*math.sin(ph + i*1.3)), col)
    out = img.resize((W, W), Image.LANCZOS)
    bob = round(4*math.sin(ph))
    canvas = Image.new("RGBA", (W, W), (0, 0, 0, 0))
    canvas.paste(out, (0, bob))
    return canvas

def hue_shift(img, deg):
    if deg == 0: return img
    a = img.getchannel("A")
    h, s, v = img.convert("RGB").convert("HSV").split()
    off = int(round(deg/360*256))
    out = Image.merge("HSV", (h.point(lambda x: (x+off) % 256), s, v)).convert("RGB")
    out.putalpha(a)
    return out

def main():
    base = {}
    for st in STATES:
        sheet = Image.new("RGBA", (W*FRAMES, W), (0, 0, 0, 0))
        for t in range(FRAMES):
            sheet.paste(frame(st, t), (t*W, 0))
        base[st] = sheet
    for name, deg in VARIANTS.items():
        d = os.path.join(OUT, name); os.makedirs(d, exist_ok=True)
        for st, sheet in base.items():
            hue_shift(sheet, deg).save(os.path.join(d, f"{st}.png"))
    prev = Image.new("RGBA", (W*len(STATES), W*len(VARIANTS)), (24, 26, 40, 255))
    for r, (name, deg) in enumerate(VARIANTS.items()):
        for c, st in enumerate(STATES):
            prev.alpha_composite(hue_shift(base[st].crop((0, 0, W, W)), deg), (c*W, r*W))
    prev.save(os.path.join(OUT, "preview.png"))
    print("done:", os.path.abspath(OUT))

if __name__ == "__main__":
    sys.exit(main())
