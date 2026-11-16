import QtQuick
import QtQuick.Controls

// One dropdown for the menu-strip pickers (agent / model / effort): it drops down from its control
// and overlays the conversation with a top-to-bottom reveal; Main grows the overlay while open.
Popup {
    id: root
    property var theme
    property real s: 1.0
    property string title: ""
    property var items: []
    property string currentKey: ""
    property bool searchable: true
    property string emptyText: qsTr("Nothing available")
    signal picked(string key)

    // Vertical position is set by the opener (a dropdown dropping below its control).
    x: 0
    y: 0
    width: Math.round(300 * s)
    padding: 0
    modal: false
    focus: true
    // Presses inside the menu strip are not "outside" here: the chip that opened the popup toggles it,
    // and the other chips switch to their own dropdown (see MenuStrip.openPicker).
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
    opacity: 0
    // Reveal drops down from the top edge of the control.
    transformOrigin: Item.Top
    scale: 0.92
    Behavior on scale { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }

    // Max list pixel height, set by the opener so the whole dropdown fits inside the window
    // instead of being clipped at its bottom edge. Falls back to a sensible cap.
    property real listMaxHeight: Math.round(260 * s)

    readonly property var filtered: {
        const q = search.text.toLowerCase().trim()
        if (q === "")
            return items
        const out = []
        for (let i = 0; i < items.length; ++i) {
            const item = items[i]
            if (String(item.label || "").toLowerCase().indexOf(q) >= 0
                    || String(item.sub || "").toLowerCase().indexOf(q) >= 0
                    || String(item.key || "").toLowerCase().indexOf(q) >= 0)
                out.push(item)
        }
        return out
    }

    function accept(item) {
        if (!item)
            return
        picked(item.key)
        close()
    }

    onOpened: {
        opacity = 1
        scale = 1
        if (searchable)
            search.forceActiveFocus()
        else
            list.forceActiveFocus()
    }
    onClosed: {
        opacity = 0
        scale = 0.92
        search.text = ""
    }
    Behavior on opacity { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }

    background: Rectangle {
        color: root.theme.input
        radius: Math.round(10 * root.s)
        border.width: 1
        border.color: root.theme.border
    }

    contentItem: Column {
        id: content
        width: root.width
        padding: Math.round(6 * root.s)
        topPadding: Math.round(8 * root.s)
        bottomPadding: Math.round(7 * root.s)
        spacing: Math.round(5 * root.s)

        Row {
            width: content.width - content.leftPadding - content.rightPadding
            spacing: Math.round(6 * root.s)

            Label {
                text: root.title
                color: root.theme.textDim
                font.pixelSize: Math.round(9 * root.s)
                font.letterSpacing: Math.round(0.6 * root.s)
                elide: Text.ElideRight
                width: parent.width - hint.width - parent.spacing
            }
            Label {
                id: hint
                text: qsTr("esc")
                color: root.theme.textDim
                opacity: 0.6
                font.pixelSize: Math.round(8 * root.s)
            }
        }

        TextField {
            id: search
            visible: root.searchable && root.items.length > 8
            width: content.width - content.leftPadding - content.rightPadding
            implicitHeight: Math.round(26 * root.s)
            placeholderText: qsTr("Search…")
            color: root.theme.text
            placeholderTextColor: root.theme.textDim
            selectionColor: root.theme.accent
            font.pixelSize: Math.round(11 * root.s)
            leftPadding: Math.round(8 * root.s)
            rightPadding: Math.round(8 * root.s)
            background: Rectangle {
                radius: Math.round(7 * root.s)
                color: Qt.rgba(root.theme.text.r, root.theme.text.g, root.theme.text.b, 0.05)
                border.width: 1
                border.color: search.activeFocus ? root.theme.accent : "transparent"
            }
            onAccepted: root.accept(root.filtered.length > 0 ? root.filtered[0] : null)
            Keys.onDownPressed: list.incrementCurrentIndex()
            Keys.onUpPressed: list.decrementCurrentIndex()
        }

        ListView {
            id: list
            width: content.width - content.leftPadding - content.rightPadding
            visible: root.filtered.length > 0
            height: visible ? Math.min(contentHeight, Math.max(Math.round(96 * root.s), root.listMaxHeight)) : 0
            clip: true
            model: root.filtered
            currentIndex: -1
            boundsBehavior: ListView.StopAtBounds
            spacing: 1
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: Rectangle {
                id: row
                required property var modelData
                required property int index

                width: ListView.view.width
                height: Math.round(32 * root.s)
                radius: Math.round(6 * root.s)
                color: modelData.key === root.currentKey
                       ? Qt.rgba(root.theme.accent.r, root.theme.accent.g, root.theme.accent.b, 0.13)
                       : hover.hovered || ListView.isCurrentItem
                         ? Qt.rgba(root.theme.text.r, root.theme.text.g, root.theme.text.b, 0.07)
                         : "transparent"

                Column {
                    anchors.left: parent.left
                    anchors.leftMargin: Math.round(8 * root.s)
                    anchors.right: check.left
                    anchors.rightMargin: Math.round(6 * root.s)
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 0

                    Label {
                        width: parent.width
                        text: row.modelData.label
                        color: root.theme.text
                        font.pixelSize: Math.round(11 * root.s)
                        elide: Text.ElideRight
                    }
                    Label {
                        width: parent.width
                        visible: text !== ""
                        text: row.modelData.sub !== undefined ? row.modelData.sub : ""
                        color: root.theme.textDim
                        font.pixelSize: Math.round(8.5 * root.s)
                        elide: Text.ElideRight
                    }
                }
                Label {
                    id: check
                    anchors.right: parent.right
                    anchors.rightMargin: Math.round(9 * root.s)
                    anchors.verticalCenter: parent.verticalCenter
                    visible: row.modelData.key === root.currentKey
                    text: "✓"
                    color: root.theme.accent
                    font.pixelSize: Math.round(11 * root.s)
                    font.bold: true
                }

                HoverHandler { id: hover; cursorShape: Qt.PointingHandCursor }
                TapHandler { onTapped: root.accept(row.modelData) }
            }
        }

        Label {
            width: content.width - content.leftPadding - content.rightPadding
            visible: root.filtered.length === 0
            text: root.emptyText
            color: root.theme.textDim
            font.pixelSize: Math.round(10 * root.s)
            horizontalAlignment: Text.AlignHCenter
            topPadding: Math.round(8 * root.s)
            bottomPadding: Math.round(8 * root.s)
        }
    }
}
