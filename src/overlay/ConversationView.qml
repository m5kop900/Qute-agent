import QtQuick
import QtQuick.Controls

// The chat: one bubble per user prompt and per assistant answer, a collapsible reasoning block,
// compact tool/permission cards and small notices. Newest items are at the bottom; the list follows
// them while the user is at the bottom and stays put when they scroll up to read.
ListView {
    id: view
    property var theme
    property real s: 1.0
    property bool stickToBottom: true
    property real scrollYBeforeModelReset: 0
    property bool restoringScrollAfterModelReset: false

    // Model resets are a last resort in ConversationModel, but when one is needed
    // they must not throw away the user's current reading position.
    Connections {
        target: conversation

        function onModelAboutToBeReset() {
            view.scrollYBeforeModelReset = view.contentY
            view.restoringScrollAfterModelReset = true
        }

        function onModelReset() {
            if (!view.restoringScrollAfterModelReset)
                return
            view.restoringScrollAfterModelReset = false
            Qt.callLater(function() {
                if (view.stickToBottom) {
                    view.positionViewAtEnd()
                } else {
                    const maxY = Math.max(view.originY, view.contentHeight - view.height)
                    view.contentY = Math.max(view.originY, Math.min(view.scrollYBeforeModelReset, maxY))
                }
            })
        }
    }
    readonly property string assistantName: store && store.activeModel !== ""
                                            ? String(store.activeModel).split("/").pop()
                                            : qsTr("Assistant")

    model: conversation
    clip: true
    spacing: Math.round(10 * s * config.spacingScale)
    boundsBehavior: ListView.StopAtBounds
    // Keep delegates alive around the viewport. With lazy delegates of different heights, Qt estimates the
    // unloaded ones, so the scrollbar thumb jumps as you drag. A large cache keeps the measured heights.
    cacheBuffer: Math.round(20000 * s)
    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

    // Follow the newest item while the list grows, while the card animates its height (the list starts at
    // height 0 on the first prompt), and after a refresh. Scrolling up by hand turns following off.
    function followEnd() {
        stickToBottom = true;
        Qt.callLater(view.positionViewAtEnd);
    }

    onCountChanged: if (stickToBottom) Qt.callLater(view.positionViewAtEnd)
    onContentHeightChanged: if (stickToBottom) Qt.callLater(view.positionViewAtEnd)
    onHeightChanged: if (stickToBottom) Qt.callLater(view.positionViewAtEnd)
    onMovementEnded: stickToBottom = atYEnd

    // While a session's history is fetched the view is empty: say so instead of showing a blank card.
    Column {
        visible: store.loadingHistory && view.count === 0
        width: view.width
        y: Math.max(0, Math.round(view.height / 2 - height / 2))
        spacing: Math.round(8 * view.s)

        BusyIndicator {
            anchors.horizontalCenter: parent.horizontalCenter
            running: parent.visible
            width: Math.round(28 * view.s)
            height: width
        }
        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Loading session…")
            color: view.theme.textDim
            font.pixelSize: Math.round(11 * view.s)
            font.italic: true
        }
    }

    delegate: Loader {
        id: row
        width: view.width
        // Each delegate reports its own height through implicitHeight. Reading item.height instead gave 0 for
        // the Item-based bubbles, so every row was laid out on top of the previous one.
        height: item ? item.implicitHeight : 0
        sourceComponent: {
            if (model.kind === "user") return choice0
            if (model.kind === "assistant-text") return choice1
            if (model.kind === "reasoning") return choice2
            if (model.kind === "tool") return choice3
            if (model.kind === "request") return choice4
            if (model.kind === "compaction") return choice6
            return choice5
        }

        // delegate for kind "user"
        Component {
            id: choice0
                Item {
                    id: userItem
                    width: view.width
                    implicitHeight: userCol.implicitHeight
                    height: implicitHeight
                    // Names of the files sent with this prompt (empty for a plain prompt).
                    readonly property var attachmentNames: model.attachments !== undefined ? model.attachments : []

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
                            radius: Math.round(12 * view.s * config.radiusScale)
                            color: Qt.rgba(view.theme.accent.r, view.theme.accent.g, view.theme.accent.b, 0.16)
                            border.width: 1
                            border.color: Qt.rgba(view.theme.accent.r, view.theme.accent.g, view.theme.accent.b, 0.35)

                            // Right-click anywhere on the bubble, the padding included, copies the whole message.
                            // Right-click on the text itself opens the text's own menu (Copy / Select all).
                            MouseArea {
                                id: bubbleArea
                                anchors.fill: parent
                                acceptedButtons: Qt.RightButton
                                onClicked: function (mouse) { bubbleMenu.openAt(bubbleArea, mouse.x, mouse.y) }
                            }

                            ThemedMenu {
                                id: bubbleMenu
                                theme: view.theme
                                s: view.s
                                actions: [
                                    {text: qsTr("Copy message"), enabled: true, run: function () {
                                        userText.selectAll()
                                        userText.copy()
                                        userText.deselect()
                                    }}
                                ]
                            }

                            SelectableText {
                                id: userText
                                x: Math.round(10 * view.s)
                                y: Math.round(10 * view.s)
                                width: parent.width - 2 * Math.round(10 * view.s)
                                theme: view.theme
                                text: model.text
                                textFormat: TextEdit.PlainText
                                color: view.theme.text
                                font.pixelSize: Math.round(13 * view.s)
                            }
                        }

                        // The files sent with this prompt, under the bubble (right-aligned like it).
                        Flow {
                            visible: userItem.attachmentNames.length > 0
                            width: parent.width
                            layoutDirection: Qt.RightToLeft
                            spacing: Math.round(5 * view.s)

                            Repeater {
                                model: userItem.attachmentNames
                                delegate: Rectangle {
                                    required property string modelData
                                    width: chipText.implicitWidth + Math.round(16 * view.s)
                                    height: chipText.implicitHeight + Math.round(8 * view.s)
                                    radius: Math.round(8 * view.s * config.radiusScale)
                                    color: Qt.rgba(view.theme.text.r, view.theme.text.g, view.theme.text.b, 0.06)
                                    border.width: 1
                                    border.color: view.theme.border

                                    Label {
                                        id: chipText
                                        anchors.centerIn: parent
                                        text: "\u{1F4CE} " + modelData
                                        font.pixelSize: Math.round(10 * view.s)
                                        color: view.theme.textDim
                                    }
                                }
                            }
                        }
                    }
                }
        
        }

        // delegate for kind "assistant-text"
        Component {
            id: choice1
                Item {
                    width: view.width
                    implicitHeight: asstCol.implicitHeight
                    height: implicitHeight

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
                            radius: Math.round(12 * view.s * config.radiusScale)
                            color: view.theme.input
                            border.width: 1
                            border.color: view.theme.border

                            // Right-click on the reply's bubble (padding included) copies the whole reply text.
                            // The reply is several text parts, so the copy goes through a hidden helper holding the text.
                            MouseArea {
                                id: asstArea
                                anchors.fill: parent
                                acceptedButtons: Qt.RightButton
                                onClicked: function (mouse) { asstMenu.openAt(asstArea, mouse.x, mouse.y) }
                            }

                            TextEdit {
                                id: asstCopyHelper
                                width: 0
                                height: 0
                                opacity: 0
                                readOnly: true
                                text: model.text
                            }

                            ThemedMenu {
                                id: asstMenu
                                theme: view.theme
                                s: view.s
                                actions: [
                                    {text: qsTr("Copy message"), enabled: true, run: function () {
                                        asstCopyHelper.selectAll()
                                        asstCopyHelper.copy()
                                    }}
                                ]
                            }

                            MessageText {
                                id: asstText
                                x: Math.round(10 * view.s)
                                y: Math.round(10 * view.s)
                                width: parent.width - 2 * Math.round(10 * view.s)
                                text: model.text
                                theme: view.theme
                                s: view.s
                                scrollView: view
                                onLinkActivated: (link) => Qt.openUrlExternally(link)
                            }
                        }
                    }
                }
        
        }

        // delegate for kind "reasoning"
        Component {
            id: choice2
                Item {
                    id: reason
                    width: view.width
                    implicitHeight: reasonCol.implicitHeight
                    height: implicitHeight
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
                            radius: Math.round(8 * view.s * config.radiusScale)
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
                            radius: Math.round(10 * view.s * config.radiusScale)
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

                                SelectableText {
                                    id: reasonText
                                    width: parent.width - Math.round(8 * view.s)
                                    theme: view.theme
                                    text: model.text
                                    textFormat: TextEdit.MarkdownText
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

        // delegate for kind "tool"
        Component {
            id: choice3
                Rectangle {
                    id: toolCard
                    width: view.width
                    implicitHeight: toolCol.implicitHeight + Math.round(12 * view.s)
                    radius: Math.round(9 * view.s * config.radiusScale)
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
                                    color: model.status === "error" ? view.theme.danger
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
                            radius: Math.round(7 * view.s * config.radiusScale)
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
                                    font.family: view.theme.codeFamily
                                    font.pixelSize: Math.round(10.5 * view.s)
                                    color: view.theme.text
                                }
                            }
                        }
                    }
                }
        
        }

        // delegate for kind "request"
        Component {
            id: choice4
                Rectangle {
                    width: view.width
                    implicitHeight: reqCol.implicitHeight + Math.round(12 * view.s)
                    radius: Math.round(9 * view.s * config.radiusScale)
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

        // delegate for kind "compaction": the summary the server wrote when the session was compacted.
        // One line when collapsed; the full summary (markdown) scrolls inside the card when opened.
        Component {
            id: choice6
                Item {
                    id: compact
                    width: view.width
                    implicitHeight: compactCard.height
                    height: implicitHeight
                    property bool open: false

                    Rectangle {
                        id: compactCard
                        width: parent.width
                        height: compactCol.implicitHeight + Math.round(16 * view.s)
                        radius: Math.round(10 * view.s * config.radiusScale)
                        color: Qt.rgba(view.theme.accent.r, view.theme.accent.g, view.theme.accent.b, 0.07)
                        border.width: 1
                        border.color: Qt.rgba(view.theme.accent.r, view.theme.accent.g, view.theme.accent.b, 0.3)

                        Column {
                            id: compactCol
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: Math.round(8 * view.s)
                            spacing: Math.round(6 * view.s)

                            Item {                      // header: toggles the full summary
                                id: compactHead
                                width: parent.width
                                height: Math.round(20 * view.s)
                                HoverHandler { cursorShape: Qt.PointingHandCursor }
                                TapHandler { onTapped: compact.open = !compact.open }

                                Row {
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: Math.round(6 * view.s)
                                    Label {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: "\u2726"
                                        color: view.theme.accent
                                        font.pixelSize: Math.round(11 * view.s)
                                    }
                                    Label {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: qsTr("Session compacted")
                                        color: view.theme.accent
                                        font.pixelSize: Math.round(11 * view.s)
                                        font.bold: true
                                    }
                                    Label {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: compact.open ? "\u25BE" : "\u25B8"
                                        color: view.theme.textDim
                                        font.pixelSize: Math.round(10 * view.s)
                                    }
                                }
                            }

                            Label {                     // collapsed: a one-line preview of the summary
                                width: parent.width
                                visible: !compact.open
                                text: model.text
                                elide: Text.ElideRight
                                maximumLineCount: 1
                                font.pixelSize: Math.round(10 * view.s)
                                color: view.theme.textDim
                            }

                            Flickable {                 // expanded: the whole summary, scrollable, capped in height
                                id: summaryFlick
                                visible: compact.open
                                width: parent.width
                                height: visible ? Math.min(summaryText.implicitHeight, Math.round(260 * view.s)) : 0
                                contentWidth: width
                                contentHeight: summaryText.implicitHeight
                                clip: true
                                boundsBehavior: Flickable.StopAtBounds
                                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                                MessageText {
                                    id: summaryText
                                    width: summaryFlick.width - Math.round(8 * view.s)
                                    text: model.text
                                    theme: view.theme
                                    s: view.s
                                    scrollView: view
                                    onLinkActivated: (link) => Qt.openUrlExternally(link)
                                }
                            }
                        }
                    }
                }
        }

        // delegate for kind (any other)
        Component {
            id: choice5
                Label {
                    width: view.width
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    text: model.text
                    font.pixelSize: Math.round(10.5 * view.s)
                    font.italic: true
                    color: model.status === "error" ? view.theme.danger : view.theme.textDim
                }
        
        }
    }

}
