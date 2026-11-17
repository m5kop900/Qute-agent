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

    // 0 = prompt only, 1 = reply visible, 2 = long reply. Only the card inside changes height; the OS
    // window keeps its maximum size (continuous native resize is janky on Wayland/KWin). The card is
    // anchored to the TOP of the window and grows downward: the compositor stops an interactive move
    // once the window's top edge reaches the screen top, so a transparent strip above the card would
    // become a hard limit for dragging the overlay up. Clicks in the slack below the card pass through
    // via the input region (see updateInputRegion).
    readonly property int desiredLevel: {
        if (client.responseText.length > 500 || conversation.count > 4) return 2
        if (client.responseText.length > 0 || busy || conversation.count > 0) return 1
        if (!client.connected && client.lastError !== "") return 1
        return 0
    }
    readonly property var panelHeights: [140, 250, 430]
    readonly property var windowHeights: [190, 350, 530]
    // Fixed OS window size; only the card inside changes height (see above).
    readonly property int maxHeight: Math.round(windowHeights[2] * s)
    // Card geometry in window coordinates. The card starts at contentTop and its bottom edge is the
    // prompt box; contentHeight/maxContentHeight are read by the C++ platform shell for placement.
    readonly property real contentTop: Math.round(10 * s)
    readonly property int contentHeight: Math.round(contentTop + panelHeights[desiredLevel] * s)
    readonly property int maxContentHeight: Math.round(contentTop + panelHeights[2] * s)

    width: Math.round(560 * s)
    height: maxHeight

    Theme { id: appTheme; cfg: config }

    // Input region: only the visible card (or an open dropdown) takes mouse input; the slack below it
    // is transparent to the mouse and clicks reach whatever is underneath.
    function updateInputRegion() {
        // Use the panel's real height, not the target level's, so the region matches what is drawn
        // during the 220 ms height animation as well.
        let top = win.contentTop
        let bottom = win.contentTop + panel.height
        if (menuStrip.popupOpen) {
            // A dropdown can open above or below the strip: the clickable area covers it either way.
            const stripY = menuStrip.mapToItem(win.contentItem, 0, 0).y
            top = Math.min(top, Math.floor(stripY + menuStrip.openPopupTop - Math.round(12 * win.s)))
            bottom = Math.max(bottom, Math.ceil(stripY + menuStrip.openPopupBottom + Math.round(12 * win.s)))
        }
        top = Math.max(0, Math.round(top))
        bottom = Math.min(Math.round(bottom), win.height)
        controller.setInputRegion(0, top, win.width, bottom - top)
    }
    onDesiredLevelChanged: updateInputRegion()
    onWidthChanged: updateInputRegion()
    onHeightChanged: updateInputRegion()
    Connections {
        target: menuStrip
        function onPopupOpenChanged() { win.updateInputRegion() }
        function onOpenPopupBottomChanged() { win.updateInputRegion() }
        function onOpenPopupTopChanged() { win.updateInputRegion() }
    }

    Connections {
        target: controller
        function onShown() {
            win.updateInputRegion()
            composer.focusInput()
        }
    }

    // Escape closes an open dropdown first; only when none is open does it hide the overlay.
    Shortcut { sequence: "Escape"; enabled: !menuStrip.popupOpen; onActivated: controller.hide() }

    Item {
        anchors.fill: parent

        SpriteView {
            id: sprite
            width: Math.round(128 * win.s)
            height: width
            // Sits on the prompt box's bottom edge, as before: the card grows downward, so the sprite
            // follows the card's bottom edge instead of staying pinned to the window top.
            y: win.contentTop + panel.height - height
            stateName: win.st
        }

        // Hold the mouse on the sprite and drag: the compositor moves the whole overlay.
        MouseArea {
            anchors.fill: sprite
            acceptedButtons: Qt.LeftButton
            cursorShape: Qt.OpenHandCursor
            onPressed: win.startSystemMove()
        }

        Rectangle {
            id: panel
            x: sprite.width + Math.round(4 * win.s)
            y: win.contentTop     // grows downward; the top edge stays at the window top
            width: parent.width - x - Math.round(6 * win.s)
            height: Math.round(win.panelHeights[win.desiredLevel] * win.s)
            radius: Math.round(16 * win.s)
            color: appTheme.panel
            border.width: 1
            border.color: appTheme.border
            Behavior on height { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
            // Keep the click mask in sync with what is actually drawn, every frame of the animation.
            onHeightChanged: win.updateInputRegion()

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Math.round(10 * win.s)
                spacing: Math.round(8 * win.s)

                MenuStrip {
                    id: menuStrip
                    Layout.fillWidth: true
                    theme: appTheme
                    s: win.s
                    sessionStore: store
                    modelCatalog: catalog
                    enabled: client.connected
                }

                ConversationView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: win.desiredLevel > 0
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
                        text: qsTr("Settings")
                        onClicked: controller.runSettings()
                    }
                    Button {
                        visible: client.opencodeMissing
                        text: qsTr("Set up opencode")
                        onClicked: controller.runSetup()
                    }
                }

                Rectangle {                       // status bar: activity + permission answers + stop
                    id: statusBar
                    Layout.fillWidth: true
                    visible: win.busy
                    implicitHeight: Math.round(30 * win.s)
                    radius: Math.round(9 * win.s)
                    color: Qt.rgba(appTheme.text.r, appTheme.text.g, appTheme.text.b, 0.05)
                    border.width: 1
                    border.color: win.st === "waiting" ? Qt.rgba(appTheme.accent.r, appTheme.accent.g, appTheme.accent.b, 0.4)
                                                       : appTheme.border

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Math.round(10 * win.s)
                        anchors.rightMargin: Math.round(5 * win.s)
                        spacing: Math.round(7 * win.s)

                        Rectangle {                   // pulsing activity dot
                            Layout.preferredWidth: Math.round(6 * win.s)
                            Layout.preferredHeight: Layout.preferredWidth
                            radius: width / 2
                            color: win.st === "error" ? "#e5484d" : appTheme.accent
                            SequentialAnimation on opacity {
                                loops: Animation.Infinite
                                running: statusBar.visible
                                NumberAnimation { to: 0.25; duration: 650; easing.type: Easing.InOutSine }
                                NumberAnimation { to: 1.0; duration: 650; easing.type: Easing.InOutSine }
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            color: appTheme.textDim
                            font.pixelSize: Math.round(11 * win.s)
                            text: win.st === "waiting" ? (agent.requestKind === "form" ? qsTr("opencode asks: %1 (answer it in opencode, or cancel)").arg(agent.detail)
                                                                           : (agent.detail || qsTr("Permission needed")))
                                                       : (agent.detail ? agent.detail + "…" : qsTr("Working…"))
                        }

                        PillButton { theme: appTheme; s: win.s; visible: win.st === "waiting" && agent.requestKind !== "form"; text: qsTr("Allow");  onClicked: client.answerPermission(agent.permissionId, "once") }
                        PillButton { theme: appTheme; s: win.s; visible: win.st === "waiting" && agent.requestKind !== "form"; text: qsTr("Always"); onClicked: client.answerPermission(agent.permissionId, "always") }
                        PillButton { theme: appTheme; s: win.s; visible: win.st === "waiting" && agent.requestKind !== "form"; text: qsTr("Deny");   tint: "#e5484d"; onClicked: client.answerPermission(agent.permissionId, "reject") }
                        PillButton { theme: appTheme; s: win.s; visible: win.st === "waiting" && agent.requestKind === "form"; text: qsTr("Cancel question"); onClicked: client.cancelForm(agent.permissionId) }
                        PillButton { theme: appTheme; s: win.s; visible: win.st !== "waiting"; text: qsTr("Stop"); glyph: "■"; tint: appTheme.textDim; onClicked: client.abort() }
                    }
                }

                AttachmentStrip {                 // F1 fills this; empty for now
                    Layout.fillWidth: true
                    theme: appTheme
                    s: win.s
                }

                Composer {
                    id: composer
                    Layout.fillWidth: true
                    theme: appTheme
                    s: win.s
                    enabled: client.connected
                    offText: client.lastError !== "" ? qsTr("opencode is not available") : qsTr("Connecting to opencode…")
                    onSubmitted: function (text) { client.sendPrompt(text) }
                    onCommand: function (text) { win.runCommand(text) }
                }
            }
        }
    }

    // Slash commands typed in the prompt box. They run in Qute, not on the server.
    function runCommand(line) {
        const m = /^\/(\S+)\s*(.*)$/.exec(line)
        const name = m ? m[1] : ""
        const arg = m ? m[2].trim() : ""
        if (name === "directory") {
            if (arg === "") {
                conversation.addNotice(qsTr("Working directory: %1").arg(config.workDir))
                return
            }
            const err = controller.setWorkDirectory(arg)
            if (err !== "") {
                conversation.addNotice(err)
                return
            }
            // A session is bound to its directory, so changing it starts a fresh session there.
            conversation.clear()
            client.newSession()
            conversation.addNotice(qsTr("New session in %1").arg(config.workDir))
        } else if (name === "openinopencode") {
            const err = controller.openInOpencode(client.sessionId, store.workDir)
            if (err !== "")
                conversation.addNotice(err)
            else
                controller.hide()
        } else {
            conversation.addNotice(qsTr("Unknown command: /%1").arg(name))
        }
    }

    Component.onCompleted: updateInputRegion()
}
