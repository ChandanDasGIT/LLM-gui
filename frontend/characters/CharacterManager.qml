import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// The character manager overlay: the list on the left, the editor on the right.
Popup {
    id: root
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(parent.width - 2 * Theme.spaceLg, 820)
    height: Math.min(parent.height - 2 * Theme.spaceLg, 600)
    modal: true
    focus: true
    padding: Theme.spaceLg

    property int selected: 0
    property bool confirmDelete: false

    Overlay.modal: Rectangle { color: "#99000000" }

    background: Rectangle {
        radius: Theme.panelRadius
        color: Theme.popupBg
        border.color: Theme.popupBorder
    }

    CharactersWrap {
        id: characters
        objectName: "charactersWrap"
        onModelReset: root.select(Math.min(root.selected, rowCount() - 1))
    }

    function select(row) {
        root.selected = Math.max(0, row);
        root.confirmDelete = false;
        const c = characters.get(root.selected);
        editor.load(c.builtIn, c.name, c.systemPrompt, c.greeting, c.preferredModel, c.useTemperature, c.temperature);
    }

    onOpened: select(Math.max(0, characters.activeIndex))

    contentItem: ColumnLayout {
        spacing: Theme.spaceMd

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "Characters"
                color: Theme.textStrong
                font.family: Theme.monospaceFontFamily
                font.pixelSize: Theme.headingSize
                font.bold: true
            }
            Item { Layout.fillWidth: true }
            ThemedButton { text: "×"; tip: "Close (Esc)"; onClicked: root.close() }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spaceLg

            // ---- list ----
            ColumnLayout {
                Layout.preferredWidth: 220
                Layout.fillHeight: true
                spacing: Theme.spaceSm

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 2
                    model: characters
                    boundsBehavior: Flickable.StopAtBounds
                    delegate: Rectangle {
                        id: row
                        required property string name
                        required property bool builtIn
                        required property bool active
                        required property int index
                        readonly property bool isSelected: root.selected === index
                        width: ListView.view.width
                        height: 30
                        radius: Theme.controlRadius
                        color: isSelected ? Theme.selectionBg : rowHover.hovered ? Theme.controlBgHover : "transparent"
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.spaceSm
                            anchors.rightMargin: Theme.spaceSm
                            spacing: Theme.spaceSm
                            Text {
                                text: row.active ? "●" : " "
                                color: Theme.accent
                                font.family: Theme.monospaceFontFamily
                                font.pixelSize: Theme.smallSize
                            }
                            Text {
                                Layout.fillWidth: true
                                text: row.name
                                color: row.isSelected ? Theme.textStrong : Theme.text
                                elide: Text.ElideRight
                                font.family: Theme.monospaceFontFamily
                                font.pixelSize: Theme.bodySize
                            }
                            Text {
                                visible: row.builtIn
                                text: "built in"
                                color: Theme.textDisabled
                                font.family: Theme.monospaceFontFamily
                                font.pixelSize: Theme.captionSize
                            }
                        }
                        HoverHandler { id: rowHover; cursorShape: Qt.PointingHandCursor }
                        TapHandler { onTapped: root.select(row.index) }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceXs
                    ThemedButton {
                        Layout.fillWidth: true
                        text: "New"
                        onClicked: { const r = characters.create(); if (r >= 0) root.select(r) }
                    }
                    ThemedButton {
                        Layout.fillWidth: true
                        text: "Duplicate"
                        onClicked: { const r = characters.duplicate(root.selected); if (r >= 0) root.select(r) }
                    }
                    ThemedButton {
                        Layout.fillWidth: true
                        text: root.confirmDelete ? "Sure?" : "Delete"
                        enabled: !editor.builtIn
                        onClicked: {
                            if (!root.confirmDelete) { root.confirmDelete = true; return; }
                            if (characters.remove(root.selected)) root.select(root.selected - 1);
                        }
                    }
                }
            }

            Rectangle { Layout.fillHeight: true; implicitWidth: 1; color: Theme.divider }

            // ---- editor ----
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: Theme.spaceMd

                CharacterEditor {
                    id: editor
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                Text {
                    Layout.fillWidth: true
                    visible: characters.message.length > 0
                    text: "⎿  " + characters.message
                    color: Theme.warning
                    wrapMode: Text.Wrap
                    font.family: Theme.monospaceFontFamily
                    font.pixelSize: Theme.smallSize
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceSm
                    Text {
                        Layout.fillWidth: true
                        text: "Use makes it the character of the open chat."
                        color: Theme.textDisabled
                        font.family: Theme.monospaceFontFamily
                        font.pixelSize: Theme.captionSize
                    }
                    ThemedButton {
                        text: "Revert"
                        enabled: !editor.builtIn
                        onClicked: root.select(root.selected)
                    }
                    ThemedButton {
                        text: "Save"
                        enabled: !editor.builtIn
                        onClicked: {
                            const r = characters.save(root.selected, editor.editedName, editor.editedSystemPrompt,
                                                      editor.editedGreeting, editor.editedPreferredModel,
                                                      editor.editedUseTemperature, editor.editedTemperature);
                            if (r >= 0) root.select(r);
                        }
                    }
                    ThemedButton {
                        text: "Use"
                        accent: true
                        onClicked: characters.use(root.selected)
                    }
                }
            }
        }
    }
}
