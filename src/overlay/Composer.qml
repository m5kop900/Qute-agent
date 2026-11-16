import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// The Ask Qute composer: one rounded input surface that owns the prompt plus the agent mode,
// model, reasoning effort and context usage controls. Everything is bound to real state:
// sessionStore.activeAgent / sessionStore.activeModel* and modelCatalog.models/primaryAgents (server data).
Item {
    id: root
    property var theme
    property var sessionStore
    property var modelCatalog
    property real s: 1.0
    property bool enabled: true
    property string offText: qsTr("Connecting to opencode…")
    signal submitted(string text)

    readonly property bool popupOpen: modePopup.visible || modelPopup.visible || effortPopup.visible
    readonly property bool hasText: input.text.trim().length > 0
    readonly property bool engaged: input.activeFocus || popupOpen
    readonly property var primaryAgents: modelCatalog ? modelCatalog.primaryAgents : []
    readonly property var models: modelCatalog ? modelCatalog.models : []
    readonly property string agentId: (sessionStore && sessionStore.activeAgent !== "") ? sessionStore.activeAgent : "build"
    readonly property var currentAgent: {
        const count = modelCatalog ? modelCatalog.agents.length : 0   // re-evaluate when agents arrive
        return (count > 0 && modelCatalog && modelCatalog.agentById) ? modelCatalog.agentById(agentId) : ({})
    }
    readonly property string agentName: currentAgent.name ? currentAgent.name : titleCase(agentId)
    readonly property var currentModel: {
        const count = models.length                                    // re-evaluate when models arrive
        if (count === 0 || !sessionStore || sessionStore.activeModelId === "" || !modelCatalog)
            return ({})
        return modelCatalog.modelByRef(sessionStore.activeModelProvider, sessionStore.activeModelId)
    }
    readonly property string modelName: currentModel.name ? currentModel.name
                                        : (sessionStore && sessionStore.activeModelId !== "" ? sessionStore.activeModelId : qsTr("Model"))
    readonly property var variants: currentModel.variants ? currentModel.variants : []
    readonly property bool effortVisible: sessionStore && sessionStore.activeModelId !== ""
                                         && (variants.length > 0 || sessionStore.activeVariant !== "")
    readonly property string effortName: {
        if (!sessionStore || sessionStore.activeVariant === "")
            return qsTr("Auto")
        for (let i = 0; i < variants.length; ++i)
            if (variants[i].id === sessionStore.activeVariant)
                return variants[i].label
        return titleCase(sessionStore.activeVariant)
    }

    implicitHeight: Math.round(78 * s)

    readonly property var agentItems: {
        const out = []
        for (let i = 0; i < primaryAgents.length; ++i) {
            const a = primaryAgents[i]
            out.push({key: a.id, label: a.name, sub: a.description || ""})
        }
        return out
    }
    readonly property var modelItems: {
        const out = []
        for (let i = 0; i < models.length; ++i) {
            const m = models[i]
            out.push({key: m.providerID + "/" + m.modelID, label: m.name, sub: m.providerID,
                      providerID: m.providerID, modelID: m.modelID, variants: m.variants || []})
        }
        return out
    }
    readonly property var effortItems: {
        const out = [{key: "", label: qsTr("Auto"), sub: qsTr("Provider default")}]
        for (let i = 0; i < variants.length; ++i)
            out.push({key: variants[i].id, label: variants[i].label, sub: ""})
        return out
    }

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

    function openPicker(popup, chip) {
        popup.x = Math.max(Math.round(2 * s),
                           Math.min(root.width - popup.width - Math.round(2 * s),
                                    chip.mapToItem(root, 0, 0).x))
        popup.open()
    }

    function pickModel(key) {
        if (!root.sessionStore)
            return
        for (let i = 0; i < models.length; ++i) {
            const m = models[i]
            if ((m.providerID + "/" + m.modelID) !== key)
                continue
            // Keep the effort if the new model exposes the same variant, otherwise use its default.
            let variant = ""
            const wanted = root.sessionStore.activeVariant
            const list = m.variants || []
            for (let j = 0; j < list.length; ++j)
                if (list[j].id === wanted)
                    variant = wanted
            root.sessionStore.selectModel(m.providerID, m.modelID, variant)
            return
        }
    }

    function pickVariant(key) {
        if (!root.sessionStore || root.sessionStore.activeModelId === "")
            return
        root.sessionStore.selectModel(root.sessionStore.activeModelProvider, root.sessionStore.activeModelId, key)
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
            spacing: Math.round(1 * s)

            ControlChip {
                id: modeChip
                theme: root.theme
                s: root.s
                text: root.agentName
                strong: true
                dot: true
                maxTextWidth: Math.round(74 * root.s)
                interactive: root.enabled && root.agentItems.length > 0
                onClicked: root.openPicker(modePopup, modeChip)

                ToolTip.visible: modeChip.hovered
                ToolTip.delay: 350
                ToolTip.text: qsTr("Agent mode · %1").arg(root.agentId)
            }

            ControlChip {
                id: modelChip
                theme: root.theme
                s: root.s
                text: root.modelName
                maxTextWidth: Math.round(150 * root.s)
                interactive: root.enabled && root.modelItems.length > 0
                onClicked: root.openPicker(modelPopup, modelChip)

                ToolTip.visible: modelChip.hovered
                ToolTip.delay: 350
                ToolTip.text: root.sessionStore && root.sessionStore.activeModelId !== ""
                              ? qsTr("Model · %1").arg(root.sessionStore.activeModel)
                              : qsTr("Choose a model")
            }

            ControlChip {
                id: effortChip
                theme: root.theme
                s: root.s
                visible: root.effortVisible
                text: root.effortName
                maxTextWidth: Math.round(64 * root.s)
                interactive: root.enabled && root.effortItems.length > 1
                onClicked: root.openPicker(effortPopup, effortChip)

                ToolTip.visible: effortChip.hovered
                ToolTip.delay: 350
                ToolTip.text: qsTr("Reasoning effort · %1").arg(root.sessionStore ? root.sessionStore.activeVariant || qsTr("auto") : "")
            }

            Item { Layout.fillWidth: true }

            ContextRing {
                Layout.preferredWidth: Math.round(22 * root.s)
                Layout.preferredHeight: Math.round(22 * root.s)
                theme: root.theme
                s: root.s
                percent: root.sessionStore ? root.sessionStore.contextPercent : -1
                tokens: root.sessionStore ? root.sessionStore.contextTokens : 0
                limit: root.sessionStore ? root.sessionStore.contextLimit : 0
            }

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

                ToolTip.visible: sendHover.hovered
                ToolTip.delay: 400
                ToolTip.text: qsTr("Send · Enter")
            }
        }
    }

    PickerPopup {
        id: modePopup
        parent: root
        theme: root.theme
        s: root.s
        title: qsTr("AGENT MODE")
        items: root.agentItems
        currentKey: root.agentId
        searchable: false
        emptyText: qsTr("The server exposes no primary agent")
        onPicked: function (key) { if (root.sessionStore) root.sessionStore.selectAgent(key) }
    }

    PickerPopup {
        id: modelPopup
        parent: root
        theme: root.theme
        s: root.s
        title: qsTr("MODEL")
        items: root.modelItems
        currentKey: root.sessionStore ? root.sessionStore.activeModelProvider + "/" + root.sessionStore.activeModelId : ""
        emptyText: qsTr("No models reported by opencode")
        onPicked: function (key) { root.pickModel(key) }
    }

    PickerPopup {
        id: effortPopup
        parent: root
        theme: root.theme
        s: root.s
        title: qsTr("REASONING EFFORT")
        items: root.effortItems
        currentKey: root.sessionStore ? root.sessionStore.activeVariant : ""
        searchable: false
        onPicked: function (key) { root.pickVariant(key) }
    }
}
