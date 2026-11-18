import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Settings window. Edits live in `config`; "Save" writes ~/.config/qute/config.json,
// which the running overlay picks up automatically.
ApplicationWindow {
    id: win
    width: 560
    height: 720
    visible: true
    title: qsTr("Qute Agent Settings")

    Connections {                       // keep the preview in sync with pack/variant edits
        target: config
        function onChanged() { packs.load(config.packId, config.variant) }
    }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 16
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 14

            GroupBox {
                title: qsTr("Appearance")
                Layout.fillWidth: true
                GridLayout {
                    columns: 2
                    anchors.fill: parent
                    columnSpacing: 12

                    Label { text: qsTr("Preview") }
                    AnimatedSprite {
                        id: preview
                        readonly property var info: (packs.revision, packs.stateInfo("idle"))
                        Layout.preferredWidth: 96; Layout.preferredHeight: 96
                        source: info.source; frameCount: info.frames
                        frameWidth: packs.frameWidth; frameHeight: packs.frameHeight
                        frameRate: info.fps; running: true
                    }

                    Label { text: qsTr("Sprite pack") }
                    ComboBox {
                        Layout.fillWidth: true
                        model: packs.availablePacks
                        textRole: "name"
                        currentIndex: { for (var i = 0; i < model.length; ++i) if (model[i].id === config.packId) return i; return 0 }
                        onActivated: (i) => { config.packId = model[i].id; var v = model[i].variants; if (v.length && v.indexOf(config.variant) < 0) config.variant = v[0] }
                    }

                    Label { text: qsTr("Color variant") }
                    ComboBox {
                        Layout.fillWidth: true
                        model: packs.variantsOf(config.packId)
                        currentIndex: Math.max(0, model.indexOf(config.variant))
                        onActivated: (i) => config.variant = model[i]
                    }

                    Label { text: qsTr("Theme") }
                    ComboBox {
                        Layout.fillWidth: true
                        model: ["system", "light", "dark"]
                        currentIndex: Math.max(0, model.indexOf(config.themeMode))
                        onActivated: (i) => config.themeMode = model[i]
                    }

                    Label { text: qsTr("Accent color") }
                    RowLayout {
                        TextField {
                            Layout.fillWidth: true
                            placeholderText: qsTr("empty = system accent, e.g. #6ea8ff")
                            text: config.accentColor
                            onEditingFinished: config.accentColor = text
                        }
                    }

                    Label { text: qsTr("Scale") }
                    RowLayout {
                        Slider { Layout.fillWidth: true; from: 0.6; to: 2.0; stepSize: 0.05; value: config.scale; onMoved: config.scale = value }
                        Label { text: config.scale.toFixed(2) + "x" }
                    }
                }
            }

            GroupBox {
                title: qsTr("opencode server")
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
                    Label { text: qsTr("Auto-start server") }
                    CheckBox { checked: config.autoStartServer; onToggled: config.autoStartServer = checked }
                    Label { text: qsTr("opencode command") }
                    TextField { Layout.fillWidth: true; text: config.serverCommand; onEditingFinished: config.serverCommand = text }
                    Label { text: qsTr("Project folder") }
                    TextField { Layout.fillWidth: true; placeholderText: qsTr("empty = server default"); text: config.workDir; onEditingFinished: config.workDir = text }
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
                title: qsTr("Behavior")
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    CheckBox { text: qsTr("Open next to the mouse cursor"); checked: config.placeAtCursor; onToggled: config.placeAtCursor = checked }
                    CheckBox { text: qsTr("Hide when the window loses focus"); checked: config.hideOnFocusLoss; onToggled: config.hideOnFocusLoss = checked }
                }
            }

            GroupBox {
                title: qsTr("Keyboard shortcut")
                Layout.fillWidth: true
                Label {
                    anchors.fill: parent
                    wrapMode: Text.Wrap
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

            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                Button { text: qsTr("Revert"); onClicked: config.load() }
                Button {
                    text: qsTr("Save")
                    highlighted: true
                    onClicked: { if (!config.save()) saveError.open() }
                }
            }
        }
    }

    Dialog {
        id: saveError
        title: qsTr("Could not save")
        standardButtons: Dialog.Ok
        anchors.centerIn: parent
        Label { text: qsTr("Writing the config file failed. Check permissions of ~/.config/qute/") }
    }
}
