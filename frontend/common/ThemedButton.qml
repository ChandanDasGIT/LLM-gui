import QtQuick
import QtQuick.Controls.Basic

Button {
    id: control
    property bool accent: false
    property string tip

    implicitHeight: 26
    leftPadding: Theme.spaceSm
    rightPadding: Theme.spaceSm
    font.family: Theme.monospaceFontFamily
    font.pixelSize: Theme.smallSize
    hoverEnabled: true
    focusPolicy: Qt.NoFocus

    ToolTip.visible: hovered && tip.length > 0
    ToolTip.text: tip
    ToolTip.delay: 450

    contentItem: Text {
        text: control.text
        font: control.font
        color: !control.enabled ? Theme.textDisabled
             : control.accent ? Theme.accent
             : control.hovered ? Theme.textStrong : Theme.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        radius: Theme.controlRadius
        color: control.down ? Theme.controlBgActive
             : control.hovered ? Theme.controlBgHover
             : control.accent ? Theme.accentBg : Theme.controlBg
        border.color: control.accent && control.enabled ? Theme.accent : Theme.controlBorder
    }
}
