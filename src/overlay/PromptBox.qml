import QtQuick
import QtQuick.Controls

TextField {
    id: root
    property var theme
    property real s: 1.0
    signal submitted(string text)

    function focusInput() { forceActiveFocus() }

    placeholderText: enabled ? qsTr("Ask qute…") : qsTr("Connecting to opencode…")
    color: theme.text
    placeholderTextColor: theme.textDim
    selectionColor: theme.accent
    font.pixelSize: Math.round(14 * s)
    leftPadding: Math.round(12 * s)
    rightPadding: Math.round(12 * s)
    implicitHeight: Math.round(38 * s)

    background: Rectangle {
        radius: height / 2
        color: root.theme.input
        border.width: 1
        border.color: root.activeFocus ? root.theme.accent : root.theme.border
    }

    onAccepted: {
        var t = text.trim()
        if (t.length > 0) {
            submitted(t)
            text = ""
        }
    }
    Keys.onEscapePressed: controller.hide()
}
