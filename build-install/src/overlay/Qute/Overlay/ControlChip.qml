import QtQuick
import QtQuick.Controls

// Small pill used inside the composer for agent mode / model / effort.
// Text-only on purpose: the controls must not compete with the message input.
Rectangle {
    id: root
    property var theme
    property real s: 1.0
    property string text: ""
    property bool active: false
    property bool strong: false
    property bool dot: false
    property bool interactive: true
    property real maxTextWidth: Math.round(240 * s)
    property alias hovered: hover.hovered
    signal clicked()

    implicitHeight: Math.round(22 * s)
    implicitWidth: Math.min(maxTextWidth + Math.round(34 * s), row.implicitWidth + Math.round(16 * s))
    radius: Math.round(6 * s * config.radiusScale)
    color: active ? Qt.rgba(theme.accent.r, theme.accent.g, theme.accent.b, 0.16)
                  : hover.hovered && interactive ? Qt.rgba(theme.text.r, theme.text.g, theme.text.b, 0.06)
                  : "transparent"
    border.width: 1
    border.color: active ? Qt.rgba(theme.accent.r, theme.accent.g, theme.accent.b, 0.65) : "transparent"
    Behavior on color { ColorAnimation { duration: 110 } }
    Behavior on border.color { ColorAnimation { duration: 110 } }

    Row {
        id: row
        anchors.centerIn: parent
        spacing: Math.round(4 * s)

        Rectangle {
            id: marker
            visible: root.dot
            width: Math.round(4 * s)
            height: width
            radius: width / 2
            color: root.theme.accent
        }
        Label {
            id: label
            text: root.text
            color: root.strong ? root.theme.text : root.theme.textDim
            font.pixelSize: Math.round(10.5 * s)
            font.bold: root.strong
            elide: Text.ElideRight
            width: Math.min(implicitWidth, root.maxTextWidth)
        }
        Label {
            visible: root.interactive
            text: "⌄"
            color: root.theme.textDim
            opacity: 0.75
            font.pixelSize: Math.round(10 * s)
        }
    }

    HoverHandler {
        id: hover
        enabled: root.interactive
        cursorShape: Qt.PointingHandCursor
    }
    TapHandler {
        enabled: root.interactive
        onTapped: root.clicked()
    }
}
