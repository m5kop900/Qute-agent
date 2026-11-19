import QtQuick
import QtQuick.Controls

// A small list that sits above the prompt box: slash-command suggestions and the session list.
// It never takes keyboard focus. The prompt box keeps the keys and moves the highlight through `index`;
// a click picks an entry directly.
Item {
    id: root
    property var theme
    property real s: 1.0
    property var items: []
    property int index: 0
    property string currentKey: ""
    property real maxHeight: Math.round(160 * s)
    signal picked(string key)

    // True only while the mouse is choosing the row. Hover moves the highlight without scrolling: scrolling
    // moved the rows under the pointer, which picked another row, which scrolled again (the highlight and the
    // row that Enter picks drifted apart). Keyboard moves still scroll the highlighted row into view.
    property bool hoverDriven: false
    onIndexChanged: {
        if (!hoverDriven && index >= 0 && index < items.length)
            list.positionViewAtIndex(index, ListView.Contain)
    }

    readonly property real pad: Math.round(6 * s)
    readonly property real rowHeight: Math.round(34 * s)
    implicitHeight: Math.min(maxHeight, items.length * rowHeight) + 2 * pad
    visible: items.length > 0

    Rectangle {
        anchors.fill: parent
        radius: Math.round(10 * root.s)
        color: root.theme.input
        border.width: 1
        border.color: root.theme.border
    }

    ListView {
        id: list
        anchors.fill: parent
        anchors.margins: root.pad
        clip: true
        model: root.items
        boundsBehavior: ListView.StopAtBounds
        currentIndex: root.index
        highlightFollowsCurrentItem: false
        highlightMoveDuration: 0
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        delegate: Rectangle {
            id: row
            required property var modelData
            required property int index

            width: ListView.view.width
            height: root.rowHeight
            radius: Math.round(6 * root.s)
            color: row.index === root.index
                   ? Qt.rgba(root.theme.accent.r, root.theme.accent.g, root.theme.accent.b, 0.14)
                   : "transparent"

            Column {
                anchors.left: parent.left
                anchors.leftMargin: Math.round(8 * root.s)
                anchors.right: parent.right
                anchors.rightMargin: Math.round(72 * root.s)
                anchors.verticalCenter: parent.verticalCenter

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
                    text: row.modelData.sub !== undefined ? String(row.modelData.sub) : ""
                    color: root.theme.textDim
                    font.pixelSize: Math.round(8.5 * root.s)
                    elide: Text.ElideRight
                }
            }
            // Small tag: "QUTE" = Qute's own command, "TUI" = runs only in the opencode TUI, "AGENT" = agent.
            Label {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: Math.round(24 * root.s)
                visible: text !== ""
                text: row.modelData.tag !== undefined ? String(row.modelData.tag) : ""
                color: root.theme.textDim
                font.pixelSize: Math.round(8 * root.s)
                font.bold: true
                font.letterSpacing: Math.round(0.6 * root.s)
            }
            Label {
                id: check
                anchors.right: parent.right
                anchors.rightMargin: Math.round(9 * root.s)
                anchors.verticalCenter: parent.verticalCenter
                visible: root.currentKey !== "" && row.modelData.key === root.currentKey
                text: "✓"
                color: root.theme.accent
                font.pixelSize: Math.round(11 * root.s)
                font.bold: true
            }
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true
                onEntered: {
                    root.hoverDriven = true
                    root.index = row.index
                    root.hoverDriven = false
                }
                onClicked: root.picked(String(row.modelData.key))
            }
        }
    }
}
