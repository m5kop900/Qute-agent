import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Compact themed pill used in the status row (Stop, Allow, Always, Deny, ...). Replaces the default
// Qt Controls button look, which clashed with the overlay.
AbstractButton {
    id: root
    property var theme
    property real s: 1.0
    property color tint: theme.accent
    property string glyph: ""

    implicitHeight: Math.round(22 * s)
    leftPadding: Math.round(9 * s)
    rightPadding: Math.round(10 * s)
    topPadding: 0
    bottomPadding: 0
    hoverEnabled: true

    background: Rectangle {
        radius: height / 2
        color: root.down ? Qt.rgba(root.tint.r, root.tint.g, root.tint.b, 0.28)
             : root.hovered ? Qt.rgba(root.tint.r, root.tint.g, root.tint.b, 0.17)
             : Qt.rgba(root.tint.r, root.tint.g, root.tint.b, 0.09)
        border.width: 1
        border.color: Qt.rgba(root.tint.r, root.tint.g, root.tint.b, 0.45)
        Behavior on color { ColorAnimation { duration: 110 } }
    }

    contentItem: RowLayout {
        spacing: Math.round(4 * root.s)
        Label {
            visible: root.glyph !== ""
            Layout.alignment: Qt.AlignVCenter
            text: root.glyph
            color: root.tint
            font.pixelSize: Math.round(7.5 * root.s)
        }
        Label {
            Layout.alignment: Qt.AlignVCenter
            text: root.text
            color: root.tint
            font.pixelSize: Math.round(10 * root.s)
            font.bold: true
        }
    }

    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
