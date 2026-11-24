import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Models suited to this machine: the hardware line, category chips, then one row per model
// with its fit badge and Install (or Use when it is installed).
ColumnLayout {
    id: root
    required property ModelManagerWrap manager

    spacing: Theme.spaceMd

    RowLayout {
        Layout.fillWidth: true
        Text {
            Layout.fillWidth: true
            text: root.manager.hardwareText
            color: Theme.text
            font.family: Theme.monospaceFontFamily
            font.pixelSize: Theme.smallSize
        }
        CheckBox {
            id: showAllBox
            checked: root.manager.showAll
            onToggled: root.manager.showAll = checked
            text: "show all"
            font.family: Theme.monospaceFontFamily
            font.pixelSize: Theme.smallSize
            indicator: Rectangle {
                x: showAllBox.leftPadding
                anchors.verticalCenter: parent.verticalCenter
                implicitWidth: 14
                implicitHeight: 14
                radius: 3
                color: showAllBox.checked ? Theme.accentBg : Theme.field
                border.color: showAllBox.checked ? Theme.accent : Theme.fieldBorder
                Text {
                    anchors.centerIn: parent
                    visible: showAllBox.checked
                    text: "✓"
                    color: Theme.accent
                    font.pixelSize: Theme.smallSize
                }
            }
            contentItem: Text {
                leftPadding: showAllBox.indicator.width + Theme.spaceXs
                text: showAllBox.text
                font: showAllBox.font
                color: Theme.textMuted
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    Flow {
        Layout.fillWidth: true
        spacing: Theme.spaceSm
        Repeater {
            model: root.manager.categories
            Rectangle {
                id: chip
                required property string modelData
                readonly property bool selected: root.manager.category === modelData
                implicitWidth: chipText.implicitWidth + 2 * Theme.spaceMd
                implicitHeight: 24
                radius: 12
                color: selected ? Theme.accentBg : chipHover.hovered ? Theme.controlBgHover : Theme.controlBg
                border.color: selected ? Theme.accent : Theme.controlBorder
                Text {
                    id: chipText
                    anchors.centerIn: parent
                    text: chip.modelData
                    color: chip.selected ? Theme.accent : Theme.text
                    font.family: Theme.monospaceFontFamily
                    font.pixelSize: Theme.smallSize
                }
                HoverHandler { id: chipHover; cursorShape: Qt.PointingHandCursor }
                TapHandler { onTapped: root.manager.category = chip.modelData }
            }
        }
    }

    ListView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        spacing: Theme.spaceXs
        model: root.manager.recommended
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {}
        delegate: ModelRow {
            required property var model
            width: ListView.view.width
            title: model.name
            note: model.note
            details: model.details
            badgeText: model.badge
            badgeColor: model.badgeLevel === 0 ? Theme.ok : model.badgeLevel === 1 ? Theme.warning : Theme.danger
            current: model.current
            primaryText: !model.installed ? "Install" : model.current ? "" : "Use"
            progress: root.manager.installingName === model.name ? root.manager.installProgress : -1
            progressText: root.manager.installText
            onPrimaryClicked: model.installed ? root.manager.use(model.name) : root.manager.install(model.name)
            onCancelClicked: root.manager.cancelInstall()
        }
    }

    Text {
        Layout.fillWidth: true
        text: root.manager.snapshotText
        color: Theme.textDisabled
        wrapMode: Text.Wrap
        font.family: Theme.monospaceFontFamily
        font.pixelSize: Theme.captionSize
    }
}
