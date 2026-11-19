import QtQuick

// Colors and fonts for the overlay. The resolution rules (system palette, light/dark mode, accent
// precedence, font family) live in the C++ Theme class (src/core/theme.cpp), shared with the units
// tests; this object only exposes them to the QML bindings. "theme" is the Theme instance set by main.cpp.
QtObject {
    readonly property bool dark: theme.dark
    readonly property color accent: theme.accent
    readonly property color accentAlt: theme.accentAlt
    readonly property color panel: theme.panel
    readonly property color input: theme.input
    readonly property color text: theme.text
    readonly property color textDim: theme.textDim
    readonly property color border: theme.border
    readonly property color danger: theme.danger
    readonly property color warning: theme.warning
    readonly property color success: theme.success
    readonly property color codeBackground: theme.codeBackground
    readonly property color syntaxString: theme.syntaxString
    readonly property color syntaxNumber: theme.syntaxNumber
    readonly property string family: theme.family
    readonly property string codeFamily: theme.codeFamily
}
