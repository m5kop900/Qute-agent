// Qute Agent placement helper for GNOME Shell 45+.
//
// Mutter does not allow a normal Wayland client to read the global cursor position or to place its
// own window. This extension runs inside the compositor: it finds the window titled "qute-overlay",
// reads global.get_pointer(), clamps to the monitor's work area and moves the window just like the
// KWin script does on Plasma. The app talks to it over D-Bus (src/platform/gnomeshell.cpp).

import Gio from 'gi://Gio';

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
  </interface>
</node>`;

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
}
