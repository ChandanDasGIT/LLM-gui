import QtQuick
import QtQuick.Controls.Basic

ProgressBar {
    id: control
    implicitHeight: 6
    implicitWidth: 160

    background: Rectangle {
        radius: 3
        color: Theme.controlBg
        border.color: Theme.controlBorder
    }
    contentItem: Item {
        Rectangle {
            width: control.indeterminate ? parent.width : control.visualPosition * parent.width
            height: parent.height
            radius: 3
            color: Theme.accent
            opacity: control.indeterminate ? 0.4 : 1.0
        }
    }
}
