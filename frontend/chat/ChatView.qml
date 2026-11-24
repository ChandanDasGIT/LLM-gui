import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts

// The chat: the new-chat screen or the transcript in a terminal style, with the prompt box
// under it. Images can be dropped on it when the model can see them.
Item {
    id: root
    signal openModelsRequested()
    signal openCharactersRequested()

    readonly property int columnWidth: 680

    ChatWrap {
        id: chat
        objectName: "chatWrap"
        onOpenModelsRequested: root.openModelsRequested()
        onOpenCharactersRequested: root.openCharactersRequested()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: Theme.spaceLg
        anchors.bottomMargin: Theme.spaceMd
        spacing: Theme.spaceMd

        // ---- transcript or new-chat screen, centred with a readable width ----
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: transcript
                objectName: "transcript"
                visible: !chat.isNewChat
                anchors.fill: parent
                clip: true
                model: chat
                spacing: 0
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {}

                // Follows the newest text while the view sits at the bottom.
                property bool following: true
                onMovementEnded: following = atYEnd
                onContentHeightChanged: if (following) positionViewAtEnd()
                onCountChanged: { following = true; positionViewAtEnd() }

                delegate: MessageBlock {
                    width: ListView.view.width
                    columnWidth: root.columnWidth
                }
            }

            Flickable {
                visible: chat.isNewChat
                anchors.fill: parent
                clip: true
                contentHeight: Math.max(height, newChatPanel.implicitHeight)
                boundsBehavior: Flickable.StopAtBounds
                NewChatPanel {
                    id: newChatPanel
                    width: Math.min(parent.width - 2 * Theme.spaceLg, root.columnWidth)
                    height: Math.max(implicitHeight, parent.height)
                    x: (parent.width - width) / 2
                    models: chat.models
                    characters: chat.characters
                    modelIndex: chat.modelIndex
                    characterIndex: chat.characterIndex
                    onModelPicked: (index) => chat.pickModel(index)
                    onCharacterPicked: (index) => chat.pickCharacter(index)
                    onManageModelsRequested: root.openModelsRequested()
                    onManageCharactersRequested: root.openCharactersRequested()
                }
            }
        }

        PromptBox {
            id: prompt
            Layout.fillWidth: true
            Layout.maximumWidth: root.columnWidth
            Layout.alignment: Qt.AlignHCenter
            Layout.leftMargin: Theme.spaceLg
            Layout.rightMargin: Theme.spaceLg
            models: chat.models
            characters: chat.characters
            modelIndex: chat.modelIndex
            characterIndex: chat.characterIndex
            busy: chat.busy
            canAttach: chat.canAttach
            pendingImages: chat.pendingImages
            onModelPicked: (index) => chat.pickModel(index)
            onCharacterPicked: (index) => chat.pickCharacter(index)
            onSendRequested: (text) => { if (chat.send(text)) prompt.clearInput() }
            onStopRequested: chat.stop()
            onAttachClicked: imagePicker.open()
            onRemoveAttachment: (index) => chat.removeAttachment(index)
        }
    }

    DropArea {
        anchors.fill: parent
        enabled: chat.canAttach
        onDropped: (drop) => chat.attach(drop.urls)
        Rectangle {
            anchors.fill: parent
            visible: parent.containsDrag
            color: "transparent"
            border.color: Theme.accent
            border.width: 2
            radius: Theme.panelRadius
        }
    }

    FileDialog {
        id: imagePicker
        title: "Attach images"
        fileMode: FileDialog.OpenFiles
        nameFilters: ["Images (*.png *.jpg *.jpeg *.webp)"]
        onAccepted: chat.attach(selectedFiles)
    }

    Component.onCompleted: prompt.focusInput()
}
