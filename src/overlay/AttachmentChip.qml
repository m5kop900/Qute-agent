import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// One attachment chip: thumbnail/icon, name, size, and a remove button.
// Clicking ✕ emits removed() so the owner (Main / AttachmentStrip) drops it from the list.
Item {
    id: root
    property var theme
    property real s: 1.0
    property var attachment   // {uri, name, size, mime, dataUrl, sourceType}
    signal removed()

    readonly property bool isImage: attachment && attachment.mime
                                  && String(attachment.mime).startsWith("image")

    implicitWidth: Math.round(200 * s)
    implicitHeight: Math.round(36 * s)

    RowLayout {
        anchors.fill: parent
        spacing: Math.round(6 * s)

        // thumbnail / icon
        Rectangle {
            Layout.preferredWidth: Math.round(28 * s)
            Layout.preferredHeight: Layout.preferredWidth
            radius: Math.round(6 * s * config.radiusScale)
            color: root.theme.input
            border.width: 1
            border.color: root.theme.border

            Image {
                id: thumb
                anchors.fill: parent
                anchors.margins: Math.round(3 * s)
                source: root.isImage && attachment.dataUrl ? attachment.dataUrl : ""
                fillMode: Image.PreserveAspectFit
                clip: true
                visible: root.isImage && attachment.dataUrl
                onStatusChanged: if (status === Image.Error) visible = false
            }
            Label {
                anchors.fill: parent
                anchors.centerIn: parent
                visible: !thumb.visible
                text: root.isImage ? "\u{1F4F7}" : "\u{1F4C4}"
                font.pixelSize: Math.round(14 * s)
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Math.round(1 * s)

            Label {
                Layout.fillWidth: true
                elide: Text.ElideRight
                text: attachment.name
                color: root.theme.text
                font.pixelSize: Math.round(10.5 * s)
                font.bold: true
            }
            Label {
                text: attachment.size >= 0
                    ? (attachment.size < 1024
                        ? qsTr("%1 B").arg(attachment.size)
                        : attachment.size < 1048576
                            ? qsTr("%1 KB").arg(Math.round(attachment.size / 1024))
                            : qsTr("%1 MB").arg(Math.round(attachment.size / 1048576) / 10))
                    : qsTr("unknown size")
                color: root.theme.textDim
                font.pixelSize: Math.round(8.5 * s)
            }
        }

        ToolButton {
            text: "\u{2716}"
            font.pixelSize: Math.round(11 * s)
            Layout.preferredWidth: Math.round(24 * s)
            Layout.preferredHeight: Math.round(24 * s)
            onClicked: root.removed()
        }
    }
}
