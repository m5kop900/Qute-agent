import QtQuick

// Colors for the overlay. The resolution rules (system palette, light/dark mode, accent precedence)
// live in the C++ Theme class (src/core/theme.cpp), shared with the units tests; this object only
// exposes them to the QML bindings. "theme" is the Theme instance set by main.cpp.
QtObject {
    readonly property bool dark: theme.dark
    readonly property color accent: theme.accent
    readonly property color panel: theme.panel
    readonly property color input: theme.input
    readonly property color text: theme.text
    readonly property color textDim: theme.textDim
    readonly property color border: theme.border
}
