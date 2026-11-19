import QtQuick

// Plays the sprite sheet of the current state. All art/timing comes from the sprite pack
// (see docs/SPRITE_PACK.md); this file knows nothing about specific characters.
//
// A sheet is one row of frames. ONE layer per state is created up front, each with a FIXED sheet that is
// preloaded in the background. A state change only fades layers in/out and never changes an Image source.
// (Changing the source of a live Image makes Qt drop the picture while the new sheet loads - that blank
// frame was the flicker.) A state is shown only once its sheet is ready; until then the old one stays.
//
// Polish on top of the pack art:
//   - the new state fades in ABOVE the old one, which stays opaque underneath, so there is no dip/blink
//   - dwell/hysteresis: quick thinking -> working -> thinking flaps do not flicker; calm states wait for a
//     short minimum dwell, urgent ones (waiting / error / done / offline) switch at once
//   - a small "pop" on every state change
//   - idle uses a capped fps; nothing runs while the window is hidden
Item {
    id: root
    property string stateName: "idle"
    property bool animate: true                 // hook for the F7 "animation on/off" setting

    // Tuning, all in one place (same for every pack).
    readonly property int fadeMs: 110           // fade-in of the new state
    readonly property int minDwellMs: 280       // a calm state stays at least this long before the next one
    readonly property int idleFpsCap: 12        // idle/offline never animate faster than this
    readonly property real fpsBoost: 1.6        // all sprite animations run this much faster than the pack says

    readonly property int frameW: packs.frameWidth
    readonly property int frameH: packs.frameHeight
    readonly property var stateList: ["offline", "idle", "thinking", "working", "waiting", "done", "error"]

    property string shownState: ""              // state on top ("" until the first sheet is ready)
    property string underState: ""              // previous state, kept opaque below while the new one fades in
    property string wantedState: ""             // requested, its sheet is still loading
    property string queued: ""                  // held back by the dwell rule
    property double lastSwitch: 0

    // Animate only while the overlay window is really on screen.
    readonly property bool windowShown: Window.window ? Window.window.visible : true
    readonly property bool running: root.visible && root.windowShown && root.animate

    function isUrgent(s) { return s === "waiting" || s === "error" || s === "done" || s === "offline" }
    function isCalm(s) { return s === "idle" || s === "offline" }
    function layerOf(name) { return layers.itemAt(stateList.indexOf(name)) }

    onStateNameChanged: request(stateName)
    Component.onCompleted: request(stateName)

    function request(s) {
        dwell.stop()
        if (s === shownState) { wantedState = ""; queued = ""; return }
        if (s === wantedState) { queued = ""; return }
        const since = Date.now() - lastSwitch
        if (shownState === "" || isUrgent(s) || since >= minDwellMs) {
            queued = ""
            wantedState = s
            tryCommit()
        } else {
            queued = s
            dwell.interval = Math.max(1, minDwellMs - since)
            dwell.start()
        }
    }

    // Show the wanted state as soon as its sheet is ready.
    function tryCommit() {
        if (wantedState === "") return
        const l = layerOf(wantedState)
        if (!l) { wantedState = ""; return }
        if (l.failed) { wantedState = ""; return }     // broken sheet: keep what is on screen
        if (!l.ready) return
        const s = wantedState
        const first = shownState === ""
        wantedState = ""
        underState = first ? "" : shownState
        shownState = s
        lastSwitch = Date.now()
        l.frame = 0
        fadeDone.restart()
        if (!first) pop.restart()
    }

    Timer { id: dwell; onTriggered: root.request(root.queued) }
    Timer { id: fadeDone; interval: root.fadeMs + 30; onTriggered: root.underState = "" }

    // ---- drawing ---------------------------------------------------------------------------------------
    component Layer: Item {
        id: layer
        required property string name
        // packs.revision is only referenced so this re-evaluates when a pack/variant is (re)loaded.
        readonly property var info: (packs.revision, packs.stateInfo(name))
        readonly property int frames: Math.max(1, info.frames)
        readonly property bool ready: img.status === Image.Ready
        readonly property bool failed: img.status === Image.Error
        readonly property bool isTop: root.shownState === name
        readonly property bool isUnder: root.underState === name
        property int frame: 0

        anchors.fill: parent
        z: isTop ? 2 : 1
        opacity: (isTop || isUnder) ? 1 : 0
        visible: opacity > 0.001
        // Only the incoming layer fades; the outgoing one just disappears once the new one is solid.
        Behavior on opacity { enabled: layer.isTop; NumberAnimation { duration: root.fadeMs; easing.type: Easing.InOutQuad } }
        onReadyChanged: root.tryCommit()
        onFailedChanged: root.tryCommit()

        Timer {
            readonly property real fps: (layer.isTop && root.isCalm(layer.name))
                                        ? Math.min(layer.info.fps * root.fpsBoost, root.idleFpsCap)
                                        : layer.info.fps * root.fpsBoost
            interval: Math.round(1000 / Math.max(1, fps))
            running: (layer.isTop || layer.isUnder) && root.running && layer.frames > 1
            repeat: true
            onTriggered: layer.frame = (layer.frame + 1) % layer.frames
        }
        // A window onto ONE frame; the whole sheet sits inside it and slides sideways. The sheet is decoded
        // once. (sourceClipRect is NOT used: changing it makes Qt re-request a cropped copy of the sheet
        // for every frame, and with an async Image that copy can arrive late -> an empty frame = flicker.)
        Item {
            id: window
            readonly property real k: Math.min(layer.width / Math.max(1, root.frameW),
                                               layer.height / Math.max(1, root.frameH))
            width: root.frameW * k
            height: root.frameH * k
            x: (layer.width - width) / 2
            y: (layer.height - height) / 2
            clip: true
            Image {
                id: img
                x: -layer.frame * window.width
                y: 0
                width: layer.frames * window.width
                height: window.height
                fillMode: Image.Stretch
                source: layer.info.source           // never changes with the state, only with the pack/variant
                smooth: true
                asynchronous: true
                cache: true
            }
        }
    }

    // The "pop" lives on this wrapper, so the window's summon animation can keep using root.scale/rotation.
    Item {
        id: body
        anchors.fill: parent
        transformOrigin: Item.Bottom
        Repeater {
            id: layers
            model: root.stateList
            delegate: Layer { required property string modelData; name: modelData }
        }
    }

    SequentialAnimation {
        id: pop
        NumberAnimation { target: body; property: "scale"; to: 1.06; duration: 70; easing.type: Easing.OutQuad }
        NumberAnimation { target: body; property: "scale"; to: 1.0; duration: 170; easing.type: Easing.OutBack; easing.overshoot: 2.5 }
    }
}
