import QtQuick
import QtQuick.Controls

// Context window usage ring. percent < 0 means the limit is not known yet (shown as "–").
// Values come from SessionStore, which mirrors opencode's own calculation.
Item {
    id: root
    property var theme
    property real s: 1.0
    property int percent: -1
    property real tokens: 0
    property real limit: 0

    readonly property color ringColor: percent < 0 ? theme.border
                                                : percent >= 90 ? "#e5484d"
                                                : percent >= 70 ? "#e8a33d"
                                                : theme.accent

    implicitWidth: Math.round(22 * s)
    implicitHeight: implicitWidth

    function fmt(n) {
        if (!(n > 0))
            return "0"
        if (n >= 1000000)
            return (Math.round(n / 100000) / 10) + "M"
        if (n >= 1000)
            return (Math.round(n / 100) / 10) + "k"
        return String(Math.round(n))
    }

    Canvas {
        id: canvas
        anchors.fill: parent
        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            const lw = Math.max(1.5, Math.round(1.6 * root.s * 10) / 10)
            const cx = width / 2
            const cy = height / 2
            const r = Math.max(2, Math.min(width, height) / 2 - lw / 2 - 1)
            ctx.lineWidth = lw
            ctx.lineCap = "round"
            ctx.strokeStyle = root.theme.border
            ctx.beginPath()
            ctx.arc(cx, cy, r, 0, Math.PI * 2)
            ctx.stroke()
            if (root.percent > 0) {
                const p = Math.max(0, Math.min(100, root.percent))
                ctx.strokeStyle = root.ringColor
                ctx.beginPath()
                ctx.arc(cx, cy, r, -Math.PI / 2,
                        -Math.PI / 2 + Math.PI * 2 * Math.min(p, 99.9) / 100)
                ctx.stroke()
            }
        }
    }

    Connections {
        target: root
        function onPercentChanged() { canvas.requestPaint() }
        function onTokensChanged() { canvas.requestPaint() }
        function onLimitChanged() { canvas.requestPaint() }
    }
    Component.onCompleted: canvas.requestPaint()

    Label {
        anchors.centerIn: parent
        text: root.percent >= 0 ? String(root.percent) : "–"
        color: root.percent < 0 ? root.theme.textDim : root.theme.text
        font.pixelSize: Math.round(7.5 * root.s)
        font.bold: root.percent >= 70
    }

    HoverHandler { id: hover }
    ToolTip.visible: hover.hovered
    ToolTip.delay: 150
    ToolTip.text: root.percent >= 0
                  ? qsTr("Context: %1% used\n%2 of %3 tokens")
                        .arg(root.percent).arg(root.fmt(root.tokens)).arg(root.fmt(root.limit))
                  : (root.tokens > 0 ? qsTr("%1 tokens used\ncontext limit unknown").arg(root.fmt(root.tokens))
                                     : qsTr("Context usage not known yet"))
}
