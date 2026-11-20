import QtQuick
import QtQuick.Controls

// Read-only text the user can select with the mouse. Right-click opens a small menu: Copy (the selection, or the
// whole block when nothing is selected) and Select all. Clicking does not take keyboard focus from the prompt.
// mouseSelect: false when a parent layer does the selecting (MessageText selects across its parts).
TextEdit {
    id: root
    property var theme
    property bool mouseSelect: true

    readOnly: true
    selectByMouse: mouseSelect
    activeFocusOnPress: false
    persistentSelection: true
    wrapMode: TextEdit.Wrap
    selectionColor: theme ? Qt.rgba(theme.accent.r, theme.accent.g, theme.accent.b, 0.35) : "#4a90d9"

    TapHandler {
        enabled: root.mouseSelect
        acceptedButtons: Qt.RightButton
        onTapped: (eventPoint) => menu.openAt(root, eventPoint.position.x, eventPoint.position.y)
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
