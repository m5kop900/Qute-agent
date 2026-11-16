import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// The Ask Qute prompt: a single rounded input surface plus the send button. The agent-mode,
// model and reasoning-effort controls now live in the top menu strip (MenuStrip), not here, so
// the input row stays clear of the message field.
Item {
    id: root
    property var theme
    property real s: 1.0
    property bool enabled: true
    property string offText: qsTr("Connecting to opencode…")
    signal submitted(string text)

    readonly property bool hasText: input.text.trim().length > 0
    readonly property bool engaged: input.activeFocus

    implicitHeight: Math.round(78 * s)

    function titleCase(value) {
        return value.length > 0 ? value.charAt(0).toUpperCase() + value.slice(1) : value
    }

    function focusInput() {
        input.forceActiveFocus()
    }

    function submit() {
        if (!root.enabled)
            return
        const text = input.text.trim()
        if (text.length === 0)
            return
        input.text = ""
        root.submitted(text)
    }

    Rectangle {
        id: box
        anchors.fill: parent
        radius: Math.round(12 * s)
        color: root.theme.input
        border.width: 1
        border.color: root.engaged ? root.theme.accent
                                   : hover.hovered && root.enabled ? Qt.rgba(root.theme.text.r, root.theme.text.g, root.theme.text.b, 0.28)
                                                                  : root.theme.border
        Behavior on border.color { ColorAnimation { duration: 130 } }

        HoverHandler { id: hover }
    }

    ColumnLayout {
        anchors.fill: box
        anchors.margins: Math.round(7 * s)
        spacing: Math.round(3 * s)

        TextArea {
            id: input
            Layout.fillWidth: true
            Layout.preferredHeight: Math.round(38 * s)
            enabled: root.enabled
            placeholderText: root.enabled ? qsTr("Ask qute…") : root.offText
            placeholderTextColor: root.theme.textDim
            color: root.enabled ? root.theme.text : root.theme.textDim
            selectionColor: root.theme.accent
            selectedTextColor: root.theme.input
            font.pixelSize: Math.round(13.5 * s)
            wrapMode: TextArea.Wrap
            selectByMouse: true
            clip: true
            background: null
            padding: 0
            leftPadding: Math.round(3 * s)
            topPadding: Math.round(1 * s)

            Keys.onPressed: function (event) {
                if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                    if (event.modifiers & Qt.ShiftModifier)
                        return
                    root.submit()
                    event.accepted = true
                }
            }
            Keys.onEscapePressed: controller.hide()
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.round(22 * s)

            Item { Layout.fillWidth: true }

            Rectangle {
                id: sendButton
                Layout.preferredWidth: Math.round(22 * root.s)
                Layout.preferredHeight: Math.round(22 * root.s)
                radius: width / 2
                readonly property bool ready: root.hasText && root.enabled
                color: ready ? Qt.rgba(root.theme.accent.r, root.theme.accent.g, root.theme.accent.b, 0.18)
                             : "transparent"
                border.width: 1
                border.color: ready ? Qt.rgba(root.theme.accent.r, root.theme.accent.g, root.theme.accent.b, 0.55)
                                    : root.theme.border
                Behavior on color { ColorAnimation { duration: 120 } }

                Label {
                    anchors.centerIn: parent
                    text: "↑"
                    color: sendButton.ready ? root.theme.accent : root.theme.textDim
                    font.pixelSize: Math.round(12 * root.s)
                    font.bold: true
                }
                HoverHandler { id: sendHover; cursorShape: sendButton.ready ? Qt.PointingHandCursor : Qt.ArrowCursor }
                TapHandler {
                    enabled: sendButton.ready
                    onTapped: root.submit()
                }

            }
            Hint {
                target: sendButton
                theme: root.theme
                s: root.s
                shown: sendHover.hovered
                delayMs: 400
                text: qsTr("Send · Enter")
            }
        }
    }
}
