import QtQuick
import QtQuick.Controls

// A small context menu drawn with the overlay's own theme (same panel, border, radius and hover as the rest of
// the UI), not the platform style. Opened at a point of its parent with open(); closes on Esc or outside click.
Popup {
    id: root
    property var theme
    property real s: 1.0
    // [{text: string, enabled: bool, run: function}]
    property var actions: []

    padding: Math.round(4 * s)
    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle {
        radius: Math.round(10 * root.s)
        color: root.theme.panel
        border.width: 1
        border.color: root.theme.border
    }

    contentItem: Column {
        spacing: 0
        Repeater {
            model: root.actions
            delegate: Rectangle {
                id: row
                required property var modelData
                width: Math.round(132 * root.s)
                height: Math.round(28 * root.s)
                radius: Math.round(6 * root.s)
                color: area.containsMouse && row.modelData.enabled
                       ? Qt.rgba(root.theme.accent.r, root.theme.accent.g, root.theme.accent.b, 0.14)
                       : "transparent"

                Label {
                    anchors.left: parent.left
                    anchors.leftMargin: Math.round(10 * root.s)
                    anchors.verticalCenter: parent.verticalCenter
                    text: row.modelData.text
                    color: row.modelData.enabled ? root.theme.text : root.theme.textDim
                    font.pixelSize: Math.round(11.5 * root.s)
                }
                MouseArea {
                    id: area
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: row.modelData.enabled
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.close()
                        row.modelData.run()
                    }
                }
            }
        }
    }
}
