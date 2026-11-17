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

    // Set by PlatformShell::reveal() once the window is at its final position.
    property bool placed: false

    readonly property real s: config.scale
    readonly property string st: agent.stateName
    readonly property bool busy: st === "thinking" || st === "working" || st === "waiting"

    // 0 = prompt only, 1 = reply visible, 2 = long reply. The card is anchored to the window BOTTOM and
    // grows UPWARD: the prompt box never moves when a reply appears. The OS window is resized to match,
    // so the window top edge is always the card's top edge - the compositor stops an interactive window
    // move once its top edge reaches the screen top, so invisible space above the card would be a hard
    // limit for dragging the overlay up. Clicks below the card pass through (see updateInputRegion).
    readonly property int desiredLevel: {
        if (client.responseText.length > 500 || conversation.count > 4) return 2
        if (client.responseText.length > 0 || busy || conversation.count > 0) return 1
        if (!client.connected && client.lastError !== "") return 1
        return 0
    }
    readonly property var panelHeights: [140, 250, 430]
    // Empty room kept inside the window below the card: the menu strip dropdowns open into it.
    readonly property int dropRoom: Math.round(200 * s)
    // Requested card geometry in window coordinates. The card's bottom edge is the prompt box;
    // contentHeight/dropRoom are read by the C++ platform shell, which resizes and moves the window
    // (the compositor must do it: a client-side resize is center-anchored and would shift the card).
    readonly property real contentTop: Math.round(10 * s)
    readonly property int contentHeight: Math.round(contentTop + panelHeights[desiredLevel] * s)
    // The card grows upward from cardBottom. Until the window has been resized, never draw it taller
    // than the window can show, so nothing is ever clipped at the top.
    readonly property real cardBottom: win.height - dropRoom
    readonly property int cardHeight: Math.min(Math.round(panelHeights[desiredLevel] * s),
                                               Math.max(0, Math.round(cardBottom - contentTop)))

    width: Math.round(560 * s)
    // The height is applied by the platform shell (see contentHeight/dropRoom); this only gives the
    // surface a sensible size before the first placement.
    // A new level (or another UI scale) needs a window tall enough for the card: ask the shell for it.
    onContentHeightChanged: controller.resizeToContent(win.contentHeight, win.dropRoom)

    Theme { id: appTheme; cfg: config }

    // Input region: only the visible card (or an open dropdown) takes mouse input; the slack below it
    // is transparent to the mouse and clicks reach whatever is underneath.
    function updateInputRegion() {
        // Use the panel's real height, not the target level's, so the region matches what is drawn
        // during the 220 ms height animation as well.
        let top = win.cardBottom - panel.height
        let bottom = win.cardBottom
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
        // The window is transparent, so hiding its content hides the window. It stays hidden until the
        // platform shell has moved it to the cursor (PlatformShell::reveal), so it never shows at the
        // compositor's default spot first.
        opacity: win.placed ? 1 : 0

        // Summon ring: a soft accent circle that flashes out from behind the mascot.
        Rectangle {
            id: summonRing
            width: Math.round(64 * win.s)
            height: width
            radius: width / 2
            x: sprite.x + sprite.width / 2 - width / 2
            y: sprite.y + sprite.height / 2 - height / 2
            color: "transparent"
            border.width: Math.max(1, Math.round(2 * win.s))
            border.color: appTheme.accent
            opacity: 0
            scale: 1
            transformOrigin: Item.Center
        }

        SpriteView {
            id: sprite
            width: Math.round(128 * win.s)
            height: width
            // Sits on the prompt box's bottom edge, which stays put while the card grows upward.
            y: win.cardBottom - height
            stateName: win.st
            scale: 1
            rotation: 0
            opacity: 1
            transformOrigin: Item.Center
        }

        // Sparkles burst out of the mascot on summon. Each one plays its own short path (see play()).
        Repeater {
            id: sparkles
            model: 7
            delegate: Text {
                id: spark
                required property int index
                property real t: 0
                readonly property real ang: index / 7 * 2 * Math.PI - Math.PI / 2
                readonly property real dist: Math.round(64 * win.s)
                x: sprite.x + sprite.width / 2 - width / 2 + Math.cos(ang) * dist * t
                y: sprite.y + sprite.height / 2 - height / 2 + Math.sin(ang) * dist * t
                opacity: Math.sin(Math.PI * t)
                text: ["\u2726", "\u2661", "\u2727", "\u273f", "\u2605", "\u2661", "\u2727"][index]
                color: index % 2 ? "#ff9ccc" : appTheme.accent
                font.pixelSize: Math.round(14 * win.s)
                function play() { burst.restart() }
                SequentialAnimation {
                    id: burst
                    PauseAnimation { duration: spark.index * 14 }
                    NumberAnimation { target: spark; property: "t"; from: 0; to: 1; duration: 460; easing.type: Easing.OutCubic }
                }
            }
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
            y: win.cardBottom - height   // grows upward; the bottom edge (prompt box) stays put
            width: parent.width - x - Math.round(6 * win.s)
            height: win.cardHeight
            radius: Math.round(16 * win.s)
            color: appTheme.panel
            transform: Translate { id: panelShift; y: 0 }
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
                    id: chat
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
                    onSubmitted: function (text) { chat.followEnd(); client.sendPrompt(text) }
                    onCommand: function (text) { win.runCommand(text) }
                }
            }
        }
    }

    // Summon: the mascot pops in with a little overshoot and wiggle, a ring flashes behind it, sparkles
    // burst out, and the card slides up. Runs each time the window is shown at the cursor (placed goes
    // true), takes about 400 ms, and never touches the prompt: typing works while it plays.
    onPlacedChanged: if (placed) summon()

    function summon() {
        summonAnim.restart()
        for (let i = 0; i < sparkles.count; ++i)
            sparkles.itemAt(i).play()
    }

    SequentialAnimation {
        id: summonAnim
        ScriptAction {
            script: {
                sprite.scale = 0.2
                sprite.rotation = 0
                sprite.opacity = 0
                summonRing.scale = 0.4
                summonRing.opacity = 0.9
                panel.opacity = 0
                panelShift.y = 14
            }
        }
        ParallelAnimation {
            NumberAnimation { target: sprite; property: "scale"; to: 1.12; duration: 240; easing.type: Easing.OutBack; easing.overshoot: 2.2 }
            NumberAnimation { target: sprite; property: "opacity"; to: 1; duration: 140; easing.type: Easing.OutQuad }
            SequentialAnimation {
                NumberAnimation { target: sprite; property: "rotation"; to: -12; duration: 110; easing.type: Easing.OutQuad }
                NumberAnimation { target: sprite; property: "rotation"; to: 9; duration: 120; easing.type: Easing.InOutQuad }
                NumberAnimation { target: sprite; property: "rotation"; to: 0; duration: 160; easing.type: Easing.OutElastic; easing.amplitude: 1.0; easing.period: 0.4 }
            }
            NumberAnimation { target: summonRing; property: "scale"; to: 2.6; duration: 380; easing.type: Easing.OutCubic }
            NumberAnimation { target: summonRing; property: "opacity"; to: 0; duration: 380; easing.type: Easing.OutQuad }
            NumberAnimation { target: panel; property: "opacity"; to: 1; duration: 220; easing.type: Easing.OutCubic }
            NumberAnimation { target: panelShift; property: "y"; to: 0; duration: 260; easing.type: Easing.OutBack }
        }
        NumberAnimation { target: sprite; property: "scale"; to: 1; duration: 120; easing.type: Easing.InOutQuad }
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

    Component.onCompleted: {
        win.height = win.contentHeight + win.dropRoom
        updateInputRegion()
    }
}
