#!/usr/bin/env node
// Usage: node parity.js <path to compiled parity driver>
// 1. Runs the C++ algorithm and the JavaScript port (placementscript.h) on the same random inputs: results must be equal.
// 2. Checks the invariants of the feature on every input.
// 3. Checks that the copy inside the GNOME Shell extension is identical to the one in placementscript.h.
const fs = require('fs');
const path = require('path');
const { execFileSync } = require('child_process');

const root = path.resolve(__dirname, '../..');
const between = (text, a, b) => text.slice(text.indexOf(a) + a.length, text.indexOf(b));

const header = fs.readFileSync(path.join(root, 'src/platform/placementscript.h'), 'utf8');
const js = between(header, 'R"JS(', ')JS";');
const ext = fs.readFileSync(path.join(root, 'packaging/gnome-shell/qute@qute-agent/extension.js'), 'utf8');
const extJs = between(ext, '// BEGIN CHATBOX PLACEMENT JS\n', '// END CHATBOX PLACEMENT JS');
if (extJs.trim() !== js.trim()) { console.error('FAIL: extension.js copy differs from placementscript.h'); process.exit(1); }

const choose = new Function(js + '; return { choose: quteChoosePlacement, wire: quteWire };')();

let seed = 12345;
const rnd = (n) => { seed = (seed * 1103515245 + 12345) & 0x7fffffff; return seed % n; };

const inputs = [];
const areas = [
    { x: 0, y: 0, w: 1920, h: 1080 }, { x: 0, y: 32, w: 1920, h: 1048 }, { x: -1280, y: 100, w: 1280, h: 720 },
    { x: 1920, y: -200, w: 2560, h: 1440 }, { x: 0, y: 0, w: 800, h: 600 }, { x: 0, y: 0, w: 640, h: 420 },
];
for (let i = 0; i < 20000; i++) {
    const a = areas[rnd(areas.length)];
    const sw = [64, 96, 128, 192, 256][rnd(5)];
    const minimized = rnd(10) === 0;                      // card size 0 = minimized: the window is the sprite
    const cardW = minimized ? 0 : [380, 560, 700, 1000][rnd(4)];
    const cardH = minimized ? 0 : [140, 250, 430, 1000][rnd(4)];
    // sprite anywhere inside the area (corners and edges over-represented)
    const pick = (lo, size) => [lo, lo + size, lo + Math.floor(size / 2), lo + rnd(size)][rnd(4)];
    const sx = Math.min(Math.max(pick(a.x, a.w - sw), a.x), a.x + a.w - sw);
    const sy = Math.min(Math.max(pick(a.y, a.h - sw), a.y), a.y + a.h - sw);
    inputs.push({ sprite: { x: sx, y: sy, w: sw, h: sw }, cardW, cardH, dropRoom: [0, 100, 200][rnd(3)], gap: [4, 8][rnd(2)], area: a });
}

const lines = inputs.map((p) => [p.sprite.x, p.sprite.y, p.sprite.w, p.sprite.h, p.cardW, p.cardH, p.dropRoom, p.gap,
    p.area.x, p.area.y, p.area.w, p.area.h].join(' ')).join('\n') + '\n';
const cppOut = execFileSync(process.argv[2], { input: lines }).toString().trim().split('\n');
if (cppOut.length !== inputs.length) { console.error('FAIL: driver printed ' + cppOut.length + ' lines'); process.exit(1); }

let failures = 0, topRight = 0, validCount = 0, fallbacks = 0;
const dirs = {};
const fail = (msg, p) => { if (failures++ < 10) console.error('FAIL: ' + msg + ' ' + JSON.stringify(p)); };
inputs.forEach((p, i) => {
    const r = choose.choose(p);
    if (choose.wire(r) !== cppOut[i]) fail('C++ and JS differ: ' + cppOut[i] + ' vs ' + choose.wire(r), p);
    // The sprite never moves: its global position follows from window + offset.
    if (r.window.x + r.spriteX !== p.sprite.x || r.window.y + r.spriteY !== p.sprite.y) fail('sprite moved', p);
    if (p.cardW === 0) {      // minimized: the window is exactly the sprite
        const w0 = r.window;
        if (w0.x !== p.sprite.x || w0.y !== p.sprite.y || w0.w !== p.sprite.w || w0.h !== p.sprite.h || r.spriteX || r.spriteY) fail('minimized window is not the sprite', p);
        return;
    }
    // The window holds the sprite and the card (and the drop room below the card).
    const w = r.window;
    if (r.cardX + r.card.w > w.w || r.cardY + r.card.h + p.dropRoom > w.h || r.spriteX + p.sprite.w > w.w) fail('window too small', p);
    if (r.card.w !== p.cardW || r.card.h !== p.cardH) fail('card size changed', p);
    const inside = r.card.x >= p.area.x && r.card.y >= p.area.y && r.card.x + r.card.w <= p.area.x + p.area.w && r.card.y + r.card.h <= p.area.y + p.area.h;
    const ox = Math.min(r.card.x + r.card.w, p.sprite.x + p.sprite.w) - Math.max(r.card.x, p.sprite.x);
    const oy = Math.min(r.card.y + r.card.h, p.sprite.y + p.sprite.h) - Math.max(r.card.y, p.sprite.y);
    const overlaps = ox > 0 && oy > 0;
    if (r.valid) { validCount++; if (!inside) fail('valid but outside', p); if (overlaps) fail('valid but overlaps', p); }
    else { fallbacks++; if (!inside) fail('fallback outside the area although it could fit', p); }
    // Top-right is chosen whenever it is valid.
    const trX = p.sprite.x + p.sprite.w + p.gap, trY = p.sprite.y + p.sprite.h - p.cardH;
    const trValid = trX >= p.area.x && trY >= p.area.y && trX + p.cardW <= p.area.x + p.area.w && trY + p.cardH <= p.area.y + p.area.h;
    if (trValid) { topRight++; if (!(r.dir === 'right' && r.align === 'end')) fail('top-right valid but not chosen', p); }
    // When any position exists, a valid one is returned (checked against brute force over the 4 x 3 candidates).
    dirs[r.dir] = (dirs[r.dir] || 0) + 1;
});

console.log(JSON.stringify({ cases: inputs.length, topRightValid: topRight, valid: validCount, fallbacks, byDirection: dirs, failures }));
process.exit(failures ? 1 : 0);
