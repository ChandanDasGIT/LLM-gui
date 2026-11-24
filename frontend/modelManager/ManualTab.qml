import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts

// Install any Ollama tag by name, or import a local .gguf file under a name you choose.
ColumnLayout {
    id: root
    required property ModelManagerWrap manager

    spacing: Theme.spaceLg

    component Heading: Text {
        color: Theme.textStrong
        font.family: Theme.monospaceFontFamily
        font.pixelSize: Theme.bodySize
        font.bold: true
    }
    component Hint: Text {
        Layout.fillWidth: true
        color: Theme.textDisabled
        wrapMode: Text.Wrap
        font.family: Theme.monospaceFontFamily
        font.pixelSize: Theme.captionSize
    }

    // ---- by tag ----
    ColumnLayout {
        Layout.fillWidth: true
        spacing: Theme.spaceSm
        Heading { text: "Install by tag" }
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceSm
            ThemedField {
                id: tagField
                Layout.fillWidth: true
                placeholderText: "qwen3:8b  or  hf.co/user/repo:Q4_K_M"
                onAccepted: root.manager.install(text)
            }
            ThemedButton {
                text: "Install"
                accent: true
                enabled: tagField.text.trim().length > 0
                onClicked: root.manager.install(tagField.text)
            }
        }
        Hint { text: "Any tag from ollama.com/library, or a GGUF repository on Hugging Face." }
    }

    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.divider }

    // ---- GGUF import ----
    ColumnLayout {
        Layout.fillWidth: true
        spacing: Theme.spaceSm
        Heading { text: "Import a .gguf file" }
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceSm
            ThemedButton {
                text: "Choose file…"
                enabled: !root.manager.importing
                onClicked: ggufPicker.open()
            }
            Text {
                Layout.fillWidth: true
                text: root.manager.importFileText
                color: Theme.textMuted
                elide: Text.ElideMiddle
                font.family: Theme.monospaceFontFamily
                font.pixelSize: Theme.smallSize
            }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceSm
            Text {
                text: "Name"
                color: Theme.textMuted
                font.family: Theme.monospaceFontFamily
                font.pixelSize: Theme.smallSize
            }
            ThemedField {
                id: nameField
                Layout.fillWidth: true
                enabled: !root.manager.importing
                placeholderText: "my-model"
            }
            ThemedButton {
                text: "Import"
                accent: true
                enabled: !root.manager.importing && nameField.text.trim().length > 0
                onClicked: root.manager.importGguf(nameField.text)
            }
        }
        RowLayout {
            Layout.fillWidth: true
            visible: root.manager.importing
            spacing: Theme.spaceSm
            ThemedProgress {
                Layout.fillWidth: true
                value: root.manager.importProgress
            }
            Text {
                text: root.manager.importText
                color: Theme.textMuted
                font.family: Theme.monospaceFontFamily
                font.pixelSize: Theme.captionSize
            }
            ThemedButton { text: "Cancel"; onClicked: root.manager.cancelImport() }
        }
        Hint {
            text: "Ollama keeps its own copy, so the file takes twice the disk space until you delete the original. "
                  + "If the file has no chat template, the model may answer badly."
        }
    }

    Item { Layout.fillHeight: true }

    FileDialog {
        id: ggufPicker
        title: "Choose a .gguf file"
        nameFilters: ["GGUF models (*.gguf)"]
        onAccepted: root.manager.chooseImportFile(selectedFile)
    }
}
