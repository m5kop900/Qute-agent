import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Settings window (F7). Edits live in `config`; "Save" writes ~/.config/qute/config.json,
// which the running overlay picks up automatically. The palette comes from the shared C++ Theme
// (src/core/theme.*), the same object the overlay uses, so both apps look the same.
ApplicationWindow {
    id: win
    width: 880
    height: 700
    minimumWidth: 760
    minimumHeight: 540
    visible: true
    title: qsTr("Qute Agent Settings")
    color: theme.windowColor

    palette.window: theme.windowColor
    palette.windowText: theme.text
    palette.base: theme.baseColor
    palette.alternateBase: theme.alternateBase
    palette.text: theme.text
    palette.button: theme.buttonColor
    palette.buttonText: theme.buttonText
    palette.highlight: theme.accent
    palette.highlightedText: theme.highlightedText
    palette.brightText: "#ffffff"
    palette.mid: theme.border
    palette.midlight: theme.border
    palette.dark: theme.border
    palette.shadow: theme.border
    palette.placeholderText: theme.textDim
    palette.toolTipBase: theme.panel
    palette.toolTipText: theme.text

    property string page: "general"
    property string status: ""
    property bool dirty: false          // true when the edits have not been written to config.json yet
    Component.onCompleted: dirty = false
    readonly property var pageModel: [
        { key: "general",    label: qsTr("General"),            glyph: "\u2699" },
        { key: "appearance", label: qsTr("Appearance"),         glyph: "\u2726" },
        { key: "opencode",   label: qsTr("opencode"),           glyph: "\u2318" },
        { key: "session",    label: qsTr("Session & Terminal"), glyph: "\u25a3" },
        { key: "context",    label: qsTr("Context"),            glyph: "\u25d4" },
        { key: "shortcuts",  label: qsTr("Shortcuts"),          glyph: "\u2328" },
        { key: "about",      label: qsTr("About"),              glyph: "\u24d8" }
    ]

    Connections {                       // keep the sprite preview in sync; any edit makes the form dirty
        target: config
        function onChanged() {
            packs.load(config.packId, config.variant)
            win.dirty = true
        }
    }

    // ---- navigation ------------------------------------------------------------------------------------
    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.preferredWidth: 205
            Layout.fillHeight: true
            color: theme.buttonColor

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 3

                Label {
                    Layout.fillWidth: true
                    Layout.margins: 8
                    text: qsTr("Qute Agent")
                    font.bold: true
                    font.pixelSize: 18
                    color: theme.text
                }

                Repeater {
                    model: win.pageModel
                    delegate: Button {
                        Layout.fillWidth: true
                        text: modelData.glyph + "   " + modelData.label
                        checkable: true
                        checked: win.page === modelData.key
                        onClicked: { win.page = modelData.key; win.status = "" }
                    }
                }

                Item { Layout.fillHeight: true }

                Label {
                    Layout.fillWidth: true
                    Layout.margins: 8
                    wrapMode: Text.Wrap
                    text: win.dirty ? qsTr("\u25cf Unsaved changes") : qsTr("All changes saved")
                    font.pixelSize: 11
                    font.bold: win.dirty
                    color: win.dirty ? theme.warning : theme.textDim
                }

                Label {
                    Layout.fillWidth: true
                    Layout.margins: 8
                    text: qsTr("config v%1").arg(config.configVersion)
                    font.pixelSize: 10
                    color: theme.textDim
                }
            }
        }

        Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: theme.border }

        // ---- pages --------------------------------------------------------------------------------------
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 16
            spacing: 10

            ScrollView {
                id: scroller
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: availableWidth
                clip: true

                ColumnLayout {
                    width: scroller.availableWidth
                    spacing: 14

                    // ---- General ------------------------------------------------------------------------
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        visible: win.page === "general"

                        GroupBox {
                            title: qsTr("Window")
                            Layout.fillWidth: true
                            ColumnLayout {
                                anchors.fill: parent
                                CheckBox { text: qsTr("Open next to the mouse cursor"); checked: config.placeAtCursor; onToggled: config.placeAtCursor = checked }
                                CheckBox { text: qsTr("Hide when the window loses focus"); checked: config.hideOnFocusLoss; onToggled: config.hideOnFocusLoss = checked }
                            }
                        }

                        GroupBox {
                            title: qsTr("Behavior")
                            Layout.fillWidth: true
                            ColumnLayout {
                                anchors.fill: parent
                                CheckBox {
                                    text: qsTr("Animate the mascot (summon, state changes, idle)")
                                    checked: config.animations
                                    onToggled: config.animations = checked
                                }
                                Label {
                                    Layout.fillWidth: true
                                    wrapMode: Text.Wrap
                                    font.pixelSize: 11
                                    color: theme.textDim
                                    text: qsTr("Turning animations off saves CPU and leaves the mascot on a still frame. Automatic compaction lives on the Context page.")
                                }
                            }
                        }
                    }

                    // ---- Appearance ---------------------------------------------------------------------
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        visible: win.page === "appearance"

                        // ---- Live preview: shows the current colors and fonts before you save -----------------
                        GroupBox {
                            title: qsTr("Preview")
                            Layout.fillWidth: true

                            Rectangle {
                                anchors.fill: parent
                                implicitHeight: previewCol.implicitHeight + 24
                                radius: config.cornerRadius
                                color: theme.panel
                                border.width: 1
                                border.color: theme.border

                                ColumnLayout {
                                    id: previewCol
                                    anchors.fill: parent
                                    anchors.margins: 12
                                    spacing: 8

                                    Rectangle {                    // user prompt bubble
                                        Layout.alignment: Qt.AlignRight
                                        radius: Math.min(height / 2, config.cornerRadius)
                                        color: Qt.rgba(theme.accent.r, theme.accent.g, theme.accent.b, 0.18)
                                        border.width: 1
                                        border.color: theme.accent
                                        implicitWidth: promptText.implicitWidth + 24
                                        implicitHeight: promptText.implicitHeight + 12
                                        Text {
                                            id: promptText
                                            anchors.centerIn: parent
                                            text: qsTr("Refactor this function")
                                            color: theme.text
                                            font.family: theme.family
                                            font.pixelSize: 13
                                        }
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        wrapMode: Text.Wrap
                                        text: qsTr("Sure, here is the change. The sprite switches to the working state.")
                                        color: theme.text
                                        font.family: theme.family
                                        font.pixelSize: 13
                                    }

                                    Rectangle {                    // code block
                                        Layout.fillWidth: true
                                        radius: 8
                                        color: theme.codeBackground
                                        implicitHeight: codeText.implicitHeight + 16
                                        Text {
                                            id: codeText
                                            anchors.fill: parent
                                            anchors.margins: 8
                                            textFormat: Text.RichText
                                            text: "const name = <font color='" + theme.syntaxString + "'>\"qute\"</font>;<br>"
                                                  + "return <font color='" + theme.syntaxNumber + "'>42</font> * 2;"
                                            color: theme.text
                                            font.family: theme.codeFamily
                                            font.pixelSize: 12
                                        }
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8
                                        Rectangle {
                                            Layout.preferredWidth: 8; Layout.preferredHeight: 8
                                            radius: 4
                                            color: theme.success
                                        }
                                        Text {
                                            text: qsTr("done")
                                            color: theme.textDim
                                            font.family: theme.family
                                            font.pixelSize: 11
                                        }
                                        Item { Layout.fillWidth: true }
                                        Text {
                                            text: qsTr("Error state")
                                            color: theme.danger
                                            font.family: theme.family
                                            font.pixelSize: 11
                                        }
                                    }
                                }
                            }
                        }

                        // ---- Colors -------------------------------------------------------------------------
                        GroupBox {
                            title: qsTr("Colors")
                            Layout.fillWidth: true
                            GridLayout {
                                columns: 2
                                anchors.fill: parent
                                columnSpacing: 12
                                rowSpacing: 10

                                Label { text: qsTr("Color scheme") }
                                ComboBox {
                                    Layout.fillWidth: true
                                    model: [qsTr("Follow system"), qsTr("Light"), qsTr("Dark")]
                                    readonly property var values: ["system", "light", "dark"]
                                    currentIndex: Math.max(0, values.indexOf(config.themeMode))
                                    onActivated: (i) => config.themeMode = values[i]
                                }

                                Label { text: qsTr("Accent color") }
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8
                                    TextField {
                                        Layout.fillWidth: true
                                        placeholderText: qsTr("empty = system accent, e.g. #6ea8ff")
                                        text: config.accentColor
                                        onEditingFinished: config.accentColor = text.trim()
                                    }
                                    Rectangle {
                                        Layout.preferredWidth: 24; Layout.preferredHeight: 24
                                        radius: 6
                                        color: theme.accent
                                        border.width: 1
                                        border.color: theme.border
                                    }
                                    Button { text: qsTr("Use system"); onClicked: config.accentColor = "" }
                                }

                                Item { }
                                Row {
                                    spacing: 6
                                    Repeater {
                                        model: ["#6ea8ff", "#9b7bff", "#3ecf8e", "#ff9ccc", "#f5a524", "#e5484d"]
                                        delegate: Rectangle {
                                            required property string modelData
                                            width: 22; height: 22; radius: 11
                                            color: modelData
                                            border.width: config.accentColor.toLowerCase() === modelData ? 2 : 1
                                            border.color: config.accentColor.toLowerCase() === modelData ? theme.text : theme.border
                                            MouseArea {
                                                anchors.fill: parent
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: config.accentColor = modelData
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // ---- Fonts --------------------------------------------------------------------------
                        GroupBox {
                            title: qsTr("Fonts")
                            Layout.fillWidth: true
                            GridLayout {
                                columns: 2
                                anchors.fill: parent
                                columnSpacing: 12
                                rowSpacing: 10

                                readonly property var families: [qsTr("System default")].concat(Qt.fontFamilies())

                                Label { text: qsTr("Interface font") }
                                ComboBox {
                                    id: uiFontBox
                                    Layout.fillWidth: true
                                    model: parent.families
                                    currentIndex: config.fontFamily === "" ? 0 : Math.max(0, parent.families.indexOf(config.fontFamily))
                                    onActivated: (i) => config.fontFamily = i === 0 ? "" : parent.families[i]
                                }

                                Label { text: qsTr("Code font") }
                                ComboBox {
                                    id: codeFontBox
                                    Layout.fillWidth: true
                                    model: parent.families
                                    currentIndex: config.codeFontFamily === "" ? 0 : Math.max(0, parent.families.indexOf(config.codeFontFamily))
                                    onActivated: (i) => config.codeFontFamily = i === 0 ? "" : parent.families[i]
                                }

                                Item { }
                                Label {
                                    Layout.fillWidth: true
                                    wrapMode: Text.Wrap
                                    font.pixelSize: 11
                                    color: theme.textDim
                                    text: qsTr("System default follows your desktop font settings. The overlay updates live.")
                                }
                            }
                        }

                        // ---- Qt style -----------------------------------------------------------------------
                        GroupBox {
                            title: qsTr("Qt style")
                            Layout.fillWidth: true
                            GridLayout {
                                columns: 2
                                anchors.fill: parent
                                columnSpacing: 12
                                rowSpacing: 10

                                Label { text: qsTr("Controls style") }
                                ComboBox {
                                    Layout.fillWidth: true
                                    model: [qsTr("Custom (recommended)"), qsTr("System (your desktop's Qt style)"), qsTr("Fusion")]
                                    readonly property var values: ["custom", "system", "Fusion"]
                                    currentIndex: Math.max(0, values.indexOf(config.controlStyle))
                                    onActivated: (i) => config.controlStyle = values[i]
                                }

                                Item { }
                                Label {
                                    Layout.fillWidth: true
                                    wrapMode: Text.Wrap
                                    font.pixelSize: 11
                                    color: theme.textDim
                                    text: qsTr("Palette, fonts and accent always come from the theme above. This choice affects built-in widgets and needs a restart of the overlay (qute quit, then qute --daemon).")
                                }
                            }
                        }

                        GroupBox {
                            title: qsTr("Overlay")
                            Layout.fillWidth: true
                            GridLayout {
                                columns: 3
                                anchors.fill: parent
                                columnSpacing: 12

                                Label { text: qsTr("Scale") }
                                Slider {
                                    Layout.fillWidth: true
                                    from: 0.6; to: 2.0; stepSize: 0.05
                                    value: config.scale
                                    onMoved: config.scale = value
                                }
                                Label { text: config.scale.toFixed(2) + "x" }

                                Label { text: qsTr("Opacity") }
                                Slider {
                                    Layout.fillWidth: true
                                    from: 0.3; to: 1.0; stepSize: 0.05
                                    value: config.overlayOpacity
                                    onMoved: config.overlayOpacity = value
                                }
                                Label { text: Math.round(config.overlayOpacity * 100) + " %" }

                                Label { text: qsTr("Corner radius") }
                                Slider {
                                    Layout.fillWidth: true
                                    from: 0; to: 40; stepSize: 1
                                    value: config.cornerRadius
                                    onMoved: config.cornerRadius = Math.round(value)
                                }
                                Label { text: config.cornerRadius + " px" }

                                Label { text: qsTr("Density") }
                                ComboBox {
                                    Layout.fillWidth: true
                                    model: ["compact", "normal", "spacious"]
                                    currentIndex: Math.max(0, model.indexOf(config.density))
                                    onActivated: (i) => config.density = model[i]
                                }
                                Label { text: "" }
                            }
                        }

                        GroupBox {
                            title: qsTr("Sprite pack")
                            Layout.fillWidth: true
                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 10

                                RowLayout {
                                    spacing: 14

                                    Rectangle {
                                        Layout.preferredWidth: 104
                                        Layout.preferredHeight: 104
                                        radius: 12
                                        color: theme.panel
                                        border.width: 1
                                        border.color: theme.border

                                        AnimatedSprite {
                                            id: preview
                                            anchors.centerIn: parent
                                            readonly property var info: (packs.revision, packs.stateInfo("idle"))
                                            width: 88; height: 88
                                            source: info.source
                                            frameCount: info.frames
                                            frameWidth: packs.frameWidth
                                            frameHeight: packs.frameHeight
                                            frameRate: info.fps
                                            running: true
                                        }
                                    }

                                    GridLayout {
                                        Layout.fillWidth: true
                                        columns: 2
                                        columnSpacing: 12

                                        Label { text: qsTr("Character") }
                                        ComboBox {
                                            Layout.fillWidth: true
                                            model: packs.availablePacks
                                            textRole: "name"
                                            currentIndex: { for (var i = 0; i < model.length; ++i) if (model[i].id === config.packId) return i; return 0 }
                                            onActivated: (i) => {
                                                config.packId = model[i].id
                                                var v = model[i].variants
                                                if (v.length && v.indexOf(config.variant) < 0)
                                                    config.variant = v[0]
                                            }
                                        }

                                        Label { text: qsTr("Color variant") }
                                        ComboBox {
                                            Layout.fillWidth: true
                                            model: packs.variantsOf(config.packId)
                                            currentIndex: Math.max(0, model.indexOf(config.variant))
                                            onActivated: (i) => config.variant = model[i]
                                        }
                                    }
                                }

                                RowLayout {
                                    Button { text: qsTr("Open packs folder"); onClicked: packs.openPacksFolder() }
                                    Button {
                                        text: qsTr("Rescan")
                                        onClicked: { packs.rescan(); packs.load(config.packId, config.variant) }
                                    }
                                    Button {
                                        text: qsTr("Validate this pack")
                                        onClicked: {
                                            const r = packs.validatePack(config.packId)
                                            let lines = []
                                            if (r.ok)
                                                lines.push(qsTr("No problems found in %1.").arg(r.path))
                                            for (let i = 0; i < r.errors.length; ++i)
                                                lines.push(qsTr("Error: %1").arg(r.errors[i]))
                                            for (let i = 0; i < r.warnings.length; ++i)
                                                lines.push(qsTr("Warning: %1").arg(r.warnings[i]))
                                            validationDialog.report = lines.join("\n")
                                            validationDialog.open()
                                        }
                                    }
                                    Button {
                                        text: qsTr("Validate user packs")
                                        onClicked: {
                                            const list = packs.validateUserPacks()
                                            let lines = []
                                            if (list.length === 0) {
                                                lines.push(qsTr("No packs in %1.").arg(packs.packsFolder()))
                                            }
                                            for (let i = 0; i < list.length; ++i) {
                                                const r = list[i]
                                                lines.push((r.ok ? qsTr("OK: ") : qsTr("FAILED: ")) + r.name)
                                                for (let j = 0; j < r.errors.length; ++j)
                                                    lines.push(qsTr("  Error: %1").arg(r.errors[j]))
                                                for (let j = 0; j < r.warnings.length; ++j)
                                                    lines.push(qsTr("  Warning: %1").arg(r.warnings[j]))
                                            }
                                            validationDialog.report = lines.join("\n")
                                            validationDialog.open()
                                        }
                                    }
                                    Item { Layout.fillWidth: true }
                                }

                                Label {
                                    Layout.fillWidth: true
                                    wrapMode: Text.Wrap
                                    font.pixelSize: 11
                                    color: theme.textDim
                                    text: qsTr("Put custom packs in %1 (one folder per pack with manifest.json). Earlier search paths win on duplicate ids.")
                                              .arg(packs.packsFolder())
                                }
                            }
                        }
                    }

                    // ---- opencode -----------------------------------------------------------------------
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        visible: win.page === "opencode"

                        GroupBox {
                            title: qsTr("Server")
                            Layout.fillWidth: true
                            GridLayout {
                                columns: 2
                                anchors.fill: parent
                                columnSpacing: 12

                                Label { text: qsTr("Server URL") }
                                TextField { Layout.fillWidth: true; text: config.serverUrl; onEditingFinished: config.serverUrl = text }
                                Label { text: qsTr("Username") }
                                TextField { Layout.fillWidth: true; text: config.serverUsername; onEditingFinished: config.serverUsername = text }
                                Label { text: qsTr("Password") }
                                TextField { Layout.fillWidth: true; echoMode: TextInput.Password; text: config.serverPassword; onEditingFinished: config.serverPassword = text }
                                Label { text: qsTr("API generation") }
                                ComboBox {
                                    Layout.fillWidth: true
                                    model: ["auto", "v1", "v2"]
                                    currentIndex: Math.max(0, model.indexOf(config.apiFlavor))
                                    onActivated: (i) => config.apiFlavor = model[i]
                                }
                            }
                        }

                        GroupBox {
                            title: qsTr("Process")
                            Layout.fillWidth: true
                            GridLayout {
                                columns: 2
                                anchors.fill: parent
                                columnSpacing: 12

                                Label { text: qsTr("Auto-start server") }
                                CheckBox { checked: config.autoStartServer; onToggled: config.autoStartServer = checked }
                                Label { text: qsTr("opencode command") }
                                TextField { Layout.fillWidth: true; text: config.serverCommand; onEditingFinished: config.serverCommand = text }
                                Label { text: qsTr("Project folder") }
                                TextField { Layout.fillWidth: true; placeholderText: qsTr("empty = server default"); text: config.workDir; onEditingFinished: config.workDir = text }
                            }
                        }

                        GroupBox {
                            title: qsTr("Setup")
                            Layout.fillWidth: true
                            ColumnLayout {
                                anchors.fill: parent
                                Label {
                                    Layout.fillWidth: true
                                    wrapMode: Text.Wrap
                                    text: qsTr("The terminal wizard configures opencode, providers and MCP servers:\n\n`qute opencode setup`\n\nChecks: `qute opencode doctor`")
                                    textFormat: Text.MarkdownText
                                }
                            }
                        }
                    }

                    // ---- Session & Terminal -------------------------------------------------------------
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        visible: win.page === "session"

                        GroupBox {
                            title: qsTr("Session")
                            Layout.fillWidth: true
                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 8

                                Label {
                                    Layout.fillWidth: true
                                    wrapMode: Text.Wrap
                                    font.pixelSize: 11
                                    color: theme.textDim
                                    text: qsTr("Qute resumes the last session on start. Clear the id to start a new session next time.")
                                }
                                RowLayout {
                                    TextField {
                                        Layout.fillWidth: true
                                        placeholderText: qsTr("last session id")
                                        text: config.lastSessionId
                                        onEditingFinished: config.lastSessionId = text
                                    }
                                    Button { text: qsTr("Clear"); onClicked: config.lastSessionId = "" }
                                }
                                Label {
                                    Layout.fillWidth: true
                                    wrapMode: Text.Wrap
                                    font.pixelSize: 11
                                    color: theme.textDim
                                    text: qsTr("Inside the overlay: /sessions switches session, /new starts a fresh one, /openinopencode opens this one in the real opencode TUI.")
                                }
                            }
                        }

                        GroupBox {
                            title: qsTr("Terminal")
                            Layout.fillWidth: true
                            ColumnLayout {
                                anchors.fill: parent
                                Label {
                                    Layout.fillWidth: true
                                    wrapMode: Text.Wrap
                                    text: qsTr("`/openinopencode` launches the opencode TUI attached to the current session. Qute auto-detects the terminal (konsole, gnome-terminal, xfce4-terminal, xterm, ...) and starts it in the session's folder, with an argv list, never a shell string.")
                                    textFormat: Text.MarkdownText
                                }
                            }
                        }
                    }

                    // ---- Context ------------------------------------------------------------------------
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        visible: win.page === "context"

                        GroupBox {
                            title: qsTr("Automatic compaction")
                            Layout.fillWidth: true
                            ColumnLayout {
                                anchors.fill: parent

                                CheckBox {
                                    text: qsTr("Compact the session automatically when the context fills up")
                                    checked: config.autoCompactEnabled
                                    onToggled: config.autoCompactEnabled = checked
                                }
                                RowLayout {
                                    enabled: config.autoCompactEnabled
                                    Label { text: qsTr("Compact at") }
                                    Slider {
                                        Layout.fillWidth: true
                                        from: 50; to: 95; stepSize: 5
                                        value: config.autoCompactThreshold
                                        onMoved: config.autoCompactThreshold = Math.round(value)
                                    }
                                    Label { text: config.autoCompactThreshold + " %" }
                                }
                                Label {
                                    Layout.fillWidth: true
                                    wrapMode: Text.Wrap
                                    font.pixelSize: 11
                                    color: theme.textDim
                                    text: qsTr("Qute asks the server to compact; there is no separate summarizer. The ring in the overlay shows the used context.")
                                }
                            }
                        }

                        GroupBox {
                            title: qsTr("Attachments")
                            Layout.fillWidth: true
                            GridLayout {
                                columns: 3
                                anchors.fill: parent
                                columnSpacing: 12

                                Label { text: qsTr("Maximum file size") }
                                SpinBox {
                                    Layout.fillWidth: true
                                    from: 0; to: 512
                                    value: config.maxAttachmentSizeMB
                                    onValueModified: config.maxAttachmentSizeMB = value
                                }
                                Label { text: qsTr("MB (0 = no limit)") }
                            }
                        }
                    }

                    // ---- Shortcuts ----------------------------------------------------------------------
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        visible: win.page === "shortcuts"

                        GroupBox {
                            title: qsTr("In the overlay")
                            Layout.fillWidth: true
                            Label {
                                anchors.fill: parent
                                wrapMode: Text.Wrap
                                font.pixelSize: 12
                                color: theme.text
                                textFormat: Text.MarkdownText
                                text: qsTr("- **Enter** send · **Shift+Enter** newline\n"
                                         + "- **Shift+Tab** switch build/plan mode\n"
                                         + "- **/** command list · **@** files and agents\n"
                                         + "- **Ctrl+V** paste an image · drop files onto the box\n"
                                         + "- **Esc** close a list, otherwise hide the overlay\n"
                                         + "- **Ctrl+C** copy selected text · **/copy** last answer\n"
                                         + "- **/sessions**, **/new**, **/models**, **/agents**, **/compact**, **/status**")
                            }
                        }

                        GroupBox {
                            title: qsTr("Show the overlay")
                            Layout.fillWidth: true
                            Label {
                                anchors.fill: parent
                                wrapMode: Text.Wrap
                                font.pixelSize: 12
                                color: theme.text
                                textFormat: Text.MarkdownText
                                text: qsTr("Qute Agent does not register a shortcut itself. Bind this command in your desktop:\n\n`qute toggle`\n\n"
                                         + "- **KDE Plasma:** System Settings → Keyboard → Shortcuts → Add Command\n"
                                         + "- **GNOME:** Settings → Keyboard → Custom Shortcuts\n"
                                         + "- **XFCE:** Settings → Keyboard → Application Shortcuts\n"
                                         + "- **Windows:** shortcut (.lnk) with a hotkey, or AutoHotkey\n"
                                         + "- **macOS:** Shortcuts app → Run Shell Script (`qute toggle`) → add a keyboard shortcut (or skhd / Hammerspoon / Raycast)\n\n"
                                         + "Autostart the daemon with `qute --daemon`.")
                            }
                        }
                    }

                    // ---- About --------------------------------------------------------------------------
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        visible: win.page === "about"

                        GroupBox {
                            title: qsTr("Qute Agent")
                            Layout.fillWidth: true
                            ColumnLayout {
                                anchors.fill: parent
                                Label {
                                    Layout.fillWidth: true
                                    wrapMode: Text.Wrap
                                    color: theme.text
                                    text: qsTr("Version %1 · C++20 + Qt 6 / QML\ndesktop sprite assistant for opencode")
                                              .arg(Qt.application.version)
                                }
                            }
                        }

                        GroupBox {
                            title: qsTr("Files")
                            Layout.fillWidth: true
                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 8
                                Label { text: qsTr("Config file"); color: theme.textDim; font.pixelSize: 11 }
                                Label { Layout.fillWidth: true; wrapMode: Text.Wrap; color: theme.text; text: config.configPath }
                                Label { text: qsTr("Sprite packs"); color: theme.textDim; font.pixelSize: 11 }
                                Label { Layout.fillWidth: true; wrapMode: Text.Wrap; color: theme.text; text: packs.packsFolder() }
                                RowLayout {
                                    Button { text: qsTr("Open packs folder"); onClicked: packs.openPacksFolder() }
                                    Button {
                                        text: qsTr("Open config folder")
                                        onClicked: Qt.openUrlExternally("file://" + config.configPath.substring(0, config.configPath.lastIndexOf("/")))
                                    }
                                    Item { Layout.fillWidth: true }
                                }
                                Label {
                                    Layout.fillWidth: true
                                    wrapMode: Text.Wrap
                                    font.pixelSize: 11
                                    color: theme.textDim
                                    text: qsTr("The config is versioned (v%1); unknown keys are preserved on save, so custom tools and future options survive an upgrade.")
                                              .arg(config.configVersion)
                                }
                            }
                        }
                    }
                }
            }

            // ---- save bar -------------------------------------------------------------------------------
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: theme.border
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    elide: Text.ElideRight
                    text: win.status
                    color: win.status.indexOf(qsTr("Could not")) === 0 ? theme.danger : theme.textDim
                    font.pixelSize: 11
                }
                Button {
                    text: qsTr("Restart overlay")
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Restart the overlay so that saved settings take effect")
                    onClicked: win.restartOverlay()
                }
                Button {
                    text: qsTr("Revert")
                    enabled: win.dirty
                    onClicked: {
                        config.load()
                        win.dirty = false
                        win.status = qsTr("Reloaded from %1").arg(config.configPath)
                    }
                }
                Button {
                    text: win.dirty ? qsTr("Save changes") : qsTr("Saved")
                    highlighted: win.dirty
                    onClicked: {
                        if (config.save()) {
                            win.dirty = false
                            win.status = qsTr("Saved.")
                            if (overlay.isRunning())
                                restartDialog.open()     // the overlay applies saved settings only after a restart
                        } else {
                            win.status = qsTr("Could not save")
                            saveError.open()
                        }
                    }
                }
            }
        }
    }

    // Shown after a save while the overlay runs: its settings are read at start-up.
    function restartOverlay() {
        const ok = overlay.restart()
        win.status = ok ? qsTr("Overlay restarted with the saved settings.")
                        : qsTr("Could not restart the overlay. Run \"qute quit\", then \"qute --daemon\" in a terminal.")
    }

    Dialog {
        id: restartDialog
        title: qsTr("Apply to the overlay")
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Yes | Dialog.No
        Component.onCompleted: {
            standardButton(Dialog.Yes).text = qsTr("Restart now")
            standardButton(Dialog.No).text = qsTr("Later")
        }
        onAccepted: win.restartOverlay()
        Label {
            width: Math.min(420, win.width - 120)
            wrapMode: Text.Wrap
            text: qsTr("The overlay applies its settings when it starts. Restart it now to apply the changes?")
            color: theme.text
        }
    }

    Dialog {
        id: saveError
        title: qsTr("Could not save")
        standardButtons: Dialog.Ok
        anchors.centerIn: parent
        Label { text: qsTr("Writing the config file failed. Check permissions of ~/.config/qute/") }
    }

    Dialog {
        id: validationDialog
        property string report: ""
        title: qsTr("Sprite pack check")
        modal: true
        anchors.centerIn: parent
        width: Math.min(640, win.width - 80)
        standardButtons: Dialog.Ok
        contentItem: ScrollView {
            implicitHeight: Math.min(420, reportLabel.implicitHeight + 8)
            Label {
                id: reportLabel
                width: validationDialog.availableWidth
                text: validationDialog.report
                wrapMode: Text.Wrap
                color: theme.text
            }
        }
    }
}
