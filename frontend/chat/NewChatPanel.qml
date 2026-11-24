import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// The new-chat screen: pick a model and a character, then type the first message below.
ColumnLayout {
    id: root
    required property var models          // rows: name, details, vision
    required property var characters      // rows: id, name, greeting
    property int modelIndex: 0
    property int characterIndex: 0
    signal modelPicked(int index)
    signal characterPicked(int index)
    signal manageModelsRequested()
    signal manageCharactersRequested()

    spacing: Theme.spaceMd

    component Heading: Text {
        color: Theme.textMuted
        font.family: Theme.monospaceFontFamily
        font.pixelSize: Theme.captionSize
        font.letterSpacing: 1
    }

    component Card: Rectangle {
        id: card
        property bool selected: false
        property bool dashed: false
        default property alias content: inner.data
        signal clicked()
        implicitWidth: 200
        implicitHeight: 64
        radius: Theme.panelRadius
        color: selected ? Theme.accentBg : cardHover.hovered ? Theme.cardBgAlt : Theme.cardBg
        border.color: selected ? Theme.accent : cardHover.hovered ? Theme.controlBorder : Theme.divider
        Item {
            id: inner
            anchors.fill: parent
            anchors.margins: Theme.spaceMd
        }
        HoverHandler { id: cardHover; cursorShape: Qt.PointingHandCursor }
        TapHandler { onTapped: card.clicked() }
    }

    Item { Layout.fillHeight: true }

    Text {
        Layout.fillWidth: true
        text: "Start a new chat"
        color: Theme.textStrong
        font.family: Theme.monospaceFontFamily
        font.pixelSize: Theme.headingSize + 3
        font.bold: true
    }
    Text {
        Layout.fillWidth: true
        Layout.bottomMargin: Theme.spaceMd
        text: "Pick a model and a character. You can change both later from the prompt box."
        color: Theme.textMuted
        wrapMode: Text.Wrap
        font.family: Theme.monospaceFontFamily
        font.pixelSize: Theme.smallSize
    }

    Heading { text: "MODEL" }
    Text {
        Layout.fillWidth: true
        visible: root.models.length === 0
        text: "No models installed yet, or Ollama is not running."
        color: Theme.warning
        wrapMode: Text.Wrap
        font.family: Theme.monospaceFontFamily
        font.pixelSize: Theme.smallSize
    }
    Flow {
        Layout.fillWidth: true
        spacing: Theme.spaceSm
        Repeater {
            model: root.models
            Card {
                id: modelCard
                required property var modelData
                required property int index
                selected: root.modelIndex === index
                onClicked: root.modelPicked(index)
                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width
                    spacing: 3
                    Text {
                        width: parent.width
                        text: modelCard.modelData.name
                        color: modelCard.selected ? Theme.accent : Theme.textStrong
                        elide: Text.ElideRight
                        font.family: Theme.monospaceFontFamily
                        font.pixelSize: Theme.bodySize
                    }
                    Text {
                        width: parent.width
                        text: modelCard.modelData.details + (modelCard.modelData.vision ? " · sees images" : "")
                        color: Theme.textDisabled
                        elide: Text.ElideRight
                        font.family: Theme.monospaceFontFamily
                        font.pixelSize: Theme.captionSize
                    }
                }
            }
        }
        Card {
            onClicked: root.manageModelsRequested()
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "+  Get more models…"
                color: Theme.textMuted
                font.family: Theme.monospaceFontFamily
                font.pixelSize: Theme.bodySize
            }
        }
    }

    Heading { text: "CHARACTER"; Layout.topMargin: Theme.spaceMd }
    Flow {
        Layout.fillWidth: true
        spacing: Theme.spaceSm
        Repeater {
            model: root.characters
            Card {
                id: characterCard
                required property var modelData
                required property int index
                selected: root.characterIndex === index
                onClicked: root.characterPicked(index)
                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width
                    spacing: 3
                    Text {
                        width: parent.width
                        text: characterCard.modelData.name
                        color: characterCard.selected ? Theme.accent : Theme.textStrong
                        elide: Text.ElideRight
                        font.family: Theme.monospaceFontFamily
                        font.pixelSize: Theme.bodySize
                    }
                    Text {
                        width: parent.width
                        text: characterCard.modelData.greeting.length > 0 ? "“" + characterCard.modelData.greeting + "”" : "no greeting"
                        color: Theme.textDisabled
                        elide: Text.ElideRight
                        font.family: Theme.monospaceFontFamily
                        font.pixelSize: Theme.captionSize
                        font.italic: characterCard.modelData.greeting.length > 0
                    }
                }
            }
        }
        Card {
            onClicked: root.manageCharactersRequested()
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "Manage characters…"
                color: Theme.textMuted
                font.family: Theme.monospaceFontFamily
                font.pixelSize: Theme.bodySize
            }
        }
    }

    Item { Layout.fillHeight: true }
}
