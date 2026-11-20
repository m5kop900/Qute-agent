import QtQuick
import QtQuick.Controls

// The attachment file manager: its own window, centred on the screen (not inside the overlay card).
// Places on the left, the folder path on top, a search box, and the files Qute can send. Double-click
// or Enter opens a folder; click, Ctrl+click, Shift+click or Space selects files; Attach hands the
// selection back to the prompt box. Esc closes it without attaching.
Window {
    id: root
    property var theme
    property real s: 1.0
    property var files: null                // the attachments model (listDirectory, defaultDirectory, homeDirectory, isDirectory)
    property string currentPath: ""
    property var rawEntries: []             // rows of the current folder, as listDirectory returns them
    property var selectedKeys: []           // absolute paths of the selected files, in pick order
    property int cursor: 0                  // highlighted row in visibleEntries
    signal attachRequested(var paths)
    signal systemDialogRequested()

    width: Math.round(760 * s)
    height: Math.round(500 * s)
    minimumWidth: width
    maximumWidth: width
    minimumHeight: height
    maximumHeight: height
    title: qsTr("Attach files")
    color: "transparent"
    flags: Qt.Dialog | Qt.FramelessWindowHint
    modality: Qt.WindowModal

    readonly property int pad: Math.round(12 * s)
    readonly property int rowH: Math.round(34 * s)

    // ---- state -----------------------------------------------------------------------------------------
    function openAt(path) {
        selectedKeys = []
        search.text = ""
        goTo(path)
        show()
        raise()
        requestActivate()
        list.forceActiveFocus()
    }
    function goTo(path) {
        if (!files || path === "")
            return
        currentPath = path
        rawEntries = files.listDirectory(path)
        cursor = 0
        list.positionViewAtBeginning()
    }
    function parentPath(p) {
        const up = p.replace(/\/+$/, "").replace(/\/[^\/]*$/, "")
        return up === "" ? "/" : up
    }
    function isSelected(key) {
        return selectedKeys.indexOf(key) >= 0
    }
    function toggleKey(key) {
        selectedKeys = isSelected(key) ? selectedKeys.filter(function (k) { return k !== key })
                                       : selectedKeys.concat([key])
    }
    function selectRange(from, to) {
        const a = Math.min(from, to), b = Math.max(from, to)
        const keys = []
        for (let i = a; i <= b && i < visibleEntries.length; ++i)
            if (String(visibleEntries[i].kind) === "file")
                keys.push(String(visibleEntries[i].key))
        selectedKeys = keys
    }
    function activate(row) {
        if (row < 0 || row >= visibleEntries.length)
            return
        const e = visibleEntries[row]
        if (String(e.kind) === "dir") {
            goTo(String(e.key))
        } else if (String(e.kind) === "file") {
            if (selectedKeys.length === 0)
                selectedKeys = [String(e.key)]
            attach()
        }
    }
    function attach() {
        if (selectedKeys.length === 0)
            return
        attachRequested(selectedKeys.slice())
        close()
    }

    // ---- derived data ----------------------------------------------------------------------------------
    readonly property var visibleEntries: {
        const q = search.text.trim().toLowerCase()
        const out = []
        for (let i = 0; i < rawEntries.length; ++i) {
            const e = rawEntries[i]
            if (q !== "" && String(e.label).toLowerCase().indexOf(q) < 0)
                continue
            out.push(e)
        }
        return out
    }
    readonly property var places: {
        const home = files ? files.homeDirectory() : ""
        const out = [{label: qsTr("Home"), path: home, icon: "⌂"}]
        const work = files ? files.defaultDirectory() : ""
        if (work !== "" && work !== home)
            out.push({label: qsTr("Working folder"), path: work, icon: "◆"})
        const names = [["Desktop", qsTr("Desktop")], ["Documents", qsTr("Documents")], ["Downloads", qsTr("Downloads")]]
        for (let i = 0; i < names.length; ++i) {
            const p = home + "/" + names[i][0]
            if (files && files.isDirectory(p))
                out.push({label: names[i][1], path: p, icon: "▢"})
        }
        out.push({label: qsTr("Computer"), path: "/", icon: "▣"})
        return out
    }
    readonly property var crumbs: {
        const parts = currentPath.split("/").filter(function (p) { return p !== "" })
        const out = [{label: "/", path: "/"}]
        let acc = ""
        for (let i = 0; i < parts.length; ++i) {
            acc += "/" + parts[i]
            out.push({label: parts[i], path: acc})
        }
        return out
    }
    readonly property int fileCount: {
        let n = 0
        for (let i = 0; i < rawEntries.length; ++i)
            if (String(rawEntries[i].kind) === "file")
                n++
        return n
    }

    // ---- the window ------------------------------------------------------------------------------------
    Shortcut {
        sequence: "Escape"
        onActivated: root.close()
    }

    Rectangle {
        anchors.fill: parent
        radius: Math.round(14 * root.s * config.radiusScale)
        color: root.theme.input
        border.width: 1
        border.color: root.theme.border
        clip: true

        // Title bar: drag anywhere on it to move the window; the close button sits on the right.
        Item {
            id: titleBar
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: Math.round(42 * root.s)

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onPressed: root.startSystemMove()
            }
            Label {
                anchors.left: parent.left
                anchors.leftMargin: root.pad
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("ATTACH FILES")
                color: root.theme.textDim
                font.pixelSize: Math.round(9 * root.s)
                font.bold: true
                font.letterSpacing: Math.round(0.8 * root.s)
            }
            Rectangle {
                id: closeBtn
                anchors.right: parent.right
                anchors.rightMargin: root.pad
                anchors.verticalCenter: parent.verticalCenter
                width: Math.round(22 * root.s)
                height: width
                radius: width / 2
                color: closeHover.hovered ? Qt.rgba(root.theme.text.r, root.theme.text.g, root.theme.text.b, 0.10) : "transparent"
                Label {
                    anchors.centerIn: parent
                    text: "✕"
                    color: root.theme.textDim
                    font.pixelSize: Math.round(11 * root.s)
                }
                HoverHandler { id: closeHover; cursorShape: Qt.PointingHandCursor }
                TapHandler { onTapped: root.close() }
            }
        }

        // Footer: the selection count, the desktop dialog link, Cancel and Attach.
        Item {
            id: footer
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: Math.round(56 * root.s)

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 1
                color: root.theme.border
            }
            Label {
                id: countLabel
                anchors.left: parent.left
                anchors.leftMargin: root.pad
                anchors.verticalCenter: parent.verticalCenter
                text: root.selectedKeys.length === 0 ? qsTr("Select files to attach")
                                                     : qsTr("%1 selected").arg(root.selectedKeys.length)
                color: root.selectedKeys.length === 0 ? root.theme.textDim : root.theme.text
                font.pixelSize: Math.round(11 * root.s)
            }
            Label {
                id: otherLink
                anchors.left: countLabel.right
                anchors.leftMargin: Math.round(16 * root.s)
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Other location…")
                color: root.theme.accent
                font.pixelSize: Math.round(10 * root.s)
                HoverHandler { cursorShape: Qt.PointingHandCursor }
                TapHandler { onTapped: root.systemDialogRequested() }
            }
            Label {
                id: allLink
                anchors.left: otherLink.right
                anchors.leftMargin: Math.round(14 * root.s)
                anchors.verticalCenter: parent.verticalCenter
                visible: root.fileCount > 0
                text: root.selectedKeys.length >= root.fileCount && root.fileCount > 0 ? qsTr("Clear selection")
                                                                                      : qsTr("Select all")
                color: root.theme.accent
                font.pixelSize: Math.round(10 * root.s)
                HoverHandler { cursorShape: Qt.PointingHandCursor }
                TapHandler {
                    onTapped: {
                        if (root.selectedKeys.length >= root.fileCount)
                            root.selectedKeys = []
                        else
                            root.selectRange(0, root.visibleEntries.length - 1)
                    }
                }
            }
            Row {
                anchors.right: parent.right
                anchors.rightMargin: root.pad
                anchors.verticalCenter: parent.verticalCenter
                spacing: Math.round(8 * root.s)
                PillButton {
                    theme: root.theme
                    s: root.s
                    text: qsTr("Cancel")
                    tint: root.theme.textDim
                    onClicked: root.close()
                }
                PillButton {
                    theme: root.theme
                    s: root.s
                    text: root.selectedKeys.length > 0 ? qsTr("Attach %1").arg(root.selectedKeys.length) : qsTr("Attach")
                    tint: root.theme.accent
                    opacity: root.selectedKeys.length > 0 ? 1 : 0.4
                    onClicked: root.attach()
                }
            }
        }

        // Places.
        Rectangle {
            id: sidebar
            anchors.left: parent.left
            anchors.top: titleBar.bottom
            anchors.bottom: footer.top
            width: Math.round(176 * root.s)
            color: Qt.rgba(root.theme.text.r, root.theme.text.g, root.theme.text.b, 0.025)

            ListView {
                anchors.fill: parent
                anchors.margins: Math.round(8 * root.s)
                model: root.places
                spacing: Math.round(2 * root.s)
                clip: true
                boundsBehavior: ListView.StopAtBounds
                delegate: Rectangle {
                    required property var modelData
                    readonly property bool current: root.currentPath === String(modelData.path)
                    width: ListView.view.width
                    height: Math.round(30 * root.s)
                    radius: Math.round(7 * root.s * config.radiusScale)
                    color: current ? Qt.rgba(root.theme.accent.r, root.theme.accent.g, root.theme.accent.b, 0.14)
                                   : placeHover.hovered ? Qt.rgba(root.theme.text.r, root.theme.text.g, root.theme.text.b, 0.06)
                                   : "transparent"
                    Label {
                        id: placeIcon
                        anchors.left: parent.left
                        anchors.leftMargin: Math.round(8 * root.s)
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.round(14 * root.s)
                        text: modelData.icon
                        color: current ? root.theme.accent : root.theme.textDim
                        font.pixelSize: Math.round(11 * root.s)
                        horizontalAlignment: Text.AlignHCenter
                    }
                    Label {
                        anchors.left: placeIcon.right
                        anchors.leftMargin: Math.round(8 * root.s)
                        anchors.right: parent.right
                        anchors.rightMargin: Math.round(6 * root.s)
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.label
                        color: current ? root.theme.accent : root.theme.text
                        font.pixelSize: Math.round(11 * root.s)
                        font.bold: current
                        elide: Text.ElideRight
                    }
                    HoverHandler { id: placeHover; cursorShape: Qt.PointingHandCursor }
                    TapHandler {
                        onTapped: {
                            root.selectedKeys = []
                            root.goTo(String(modelData.path))
                        }
                    }
                }
            }
        }

        // Main column: path bar, search, column headers and the list.
        Item {
            anchors.left: sidebar.right
            anchors.right: parent.right
            anchors.top: titleBar.bottom
            anchors.bottom: footer.top
            anchors.margins: root.pad

            // Path bar: up button and clickable breadcrumbs.
            Item {
                id: pathBar
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: Math.round(28 * root.s)

                Rectangle {
                    id: upBtn
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.round(26 * root.s)
                    height: width
                    radius: Math.round(7 * root.s * config.radiusScale)
                    color: upHover.hovered ? Qt.rgba(root.theme.text.r, root.theme.text.g, root.theme.text.b, 0.08) : "transparent"
                    border.width: 1
                    border.color: root.theme.border
                    Label {
                        anchors.centerIn: parent
                        text: "↑"
                        color: root.theme.text
                        font.pixelSize: Math.round(13 * root.s)
                    }
                    HoverHandler { id: upHover; cursorShape: Qt.PointingHandCursor }
                    TapHandler { onTapped: root.goTo(root.parentPath(root.currentPath)) }
                }
                Flickable {
                    anchors.left: upBtn.right
                    anchors.leftMargin: Math.round(8 * root.s)
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    height: parent.height
                    contentWidth: crumbRow.width
                    contentHeight: height
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    Row {
                        id: crumbRow
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: Math.round(4 * root.s)
                        Repeater {
                            model: root.crumbs
                            delegate: Item {
                                required property var modelData
                                required property int index
                                readonly property bool last: index === root.crumbs.length - 1
                                readonly property real sepW: index > 0 ? sep.implicitWidth + Math.round(4 * root.s) : 0
                                width: sepW + crumbText.implicitWidth
                                height: crumbRow.height
                                Label {
                                    id: sep
                                    visible: index > 0
                                    text: "›"
                                    color: root.theme.textDim
                                    font.pixelSize: Math.round(12 * root.s)
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Label {
                                    id: crumbText
                                    x: parent.sepW
                                    text: modelData.label
                                    color: last ? root.theme.text : root.theme.textDim
                                    font.pixelSize: Math.round(11 * root.s)
                                    font.bold: last
                                    anchors.verticalCenter: parent.verticalCenter
                                    HoverHandler { cursorShape: Qt.PointingHandCursor }
                                    TapHandler {
                                        onTapped: {
                                            root.selectedKeys = []
                                            root.goTo(String(modelData.path))
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Search: filters the current folder by name.
            TextField {
                id: search
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: pathBar.bottom
                anchors.topMargin: Math.round(8 * root.s)
                height: Math.round(28 * root.s)
                placeholderText: qsTr("Search this folder…")
                placeholderTextColor: root.theme.textDim
                color: root.theme.text
                selectionColor: root.theme.accent
                selectedTextColor: root.theme.input
                font.pixelSize: Math.round(11 * root.s)
                leftPadding: Math.round(10 * root.s)
                rightPadding: Math.round(10 * root.s)
                background: Rectangle {
                    radius: Math.round(8 * root.s * config.radiusScale)
                    color: Qt.rgba(root.theme.text.r, root.theme.text.g, root.theme.text.b, 0.05)
                    border.width: 1
                    border.color: search.activeFocus ? root.theme.accent : "transparent"
                }
                onTextChanged: root.cursor = 0
                Keys.onDownPressed: list.forceActiveFocus()
                onAccepted: list.forceActiveFocus()
            }

            // Column headers.
            Item {
                id: colHead
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: search.bottom
                anchors.topMargin: Math.round(10 * root.s)
                height: Math.round(16 * root.s)
                Label {
                    anchors.left: parent.left
                    anchors.leftMargin: Math.round(30 * root.s)
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("NAME")
                    color: root.theme.textDim
                    font.pixelSize: Math.round(8.5 * root.s)
                    font.bold: true
                    font.letterSpacing: Math.round(0.6 * root.s)
                }
                Label {
                    anchors.right: parent.right
                    anchors.rightMargin: Math.round(6 * root.s)
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("SIZE")
                    color: root.theme.textDim
                    font.pixelSize: Math.round(8.5 * root.s)
                    font.bold: true
                    font.letterSpacing: Math.round(0.6 * root.s)
                }
            }

            // The folder's rows. Keys are handled here, so the list always has the keyboard.
            ListView {
                id: list
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: colHead.bottom
                anchors.topMargin: Math.round(4 * root.s)
                anchors.bottom: parent.bottom
                clip: true
                focus: true
                model: root.visibleEntries
                boundsBehavior: ListView.StopAtBounds
                spacing: 1
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                Label {
                    anchors.centerIn: parent
                    visible: root.visibleEntries.length === 0
                    text: root.rawEntries.length > 0 && search.text !== ""
                          ? qsTr("Nothing matches the search")
                          : root.rawEntries.length > 0 ? "" : qsTr("This folder is empty")
                    color: root.theme.textDim
                    font.pixelSize: Math.round(11 * root.s)
                }

                Keys.onPressed: function (event) {
                    const n = root.visibleEntries.length
                    if (event.key === Qt.Key_Down || event.key === Qt.Key_Up) {
                        if (n > 0) {
                            const step = event.key === Qt.Key_Down ? 1 : -1
                            root.cursor = Math.max(0, Math.min(n - 1, root.cursor + step))
                            if (event.modifiers & Qt.ShiftModifier)
                                root.selectRange(root.cursor - step, root.cursor)
                            positionViewAtIndex(root.cursor, ListView.Contain)
                        }
                        event.accepted = true
                    } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                        root.activate(root.cursor)
                        event.accepted = true
                    } else if (event.key === Qt.Key_Space) {
                        const e = root.visibleEntries[root.cursor]
                        if (e && String(e.kind) === "file")
                            root.toggleKey(String(e.key))
                        event.accepted = true
                    } else if (event.key === Qt.Key_Backspace) {
                        root.goTo(root.parentPath(root.currentPath))
                        event.accepted = true
                    } else if (event.key === Qt.Key_A && (event.modifiers & Qt.ControlModifier)) {
                        root.selectRange(0, n - 1)
                        event.accepted = true
                    }
                }

                delegate: Rectangle {
                    id: row
                    required property var modelData
                    required property int index

                    readonly property bool isDir: String(modelData.kind) === "dir"
                    readonly property bool isFile: String(modelData.kind) === "file"
                    readonly property bool picked: isFile && root.isSelected(String(modelData.key))
                    readonly property bool highlighted: index === root.cursor
                    readonly property string extension: isFile && String(modelData.label).indexOf(".") >= 0
                                                        ? String(modelData.label).split(".").pop().toUpperCase() : ""

                    width: ListView.view.width
                    height: root.rowH
                    radius: Math.round(7 * root.s * config.radiusScale)
                    color: picked ? Qt.rgba(root.theme.accent.r, root.theme.accent.g, root.theme.accent.b, 0.16)
                         : highlighted ? Qt.rgba(root.theme.text.r, root.theme.text.g, root.theme.text.b, 0.07)
                         : "transparent"

                    // Check circle for files; a folder mark for folders.
                    Rectangle {
                        id: check
                        anchors.left: parent.left
                        anchors.leftMargin: Math.round(8 * root.s)
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.round(14 * root.s)
                        height: width
                        radius: width / 2
                        visible: row.isFile
                        color: row.picked ? root.theme.accent : "transparent"
                        border.width: 1
                        border.color: row.picked ? root.theme.accent : root.theme.textDim
                        Label {
                            anchors.centerIn: parent
                            visible: row.picked
                            text: "✓"
                            color: root.theme.input
                            font.pixelSize: Math.round(9 * root.s)
                            font.bold: true
                        }
                    }
                    Label {
                        anchors.left: parent.left
                        anchors.leftMargin: Math.round(8 * root.s)
                        anchors.verticalCenter: parent.verticalCenter
                        visible: row.isDir
                        width: Math.round(14 * root.s)
                        text: "▸"
                        color: root.theme.accent
                        font.pixelSize: Math.round(12 * root.s)
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Label {
                        id: nameLabel
                        anchors.left: parent.left
                        anchors.leftMargin: Math.round(32 * root.s)
                        anchors.right: tagLabel.left
                        anchors.rightMargin: Math.round(8 * root.s)
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.label
                        color: root.theme.text
                        font.pixelSize: Math.round(11.5 * root.s)
                        font.bold: row.isDir
                        elide: Text.ElideRight
                    }
                    Label {
                        id: tagLabel
                        anchors.right: sizeLabel.left
                        anchors.rightMargin: Math.round(12 * root.s)
                        anchors.verticalCenter: parent.verticalCenter
                        visible: row.extension !== ""
                        text: row.extension
                        color: root.theme.textDim
                        font.pixelSize: Math.round(8 * root.s)
                        font.bold: true
                        font.letterSpacing: Math.round(0.6 * root.s)
                    }
                    Label {
                        id: sizeLabel
                        anchors.right: parent.right
                        anchors.rightMargin: Math.round(8 * root.s)
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.round(84 * root.s)
                        horizontalAlignment: Text.AlignRight
                        text: row.isFile ? String(modelData.sub) : ""
                        color: root.theme.textDim
                        font.pixelSize: Math.round(9.5 * root.s)
                        elide: Text.ElideLeft
                    }

                    HoverHandler {
                        id: rowHover
                        cursorShape: Qt.PointingHandCursor
                    }
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        // A click toggles a file in the selection, so several files can be picked one after another.
                        // Shift+click selects the range from the last clicked row.
                        onClicked: function (mouse) {
                            list.forceActiveFocus()
                            const from = root.cursor
                            root.cursor = row.index
                            if (row.isDir)
                                return
                            if (mouse.modifiers & Qt.ShiftModifier)
                                root.selectRange(from, row.index)
                            else
                                root.toggleKey(String(row.modelData.key))
                        }
                        onDoubleClicked: function (mouse) {
                            if (row.isDir) {
                                root.cursor = row.index
                                root.activate(row.index)
                            }
                        }
                    }
                }
            }
        }
    }
}
