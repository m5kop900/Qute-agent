import QtQuick

// Colors for the overlay. "system" follows the Qt platform theme (Breeze on KDE, etc.),
// "light"/"dark" are fixed palettes. cfg.accentColor ("" = system) overrides the accent.
QtObject {
    id: t
    property var cfg

    readonly property SystemPalette pal: SystemPalette { colorGroup: SystemPalette.Active }
    readonly property string mode: cfg ? cfg.themeMode : "system"
    readonly property bool useSystem: mode === "system"
    // Qt 6.5+: Qt.styleHints.colorScheme (Qt.Dark / Qt.Light / Qt.Unknown)
    readonly property bool dark: mode === "dark" || (useSystem && Qt.styleHints.colorScheme === Qt.Dark)

    readonly property color accent: (cfg && cfg.accentColor !== "") ? cfg.accentColor
                                    : (useSystem ? pal.highlight : "#6ea8ff")
    readonly property color panel: useSystem ? Qt.rgba(pal.window.r, pal.window.g, pal.window.b, 0.95)
                                   : (dark ? "#f21a1c2b" : "#f2f7f8fc")
    readonly property color input: useSystem ? pal.base : (dark ? "#262a3d" : "#ffffff")
    readonly property color text: useSystem ? pal.windowText : (dark ? "#e8eaf6" : "#1d2033")
    readonly property color textDim: Qt.rgba(text.r, text.g, text.b, 0.55)
    readonly property color border: Qt.rgba(text.r, text.g, text.b, 0.18)
}
