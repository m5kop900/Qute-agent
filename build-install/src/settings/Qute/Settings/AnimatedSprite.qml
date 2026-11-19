import QtQuick
import QtQuick.Timers

// Simple sprite-sheet player used by qute-settings' preview.
// A sheet is one row of frames; this cycles through them at `frameRate`.
// Same API as the overlay's SpriteView but without the fade/hysteresis logic.
Item {
    id: root
    property url source
    property int frameCount: 1
    property int frameWidth: 96
    property int frameHeight: 96
    property real frameRate: 6
    property bool running: true

    width: frameWidth
    height: frameHeight
    clip: true

    property int frameIndex: 0

    Image {
        id: sheet
        source: root.source
        width: root.frameWidth * root.frameCount
        height: root.frameHeight
        x: -root.frameIndex * root.frameWidth
    }

    Timer {
        interval: 1000 / Math.max(root.frameRate, 1)
        running: root.running && root.frameCount > 1
        repeat: true
        onTriggered: root.frameIndex = (root.frameIndex + 1) % root.frameCount
    }
}
