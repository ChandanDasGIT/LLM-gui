import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// One model in a list: name and note on the left, details and a badge, then the actions.
// While installing, a progress bar and Cancel take the place of the actions.
Rectangle {
    id: root
    property string title
    property string note
    property string details            // e.g. "5.2GB · licence not shown"
    property string badgeText
    property color badgeColor: Theme.textMuted
    property string primaryText        // empty: no button
    property bool primaryAccent: true
    property string secondaryText
    property bool current: false       // the model in use
    property real progress: -1         // 0..1 while installing
    property string progressText
    signal primaryClicked()
    signal secondaryClicked()
    signal cancelClicked()

    implicitHeight: layout.implicitHeight + 2 * Theme.spaceSm
    radius: Theme.controlRadius
    color: hover.hovered ? Theme.cardBgAlt : Theme.cardBg
    border.color: current ? Theme.accent : "transparent"

    HoverHandler { id: hover }

    RowLayout {
        id: layout
        anchors.fill: parent
        anchors.leftMargin: Theme.spaceMd
        anchors.rightMargin: Theme.spaceSm
        anchors.topMargin: Theme.spaceSm
        anchors.bottomMargin: Theme.spaceSm
        spacing: Theme.spaceMd

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            RowLayout {
                spacing: Theme.spaceSm
                Text {
                    text: root.title
                    color: Theme.textStrong
                    font.family: Theme.monospaceFontFamily
                    font.pixelSize: Theme.bodySize
                    font.bold: true
                }
                Text {
                    visible: root.current
                    text: "● in use"
                    color: Theme.accent
                    font.family: Theme.monospaceFontFamily
                    font.pixelSize: Theme.captionSize
                }
            }
            Text {
                Layout.fillWidth: true
                visible: text.length > 0
                text: root.note
                color: Theme.textMuted
                elide: Text.ElideRight
                font.family: Theme.monospaceFontFamily
                font.pixelSize: Theme.smallSize
            }
            Text {
                Layout.fillWidth: true
                text: root.details
                color: Theme.textDisabled
                elide: Text.ElideRight
                font.family: Theme.monospaceFontFamily
                font.pixelSize: Theme.captionSize
            }
        }

        Rectangle {
            visible: root.badgeText.length > 0
            implicitWidth: badge.implicitWidth + 2 * Theme.spaceSm
            implicitHeight: 20
            radius: 10
            color: "transparent"
            border.color: root.badgeColor
            Text {
                id: badge
                anchors.centerIn: parent
                text: root.badgeText
                color: root.badgeColor
                font.family: Theme.monospaceFontFamily
                font.pixelSize: Theme.captionSize
            }
        }

        ColumnLayout {
            visible: root.progress >= 0
            spacing: 2
            ThemedProgress {
                Layout.preferredWidth: 150
                value: root.progress
            }
            Text {
                text: root.progressText
                color: Theme.textMuted
                font.family: Theme.monospaceFontFamily
                font.pixelSize: Theme.captionSize
            }
        }
        ThemedButton {
            visible: root.progress >= 0
            text: "Cancel"
            onClicked: root.cancelClicked()
        }

        ThemedButton {
            visible: root.progress < 0 && root.secondaryText.length > 0
            text: root.secondaryText
            onClicked: root.secondaryClicked()
        }
        ThemedButton {
            visible: root.progress < 0 && root.primaryText.length > 0
            text: root.primaryText
            accent: root.primaryAccent
            onClicked: root.primaryClicked()
        }
    }
}
