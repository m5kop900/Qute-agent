#pragma once
// JavaScript port of src/core/chatboxplacement.cpp for the compositor scripts that must decide INSIDE the
// compositor (KWin script, GNOME Shell extension): a Wayland client cannot read its own global position or
// the work area, so the decision cannot be made in C++ there.
//
// Keep it in step with the C++ version. tests/placement/ runs both on the same random inputs and fails when
// they differ; packaging/gnome-shell/.../extension.js carries a copy between the BEGIN/END markers, which
// the same test compares with this one.
//
// BEGIN CHATBOX PLACEMENT JS
static const char kChatboxPlacementJs[] = R"JS(
function quteClampInt(v, lo, hi) { return Math.max(lo, Math.min(v, Math.max(lo, hi))); }
function quteInside(r, a) {
    return r.x >= a.x && r.y >= a.y && r.x + r.w <= a.x + a.w && r.y + r.h <= a.y + a.h;
}
function quteOverlap(a, b) {
    var w = Math.min(a.x + a.w, b.x + b.w) - Math.max(a.x, b.x);
    var h = Math.min(a.y + a.h, b.y + b.h) - Math.max(a.y, b.y);
    return (w > 0 && h > 0) ? w * h : 0;
}
function quteMakeCard(inp, dir, al) {
    var s = inp.sprite, a = inp.area;
    var c = { x: 0, y: 0, w: inp.cardW, h: inp.cardH };
    if (dir === "right" || dir === "left") {
        c.x = dir === "right" ? s.x + s.w + inp.gap : s.x - inp.gap - c.w;
        if (al === "end") c.y = s.y + s.h - c.h;
        else if (al === "start") c.y = s.y;
        else c.y = quteClampInt(s.y + Math.floor(s.h / 2) - Math.floor(c.h / 2), a.y, a.y + a.h - c.h);
    } else {
        c.y = dir === "below" ? s.y + s.h + inp.gap : s.y - inp.gap - c.h;
        if (al === "end") c.x = s.x + s.w - c.w;
        else if (al === "start") c.x = s.x;
        else c.x = quteClampInt(s.x + Math.floor(s.w / 2) - Math.floor(c.w / 2), a.x, a.x + a.w - c.w);
    }
    return c;
}
function quteValid(card, inp) {
    return quteInside(card, inp.area) && quteOverlap(card, inp.sprite) === 0;
}
function quteScore(card, inp) {
    var a = inp.area, s = inp.sprite;
    var room = Math.min(card.x - a.x, card.y - a.y, a.x + a.w - (card.x + card.w), a.y + a.h - (card.y + card.h));
    var clearance = Math.min(Math.max(room, 0), 200);
    var dx = (card.x + card.w / 2) - (s.x + s.w / 2);
    var dy = (card.y + card.h / 2) - (s.y + s.h / 2);
    return clearance - 0.25 * Math.sqrt(dx * dx + dy * dy);
}
function quteFinish(inp, dir, al, card, isValid) {
    var s = inp.sprite;
    var left = Math.min(s.x, card.x), top = Math.min(s.y, card.y);
    var right = Math.max(s.x + s.w, card.x + card.w);
    var bottom = Math.max(s.y + s.h, card.y + card.h + inp.dropRoom);
    return {
        valid: isValid, dir: dir, align: al, card: card,
        window: { x: left, y: top, w: right - left, h: bottom - top },
        spriteX: s.x - left, spriteY: s.y - top, cardX: card.x - left, cardY: card.y - top,
        anchorBottom: dir === "above" || ((dir === "right" || dir === "left") && al !== "start")
    };
}
function quteChoosePlacement(inp) {
    if (inp.cardW <= 0 || inp.cardH <= 0) {
        var sp = inp.sprite;
        return { valid: true, dir: "right", align: "end", card: { x: sp.x, y: sp.y, w: 0, h: 0 },
                 window: { x: sp.x, y: sp.y, w: sp.w, h: sp.h },
                 spriteX: 0, spriteY: 0, cardX: 0, cardY: 0, anchorBottom: true };
    }
    var pc = quteMakeCard(inp, "right", "end");
    if (quteValid(pc, inp))
        return quteFinish(inp, "right", "end", pc, true);
    var dirs = ["above", "left", "below", "right"], aligns = ["end", "start", "clamp"];
    var best = null, bestScore = 0;
    for (var i = 0; i < dirs.length; i++) {
        for (var j = 0; j < aligns.length; j++) {
            if (dirs[i] === "right" && aligns[j] === "end")
                continue;
            var c = quteMakeCard(inp, dirs[i], aligns[j]);
            if (!quteValid(c, inp))
                continue;
            var sc = quteScore(c, inp);
            if (!best || sc > bestScore + 1e-9) {
                best = { dir: dirs[i], al: aligns[j], card: c };
                bestScore = sc;
            }
        }
    }
    if (best)
        return quteFinish(inp, best.dir, best.al, best.card, true);
    var least = null, leastOverlap = 0;
    for (var k = 0; k < dirs.length; k++) {
        var f = quteMakeCard(inp, dirs[k], "clamp");
        f.x = quteClampInt(f.x, inp.area.x, inp.area.x + inp.area.w - f.w);
        f.y = quteClampInt(f.y, inp.area.y, inp.area.y + inp.area.h - f.h);
        var ov = quteOverlap(f, inp.sprite);
        if (!least || ov < leastOverlap) {
            least = { dir: dirs[k], card: f };
            leastOverlap = ov;
        }
    }
    return quteFinish(inp, least.dir, "clamp", least.card, false);
}
// Same wire string as ChatboxPlacement::toWire().
function quteWire(r) {
    return [r.dir, r.align, r.window.x, r.window.y, r.window.w, r.window.h, r.spriteX, r.spriteY,
            r.cardX, r.cardY, r.card.w, r.card.h, r.anchorBottom ? 1 : 0, r.valid ? 1 : 0].join(",");
}
)JS";
// END CHATBOX PLACEMENT JS
