import QtQuick

// Plays the sprite sheet of the current state. All art/timing comes from the sprite pack
// (see docs/SPRITE_PACK.md); this file knows nothing about specific characters.
Item {
    id: root
    property string stateName: "idle"

    // packs.revision is only referenced so the binding re-evaluates when a pack/variant is (re)loaded.
    readonly property var info: (packs.revision, packs.stateInfo(stateName))

    AnimatedSprite {
        anchors.fill: parent
        source: root.info.source
        frameCount: root.info.frames
        frameWidth: packs.frameWidth
        frameHeight: packs.frameHeight
        frameRate: root.info.fps
        loops: AnimatedSprite.Infinite
        running: root.visible
        interpolate: false
        smooth: true
    }
}
