import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// The input: a rounded box with "> " and the text; under it the model and character
// dropdowns, attach and send. Enter sends, Shift+Enter adds a line, Esc stops a reply.
ColumnLayout {
    id: root
    required property var models          // rows: name
    required property var characters      // rows: name
    property int modelIndex: 0
    property int characterIndex: 0
    property bool busy: false
    property bool canAttach: false
    property var pendingImages: []
    signal sendRequested(string text)
    signal stopRequested()
    signal attachClicked()
    signal removeAttachment(int index)
    signal modelPicked(int index)
    signal characterPicked(int index)

    spacing: Theme.spaceXs

    function focusInput() { input.forceActiveFocus() }
    function clearInput() { input.clear() }

    Rectangle {
        Layout.fillWidth: true
        implicitHeight: box.implicitHeight + 2 * Theme.spaceSm
        radius: 10
        color: Theme.cardBg
        border.color: input.activeFocus ? Theme.textDisabled : Theme.controlBorder

        ColumnLayout {
            id: box
            anchors.fill: parent
            anchors.margins: Theme.spaceSm
            anchors.leftMargin: Theme.spaceMd
            spacing: Theme.spaceSm

            Flow {
                Layout.fillWidth: true
                visible: root.pendingImages.length > 0
                spacing: Theme.spaceSm
                Repeater {
                    model: root.pendingImages
                    Rectangle {
                        id: chip
                        required property string modelData
                        required property int index
                        width: 64
                        height: 48
                        radius: Theme.controlRadius
                        color: Theme.windowBg
                        border.color: Theme.divider
                        Image {
                            anchors.fill: parent
                            anchors.margins: 2
                            source: chip.modelData
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            sourceSize.width: 128
                        }
                        Rectangle {
                            anchors.top: parent.top
                            anchors.right: parent.right
                            anchors.margins: -4
                            width: 16
                            height: 16
                            radius: 8
                            color: removeArea.containsMouse ? Theme.dangerBg : Theme.controlBgActive
                            Text {
                                anchors.centerIn: parent
                                text: "×"
                                color: Theme.textStrong
                                font.pixelSize: Theme.smallSize
                            }
                            MouseArea {
                                id: removeArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: root.removeAttachment(chip.index)
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: Theme.spaceXs
                spacing: 0

                Text {
                    Layout.alignment: Qt.AlignTop
                    Layout.preferredWidth: 20
                    text: ">"
                    color: Theme.accent
                    font.family: Theme.monospaceFontFamily
                    font.pixelSize: Theme.bodySize
                }
                ScrollView {
                    Layout.fillWidth: true
                    Layout.maximumHeight: 200
                    Layout.preferredHeight: input.implicitHeight
                    TextArea {
                        id: input
                        objectName: "promptInput"
                        padding: 0
                        wrapMode: TextArea.Wrap
                        color: Theme.text
                        selectionColor: Theme.selectionBg
                        placeholderText: root.busy ? "" : "Type a message…"
                        placeholderTextColor: Theme.textDisabled
                        font.family: Theme.monospaceFontFamily
                        font.pixelSize: Theme.bodySize
                        background: null
                        focus: true

                        function submit(event) {
                            if (event.modifiers & Qt.ShiftModifier) { event.accepted = false; return; }
                            root.sendRequested(input.text);
                        }
                        Keys.onReturnPressed: (event) => submit(event)
                        Keys.onEnterPressed: (event) => submit(event)
                        Keys.onEscapePressed: root.stopRequested()
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceXs

                ThemedComboBox {
                    objectName: "promptModel"
                    flat: true
                    tip: "Model"
                    model: root.models
                    textRole: "name"
                    currentIndex: root.modelIndex
                    onActivated: (index) => root.modelPicked(index)
                }
                Text {
                    text: "·"
                    color: Theme.textDisabled
                    font.pixelSize: Theme.smallSize
                }
                ThemedComboBox {
                    objectName: "promptCharacter"
                    flat: true
                    tip: "Character"
                    model: root.characters
                    textRole: "name"
                    currentIndex: root.characterIndex
                    onActivated: (index) => root.characterPicked(index)
                }
                Item { Layout.fillWidth: true }
                ThemedButton {
                    visible: root.canAttach
                    text: "+"
                    tip: "Attach an image (or drop it on the window)"
                    onClicked: root.attachClicked()
                }
                Rectangle {
                    objectName: "sendButton"
                    implicitWidth: 30
                    implicitHeight: 26
                    radius: Theme.controlRadius
                    color: sendHover.hovered ? Theme.accent : Theme.accentBg
                    border.color: Theme.accent
                    Text {
                        anchors.centerIn: parent
                        text: root.busy ? "■" : "↑"
                        color: sendHover.hovered ? Theme.windowBg : Theme.accent
                        font.pixelSize: root.busy ? Theme.smallSize : Theme.bodySize
                        font.bold: true
                    }
                    HoverHandler { id: sendHover; cursorShape: Qt.PointingHandCursor }
                    TapHandler { onTapped: root.busy ? root.stopRequested() : root.sendRequested(input.text) }
                    ToolTip.visible: sendHover.hovered
                    ToolTip.text: root.busy ? "Stop (Esc)" : "Send (Enter)"
                    ToolTip.delay: 450
                }
            }
        }
    }

    Text {
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignRight
        text: root.busy ? "esc to stop" : "enter to send · shift+enter for a new line · /clear /model /models /characters"
        color: Theme.textDisabled
        elide: Text.ElideLeft
        font.family: Theme.monospaceFontFamily
        font.pixelSize: Theme.captionSize
    }
}
