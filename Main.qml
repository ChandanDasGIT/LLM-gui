import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 1200
    height: 780
    minimumWidth: 760
    minimumHeight: 420
    visible: true
    title: "LLM-gui"
    color: Theme.windowBg

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            ChatHistory {
                Layout.preferredWidth: 260
                Layout.fillHeight: true
            }
            ChatView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                onOpenModelsRequested: modelManager.open()
                onOpenCharactersRequested: characterManager.open()
            }
        }
        StatusStrip {
            Layout.fillWidth: true
            onModelClicked: modelManager.open()
            onCharacterClicked: characterManager.open()
        }
    }

    ModelManager { id: modelManager }
    CharacterManager { id: characterManager }
}
