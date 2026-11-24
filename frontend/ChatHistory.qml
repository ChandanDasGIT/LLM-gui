import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// The sidebar: "+ New chat", then the saved chats grouped by day. Hovering a chat shows × to delete it.
Rectangle {
    id: root
    color: Theme.panelBg

    ChatHistoryWrap {
        id: history
        objectName: "chatHistoryWrap"
    }

    Rectangle {
        anchors.right: parent.right
        width: 1
        height: parent.height
        color: Theme.panelEdge
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spaceMd
        anchors.rightMargin: Theme.spaceMd + 1
        spacing: Theme.spaceMd

        Text {
            Layout.leftMargin: Theme.spaceXs
            textFormat: Text.StyledText
            text: "<font color='" + Theme.accent + "'>✻</font> LLM-gui"
            color: Theme.textStrong
            font.family: Theme.monospaceFontFamily
            font.pixelSize: Theme.headingSize
            font.bold: true
        }

        Rectangle {
            objectName: "newChatButton"
            Layout.fillWidth: true
            implicitHeight: 34
            radius: Theme.panelRadius
            color: newChatHover.hovered ? Theme.accentBg : "transparent"
            border.color: history.currentIndex < 0 ? Theme.accent : Theme.controlBorder
            Text {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: Theme.spaceMd
                text: "+  New chat"
                color: newChatHover.hovered || history.currentIndex < 0 ? Theme.accent : Theme.text
                font.family: Theme.monospaceFontFamily
                font.pixelSize: Theme.bodySize
            }
            HoverHandler { id: newChatHover; cursorShape: Qt.PointingHandCursor }
            TapHandler { onTapped: history.newChat() }
        }

        Text {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spaceSm
            visible: list.count === 0
            text: "Your chats will be listed here."
            color: Theme.textDisabled
            wrapMode: Text.Wrap
            font.family: Theme.monospaceFontFamily
            font.pixelSize: Theme.captionSize
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 2
            model: history
            boundsBehavior: Flickable.StopAtBounds
            ScrollIndicator.vertical: ScrollIndicator {}

            section.property: "day"
            section.delegate: Text {
                required property string section
                width: ListView.view.width
                topPadding: Theme.spaceMd
                bottomPadding: Theme.spaceXs
                leftPadding: Theme.spaceSm
                text: section.toUpperCase()
                color: Theme.textDisabled
                font.family: Theme.monospaceFontFamily
                font.pixelSize: Theme.captionSize
                font.letterSpacing: 1
            }

            delegate: Rectangle {
                id: chat
                required property string title
                required property string meta
                required property int index
                readonly property bool selected: history.currentIndex === index
                width: ListView.view.width
                height: 46
                radius: Theme.controlRadius
                color: selected ? Theme.controlBgActive : chatHover.hovered ? Theme.controlBgHover : "transparent"

                Rectangle {
                    visible: chat.selected
                    width: 2
                    height: parent.height - 16
                    anchors.verticalCenter: parent.verticalCenter
                    radius: 1
                    color: Theme.accent
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spaceSm + 4
                    anchors.rightMargin: chatHover.hovered ? 28 : Theme.spaceSm
                    spacing: 2
                    Item { Layout.fillHeight: true }
                    Text {
                        Layout.fillWidth: true
                        text: chat.title
                        color: chat.selected ? Theme.textStrong : Theme.text
                        elide: Text.ElideRight
                        font.family: Theme.monospaceFontFamily
                        font.pixelSize: Theme.smallSize
                    }
                    Text {
                        Layout.fillWidth: true
                        text: chat.meta
                        color: Theme.textDisabled
                        elide: Text.ElideRight
                        font.family: Theme.monospaceFontFamily
                        font.pixelSize: Theme.captionSize
                    }
                    Item { Layout.fillHeight: true }
                }

                HoverHandler { id: chatHover; cursorShape: Qt.PointingHandCursor }
                TapHandler { onTapped: history.select(chat.index) }

                Rectangle {
                    visible: chatHover.hovered
                    anchors.right: parent.right
                    anchors.rightMargin: Theme.spaceXs
                    anchors.verticalCenter: parent.verticalCenter
                    width: 22
                    height: 22
                    radius: Theme.controlRadius
                    color: deleteHover.hovered ? Theme.dangerBg : "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: "×"
                        color: deleteHover.hovered ? Theme.textStrong : Theme.textMuted
                        font.pixelSize: Theme.bodySize
                    }
                    HoverHandler { id: deleteHover }
                    TapHandler { onTapped: history.remove(chat.index) }
                    ToolTip.visible: deleteHover.hovered
                    ToolTip.text: "Delete this chat"
                    ToolTip.delay: 450
                }
            }
        }
    }
}
