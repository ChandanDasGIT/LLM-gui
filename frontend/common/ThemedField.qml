import QtQuick
import QtQuick.Controls.Basic

TextField {
    id: control
    implicitHeight: 28
    leftPadding: Theme.spaceSm
    rightPadding: Theme.spaceSm
    color: enabled ? Theme.text : Theme.textDisabled
    selectionColor: Theme.selectionBg
    placeholderTextColor: Theme.textDisabled
    font.family: Theme.monospaceFontFamily
    font.pixelSize: Theme.bodySize
    hoverEnabled: true

    background: Rectangle {
        radius: Theme.controlRadius
        color: !control.enabled ? Theme.fieldDisabled
             : control.hovered ? Theme.fieldHover : Theme.field
        border.color: control.activeFocus ? Theme.accent : Theme.fieldBorder
    }
}
