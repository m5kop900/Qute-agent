import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Top menu strip of the chat card. It owns the agent-mode / model / reasoning-effort
// controls plus the context ring. Every control is bound to live server state:
// sessionStore.activeAgent / activeModel* / activeVariant and modelCatalog.models/agents.
// Clicking a control drops a PickerPopup that overlays the conversation and opens with a
// top-to-bottom reveal (see PickerPopup), instead of expanding the whole card.
Item {
    id: root
    property var theme
    property real s: 1.0
    property var sessionStore
    property var modelCatalog
    signal settingChanged()

    readonly property bool popupOpen: modePopup.visible || modelPopup.visible || effortPopup.visible
    // Bottom edge of the open dropdown in this strip's coordinates; Main uses it to size the window.
    readonly property var openPopup: modePopup.visible ? modePopup
                                   : modelPopup.visible ? modelPopup
                                   : effortPopup.visible ? effortPopup : null
    readonly property real openPopupTop: openPopup ? openPopup.y : 0
    readonly property real openPopupBottom: openPopup ? openPopup.y + openPopup.height : 0

    // --- live selection state ---
    readonly property bool hasModel: sessionStore && sessionStore.activeModelId !== ""
    readonly property string agentId: (sessionStore && sessionStore.activeAgent !== "") ? sessionStore.activeAgent : "build"
    readonly property var currentAgent: {
        const count = modelCatalog ? modelCatalog.agents.length : 0   // re-evaluate when agents arrive
        return (count > 0 && modelCatalog && modelCatalog.agentById) ? modelCatalog.agentById(agentId) : ({})
    }
    readonly property string agentName: currentAgent.name ? currentAgent.name : titleCase(agentId)
    readonly property var currentModel: {
        const count = modelCatalog ? modelCatalog.models.length : 0   // re-evaluate when models arrive
        if (count === 0 || !hasModel || !modelCatalog)
            return ({})
        return modelCatalog.modelByRef(sessionStore.activeModelProvider, sessionStore.activeModelId)
    }
    readonly property string modelName: currentModel.name ? currentModel.name
                                        : (hasModel ? sessionStore.activeModelId : qsTr("Model"))
    readonly property var variants: currentModel.variants ? currentModel.variants : []
    readonly property bool effortVisible: hasModel && (variants.length > 0 || sessionStore.activeVariant !== "")
    readonly property string effortName: {
        if (!sessionStore || sessionStore.activeVariant === "")
            return qsTr("Auto")
        for (let i = 0; i < variants.length; ++i)
            if (variants[i].id === sessionStore.activeVariant)
                return variants[i].label
        return titleCase(sessionStore.activeVariant)
    }

    readonly property var primaryAgents: modelCatalog ? modelCatalog.primaryAgents : []
    readonly property var models: modelCatalog ? modelCatalog.models : []

    readonly property var agentItems: {
        const out = []
        for (let i = 0; i < primaryAgents.length; ++i) {
            const a = primaryAgents[i]
            out.push({key: a.id, label: a.name, sub: a.description || ""})
        }
        return out
    }
    // Only enabled models, grouped by provider (F2.2). Models the server marks disabled are not offered.
    readonly property var modelItems: {
        const list = []
        for (let i = 0; i < models.length; ++i) {
            const m = models[i]
            if (m.enabled === false)
                continue
            list.push({key: m.providerID + "/" + m.modelID, label: m.name, group: m.providerID,
                       providerID: m.providerID, modelID: m.modelID, variants: m.variants || [], sub: ""})
        }
        list.sort(function (a, b) {
            if (a.group !== b.group)
                return a.group < b.group ? -1 : 1
            return a.label < b.label ? -1 : (a.label > b.label ? 1 : 0)
        })
        return list
    }
    readonly property var effortItems: {
        const out = [{key: "", label: qsTr("Auto"), sub: qsTr("Provider default")}]
        for (let i = 0; i < variants.length; ++i)
            out.push({key: variants[i].id, label: variants[i].label, sub: ""})
        return out
    }

    implicitHeight: Math.round(34 * s)

    function titleCase(value) {
        return value.length > 0 ? value.charAt(0).toUpperCase() + value.slice(1) : value
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

    function pickAgent(key) {
        if (!root.sessionStore)
            return
        root.sessionStore.selectAgent(key)
    }

    function closeAllPickers() {
        modePopup.close()
        modelPopup.close()
        effortPopup.close()
    }

    // A chip click opens its dropdown, or closes it when it is already the open one.
    function togglePicker(popup, chip) {
        if (popup.visible) {
            popup.close()
            return
        }
        closeAllPickers()
        openPicker(popup, chip)
    }

    function openPicker(popup, chip) {
        const p = chip.mapToItem(root, 0, 0)
        popup.x = Math.max(Math.round(2 * s),
                           Math.min(p.x, root.width - popup.width - Math.round(2 * s)))
        // The window is a fixed size and clips anything outside it, so the dropdown opens toward the side
        // that has more room (below the strip normally; above it when the strip sits low in the window).
        const gap = Math.round(3 * s)
        const margin = Math.round(10 * s)
        const chrome = Math.round(90 * s)        // title, search field, paddings: everything but the list
        const minList = Math.round(96 * s)
        const stripY = root.mapToItem(root.Window.contentItem, 0, 0).y
        const belowEdge = stripY + p.y + chip.height - gap
        const aboveEdge = stripY + p.y + gap
        const roomBelow = root.Window.height - belowEdge - margin
        const roomAbove = aboveEdge - margin
        const opensAbove = roomBelow < minList + chrome && roomAbove > roomBelow
        popup.opensAbove = opensAbove
        popup.anchorEdge = opensAbove ? p.y + gap : p.y + chip.height - gap
        popup.scale = 0.92
        popup.opacity = 0
        // Long lists scroll inside whatever room there is; the list never grows past the window.
        popup.listMaxHeight = Math.max(minList, Math.floor((opensAbove ? roomAbove : roomBelow) - chrome))
        popup.open()
    }

    Rectangle {
        id: strip
        anchors.fill: parent
        radius: Math.round(9 * s * config.radiusScale)
        color: root.theme.input
        border.width: 1
        border.color: root.enabled ? root.theme.border : Qt.rgba(root.theme.text.r, root.theme.text.g, root.theme.text.b, 0.18)
        Behavior on border.color { ColorAnimation { duration: 130 } }

        RowLayout {
            anchors.fill: parent
            anchors.margins: Math.round(6 * s)
            spacing: Math.round(6 * s)

            ControlChip {
                id: modeChip
                theme: root.theme
                s: root.s
                text: root.agentName
                strong: true
                dot: true
                maxTextWidth: Math.round(74 * root.s)
                interactive: root.enabled && root.agentItems.length > 0
                onClicked: root.togglePicker(modePopup, modeChip)
            }

            ControlChip {
                id: modelChip
                theme: root.theme
                s: root.s
                text: root.modelName
                maxTextWidth: Math.round(160 * root.s)
                interactive: root.enabled && root.modelItems.length > 0
                onClicked: root.togglePicker(modelPopup, modelChip)
            }

            ControlChip {
                id: effortChip
                theme: root.theme
                s: root.s
                visible: root.effortVisible
                text: root.effortName
                maxTextWidth: Math.round(64 * root.s)
                interactive: root.enabled && root.effortItems.length > 1
                onClicked: root.togglePicker(effortPopup, effortChip)
            }

            Item { Layout.fillWidth: true }

            ContextRing {
                Layout.preferredWidth: Math.round(20 * root.s)
                Layout.preferredHeight: Math.round(20 * root.s)
                theme: root.theme
                s: root.s
                percent: root.sessionStore ? root.sessionStore.contextPercent : -1
                tokens: root.sessionStore ? root.sessionStore.contextTokens : 0
                limit: root.sessionStore ? root.sessionStore.contextLimit : 0
                cost: (root.sessionStore && root.sessionStore.usage.cost !== undefined) ? root.sessionStore.usage.cost : -1
            }
        }
    }

    Hint {
        target: modeChip
        theme: root.theme
        s: root.s
        shown: modeChip.hovered && !modePopup.visible
        text: qsTr("Agent mode · %1").arg(root.agentId)
    }
    Hint {
        target: modelChip
        theme: root.theme
        s: root.s
        shown: modelChip.hovered && !modelPopup.visible
        text: root.hasModel ? qsTr("Model · %1").arg(root.sessionStore.activeModel) : qsTr("Choose a model")
    }
    Hint {
        target: effortChip
        theme: root.theme
        s: root.s
        shown: effortChip.hovered && !effortPopup.visible
        text: qsTr("Reasoning effort · %1").arg(root.sessionStore ? root.sessionStore.activeVariant || qsTr("auto") : "")
    }

    // Pickers — dropdowns that drop from this strip and overlay the conversation.
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
        onPicked: function (key) { root.pickAgent(key) }
    }

    PickerPopup {
        id: modelPopup
        parent: root
        theme: root.theme
        s: root.s
        title: qsTr("MODEL")
        items: root.modelItems
        grouped: true
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
