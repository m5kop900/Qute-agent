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
    property string offText: qsTr("Connecting to opencode…")
    signal submitted(string text)
    signal command(string text)      // a line starting with "/" (local slash command)
    signal modeCycleRequested()      // Shift+Tab: next agent mode (build <-> plan)

    // Lists above the box: "/" suggestions and the session list (/session). Main fills them.
    property var commandItems: []            // [{key, label, sub}] local + server slash commands
    property var sessionItems: []            // [{key, label, sub}] from client.listSessions()
    property string currentSessionKey: ""
    property string listKind: ""             // "" | "commands" | "sessions"
    property int listIndex: 0
    property bool commandsDismissed: false   // Esc hides the suggestions until the text changes
    property real listMaxHeight: 160
    signal listPicked(string kind, string key)

    readonly property string typedCommand: input.text.charAt(0) === "/" ? input.text.slice(1) : ""
    readonly property var visibleItems: {
        if (listKind === "sessions")
            return sessionItems
        if (listKind !== "commands")
            return []
        const q = typedCommand.toLowerCase()
        const out = []
        for (let i = 0; i < commandItems.length; ++i) {
            const item = commandItems[i]
            if (String(item.key).toLowerCase().indexOf(q) === 0)
                out.push(item)
        }
        return out
    }
    readonly property bool listOpen: listKind !== "" && visibleItems.length > 0

    function refreshCommands() {
        // A "/" at the start opens the command list until a space is typed or Esc dismisses it.
        const t = input.text
        if (t.charAt(0) === "/" && t.indexOf(" ") < 0 && t.indexOf("\n") < 0) {
            if (!commandsDismissed)
                listKind = "commands"
        } else {
            commandsDismissed = false
            if (listKind === "commands")
                listKind = ""
        }
        listIndex = 0
    }

    function openSessions(items, currentKey) {
        sessionItems = items
        currentSessionKey = currentKey
        listKind = "sessions"
        listIndex = 0
    }

    function closeList() {
        if (listKind === "commands")
            commandsDismissed = true
        listKind = ""
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
        if (kind === "commands")
            input.text = ""           // the command runs; Main decides what happens next
        listKind = ""
        commandsDismissed = false
        listPicked(kind, String(item.key))
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
        if (text.charAt(0) === "/")
            root.command(text)
        else
            root.submitted(text)
    }

    // The list opens BELOW the prompt box, in the window's drop room, like the MenuStrip menus. It is a Popup that
    // never takes keyboard focus, so the prompt keeps the keys and the window does not resize.
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
            currentKey: root.listKind === "sessions" ? root.currentSessionKey : ""
            maxHeight: root.listMaxHeight
            onPicked: function (key) { root.pickKey(key) }
        }
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

            onTextChanged: root.refreshCommands()

            Keys.onPressed: function (event) {
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
