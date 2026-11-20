import QtQuick
import QtQuick.Controls

// A small context menu drawn with the overlay's own theme (same panel, border, radius and hover as the rest of
// the UI), not the platform style. Opened at a point of its parent with openAt(); closes on Esc, outside click
// or after a pick. Up/Down/Home/End move the highlight, Enter or Space runs it.
Popup {
    id: root
    property var theme
    property real s: 1.0
    // [{text: string, enabled: bool, run: function, shortcut: string (optional, shown dimmed on the right)}]
    // or {separator: true} for a thin line between groups.
    property var actions: []
    // Row under the mouse or reached with the keyboard (-1: none).
    property int current: -1

    padding: Math.round(5 * s)
    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    // Opens at a point of `item` (px, py in its coordinates), kept inside the overlay window: a menu near the
    // right or bottom edge is shifted back in, instead of running past the card into the empty window area.
    function openAt(item, px, py) {
        const ov = Overlay.overlay
        const margin = Math.round(6 * root.s)
        const p = item.mapToItem(ov, px, py)
        root.parent = ov
        const maxX = ov.width - root.implicitWidth - margin
        root.x = Math.max(margin, Math.min(p.x, maxX))
        const below = p.y
        const above = p.y - root.implicitHeight
        root.y = below + root.implicitHeight > ov.height - margin ? Math.max(margin, above) : below
        root.current = -1
        root.open()
    }

    function isChoice(i) {
        const a = root.actions[i]
        return !!a && a.separator !== true && a.enabled !== false
    }

    function step(delta) {
        const n = root.actions.length
        if (n === 0)
            return
        let i = root.current
        for (let k = 0; k < n; ++k) {
            i = i < 0 ? (delta > 0 ? 0 : n - 1) : (i + delta + n) % n
            if (isChoice(i)) {
                root.current = i
                return
            }
        }
    }

    function runCurrent() {
        if (!isChoice(root.current))
            return
        const a = root.actions[root.current]
        root.close()
        a.run()
    }

    enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 110; easing.type: Easing.OutCubic } }
    exit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 80 } }

    background: Item {
        // Soft drop shadow: two offset, translucent copies of the panel behind it.
        Rectangle {
            anchors.fill: parent
            anchors.topMargin: Math.round(3 * root.s)
            anchors.leftMargin: -Math.round(1 * root.s)
            anchors.rightMargin: -Math.round(1 * root.s)
            anchors.bottomMargin: -Math.round(3 * root.s)
            radius: Math.round(12 * root.s * config.radiusScale)
            color: Qt.rgba(0, 0, 0, root.theme.dark ? 0.35 : 0.16)
        }
        Rectangle {
            anchors.fill: parent
            radius: Math.round(11 * root.s * config.radiusScale)
            color: root.theme.panel
            border.width: 1
            border.color: root.theme.border
        }
    }

    contentItem: Column {
        id: column
        spacing: Math.round(1 * root.s)
        focus: true
        Keys.onPressed: function (event) {
            switch (event.key) {
            case Qt.Key_Down:  root.step(1); break
            case Qt.Key_Up:    root.step(-1); break
            case Qt.Key_Home:  root.current = -1; root.step(1); break
            case Qt.Key_End:   root.current = -1; root.step(-1); break
            case Qt.Key_Return:
            case Qt.Key_Enter:
            case Qt.Key_Space: root.runCurrent(); break
            default: return
            }
            event.accepted = true
        }

        Repeater {
            model: root.actions
            delegate: Item {
                id: row
                required property var modelData
                required property int index
                readonly property bool isSeparator: modelData.separator === true
                readonly property bool active: !isSeparator && modelData.enabled !== false
                width: Math.round(210 * root.s)
                height: isSeparator ? Math.round(9 * root.s) : Math.round(30 * root.s)

                // Separator line.
                Rectangle {
                    visible: row.isSeparator
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: Math.round(8 * root.s)
                    anchors.rightMargin: Math.round(8 * root.s)
                    anchors.verticalCenter: parent.verticalCenter
                    height: 1
                    color: root.theme.border
                }

                Rectangle {
                    visible: !row.isSeparator
                    anchors.fill: parent
                    radius: Math.round(7 * root.s * config.radiusScale)
                    color: row.active && root.current === row.index
                           ? Qt.rgba(root.theme.accent.r, root.theme.accent.g, root.theme.accent.b, area.pressed ? 0.26 : 0.16)
                           : "transparent"

                    Label {
                        anchors.left: parent.left
                        anchors.leftMargin: Math.round(12 * root.s)
                        anchors.verticalCenter: parent.verticalCenter
                        text: row.modelData.text || ""
                        color: row.active ? root.theme.text : root.theme.textDim
                        opacity: row.active ? 1 : 0.55
                        font.pixelSize: Math.round(12 * root.s)
                    }
                    Label {
                        anchors.right: parent.right
                        anchors.rightMargin: Math.round(12 * root.s)
                        anchors.verticalCenter: parent.verticalCenter
                        visible: !!row.modelData.shortcut
                        text: row.modelData.shortcut ? row.modelData.shortcut : ""
                        color: root.theme.textDim
                        opacity: row.active ? 0.85 : 0.4
                        font.pixelSize: Math.round(10 * root.s)
                    }
                }
                MouseArea {
                    id: area
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: !row.isSeparator
                    // Disabled rows swallow the click (the menu stays open) instead of passing it to the text.
                    cursorShape: row.active ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onEntered: root.current = row.active ? row.index : -1
                    onClicked: {
                        if (!row.active)
                            return
                        root.close()
                        row.modelData.run()
                    }
                }
            }
        }
    }
}
