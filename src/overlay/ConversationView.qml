import QtQuick
import QtQuick.Controls
import QtQml.Models

// The chat: one bubble per user prompt and per assistant answer, a collapsible reasoning block,
// compact tool/permission cards and small notices. Newest items are at the bottom; the list follows
// them while the user is at the bottom and stays put when they scroll up to read.
ListView {
    id: view
    property var theme
    property real s: 1.0
    property bool stickToBottom: true
    readonly property string assistantName: store && store.activeModel !== ""
                                            ? String(store.activeModel).split("/").pop()
                                            : qsTr("Assistant")

    model: conversation
    clip: true
    spacing: Math.round(10 * s)
    boundsBehavior: ListView.StopAtBounds
    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

    onCountChanged: if (stickToBottom) Qt.callLater(view.positionViewAtEnd)
    onContentHeightChanged: if (stickToBottom) Qt.callLater(view.positionViewAtEnd)
    onMovementEnded: stickToBottom = atYEnd

    delegate: DelegateChooser {
        role: "kind"

        // ---- user prompt: right-aligned, accent-tinted bubble ----
        DelegateChoice {
            roleValue: "user"
            Item {
                width: ListView.view.width
                implicitHeight: userCol.implicitHeight

                Column {
                    id: userCol
                    width: parent.width
                    spacing: Math.round(3 * view.s)

                    Label {
                        anchors.right: parent.right
                        text: qsTr("You")
                        font.pixelSize: Math.round(9.5 * view.s)
                        font.bold: true
                        color: view.theme.textDim
                    }
                    Rectangle {
                        anchors.right: parent.right
                        width: Math.min(userText.implicitWidth + Math.round(20 * view.s), parent.width * 0.88)
                        height: userText.implicitHeight + Math.round(16 * view.s)
                        radius: Math.round(12 * view.s)
                        color: Qt.rgba(view.theme.accent.r, view.theme.accent.g, view.theme.accent.b, 0.16)
                        border.width: 1
                        border.color: Qt.rgba(view.theme.accent.r, view.theme.accent.g, view.theme.accent.b, 0.35)

                        Text {
                            id: userText
                            anchors.fill: parent
                            anchors.margins: Math.round(10 * view.s)
                            text: model.text
                            textFormat: Text.PlainText
                            wrapMode: Text.Wrap
                            color: view.theme.text
                            font.pixelSize: Math.round(13 * view.s)
                        }
                    }
                }
            }
        }

        // ---- assistant answer: left-aligned panel bubble with the model name ----
        DelegateChoice {
            roleValue: "assistant-text"
            Item {
                width: ListView.view.width
                implicitHeight: asstCol.implicitHeight

                Column {
                    id: asstCol
                    width: parent.width
                    spacing: Math.round(3 * view.s)

                    Label {
                        text: view.assistantName
                        font.pixelSize: Math.round(9.5 * view.s)
                        font.bold: true
                        color: view.theme.accent
                    }
                    Rectangle {
                        width: parent.width
                        height: asstText.implicitHeight + Math.round(16 * view.s)
                        radius: Math.round(12 * view.s)
                        color: view.theme.input
                        border.width: 1
                        border.color: view.theme.border

                        Text {
                            id: asstText
                            anchors.fill: parent
                            anchors.margins: Math.round(10 * view.s)
                            text: model.text
                            textFormat: Text.MarkdownText
                            wrapMode: Text.Wrap
                            color: view.theme.text
                            linkColor: view.theme.accent
                            font.pixelSize: Math.round(13 * view.s)
                            onLinkActivated: (link) => Qt.openUrlExternally(link)
                        }
                    }
                }
            }
        }

        // ---- reasoning: collapsed "Thinking…" / "Reasoned" header, click to show the text ----
        DelegateChoice {
            roleValue: "reasoning"
            Item {
                id: reason
                width: ListView.view.width
                implicitHeight: reasonCol.implicitHeight
                property bool open: false
                readonly property bool live: model.streaming === true

                Column {
                    id: reasonCol
                    width: parent.width
                    spacing: Math.round(4 * view.s)

                    Rectangle {
                        id: header
                        width: parent.width
                        height: Math.round(26 * view.s)
                        radius: Math.round(8 * view.s)
                        color: headHover.hovered ? Qt.rgba(view.theme.text.r, view.theme.text.g, view.theme.text.b, 0.06)
                                                 : "transparent"

                        Row {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: Math.round(6 * view.s)
                            spacing: Math.round(6 * view.s)

                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                width: Math.round(6 * view.s)
                                height: width
                                radius: width / 2
                                color: view.theme.accent
                                visible: reason.live
                                SequentialAnimation on opacity {
                                    loops: Animation.Infinite
                                    running: reason.live
                                    NumberAnimation { to: 0.2; duration: 600; easing.type: Easing.InOutSine }
                                    NumberAnimation { to: 1.0; duration: 600; easing.type: Easing.InOutSine }
                                }
                            }
                            Label {
                                anchors.verticalCenter: parent.verticalCenter
                                text: reason.live ? qsTr("Thinking…") : qsTr("Reasoning")
                                font.pixelSize: Math.round(11 * view.s)
                                font.italic: true
                                color: reason.live ? view.theme.accent : view.theme.textDim
                            }
                            Label {
                                anchors.verticalCenter: parent.verticalCenter
                                text: reason.open ? "▾" : "▸"
                                font.pixelSize: Math.round(11 * view.s)
                                color: view.theme.textDim
                            }
                        }

                        HoverHandler { id: headHover; cursorShape: Qt.PointingHandCursor }
                        TapHandler { onTapped: reason.open = !reason.open }
                    }

                    Rectangle {
                        visible: reason.open
                        width: parent.width
                        height: visible ? Math.min(reasonText.implicitHeight, Math.round(160 * view.s)) + Math.round(14 * view.s) : 0
                        radius: Math.round(10 * view.s)
                        color: Qt.rgba(view.theme.text.r, view.theme.text.g, view.theme.text.b, 0.04)
                        border.width: 1
                        border.color: view.theme.border
                        clip: true

                        Flickable {
                            anchors.fill: parent
                            anchors.margins: Math.round(7 * view.s)
                            contentHeight: reasonText.implicitHeight
                            clip: true
                            boundsBehavior: Flickable.StopAtBounds
                            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                            Text {
                                id: reasonText
                                width: parent.width - Math.round(8 * view.s)
                                text: model.text
                                textFormat: Text.MarkdownText
                                wrapMode: Text.Wrap
                                color: view.theme.textDim
                                font.italic: true
                                font.pixelSize: Math.round(11.5 * view.s)
                            }
                        }
                    }
                }
            }
        }

        // ---- tool call: compact card; click it to show the full input and output ----
        DelegateChoice {
            roleValue: "tool"
            Rectangle {
                id: toolCard
                width: ListView.view.width
                implicitHeight: toolCol.implicitHeight + Math.round(12 * view.s)
                radius: Math.round(9 * view.s)
                color: Qt.rgba(view.theme.text.r, view.theme.text.g, view.theme.text.b, 0.04)
                border.width: 1
                border.color: view.theme.border
                property bool open: false

                Column {
                    id: toolCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: Math.round(8 * view.s)
                    spacing: Math.round(2 * view.s)

                    Item {                        // header: clicking it toggles the full view
                        id: toolHead
                        width: parent.width
                        height: toolNameRow.implicitHeight
                        visible: model.text !== "" || model.detail !== ""
                        HoverHandler { cursorShape: Qt.PointingHandCursor }
                        TapHandler {
                            enabled: model.text !== ""
                            onTapped: toolCard.open = !toolCard.open
                        }

                        Row {
                            id: toolNameRow
                            spacing: Math.round(6 * view.s)
                            Label {
                                text: model.name !== "" ? model.name : qsTr("tool")
                                font.pixelSize: Math.round(11 * view.s)
                                font.bold: true
                                color: view.theme.accent
                            }
                            Label {
                                text: model.status
                                font.pixelSize: Math.round(9.5 * view.s)
                                color: model.status === "error" ? "#e5484d"
                                     : model.status === "running" ? view.theme.accent
                                     : view.theme.textDim
                            }
                            Label {
                                visible: model.text !== ""
                                text: toolCard.open ? "▾" : "▸"
                                font.pixelSize: Math.round(10 * view.s)
                                color: view.theme.textDim
                            }
                        }
                    }
                    Label {
                        width: parent.width
                        visible: model.detail !== "" && !toolCard.open
                        text: model.detail
                        elide: Text.ElideRight
                        font.pixelSize: Math.round(10 * view.s)
                        color: view.theme.textDim
                    }
                    Rectangle {                   // full input / output, capped in height and scrollable
                        visible: toolCard.open
                        width: parent.width
                        height: visible ? Math.min(toolFull.implicitHeight, Math.round(260 * view.s)) + Math.round(12 * view.s) : 0
                        radius: Math.round(7 * view.s)
                        color: Qt.rgba(view.theme.text.r, view.theme.text.g, view.theme.text.b, 0.05)
                        border.width: 1
                        border.color: view.theme.border
                        clip: true

                        Flickable {
                            anchors.fill: parent
                            anchors.margins: Math.round(6 * view.s)
                            contentWidth: width
                            contentHeight: toolFull.implicitHeight
                            clip: true
                            boundsBehavior: Flickable.StopAtBounds
                            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                            TextEdit {
                                id: toolFull
                                width: parent.width - Math.round(8 * view.s)
                                text: model.text
                                readOnly: true
                                selectByMouse: true
                                wrapMode: TextEdit.Wrap
                                textFormat: TextEdit.PlainText
                                font.family: "monospace"
                                font.pixelSize: Math.round(10.5 * view.s)
                                color: view.theme.text
                            }
                        }
                    }
                }
            }
        }

        // ---- permission / question request ----
        DelegateChoice {
            roleValue: "request"
            Rectangle {
                width: ListView.view.width
                implicitHeight: reqCol.implicitHeight + Math.round(12 * view.s)
                radius: Math.round(9 * view.s)
                color: Qt.rgba(view.theme.accent.r, view.theme.accent.g, view.theme.accent.b, 0.08)
                border.width: 1
                border.color: Qt.rgba(view.theme.accent.r, view.theme.accent.g, view.theme.accent.b, 0.35)

                Column {
                    id: reqCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: Math.round(8 * view.s)
                    spacing: Math.round(2 * view.s)

                    Label {
                        text: model.requestKind === "form" ? qsTr("Question") : qsTr("Permission")
                        font.pixelSize: Math.round(11 * view.s)
                        font.bold: true
                        color: view.theme.accent
                    }
                    Label {
                        width: parent.width
                        visible: model.text !== ""
                        text: model.text
                        wrapMode: Text.Wrap
                        font.pixelSize: Math.round(11 * view.s)
                        color: view.theme.text
                    }
                    Label {
                        visible: model.status !== ""
                        text: model.status
                        font.pixelSize: Math.round(9.5 * view.s)
                        color: view.theme.textDim
                    }
                }
            }
        }

        // ---- notices (errors, compaction, connection): centered, muted ----
        DelegateChoice {
            Label {
                width: ListView.view.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                text: model.text
                font.pixelSize: Math.round(10.5 * view.s)
                font.italic: true
                color: model.status === "error" ? "#e5484d" : view.theme.textDim
            }
        }
    }
}
