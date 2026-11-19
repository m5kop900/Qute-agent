import QtQuick
import QtQuick.Controls

// Themed hover hint. Replaces the default Qt ToolTip so it matches the overlay: small, rounded,
// theme colours, a short delay and a width cap so long texts wrap instead of stretching.
ToolTip {
    id: root
    required property Item target
    property var theme
    property real s: 1.0
    property bool shown: false
    property int delayMs: 350

    parent: target
    visible: false
    timeout: -1
    closePolicy: Popup.NoAutoClose
    padding: 0

    Timer {
        id: showTimer
        interval: root.delayMs
        onTriggered: root.visible = root.shown
    }
    onShownChanged: {
        if (shown) {
            showTimer.restart()
        } else {
            showTimer.stop()
            root.visible = false
        }
    }

    background: Rectangle {
        radius: Math.round(8 * root.s * config.radiusScale)
        color: root.theme.input
        border.width: 1
        border.color: root.theme.border
    }

    contentItem: Label {
        text: root.text
        color: root.theme.text
        font.pixelSize: Math.round(10.5 * root.s)
        wrapMode: Text.WordWrap
        width: Math.min(implicitWidth, Math.round(230 * root.s))
        leftPadding: Math.round(9 * root.s)
        rightPadding: Math.round(9 * root.s)
        topPadding: Math.round(5 * root.s)
        bottomPadding: Math.round(5 * root.s)
    }
}
