import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// The model manager overlay: Recommended, Installed and Manual tabs, with the running install
// and the last message at the bottom. Esc or × closes it.
Popup {
    id: root
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(parent.width - 2 * Theme.spaceLg, 780)
    height: Math.min(parent.height - 2 * Theme.spaceLg, 600)
    modal: true
    focus: true
    padding: Theme.spaceLg

    Overlay.modal: Rectangle { color: "#99000000" }

    background: Rectangle {
        radius: Theme.panelRadius
        color: Theme.popupBg
        border.color: Theme.popupBorder
    }

    onOpened: manager.refresh()

    ModelManagerWrap {
        id: manager
        objectName: "modelManagerWrap"
    }

    contentItem: ColumnLayout {
        spacing: Theme.spaceMd

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "Models"
                color: Theme.textStrong
                font.family: Theme.monospaceFontFamily
                font.pixelSize: Theme.headingSize
                font.bold: true
            }
            Item { Layout.fillWidth: true }
            ThemedButton { text: "×"; tip: "Close (Esc)"; onClicked: root.close() }
        }

        Row {
            id: tabs
            property int current: 0
            spacing: Theme.spaceLg
            Repeater {
                model: ["Recommended", "Installed", "Manual"]
                Item {
                    id: tab
                    required property string modelData
                    required property int index
                    readonly property bool selected: tabs.current === index
                    implicitWidth: tabText.implicitWidth
                    implicitHeight: tabText.implicitHeight + 8
                    Text {
                        id: tabText
                        text: tab.modelData
                        color: tab.selected ? Theme.textStrong : tabHover.hovered ? Theme.text : Theme.textMuted
                        font.family: Theme.monospaceFontFamily
                        font.pixelSize: Theme.bodySize
                    }
                    Rectangle {
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: 2
                        color: tab.selected ? Theme.accent : "transparent"
                    }
                    HoverHandler { id: tabHover; cursorShape: Qt.PointingHandCursor }
                    TapHandler { onTapped: tabs.current = tab.index }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.divider }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabs.current
            RecommendedTab { manager: manager }
            InstalledTab { manager: manager }
            ManualTab { manager: manager }
        }

        // ---- running install and last message ----
        RowLayout {
            Layout.fillWidth: true
            visible: manager.installingName.length > 0
            spacing: Theme.spaceSm
            Text {
                text: "Installing " + manager.installingName
                color: Theme.text
                font.family: Theme.monospaceFontFamily
                font.pixelSize: Theme.smallSize
            }
            ThemedProgress {
                Layout.fillWidth: true
                value: manager.installProgress
            }
            Text {
                text: manager.installText
                color: Theme.textMuted
                font.family: Theme.monospaceFontFamily
                font.pixelSize: Theme.captionSize
            }
            ThemedButton { text: "Cancel"; onClicked: manager.cancelInstall() }
        }
        Text {
            Layout.fillWidth: true
            visible: manager.message.length > 0
            text: "⎿  " + manager.message
            color: Theme.warning
            wrapMode: Text.Wrap
            font.family: Theme.monospaceFontFamily
            font.pixelSize: Theme.smallSize
        }
    }
}
