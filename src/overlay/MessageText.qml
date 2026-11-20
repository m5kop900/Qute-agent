import QtQuick
import QtQuick.Controls

import "CodeHighlight.js" as Code

// Renders an assistant message: prose through Qt's Markdown, fenced code blocks in a box with syntax colors,
// a Copy button per code block. Selection spans the whole message: drag from any text, through code blocks,
// into any other text (the code block is not a barrier). Right-click copies the selection, or the whole message.
Item {
    id: root
    property string text: ""
    property var theme
    property real s: 1.0
    property color textColor: theme ? theme.text : "#000000"
    property int pixelSize: Math.round(13 * s)
    // The chat list this text sits in. While the left button is held on the text, the list stops
    // scrolling so the drag selects text instead of moving the list.
    property Item scrollView: null
    signal linkActivated(string link)

    implicitHeight: body.implicitHeight

    function hex(c) {
        const h = function (v) { return ("0" + Math.round(v * 255).toString(16)).slice(-2) }
        return "#" + h(c.r) + h(c.g) + h(c.b)
    }
    function palette() {
        return {
            text: hex(root.textColor),
            dim: hex(root.theme.textDim),
            keyword: hex(root.theme.accent),
            string: root.theme ? root.theme.syntaxString : "#98c379",
            number: root.theme ? root.theme.syntaxNumber : "#d19a66",
            comment: hex(root.theme.textDim)
        }
    }

    readonly property var parts: Code.split(root.text)

    // ---- selection across parts -------------------------------------------------------------------------
    // A position is (part index, character position inside that part's TextEdit).
    property int anchorIdx: -1
    property int anchorPos: 0
    property int endIdx: -1
    property int endPos: 0

    function textOf(i) {
        const d = rep.itemAt(i)
        return d ? d.textEdit : null
    }
    // Index of the part at window-of-root height y. Points in a gap or outside pick the nearest part.
    function partIndexAt(y) {
        let best = 0
        let bestDist = 1e9
        for (let i = 0; i < rep.count; ++i) {
            const d = rep.itemAt(i)
            if (!d)
                continue
            const top = d.mapToItem(root, 0, 0).y
            const bot = top + d.height
            if (y >= top && y <= bot)
                return i
            const dist = y < top ? top - y : y - bot
            if (dist < bestDist) {
                bestDist = dist
                best = i
            }
        }
        return best
    }
    function positionAt(i, mx, my) {
        const t = textOf(i)
        if (!t)
            return 0
        const p = t.mapFromItem(root, mx, my)
        return t.positionAt(p.x, p.y)
    }
    // Applies the anchor..end range to every part's TextEdit: earlier and later parts are cleared,
    // the parts in between are fully selected, and the first and last part are cut at the ends.
    function applySelection() {
        let sI = anchorIdx, sP = anchorPos, eI = endIdx, eP = endPos
        if (sI > eI || (sI === eI && sP > eP)) {
            const ti = sI, tp = sP
            sI = eI; sP = eP; eI = ti; eP = tp
        }
        for (let i = 0; i < rep.count; ++i) {
            const t = textOf(i)
            if (!t)
                continue
            if (i < sI || i > eI || anchorIdx < 0) {
                t.deselect()
            } else {
                const a = i === sI ? sP : 0
                const b = i === eI ? eP : t.length
                t.select(a, b)
            }
        }
    }
    // Extends the selection to a point given in root coordinates.
    function extendTo(x, y) {
        const i = partIndexAt(y)
        endIdx = i
        endPos = positionAt(i, x, y)
        applySelection()
    }

    // Dragging above or below the chat list: the list scrolls by itself and the selection keeps growing
    // to the list's edge, so text that scrolls into view is selected too.
    // The pointer is kept in window coordinates: the text moves while the list scrolls, the pointer does not.
    property real pointerSceneX: 0
    property real pointerSceneY: 0
    Timer {
        id: edgeScroll
        interval: 16
        repeat: true
        onTriggered: root.edgeScrollStep()
    }
    function edgeScrollStep() {
        const v = root.scrollView
        if (!v || anchorIdx < 0 || !sel.pressed) {
            edgeScroll.stop()
            return
        }
        const q = v.mapFromItem(null, pointerSceneX, pointerSceneY)
        const above = q.y < 0
        const dist = above ? -q.y : (q.y > v.height ? q.y - v.height : 0)
        if (dist <= 0) {
            edgeScroll.stop()
            return
        }
        // Faster the further the pointer is outside the list.
        const step = Math.min(Math.round(40 * root.s), Math.round(2 * root.s + dist * 0.2))
        const before = v.contentY
        const maxY = Math.max(v.originY, v.contentHeight - v.height)
        v.contentY = Math.max(v.originY, Math.min(maxY, v.contentY + (above ? -step : step)))
        if (v.contentY === before)
            return          // already at the start or the end of the list
        // The selection grows to the list's edge line, where the new text is.
        const edgeX = Math.max(0, Math.min(v.width, q.x))
        const edgeY = above ? 0 : v.height
        const p = root.mapFromItem(v, edgeX, edgeY)
        root.extendTo(p.x, Math.max(0, Math.min(root.height, p.y)))
    }

    function selectedText() {
        const out = []
        for (let i = 0; i < rep.count; ++i) {
            const t = textOf(i)
            if (t && t.selectedText.length > 0)
                out.push(t.selectedText)
        }
        return out.join("\n")
    }
    function copyText(str) {
        copyHelper.text = str
        copyHelper.selectAll()
        copyHelper.copy()
    }
    function selectAll() {
        anchorIdx = 0
        anchorPos = 0
        endIdx = rep.count - 1
        endPos = endIdx >= 0 ? (textOf(endIdx) ? textOf(endIdx).length : 0) : 0
        applySelection()
    }

    // Hidden helper: the TextEdit that puts text on the clipboard (same trick as the code block Copy).
    TextEdit {
        id: copyHelper
        width: 0
        height: 0
        opacity: 0
        readOnly: true
    }

    Column {
        id: body
        width: root.width
        spacing: Math.round(8 * root.s * config.spacingScale)

        Repeater {
            id: rep
            model: root.parts
            delegate: Item {
                id: part
                required property var modelData
                width: root.width
                height: modelData.code ? codeBox.height : prose.implicitHeight

                // The TextEdit that holds this part's text (prose or code), used by the selection layer.
                readonly property var textEdit: modelData.code ? codeText : prose

                // Called by the message's mouse layer: true when (x, y) of the message hit this part's Copy button.
                function tryCopy(x, y) {
                    if (!part.modelData.code)
                        return false
                    const p = copyBtn.mapFromItem(root, x, y)
                    if (!copyBtn.contains(p))
                        return false
                    copyCode()
                    return true
                }
                function copyCode() {
                    copyHelper.text = part.modelData.text
                    copyHelper.selectAll()
                    copyHelper.copy()
                    codeBox.copied = true
                    copiedTimer.restart()
                }

                // Prose
                SelectableText {
                    id: prose
                    visible: !part.modelData.code
                    width: parent.width
                    theme: root.theme
                    mouseSelect: false           // the message's mouse layer does the selecting
                    text: part.modelData.code ? "" : part.modelData.text
                    textFormat: TextEdit.MarkdownText
                    color: root.textColor
                    font.pixelSize: root.pixelSize
                }

                // Code block: language label and Copy button on top, highlighted code below.
                Rectangle {
                    id: codeBox
                    visible: part.modelData.code
                    width: parent.width
                    readonly property real pad: Math.round(8 * root.s)
                    readonly property real headerH: Math.round(24 * root.s)
                    height: headerH + codeText.implicitHeight + 2 * pad
                    radius: Math.round(8 * root.s * config.radiusScale)
                    color: root.theme.codeBackground
                    border.width: 1
                    border.color: root.theme.border
                    property bool copied: false

                    Timer {
                        id: copiedTimer
                        interval: 1500
                        onTriggered: codeBox.copied = false
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
                        radius: Math.round(5 * root.s * config.radiusScale)
                        color: "transparent"
                        border.width: 1
                        border.color: root.theme.border

                        Label {
                            id: copyLabel
                            anchors.centerIn: parent
                            text: codeBox.copied ? qsTr("Copied") : qsTr("Copy")
                            color: codeBox.copied ? root.theme.accent : root.theme.textDim
                            font.pixelSize: Math.round(10 * root.s)
                        }
                    }

                    SelectableText {
                        id: codeText
                        x: codeBox.pad
                        y: codeBox.headerH + codeBox.pad - Math.round(4 * root.s)
                        width: codeBox.width - 2 * codeBox.pad
                        theme: root.theme
                        mouseSelect: false
                        textFormat: TextEdit.RichText
                        text: part.modelData.code ? Code.highlight(part.modelData.text, part.modelData.lang, root.palette()) : ""
                        wrapMode: TextEdit.WrapAnywhere
                        font.family: root.theme ? root.theme.codeFamily : "monospace"
                        font.pixelSize: Math.round(12 * root.s)
                        color: root.textColor
                    }
                }
            }
        }
    }

    // ---- mouse layer: selection, Copy buttons, links, right-click menu ----------------------------------
    // Sits above all the parts. Left drag selects across parts; a left click without a drag collapses the
    // selection (and opens a link under the pointer); right-click opens the menu.
    MouseArea {
        id: sel
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        cursorShape: Qt.IBeamCursor
        property bool moved: false

        onPressed: function (mouse) {
            if (mouse.button === Qt.RightButton) {
                menu.openAt(sel, mouse.x, mouse.y)
                return
            }
            if (root.scrollView)
                root.scrollView.interactive = false
            // A Copy button of a code block takes the click.
            for (let i = 0; i < rep.count; ++i) {
                const d = rep.itemAt(i)
                if (d && d.tryCopy(mouse.x, mouse.y))
                    return
            }
            const i = root.partIndexAt(mouse.y)
            root.anchorIdx = i
            root.anchorPos = root.positionAt(i, mouse.x, mouse.y)
            root.endIdx = i
            root.endPos = root.anchorPos
            moved = false
            root.applySelection()
        }
        onPositionChanged: function (mouse) {
            if (!pressed || root.anchorIdx < 0)
                return
            moved = true
            root.extendTo(mouse.x, mouse.y)
            // Outside the list's viewport the list scrolls until the pointer is back inside it.
            const v = root.scrollView
            if (v) {
                const sp = sel.mapToItem(null, mouse.x, mouse.y)
                root.pointerSceneX = sp.x
                root.pointerSceneY = sp.y
                const q = v.mapFromItem(null, sp.x, sp.y)
                if (q.y < 0 || q.y > v.height) {
                    if (!edgeScroll.running)
                        edgeScroll.start()
                } else {
                    edgeScroll.stop()
                }
            }
        }
        onReleased: function (mouse) {
            edgeScroll.stop()
            if (root.scrollView)
                root.scrollView.interactive = true
        }
        onCanceled: {
            edgeScroll.stop()
            if (root.scrollView)
                root.scrollView.interactive = true
        }
        onClicked: function (mouse) {
            if (mouse.button !== Qt.LeftButton || moved)
                return
            const i = root.partIndexAt(mouse.y)
            const t = root.textOf(i)
            // Links: TextEdit.linkAt is used when the Qt version provides it; otherwise links are not clickable here.
            if (t && t.linkAt) {
                const p = t.mapFromItem(root, mouse.x, mouse.y)
                const link = t.linkAt(p.x, p.y)
                if (link)
                    root.linkActivated(link)
            }
        }
    }

    ThemedMenu {
        id: menu
        theme: root.theme
        s: root.s
        actions: [
            {text: qsTr("Copy"), enabled: true, run: function () {
                const picked = root.selectedText()
                root.copyText(picked.length > 0 ? picked : root.text)
            }},
            {text: qsTr("Select all"), enabled: true, run: function () { root.selectAll() }}
        ]
    }
}
