import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// The strip along the bottom: model · character on the left, connection and speed on the
// right. All text comes from StatusStripWrap.
Rectangle {
    id: root
    signal modelClicked()
    signal characterClicked()

    height: Theme.stripHeight
    color: Theme.panelBg

    StatusStripWrap {
        id: strip
        objectName: "statusStripWrap"
    }

    component Readout: Text {
        property string tip
        color: Theme.textMuted
        font.pixelSize: Theme.smallSize
        font.family: Theme.monospaceFontFamily
        Layout.alignment: Qt.AlignVCenter
        HoverHandler { id: readoutHover }
        ToolTip.visible: readoutHover.hovered && tip.length > 0
        ToolTip.text: tip
        ToolTip.delay: 450
    }
    component Separator: Rectangle {
        Layout.preferredWidth: 1
        Layout.preferredHeight: 12
        Layout.alignment: Qt.AlignVCenter
        color: Theme.divider
    }

    Rectangle {
        anchors.top: parent.top
        width: parent.width
        height: 1
        color: Theme.panelEdge
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.margin
        anchors.rightMargin: Theme.margin
        spacing: 10

        Readout {
            objectName: "statusModel"
            text: strip.modelText
            tip: "Model: click to manage models"
            color: modelHover.hovered ? Theme.textStrong : Theme.text
            HoverHandler { id: modelHover; cursorShape: Qt.PointingHandCursor }
            TapHandler { onTapped: root.modelClicked() }
        }
        Separator {}
        Readout {
            objectName: "statusCharacter"
            text: strip.characterText
            tip: "Character: click to manage characters"
            color: characterHover.hovered ? Theme.textStrong : Theme.textMuted
            HoverHandler { id: characterHover; cursorShape: Qt.PointingHandCursor }
            TapHandler { onTapped: root.characterClicked() }
        }
        Item { Layout.fillWidth: true }
        Readout {
            objectName: "statusSpeed"
            visible: text.length > 0
            text: strip.speedText
            tip: "Speed of the last reply"
        }
        Separator { visible: strip.speedText.length > 0 }
        Rectangle {
            Layout.alignment: Qt.AlignVCenter
            width: 7
            height: 7
            radius: 3.5
            color: strip.connected ? Theme.ok : strip.offline ? Theme.danger : Theme.warning
        }
        Readout {
            objectName: "statusConnection"
            text: strip.connectionText
            color: strip.offline ? Theme.danger : Theme.textMuted
        }
    }
}
