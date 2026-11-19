import QtQuick
import QtQuick.Controls

// Read-only text the user can select with the mouse. Right-click opens a small menu: Copy (the selection, or the
// whole block when nothing is selected) and Select all. Clicking does not take keyboard focus from the prompt.
TextEdit {
    id: root
    property var theme

    readOnly: true
    selectByMouse: true
    activeFocusOnPress: false
    persistentSelection: true
    wrapMode: TextEdit.Wrap
    selectionColor: theme ? Qt.rgba(theme.accent.r, theme.accent.g, theme.accent.b, 0.35) : "#4a90d9"

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: (eventPoint) => {
            menu.x = eventPoint.position.x
            menu.y = eventPoint.position.y
            menu.open()
        }
    }

    ThemedMenu {
        id: menu
        theme: root.theme
        actions: [
            {text: qsTr("Copy"), enabled: true, run: function () {
                if (root.selectedText.length === 0)
                    root.selectAll()
                root.copy()
            }},
            {text: qsTr("Select all"), enabled: true, run: function () { root.selectAll() }}
        ]
    }
}
