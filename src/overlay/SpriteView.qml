import QtQuick

// Plays the sprite sheet of the current state. All art/timing comes from the sprite pack
// (see docs/SPRITE_PACK.md); this file knows nothing about specific characters.
//
// The sheet is one row of frames. A plain timer steps through the frame index 0..n-1 and wraps
// around, so a cycle never restarts half-way and the timing is the same for every pack.
Item {
    id: root
    property string stateName: "idle"

    // packs.revision is only referenced so the binding re-evaluates when a pack/variant is (re)loaded.
    readonly property var info: (packs.revision, packs.stateInfo(stateName))
    readonly property int frameCount: Math.max(1, info.frames)
    readonly property int frameW: packs.frameWidth
    readonly property int frameH: packs.frameHeight
    property int frame: 0

    // A new state starts its own cycle from the first frame.
    onStateNameChanged: frame = 0
    onFrameCountChanged: frame = 0

    Timer {
        interval: Math.round(1000 / Math.max(1, root.info.fps))
        running: root.visible && root.frameCount > 1
        repeat: true
        onTriggered: root.frame = (root.frame + 1) % root.frameCount
    }

    Image {
        anchors.fill: parent
        source: root.info.source
        fillMode: Image.PreserveAspectFit
        smooth: true
        asynchronous: true
        cache: true
        sourceClipRect: Qt.rect(root.frame * root.frameW, 0, root.frameW, root.frameH)
    }
}
