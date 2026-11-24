import QtQuick
import QtQuick.Controls.Basic

// A dropdown. flat: no frame until hovered (for the prompt box's bottom row).
ComboBox {
    id: control
    property string tip

    implicitHeight: 26
    leftPadding: Theme.spaceSm
    rightPadding: Theme.spaceSm + 14
    font.family: Theme.monospaceFontFamily
    font.pixelSize: Theme.smallSize
    hoverEnabled: true
    focusPolicy: Qt.NoFocus

    ToolTip.visible: hovered && tip.length > 0 && !popup.visible
    ToolTip.text: tip
    ToolTip.delay: 450

    contentItem: Text {
        text: control.displayText
        font: control.font
        color: control.hovered || control.popup.visible ? Theme.textStrong : Theme.text
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    indicator: Text {
        x: control.width - width - Theme.spaceSm
        anchors.verticalCenter: parent.verticalCenter
        text: "▾"
        color: Theme.textMuted
        font.pixelSize: Theme.smallSize
    }
    background: Rectangle {
        radius: Theme.controlRadius
        color: control.popup.visible ? Theme.controlBgActive
             : control.hovered ? Theme.controlBgHover
             : control.flat ? "transparent" : Theme.field
        border.color: control.flat && !control.hovered && !control.popup.visible ? "transparent" : Theme.fieldBorder
    }

    delegate: ItemDelegate {
        id: option
        required property var model
        required property int index
        width: ListView.view.width
        height: 28
        hoverEnabled: true
        contentItem: Text {
            text: option.model[control.textRole]
            color: option.index === control.currentIndex ? Theme.accent : Theme.text
            font: control.font
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        background: Rectangle {
            radius: Theme.controlRadius
            color: option.hovered ? Theme.controlBgHover : "transparent"
        }
    }

    popup: Popup {
        y: control.height + 2
        width: Math.max(control.width, 200)
        implicitHeight: Math.min(contentItem.implicitHeight + 2 * padding, 320)
        padding: Theme.spaceXs
        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            boundsBehavior: Flickable.StopAtBounds
            ScrollIndicator.vertical: ScrollIndicator {}
        }
        background: Rectangle {
            radius: Theme.panelRadius
            color: Theme.popupBg
            border.color: Theme.popupBorder
        }
    }
}
