import QtQuick
import QtQuick.Layouts

// One message of the transcript: "> prompt" on a soft block for the user, "●" and the Markdown
// reply for the model (its thinking dimmed above it), or a notice line.
Item {
    id: root
    required property string kind
    required property string content
    required property string thinking
    required property string replyState
    required property string error
    required property var images
    required property int missingImages
    property int columnWidth: 680       // the message is centred in a column this wide (or narrower)

    implicitHeight: column.implicitHeight + Theme.spaceLg

    component Marker: Text {
        Layout.alignment: Qt.AlignTop
        Layout.preferredWidth: 20
        font.family: Theme.monospaceFontFamily
        font.pixelSize: Theme.bodySize
        lineHeight: 1.25
    }

    ColumnLayout {
        id: column
        width: Math.min(root.width - 2 * Theme.spaceLg, root.columnWidth)
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: Theme.spaceSm

        // ---- user prompt ----
        Rectangle {
            visible: root.kind === "user"
            Layout.fillWidth: true
            implicitHeight: userRow.implicitHeight + 2 * Theme.spaceSm
            radius: Theme.panelRadius
            color: Theme.cardBg

            RowLayout {
                id: userRow
                anchors.fill: parent
                anchors.margins: Theme.spaceSm
                anchors.leftMargin: Theme.spaceMd
                spacing: 0
                Marker { text: ">"; color: Theme.textMuted }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceXs
                    TextEdit {
                        Layout.fillWidth: true
                        visible: text.length > 0
                        text: root.content
                        readOnly: true
                        selectByMouse: true
                        wrapMode: TextEdit.Wrap
                        color: Theme.text
                        selectionColor: Theme.selectionBg
                        font.family: Theme.monospaceFontFamily
                        font.pixelSize: Theme.bodySize
                    }
                    Text {
                        Layout.fillWidth: true
                        visible: root.missingImages > 0
                        text: root.missingImages === 1 ? "⎿  1 image no longer on disk"
                                                       : "⎿  " + root.missingImages + " images no longer on disk"
                        color: Theme.textDisabled
                        font.family: Theme.monospaceFontFamily
                        font.pixelSize: Theme.captionSize
                    }
                    Flow {
                        Layout.fillWidth: true
                        visible: root.images.length > 0
                        spacing: Theme.spaceSm
                        Repeater {
                            model: root.images
                            Rectangle {
                                required property string modelData
                                width: 120
                                height: 90
                                radius: Theme.controlRadius
                                color: Theme.windowBg
                                border.color: Theme.divider
                                Image {
                                    anchors.fill: parent
                                    anchors.margins: 2
                                    source: parent.modelData
                                    fillMode: Image.PreserveAspectFit
                                    asynchronous: true
                                    sourceSize.width: 240
                                }
                            }
                        }
                    }
                }
            }
        }

        // ---- model reply ----
        RowLayout {
            visible: root.kind === "assistant"
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spaceMd
            spacing: 0
            Marker {
                text: "●"
                color: root.replyState === "failed" ? Theme.danger
                     : root.replyState === "streaming" ? Theme.accent : Theme.textMuted
                SequentialAnimation on opacity {
                    running: root.replyState === "streaming"
                    loops: Animation.Infinite
                    alwaysRunToEnd: true
                    NumberAnimation { to: 0.25; duration: 500 }
                    NumberAnimation { to: 1.0; duration: 500 }
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceSm
                Rectangle {
                    Layout.fillWidth: true
                    visible: root.thinking.length > 0
                    implicitHeight: thinkingText.implicitHeight + Theme.spaceXs
                    color: "transparent"
                    Rectangle { width: 2; height: parent.height; color: Theme.divider }
                    TextEdit {
                        id: thinkingText
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: Theme.spaceMd
                        text: root.thinking
                        readOnly: true
                        selectByMouse: true
                        wrapMode: TextEdit.Wrap
                        color: Theme.textDisabled
                        selectionColor: Theme.selectionBg
                        font.family: Theme.monospaceFontFamily
                        font.pixelSize: Theme.smallSize
                        font.italic: true
                    }
                }
                TextEdit {
                    Layout.fillWidth: true
                    visible: root.content.length > 0
                    text: root.content
                    textFormat: TextEdit.MarkdownText
                    readOnly: true
                    selectByMouse: true
                    wrapMode: TextEdit.Wrap
                    color: Theme.text
                    selectionColor: Theme.selectionBg
                    font.family: Theme.monospaceFontFamily
                    font.pixelSize: Theme.bodySize
                }
                Text {
                    Layout.fillWidth: true
                    visible: root.replyState === "streaming" && root.content.length === 0 && root.thinking.length === 0
                    text: "…"
                    color: Theme.textMuted
                    font.family: Theme.monospaceFontFamily
                    font.pixelSize: Theme.bodySize
                }
                Text {
                    Layout.fillWidth: true
                    visible: root.replyState === "stopped" || root.replyState === "failed"
                    text: root.replyState === "stopped" ? "⎿  Stopped" : "⎿  " + root.error
                    color: root.replyState === "failed" ? Theme.danger : Theme.textMuted
                    wrapMode: Text.Wrap
                    font.family: Theme.monospaceFontFamily
                    font.pixelSize: Theme.smallSize
                }
            }
        }

        // ---- notice ----
        RowLayout {
            visible: root.kind === "notice"
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spaceMd
            spacing: 0
            Marker { text: "⎿"; color: Theme.textMuted }
            Text {
                Layout.fillWidth: true
                text: root.content
                color: Theme.warning
                wrapMode: Text.Wrap
                font.family: Theme.monospaceFontFamily
                font.pixelSize: Theme.smallSize
            }
        }
    }
}
