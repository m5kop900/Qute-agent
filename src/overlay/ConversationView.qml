import QtQuick
import QtQuick.Controls

// F0.3 skeleton: the conversation list. Renders the ordered ConversationModel items; the delegate
// only chooses a look per kind. F3 adds real code blocks, collapsing and message actions.
ListView {
    id: view
    property var theme
    property real s: 1.0
    property bool stickToBottom: true

    model: conversation
    clip: true
    spacing: Math.round(8 * s)
    boundsBehavior: ListView.StopAtBounds
    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

    onCountChanged: Qt.callLater(view.positionViewAtEnd)
    onContentHeightChanged: if (view.stickToBottom) Qt.callLater(view.positionViewAtEnd)
    onMovementEnded: view.stickToBottom = view.atYEnd

    delegate: Item {
        width: ListView.view.width
        implicitHeight: body.implicitHeight

        Column {
            id: body
            width: parent.width
            spacing: Math.round(3 * view.s)

            Label {
                width: parent.width
                visible: model.kind !== "assistant-text"
                text: model.kind === "user" ? qsTr("You")
                    : model.kind === "reasoning" ? qsTr("Reasoning")
                    : model.kind === "tool" ? (model.name !== "" ? model.name : qsTr("tool"))
                    : model.kind === "request" ? (model.requestKind === "form" ? qsTr("Question") : qsTr("Permission"))
                    : qsTr("Notice")
                elide: Text.ElideRight
                color: model.kind === "user" ? view.theme.text
                     : model.kind === "tool" || model.kind === "request" ? view.theme.accent
                     : view.theme.textDim
                font.pixelSize: Math.round(10 * view.s)
                font.bold: model.kind === "user"
            }

            Text {
                width: parent.width
                visible: model.text !== ""
                text: model.text
                textFormat: Text.MarkdownText
                wrapMode: Text.Wrap
                color: model.kind === "reasoning" || model.kind === "notice" ? view.theme.textDim : view.theme.text
                linkColor: view.theme.accent
                font.pixelSize: Math.round(13 * view.s)
                font.italic: model.kind === "reasoning" || model.kind === "notice"
                onLinkActivated: (link) => Qt.openUrlExternally(link)
            }

            Label {
                width: parent.width
                visible: model.kind === "tool" && model.detail !== ""
                text: model.detail
                elide: Text.ElideRight
                color: view.theme.textDim
                font.pixelSize: Math.round(10 * view.s)
            }

            Label {
                width: parent.width
                visible: (model.kind === "tool" || model.kind === "request") && model.status !== ""
                text: model.status
                color: model.status === "error" ? "#e5484d"
                     : model.status === "running" || model.status === "pending" ? view.theme.accent
                     : view.theme.textDim
                font.pixelSize: Math.round(9 * view.s)
            }
        }
    }
}
