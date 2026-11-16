import QtQuick
import QtQuick.Layouts

// F0.3 skeleton: the slot above the prompt box where F1 renders attachment chips
// (icon/name/size + remove). Empty for now, so it collapses to zero height.
RowLayout {
    id: root
    property var theme
    property real s: 1.0
    property var attachments: []

    visible: attachments.length > 0
    spacing: Math.round(6 * s)
    implicitHeight: visible ? Math.round(26 * s) : 0
}
