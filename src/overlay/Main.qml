import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Overlay window. Context properties from C++: config, packs, client, agent, controller, platformName.
// Window flags, transparency and placement are handled by the C++ PlatformShell, NOT here.
Window {
    id: win
    title: "qute-overlay"          // KWin placement script matches this exact title - do not change
    visible: false
    color: "transparent"

    readonly property real s: config.scale
    readonly property string st: agent.stateName
    readonly property bool busy: st === "thinking" || st === "working" || st === "waiting"

    // 0 = prompt only, 1 = reply visible, 2 = long reply
    readonly property int desiredLevel: {
        if (client.responseText.length > 500) return 2
        if (client.responseText.length > 0 || busy) return 1
        if (!client.connected && client.lastError !== "") return 1
        return 0
    }
    property int level: 0
    readonly property var panelHeights: [58, 190, 380]
    readonly property var windowHeights: [150, 290, 490]
    readonly property int maxHeight: Math.round(windowHeights[2] * s)   // read by the C++ platform shell

    width: Math.round(560 * s)
    function heightFor(l) { return Math.round(windowHeights[l] * s) }
    Component.onCompleted: height = heightFor(0)

    // The OS window changes size in 3 discrete steps (cheap, no per-frame resize); the panel inside
    // animates smoothly. Grow first, shrink after the panel animation finished.
    onDesiredLevelChanged: {
        if (desiredLevel > level) { level = desiredLevel; height = heightFor(level) }
        else shrinkTimer.restart()
    }
    Timer {
        id: shrinkTimer
        interval: 280
        onTriggered: { win.level = win.desiredLevel; win.height = win.heightFor(win.level) }
    }

    Theme { id: appTheme; cfg: config }

    Connections {
        target: controller
        function onShown() {
            win.level = win.desiredLevel
            win.height = win.heightFor(win.level)
            prompt.focusInput()
        }
    }

    Shortcut { sequence: "Escape"; onActivated: controller.hide() }

    Item {
        anchors.fill: parent

        SpriteView {
            id: sprite
            width: Math.round(128 * win.s)
            height: width
            stateName: win.st
        }

        Rectangle {
            id: panel
            x: sprite.width + Math.round(4 * win.s)
            y: Math.round(10 * win.s)
            width: parent.width - x - Math.round(6 * win.s)
            height: Math.round(win.panelHeights[win.desiredLevel] * win.s)
            radius: Math.round(16 * win.s)
            color: appTheme.panel
            border.width: 1
            border.color: appTheme.border
            Behavior on height { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Math.round(10 * win.s)
                spacing: Math.round(8 * win.s)

                ResponseView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: win.desiredLevel > 0
                    text: client.responseText
                    theme: appTheme
                    s: win.s
                }

                RowLayout {                       // connection problem / "opencode missing" helper
                    Layout.fillWidth: true
                    visible: !client.connected && client.lastError !== ""
                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        color: appTheme.text
                        font.pixelSize: Math.round(12 * win.s)
                        text: client.lastError
                    }
                    Button {
                        visible: client.opencodeMissing
                        text: qsTr("Set up opencode")
                        onClicked: controller.runSetup()
                    }
                }

                RowLayout {                       // permission request / stop button
                    Layout.fillWidth: true
                    visible: win.busy
                    Label {
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        color: appTheme.text
                        font.pixelSize: Math.round(12 * win.s)
                        text: win.st === "waiting" ? (agent.requestKind === "form" ? qsTr("opencode asks: %1 (answer it in opencode, or cancel)").arg(agent.detail)
                                                                       : (agent.detail || qsTr("Permission needed")))
                                                   : (agent.detail ? agent.detail + "…" : qsTr("Working…"))
                    }
                    Button { visible: win.st === "waiting" && agent.requestKind !== "form"; text: qsTr("Allow");  onClicked: client.answerPermission(agent.permissionId, "once") }
                    Button { visible: win.st === "waiting" && agent.requestKind !== "form"; text: qsTr("Always"); onClicked: client.answerPermission(agent.permissionId, "always") }
                    Button { visible: win.st === "waiting" && agent.requestKind !== "form"; text: qsTr("Deny");   onClicked: client.answerPermission(agent.permissionId, "reject") }
                    Button { visible: win.st === "waiting" && agent.requestKind === "form"; text: qsTr("Cancel question"); onClicked: client.cancelForm(agent.permissionId) }
                    Button { visible: win.st !== "waiting"; text: qsTr("Stop");   onClicked: client.abort() }
                }

                PromptBox {
                    id: prompt
                    Layout.fillWidth: true
                    theme: appTheme
                    s: win.s
                    enabled: client.connected
                    onSubmitted: function (text) { client.sendPrompt(text) }
                }
            }
        }
    }
}
