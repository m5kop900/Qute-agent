import QtQuick
import QtQuick.Controls

// The row of attachment chips above the prompt box (F1.1, F1.2, F1.3). Shows nothing when there are none.
// With more chips than fit, the row scrolls sideways: a horizontal bar appears and the mouse wheel scrolls it too.
// `model` is the attachments model from C++ (roles: uri, name, mime, size, dataUrl, sourceType, isImage).
Item {
    id: root
    property var theme
    property real s: 1.0
    property var model: null

    readonly property int count: model ? model.count : 0
    visible: count > 0
    implicitHeight: visible ? Math.round(46 * s) : 0

    Flickable {
        id: strip
        anchors.fill: parent
        contentWidth: chips.width
        contentHeight: height
        clip: true
        flickableDirection: Flickable.HorizontalFlick
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.horizontal: ScrollBar {
            policy: strip.contentWidth > strip.width ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded
        }

        Row {
            id: chips
            height: strip.height
            spacing: Math.round(6 * root.s)

            Repeater {
                model: root.model
                delegate: AttachmentChip {
                    required property int index
                    required property string name
                    required property string uri
                    required property string mime
                    required property var size
                    required property string dataUrl
                    required property string sourceType
                    theme: root.theme
                    s: root.s
                    attachment: ({ uri: uri, name: name, mime: mime, size: size, dataUrl: dataUrl, sourceType: sourceType })
                    onRemoved: root.model.remove(index)
                }
            }
        }
    }

    // Mouse wheel (vertical or horizontal) moves along the strip.
    WheelHandler {
        target: strip
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        onWheel: function (event) {
            const d = event.angleDelta.y !== 0 ? event.angleDelta.y : event.angleDelta.x
            strip.contentX = Math.max(0, Math.min(strip.contentWidth - strip.width, strip.contentX - d))
        }
    }
}
