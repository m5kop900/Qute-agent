import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// The Ask Qute prompt: a single rounded input surface plus the attach and send buttons. The agent-mode,
// model and reasoning-effort controls live in the top menu strip (MenuStrip), not here.
//
// Lists above the box (InlineList, drawn in a Popup that never takes focus):
//   "commands" - "/" at the start: Qute's own commands, opencode's built-ins and the project's commands
//   "mentions" - "@" in the text: files (from GET /api/fs/find) and agents
//   "sessions" - /sessions: the session list
//   "agents"   - /agents: the primary agents (build, plan, ...)
//   "models"   - /models: the models the server offers
//   "files"    - the "+" button: an in-app file browser (folders, and only the file types Qute can send)
Item {
    id: root
    property var theme
    property real s: 1.0
    property string offText: qsTr("Connecting to opencode…")
    signal submitted(string text)
    signal command(string text)      // a line starting with "/" (slash command)
    signal modeCycleRequested()      // Shift+Tab: next agent mode (build <-> plan)
    signal mentionQueryChanged(string query)
    signal listPicked(string kind, string key)

    property var tools                       // composerTools (filtering and "@" detection in C++)
    property alias inputText: input.text     // the text in the box (also used by the UI tests)
    property var commandItems: []            // [{key, label, sub, tag, kind, aliases}] from catalog.slashItems
    property var mentionItems: []            // [{key, label, sub, tag, kind}] files and agents, filled by Main
    property var sessionItems: []
    property var pickerItems: []             // items of the /agents, /models and file lists
    property var files: null                 // the attachments model (listDirectory, defaultDirectory, addFile)
    property string browsePath: ""           // folder shown by the file browser
    property string currentSessionKey: ""
    property string listKind: ""
    property int listIndex: 0
    property bool commandsDismissed: false   // Esc hides the "/" list until the text changes
    property string dismissedText: ""
    property real listMaxHeight: 160
    property int mentionStart: -1
    property int mentionEnd: -1

    readonly property string typedCommand: input.text.charAt(0) === "/" ? input.text.slice(1) : ""
    // Lists that pick one item and send the choice to Main (no typing into the box).
    readonly property bool pickerOpen: listKind === "sessions" || listKind === "agents" || listKind === "models"
                                       || listKind === "files"
    readonly property var visibleItems: {
        if (listKind === "sessions")
            return sessionItems
        if (listKind === "agents" || listKind === "models" || listKind === "files")
            return pickerItems
        if (listKind === "commands")
            return tools ? tools.filterSlash(commandItems, typedCommand) : []
        if (listKind === "mentions")
            return mentionItems
        return []
    }
    readonly property bool listOpen: listKind !== "" && visibleItems.length > 0

    function refreshText() {
        const t = input.text
        if (commandsDismissed && t !== dismissedText)
            commandsDismissed = false
        if (t.charAt(0) === "/" && t.indexOf(" ") < 0 && t.indexOf("\n") < 0) {
            if (!commandsDismissed)
                listKind = "commands"
            mentionStart = -1
        } else {
            commandsDismissed = false
            if (listKind === "commands")
                listKind = ""
            updateMention()
        }
        listIndex = 0
    }

    // The "@" token under the cursor opens the mention list; no token closes it.
    function updateMention() {
        if (!tools)
            return
        const tok = tools.mentionAt(input.text, input.cursorPosition)
        if (tok.active) {
            mentionStart = tok.start
            mentionEnd = tok.end
            if (listKind !== "mentions" || listIndex < 0)
                listIndex = 0
            listKind = "mentions"
            mentionQueryChanged(tok.query)
        } else if (listKind === "mentions") {
            listKind = ""
        }
    }

    function openList(kind, items, currentKey) {
        if (kind === "sessions")
            sessionItems = items
        else
            pickerItems = items
        currentSessionKey = currentKey
        listKind = kind
        listIndex = 0
    }

    function openSessions(items, currentKey) {
        openList("sessions", items, currentKey)
    }

    function closeList() {
        if (listKind === "commands") {
            commandsDismissed = true
            dismissedText = input.text
        }
        listKind = ""
    }

    function setCaret(pos) {
        input.cursorPosition = pos
    }

    function setText(text) {
        input.text = text
        input.cursorPosition = text.length
        input.forceActiveFocus()
    }

    function pickIndex(i) {
        if (i < 0 || i >= visibleItems.length)
            return
        const item = visibleItems[i]
        const kind = listKind
        // Placeholder rows ("Loading…", "No supported files here") are not choices.
        if (String(item.kind) === "loading" || String(item.kind) === "none")
            return
        if (kind === "files") {
            if (String(item.kind) === "dir") {
                browseTo(String(item.key))
            } else {
                // The list stays open, so several files can be picked one after another (Esc closes it).
                listPicked("attach", String(item.key))
            }
        } else if (kind === "commands") {
            input.text = ""          // the command runs; Main decides what happens next
            listKind = ""
            commandsDismissed = false
            listPicked("commands", String(item.key))
        } else if (kind === "mentions") {
            // Replace the "@token" with "@key " and let Main attach files (agents are sent by name with the prompt).
            const before = input.text.slice(0, mentionStart)
            const after = input.text.slice(mentionEnd)
            const key = String(item.key)
            input.text = before + "@" + key + " " + after
            input.cursorPosition = before.length + key.length + 2
            listKind = ""
            listPicked(String(item.kind), key)
        } else {
            listKind = ""
            listPicked(kind, String(item.key))
        }
    }

    function pickKey(key) {
        for (let i = 0; i < visibleItems.length; ++i)
            if (String(visibleItems[i].key) === key) {
                pickIndex(i)
                return
            }
    }

    function moveHighlight(delta) {
        const n = visibleItems.length
        if (n === 0)
            return
        listIndex = (listIndex + delta + n) % n
    }

    // The "+" button: a browser inside the overlay. It lists folders and the file types Qute can send.
    function browseTo(dir) {
        browsePath = dir
        pickerItems = files ? files.listDirectory(dir) : []
        currentSessionKey = ""
        listKind = "files"
        listIndex = 0
    }

    function openFilePicker() {
        if (!root.enabled || !files)
            return
        browseTo(files.defaultDirectory())
        input.forceActiveFocus()
    }

    // Files dropped on the box become attachments (F1.1).
    function attachUrls(urls) {
        for (let i = 0; i < urls.length; ++i) {
            const err = attachments.addFile(String(urls[i]))
            if (err !== "")
                conversation.addNotice(err)
        }
    }

    // Geometry of the open list, for the window's click mask (Main): the list hangs below the prompt box.
    readonly property real listTop: box.height + Math.round(6 * s)
    readonly property real listPixelHeight: listOpen ? list.height : 0

    readonly property bool hasText: input.text.trim().length > 0
    readonly property bool engaged: input.activeFocus

    implicitHeight: Math.round(78 * s)

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
        if (text.charAt(0) === "/")
            root.command(text)
        else
            root.submitted(text)
    }

    // The list opens BELOW the prompt box, in the window's drop room, like the MenuStrip menus.
    Popup {
        id: listPopup
        parent: root
        x: 0
        y: box.height + Math.round(6 * root.s)
        width: root.width
        padding: 0
        modal: false
        focus: false
        closePolicy: Popup.NoAutoClose
        background: Item {}
        enter: Transition {}
        exit: Transition {}
        visible: root.listOpen
        InlineList {
            id: list
            width: parent.width
            theme: root.theme
            s: root.s
            items: root.visibleItems
            index: root.listIndex
            currentKey: root.pickerOpen ? root.currentSessionKey : ""
            maxHeight: root.listMaxHeight
            onPicked: function (key) { root.pickKey(key) }
        }
    }

    Rectangle {
        id: box
        anchors.fill: parent
        radius: Math.round(12 * s * config.radiusScale)
        color: root.theme.input
        border.width: 1
        border.color: root.engaged ? root.theme.accent
                                   : hover.hovered && root.enabled ? Qt.rgba(root.theme.text.r, root.theme.text.g, root.theme.text.b, 0.28)
                                                                  : root.theme.border
        Behavior on border.color { ColorAnimation { duration: 130 } }

        HoverHandler { id: hover }

        // Drop files from the file manager anywhere on the box.
        DropArea {
            anchors.fill: parent
            onDropped: function (drop) { root.attachUrls(drop.urls) }
        }
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

            onTextChanged: root.refreshText()
            onCursorPositionChanged: if (root.listKind !== "commands" && !root.pickerOpen) root.updateMention()

            Keys.onPressed: function (event) {
                if (event.key === Qt.Key_Backtab || event.key === Qt.Key_Tab)
                    console.log("qute-debug: prompt box got key", event.key, "modifiers", event.modifiers)
                if (root.listOpen) {
                    if (event.key === Qt.Key_Down) {
                        root.moveHighlight(1)
                        event.accepted = true
                        return
                    }
                    if (event.key === Qt.Key_Up) {
                        root.moveHighlight(-1)
                        event.accepted = true
                        return
                    }
                    if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Tab) {
                        root.pickIndex(root.listIndex)
                        event.accepted = true
                        return
                    }
                }
                if (event.key === Qt.Key_Backtab || (event.key === Qt.Key_Tab && (event.modifiers & Qt.ShiftModifier))) {
                    root.modeCycleRequested()
                    event.accepted = true
                    return
                }
                // Ctrl+V with an image and no text in the clipboard: attach the image (F1.2). Text paste is untouched.
                if (event.key === Qt.Key_V && (event.modifiers & Qt.ControlModifier)
                        && attachments.clipboardHasImage() && !attachments.clipboardHasText()) {
                    const err = attachments.pasteClipboardImage()
                    if (err !== "")
                        conversation.addNotice(err)
                    event.accepted = true
                    return
                }
                if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                    if (event.modifiers & Qt.ShiftModifier)
                        return
                    root.submit()
                    event.accepted = true
                }
            }
            Keys.onEscapePressed: {
                if (root.listKind !== "")
                    root.closeList()
                else
                    controller.hide()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.round(22 * s)
            spacing: Math.round(6 * s)

            // Attach files (F1.1): opens the file dialog; drag & drop works too.
            Rectangle {
                id: attachButton
                Layout.preferredWidth: Math.round(22 * root.s)
                Layout.preferredHeight: Math.round(22 * root.s)
                radius: width / 2
                color: "transparent"
                border.width: 1
                border.color: root.theme.border
                enabled: root.enabled

                Label {
                    anchors.centerIn: parent
                    text: "+"
                    color: attachButton.enabled ? root.theme.textDim : root.theme.border
                    font.pixelSize: Math.round(13 * root.s)
                    font.bold: true
                }
                HoverHandler { cursorShape: attachButton.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor }
                TapHandler {
                    enabled: attachButton.enabled
                    onTapped: root.openFilePicker()
                }
            }

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
