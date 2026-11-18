import QtQuick
import QtQuick.Controls

import "CodeHighlight.js" as Code

// Renders an assistant message: prose through Qt's Markdown, fenced code blocks in a box with syntax colors,
// a Copy button per code block, and selectable text everywhere (right-click to copy).
Column {
    id: root
    property string text: ""
    property var theme
    property real s: 1.0
    property color textColor: theme ? theme.text : "#000000"
    property int pixelSize: Math.round(13 * s)
    signal linkActivated(string link)

    spacing: Math.round(8 * s)

    function hex(c) {
        const h = function (v) { return ("0" + Math.round(v * 255).toString(16)).slice(-2) }
        return "#" + h(c.r) + h(c.g) + h(c.b)
    }
    function palette() {
        return {
            text: hex(root.textColor),
            dim: hex(root.theme.textDim),
            keyword: hex(root.theme.accent),
            string: "#98c379",
            number: "#d19a66",
            comment: hex(root.theme.textDim)
        }
    }

    readonly property var parts: Code.split(root.text)

    Repeater {
        model: root.parts
        delegate: Item {
            id: part
            required property var modelData
            width: root.width
            height: modelData.code ? codeBox.height : prose.implicitHeight

            // Prose
            SelectableText {
                id: prose
                visible: !part.modelData.code
                width: parent.width
                theme: root.theme
                text: part.modelData.code ? "" : part.modelData.text
                textFormat: TextEdit.MarkdownText
                color: root.textColor
                font.pixelSize: root.pixelSize
                onLinkActivated: (link) => root.linkActivated(link)
            }

            // Code block: language label and Copy button on top, highlighted code below.
            Rectangle {
                id: codeBox
                visible: part.modelData.code
                width: parent.width
                readonly property real pad: Math.round(8 * root.s)
                readonly property real headerH: Math.round(24 * root.s)
                height: headerH + codeText.implicitHeight + 2 * pad
                radius: Math.round(8 * root.s)
                color: root.theme.bg !== undefined ? root.theme.bg : root.theme.input
                border.width: 1
                border.color: root.theme.border
                property bool copied: false

                Timer {
                    id: copiedTimer
                    interval: 1500
                    onTriggered: codeBox.copied = false
                }

                // Hidden helper: holds the raw code so Copy puts the code itself on the clipboard (no highlight tags).
                TextEdit {
                    id: copyHelper
                    width: 0
                    height: 0
                    opacity: 0
                    readOnly: true
                    text: part.modelData.code ? part.modelData.text : ""
                }

                Label {
                    anchors.left: parent.left
                    anchors.leftMargin: codeBox.pad
                    anchors.top: parent.top
                    anchors.topMargin: Math.round(4 * root.s)
                    text: part.modelData.code ? part.modelData.lang : ""
                    color: root.theme.textDim
                    font.pixelSize: Math.round(10 * root.s)
                }

                Rectangle {
                    id: copyBtn
                    anchors.right: parent.right
                    anchors.rightMargin: codeBox.pad
                    anchors.top: parent.top
                    anchors.topMargin: Math.round(4 * root.s)
                    width: copyLabel.implicitWidth + 2 * Math.round(8 * root.s)
                    height: Math.round(18 * root.s)
                    radius: Math.round(5 * root.s)
                    color: copyArea.containsMouse ? root.theme.border : "transparent"
                    border.width: 1
                    border.color: root.theme.border

                    Label {
                        id: copyLabel
                        anchors.centerIn: parent
                        text: codeBox.copied ? qsTr("Copied") : qsTr("Copy")
                        color: codeBox.copied ? root.theme.accent : root.theme.textDim
                        font.pixelSize: Math.round(10 * root.s)
                    }
                    MouseArea {
                        id: copyArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            copyHelper.selectAll()
                            copyHelper.copy()
                            codeBox.copied = true
                            copiedTimer.restart()
                        }
                    }
                }

                SelectableText {
                    id: codeText
                    x: codeBox.pad
                    y: codeBox.headerH + codeBox.pad - Math.round(4 * root.s)
                    width: codeBox.width - 2 * codeBox.pad
                    theme: root.theme
                    textFormat: TextEdit.RichText
                    text: part.modelData.code ? Code.highlight(part.modelData.text, part.modelData.lang, root.palette()) : ""
                    wrapMode: TextEdit.WrapAnywhere
                    font.family: "monospace"
                    font.pixelSize: Math.round(12 * root.s)
                    color: root.textColor
                }
            }
        }
    }
}
