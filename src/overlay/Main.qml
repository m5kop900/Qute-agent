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
    readonly property int baseLevel: {
        if (client.responseText.length > 500 || conversation.count > 4) return 2
        if (client.responseText.length > 0 || busy || conversation.count > 0 || store.loadingHistory) return 1
        if (!client.connected && client.lastError !== "") return 1
        return 0
    }
    // The size only ratchets: it grows as the conversation grows and shrinks back only when the conversation is
    // empty (a new session, /new, or a cleared view). Lists never change it: they open in the drop room.
    property int shownLevel: 0
    readonly property int desiredLevel: shownLevel
    onBaseLevelChanged: {
        if (baseLevel > shownLevel || (conversation.count === 0 && !busy))
            shownLevel = baseLevel
    }
    readonly property var panelHeights: [140, 250, 430]
    // Empty room kept inside the window below the card: the menu strip dropdowns open into it.
    readonly property int dropRoom: Math.round(200 * s)
    // Extra card height while files, pasted images or @mentions wait for the next prompt: the strip needs its own
    // row, otherwise the chips are squeezed out of the card (the conversation takes what is left).
    readonly property int attachExtra: attachments.count > 0 ? Math.round(56 * s) : 0
    // Requested card geometry in window coordinates. The card's bottom edge is the prompt box;
    // contentHeight/dropRoom are read by the C++ platform shell, which resizes and moves the window
    // (the compositor must do it: a client-side resize is center-anchored and would shift the card).
    readonly property real contentTop: Math.round(10 * s)
    readonly property int contentHeight: Math.round(contentTop + panelHeights[desiredLevel] * s) + attachExtra
    // The card grows upward from cardBottom. Until the window has been resized, never draw it taller
    // than the window can show, so nothing is ever clipped at the top.
    readonly property real cardBottom: win.height - dropRoom
    readonly property int cardHeight: Math.min(Math.round(panelHeights[desiredLevel] * s) + attachExtra,
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
        if (composer.listOpen) {
            // The "/", "@", /sessions, /agents, /models and file lists hang below the prompt box, in the drop room.
            // They must take clicks too, or the desktop under the room gets them.
            const boxY = composer.mapToItem(win.contentItem, 0, 0).y
            const listTop = boxY + composer.listTop
            const listBottom = listTop + composer.listPixelHeight + Math.round(6 * win.s)
            top = Math.min(top, Math.floor(listTop))
            bottom = Math.max(bottom, Math.ceil(listBottom))
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
        target: composer
        function onListOpenChanged() { win.updateInputRegion() }
        function onListPixelHeightChanged() { win.updateInputRegion() }
    }

    Connections {
        target: controller
        function onShown() {
            win.updateInputRegion()
            composer.focusInput()
        }
    }

    // Escape closes an open dropdown first; only when none is open does it hide the overlay.
    // Esc closes the open menu or list first; the overlay only hides when nothing is open.
    Shortcut { sequence: "Escape"; enabled: !menuStrip.popupOpen && !composer.listOpen; onActivated: controller.hide() }

    // Shift+Tab switches the agent mode (build <-> plan). A window shortcut, so it works wherever the focus is;
    // the prompt box also handles it, but only while it has the keyboard.
    Shortcut {
        sequences: ["Shift+Tab", "Backtab"]
        onActivated: {
            console.log("qute-debug: Shift+Tab reached the window shortcut")
            win.cycleAgent()
        }
    }

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

                AttachmentStrip {                 // files, pasted images and @mentions waiting for the next prompt (F1)
                    Layout.fillWidth: true
                    theme: appTheme
                    s: win.s
                    model: attachments
                }

                Composer {
                    id: composer
                    Layout.fillWidth: true
                    theme: appTheme
                    s: win.s
                    tools: composerTools
                    files: attachments
                    enabled: client.connected
                    offText: client.lastError !== "" ? qsTr("opencode is not available") : qsTr("Connecting to opencode…")
                    onSubmitted: function (text) { chat.followEnd(); win.sendPrompt(text) }
                    onCommand: function (text) { win.runCommand(text) }
                    onModeCycleRequested: win.cycleAgent()
                    commandItems: catalog.slashItems
                    mentionItems: win.mentionItems
                    onMentionQueryChanged: function (query) { win.mentionQuery = query; catalog.searchFiles(query) }
                    currentSessionKey: client.sessionId
                    listMaxHeight: win.dropRoom - Math.round(16 * win.s)
                    onListPicked: function (kind, key) { win.onListPicked(kind, key) }
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

    // "@" list: files from GET /api/fs/find (catalog.files) and the primary agents whose id matches the query.
    property string mentionQuery: ""
    readonly property var mentionItems: {
        const q = mentionQuery.toLowerCase()
        const out = []
        const agents = catalog.primaryAgents
        for (let i = 0; i < agents.length && out.length < 40; ++i) {
            const id = String(agents[i].id)
            if (id.toLowerCase().indexOf(q) >= 0)
                out.push({key: id, label: "@" + id, sub: qsTr("Agent"), tag: "AGENT", kind: "agent"})
        }
        const files = catalog.files
        for (let i = 0; i < files.length && out.length < 40; ++i) {
            const path = String(files[i].path)
            out.push({key: path, label: "@" + path, sub: qsTr("File"), tag: "", kind: "file"})
        }
        return out
    }

    // Agents named in the text ("@build") are sent with the prompt in the agents field.
    function mentionedAgentNames(text) {
        const names = []
        const agents = catalog.primaryAgents
        for (let i = 0; i < agents.length; ++i) {
            const id = String(agents[i].id)
            const re = new RegExp("(^|\\s)@" + id.replace(/[.*+?^${}()|[\]\\]/g, "\\$&") + "(\\s|$)")
            if (re.test(text))
                names.push(id)
        }
        return names
    }

    // Sends a prompt with the attachments. The list is cleared only when the client accepted the prompt, so a
    // failed send keeps the files for the next try.
    function sendPrompt(text) {
        const names = mentionedAgentNames(text)
        if (client.sendPromptWithFiles(text, attachments.promptFiles(), attachments.agentRefs(names)))
            attachments.clear()
    }

    // Items for the /agents and /models pickers, built from the catalog (same shape as the other lists).
    function agentItems() {
        const out = []
        const agents = catalog.primaryAgents
        for (let i = 0; i < agents.length; ++i) {
            const id = String(agents[i].id)
            out.push({key: id, label: id, sub: qsTr("Agent"), tag: "", kind: "agent"})
        }
        return out
    }

    function modelItems() {
        const out = []
        const models = catalog.models
        for (let i = 0; i < models.length; ++i) {
            const m = models[i]
            const name = String(m.name || m.modelID || m.id)
            out.push({key: String(m.id), label: name, sub: String(m.providerID || ""), tag: "", kind: "model"})
        }
        return out
    }

    function pickAgent(id) {
        if (!client.connected) {
            conversation.addNotice(qsTr("Agent mode can be changed once opencode is connected."))
            return
        }
        if (store.busy) {
            conversation.addNotice(qsTr("Wait for the current answer before changing the agent mode."))
            return
        }
        store.selectAgent(id)
        conversation.addNotice(qsTr("Agent: %1").arg(id))
    }

    function pickModel(key) {
        if (store.busy) {
            conversation.addNotice(qsTr("Wait for the current answer before changing the model."))
            return
        }
        const models = catalog.models
        for (let i = 0; i < models.length; ++i) {
            const m = models[i]
            if (String(m.id) === key) {
                store.selectModel(String(m.providerID), String(m.modelID), "")
                conversation.addNotice(qsTr("Model: %1").arg(String(m.name || m.modelID)))
                return
            }
        }
        conversation.addNotice(qsTr("Unknown model: %1").arg(key))
    }

    // One line for /status: what the next prompt will use.
    function statusLine() {
        const parts = []
        parts.push(qsTr("Agent: %1").arg(store.activeAgent !== "" ? store.activeAgent : "build"))
        parts.push(qsTr("Model: %1").arg(store.activeModel !== "" ? store.activeModel : qsTr("default")))
        if (store.contextPercent >= 0)
            parts.push(qsTr("Context: %1%").arg(store.contextPercent))
        parts.push(qsTr("Folder: %1").arg(store.sessionDirectory !== "" ? store.sessionDirectory : config.workDir))
        return parts.join("  ·  ")
    }

    // Clipboard for /copy: a hidden TextEdit does the copy, which needs no focus.
    TextEdit { id: clipHelper; visible: false }

    function onListPicked(kind, key) {
        if (kind === "sessions") {
            client.switchSession(key)
        } else if (kind === "attach") {
            const err = attachments.addFile(key)
            if (err !== "")
                conversation.addNotice(err)
        } else if (kind === "agents") {
            win.pickAgent(key)
        } else if (kind === "models") {
            win.pickModel(key)
        } else if (kind === "commands") {
            if (key === "directory")
                composer.setText("/directory ")     // needs a path: let the user type it
            else
                win.runCommand("/" + key)
        } else if (kind === "file") {
            const err = attachments.addMention(key, store.sessionDirectory || config.workDir)
            if (err !== "")
                conversation.addNotice(err)
        }
        // "agent": the "@name" text is already in the prompt; the agent is sent with it (mentionedAgentNames).
    }

    Connections {
        target: client
        function onSessionsListed(items) {
            if (items.length === 0) {
                composer.closeList()
                conversation.addNotice(qsTr("No sessions yet."))
            } else {
                composer.openSessions(items, client.sessionId)
            }
        }
        function onSessionsListFailed() {
            if (composer.listKind === "sessions")
                composer.closeList()
        }
        function onRequestFailed(message) { conversation.addNotice(message) }
    }

    // Shift+Tab: switch to the next primary agent (mode). The cycle itself lives in SessionStore. Every press
    // answers with a notice, so a press that cannot switch says why instead of doing nothing.
    function cycleAgent() {
        if (!client.connected) {
            conversation.addNotice(qsTr("Agent mode can be changed once opencode is connected."))
            return
        }
        if (store.busy) {
            conversation.addNotice(qsTr("Wait for the current answer before changing the agent mode."))
            return
        }
        const next = store.cycleAgent()
        console.log("qute-debug: cycleAgent ->", "'" + next + "'", "busy=" + store.busy, "agent=" + store.activeAgent)
        if (next === "")
            conversation.addNotice(catalog.primaryAgents.length === 0 ? qsTr("Agents are still loading.")
                                                                       : qsTr("Only one agent is available."))
        else
            conversation.addNotice(qsTr("Agent: %1").arg(next))
    }

    // Slash commands. Qute's own and opencode's built-ins that Qute can perform run here; "native" ones use the
    // API (for example /compact); "TUI" ones are only in the opencode TUI and say so; project commands ("server")
    // are sent to the session.
    function runCommand(line) {
        const m = /^\/(\S+)\s*(.*)$/.exec(line)
        const typed = m ? m[1] : ""
        const arg = m ? m[2].trim() : ""
        const name = composerTools.canonicalCommand(typed)
        let item = null
        const items = catalog.slashItems
        for (let i = 0; i < items.length; ++i) {
            if (String(items[i].key) === name) {
                item = items[i]
                break
            }
        }
        if (item === null) {
            conversation.addNotice(qsTr("Unknown command: /%1").arg(typed))
            return
        }
        if (name === "directory" || name === "cd") {
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
        } else if (name === "new") {
            // The view is cleared by the session.switched event, so the window shrinks back on its own.
            client.newSession()
        } else if (name === "sessions") {
            // The picker opens at once with a loading row; the list replaces it when the server answers.
            composer.openList("sessions", [{key: "", label: qsTr("Loading sessions…"), sub: "", tag: "", kind: "loading"}],
                              client.sessionId)
            client.listSessions()
        } else if (name === "compact") {
            client.compactSession()
        } else if (name === "openinopencode") {
            const err = controller.openInOpencode(client.sessionId, store.sessionDirectory)
            if (err !== "")
                conversation.addNotice(err)
            else
                controller.hide()
        } else if (name === "agents") {
            if (!client.connected) {
                conversation.addNotice(qsTr("Agent mode can be changed once opencode is connected."))
                return
            }
            composer.openList("agents", win.agentItems(), store.activeAgent)
        } else if (name === "models") {
            composer.openList("models", win.modelItems(), store.activeModelProvider !== "" ? store.activeModelProvider + "/" + store.activeModelId : "")
        } else if (name === "status") {
            conversation.addNotice(win.statusLine())
        } else if (name === "copy") {
            const last = conversation.lastAssistantText()
            if (last === "") {
                conversation.addNotice(qsTr("Nothing to copy yet."))
            } else {
                clipHelper.text = last
                clipHelper.selectAll()
                clipHelper.copy()
                conversation.addNotice(qsTr("Copied the last answer."))
            }
        } else if (name === "settings") {
            controller.runSettings()
        } else if (name === "exit") {
            controller.hide()
        } else if (name === "help") {
            conversation.addNotice(qsTr("Type / to see the commands: /agents, /models, /sessions, /new, /compact, /copy, /status, /directory. /openinopencode opens this session in the opencode TUI. Attach files with +, drag them in, or type @ to mention a file or agent."))
        } else if (item.kind === "server") {
            client.runServerCommand(name, arg, attachments.promptFiles(), attachments.agentRefs(win.mentionedAgentNames(arg)))
            attachments.clear()
        } else {
            conversation.addNotice(qsTr("Unknown command: /%1").arg(typed))
        }
    }

    Component.onCompleted: {
        win.shownLevel = win.baseLevel
        win.height = win.contentHeight + win.dropRoom
        updateInputRegion()
    }
}
