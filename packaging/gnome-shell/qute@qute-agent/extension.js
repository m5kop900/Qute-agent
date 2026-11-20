// Qute Agent placement helper for GNOME Shell 45+.
//
// Mutter does not allow a normal Wayland client to read the global cursor position or to place its
// own window. This extension runs inside the compositor: it finds the window titled "qute-overlay",
// reads global.get_pointer(), clamps to the monitor's work area and moves the window just like the
// KWin script does on Plasma. The app talks to it over D-Bus (src/platform/gnomeshell.cpp).

import Gio from 'gi://Gio';
import GLib from 'gi://GLib';

import {Extension} from 'resource:///org/gnome/shell/extensions/extension.js';

const BUS_NAME = 'io.github.qute.Agent';
const OBJECT_PATH = '/io/github/qute/Agent';
const IFACE = 'io.github.qute.Agent';
const WINDOW_TITLE = 'qute-overlay';

const IFACE_XML = `
<node>
  <interface name="${IFACE}">
    <method name="PlaceWindow">
      <arg type="i" direction="in" name="width"/>
      <arg type="i" direction="in" name="contentHeight"/>
      <arg type="i" direction="in" name="dropRoom"/>
    </method>
    <method name="ResizeWindow">
      <arg type="i" direction="in" name="contentHeight"/>
      <arg type="i" direction="in" name="dropRoom"/>
    </method>
    <method name="PlaceWindowSmart">
      <arg type="i" direction="in" name="spriteSize"/>
      <arg type="i" direction="in" name="cardW"/>
      <arg type="i" direction="in" name="cardH"/>
      <arg type="i" direction="in" name="dropRoom"/>
      <arg type="i" direction="in" name="gap"/>
      <arg type="s" direction="out" name="layout"/>
    </method>
    <method name="ResizeWindowSmart">
      <arg type="i" direction="in" name="spriteOffX"/>
      <arg type="i" direction="in" name="spriteOffY"/>
      <arg type="i" direction="in" name="spriteSize"/>
      <arg type="i" direction="in" name="cardW"/>
      <arg type="i" direction="in" name="cardH"/>
      <arg type="i" direction="in" name="dropRoom"/>
      <arg type="i" direction="in" name="gap"/>
      <arg type="s" direction="out" name="layout"/>
    </method>
  </interface>
</node>`;

// Smart chatbox positioning (same algorithm as src/core/chatboxplacement.cpp; tests/placement/parity.js
// checks this copy against src/platform/placementscript.h).
// BEGIN CHATBOX PLACEMENT JS

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
// END CHATBOX PLACEMENT JS

export default class QuteAgentExtension extends Extension {
    enable() {
        this._nameId = Gio.DBus.session.own_name(BUS_NAME, Gio.BusNameOwnerFlags.NONE, null, null);
        this._exported = Gio.DBusExportedObject.wrapJSObject(IFACE_XML, this);
        this._exported.export(Gio.DBus.session, OBJECT_PATH);
    }

    disable() {
        if (this._exported) {
            this._exported.unexport();
            this._exported = null;
        }
        if (this._nameId) {
            Gio.DBus.session.unown_name(this._nameId);
            this._nameId = 0;
        }
    }

    _findWindow() {
        for (const actor of global.get_window_actors()) {
            const win = actor.meta_window;
            if (win && win.get_title() === WINDOW_TITLE)
                return win;
        }
        return null;
    }

    _monitorFor(x, y) {
        const count = global.display.get_n_monitors();
        for (let i = 0; i < count; i++) {
            const g = global.display.get_monitor_geometry(i);
            if (x >= g.x && x < g.x + g.width && y >= g.y && y < g.y + g.height)
                return i;
        }
        return global.display.get_primary_monitor();
    }

    PlaceWindow(width, contentHeight, dropRoom) {
        try {
            const win = this._findWindow();
            if (!win)
                return;

            const [cursorX, cursorY] = global.get_pointer();
            const monitor = this._monitorFor(cursorX, cursorY);
            const area = global.display.get_work_area_for_monitor(monitor);

            let x = Math.round(cursorX - 20);
            let y = Math.round(cursorY + 20 - contentHeight);   // prompt box just below the cursor
            x = Math.max(area.x, Math.min(x, area.x + area.width - width));
            // Clamp on the card only; the transparent room below it may hang over the screen edge.
            y = Math.max(area.y, Math.min(y, area.y + area.height - contentHeight));

            win.move_resize_frame(true, x, y, width, contentHeight + dropRoom);
            try { win.make_above(); } catch (e) { /* older Mutter: still usable */ }
            try { win.stick(); } catch (e) { /* all workspaces is a bonus */ }
            win.activate(global.get_current_time());
        } catch (e) {
            console.error(`qute: PlaceWindow failed: ${e}`);
        }
    }

    ResizeWindow(contentHeight, dropRoom) {
        try {
            const win = this._findWindow();
            if (!win)
                return;
            const frame = win.get_frame_rect();
            const cardBottom = frame.y + frame.height - dropRoom;
            win.move_resize_frame(true, frame.x, cardBottom - contentHeight, frame.width,
                                  contentHeight + dropRoom);
        } catch (e) {
            console.error(`qute: ResizeWindow failed: ${e}`);
        }
    }

    // ---- Smart chatbox positioning -------------------------------------------------------------------
    // The window holds the sprite, the card and the drop room. The sprite's screen position is an input
    // here and is never moved to make the card fit: the card goes around it, in any of four directions.
    // Both methods answer with the layout (see ChatboxPlacement::toWire) so the QML can put the sprite and
    // the card at their offsets inside the window.
    _applySmart(win, sprite, cardW, cardH, dropRoom, gap, area) {
        const r = quteChoosePlacement({
            sprite, cardW, cardH, dropRoom, gap,
            area: {x: area.x, y: area.y, w: area.width, h: area.height},
        });
        win.move_resize_frame(true, r.window.x, r.window.y, r.window.w, r.window.h);
        return new GLib.Variant('(s)', [quteWire(r)]);
    }

    PlaceWindowSmart(spriteSize, cardW, cardH, dropRoom, gap) {
        try {
            const win = this._findWindow();
            if (!win)
                return new GLib.Variant('(s)', ['']);

            const [cursorX, cursorY] = global.get_pointer();
            const monitor = this._monitorFor(cursorX, cursorY);
            const area = global.display.get_work_area_for_monitor(monitor);

            // Where the classic placement puts the sprite: its left edge 20 px left of the cursor, its bottom
            // edge 20 px below it. Only the SPRITE is kept on screen here, the card never moves it.
            let x = Math.round(cursorX - 20);
            let y = Math.round(cursorY + 20 - spriteSize);
            x = Math.max(area.x, Math.min(x, area.x + area.width - spriteSize));
            y = Math.max(area.y, Math.min(y, area.y + area.height - spriteSize));
            const out = this._applySmart(win, {x, y, w: spriteSize, h: spriteSize}, cardW, cardH, dropRoom, gap, area);
            try { win.make_above(); } catch (e) { /* older Mutter: still usable */ }
            try { win.stick(); } catch (e) { /* all workspaces is a bonus */ }
            win.activate(global.get_current_time());
            return out;
        } catch (e) {
            console.error(`qute: PlaceWindowSmart failed: ${e}`);
            return new GLib.Variant('(s)', ['']);
        }
    }

    ResizeWindowSmart(spriteOffX, spriteOffY, spriteSize, cardW, cardH, dropRoom, gap) {
        try {
            const win = this._findWindow();
            if (!win)
                return new GLib.Variant('(s)', ['']);
            const frame = win.get_frame_rect();
            // The sprite is where it is on screen right now: window origin + its offset in the window.
            const sprite = {x: frame.x + spriteOffX, y: frame.y + spriteOffY, w: spriteSize, h: spriteSize};
            const monitor = this._monitorFor(sprite.x + Math.floor(spriteSize / 2), sprite.y + Math.floor(spriteSize / 2));
            const area = global.display.get_work_area_for_monitor(monitor);
            return this._applySmart(win, sprite, cardW, cardH, dropRoom, gap, area);
        } catch (e) {
            console.error(`qute: ResizeWindowSmart failed: ${e}`);
            return new GLib.Variant('(s)', ['']);
        }
    }
}
