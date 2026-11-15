import QtQuick
import QtQuick.Controls

// Scrollable markdown view of the streamed assistant answer. Auto-scrolls while text grows.
Flickable {
    id: fl
    property string text
    property var theme
    property real s: 1.0

    clip: true
    contentWidth: width
    contentHeight: body.implicitHeight
    boundsBehavior: Flickable.StopAtBounds
    ScrollBar.vertical: ScrollBar {}

    onContentHeightChanged: contentY = Math.max(0, contentHeight - height)

    Text {
        id: body
        width: fl.width - Math.round(10 * fl.s)
        text: fl.text
        textFormat: Text.MarkdownText
        wrapMode: Text.Wrap
        color: fl.theme.text
        linkColor: fl.theme.accent
        font.pixelSize: Math.round(13 * fl.s)
        onLinkActivated: (link) => Qt.openUrlExternally(link)
    }
}
