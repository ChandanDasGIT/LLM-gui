import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// The fields of one character: name, system prompt, greeting, preferred model, temperature.
// load() fills them; the edited values are read back from the edited* properties.
ColumnLayout {
    id: root
    property bool builtIn: false       // Default cannot be edited
    readonly property alias editedName: nameField.text
    readonly property alias editedSystemPrompt: promptArea.text
    readonly property alias editedGreeting: greetingArea.text
    readonly property alias editedPreferredModel: modelField.text
    readonly property alias editedUseTemperature: tempSwitch.checked
    readonly property alias editedTemperature: tempSlider.value

    function load(builtIn, name, systemPrompt, greeting, preferredModel, useTemperature, temperature) {
        root.builtIn = builtIn;
        nameField.text = name;
        promptArea.text = systemPrompt;
        greetingArea.text = greeting;
        modelField.text = preferredModel;
        tempSwitch.checked = useTemperature;
        tempSlider.value = temperature;
    }

    spacing: Theme.spaceSm

    component Label: Text {
        color: Theme.textMuted
        font.family: Theme.monospaceFontFamily
        font.pixelSize: Theme.smallSize
    }
    component Area: ScrollView {
        id: area
        property alias text: field.text
        property string placeholder
        Layout.fillWidth: true
        background: Rectangle {
            radius: Theme.controlRadius
            color: !area.enabled ? Theme.fieldDisabled : Theme.field
            border.color: field.activeFocus ? Theme.accent : Theme.fieldBorder
        }
        TextArea {
            id: field
            padding: Theme.spaceSm
            wrapMode: TextArea.Wrap
            color: area.enabled ? Theme.text : Theme.textDisabled
            selectionColor: Theme.selectionBg
            placeholderText: area.placeholder
            placeholderTextColor: Theme.textDisabled
            font.family: Theme.monospaceFontFamily
            font.pixelSize: Theme.bodySize
            background: null
        }
    }

    Label { text: "Name" }
    ThemedField {
        id: nameField
        Layout.fillWidth: true
        enabled: !root.builtIn
    }

    Label { text: "System prompt" }
    Area {
        id: promptArea
        Layout.fillHeight: true
        Layout.minimumHeight: 120
        enabled: !root.builtIn
        placeholder: root.builtIn ? "None: the model's own behaviour" : "How the model should act, e.g. \"You are a patient tutor…\""
    }

    Label { text: "Greeting (optional, shown when a chat starts)" }
    Area {
        id: greetingArea
        Layout.preferredHeight: 60
        enabled: !root.builtIn
        placeholder: root.builtIn ? "" : "Ahoy!"
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.spaceMd
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceXs
            Label { text: "Preferred model (optional)" }
            ThemedField {
                id: modelField
                Layout.fillWidth: true
                enabled: !root.builtIn
                placeholderText: "keep the current model"
            }
        }
        ColumnLayout {
            spacing: Theme.spaceXs
            Label { text: "Temperature" }
            RowLayout {
                spacing: Theme.spaceSm
                Switch {
                    id: tempSwitch
                    enabled: !root.builtIn
                    indicator: Rectangle {
                        implicitWidth: 30
                        implicitHeight: 16
                        radius: 8
                        color: tempSwitch.checked ? Theme.accentBg : Theme.controlBg
                        border.color: tempSwitch.checked ? Theme.accent : Theme.controlBorder
                        Rectangle {
                            x: tempSwitch.checked ? parent.width - width - 2 : 2
                            anchors.verticalCenter: parent.verticalCenter
                            width: 12
                            height: 12
                            radius: 6
                            color: tempSwitch.checked ? Theme.accent : Theme.textMuted
                        }
                    }
                }
                Slider {
                    id: tempSlider
                    Layout.preferredWidth: 120
                    enabled: tempSwitch.checked && !root.builtIn
                    from: 0
                    to: 2
                    stepSize: 0.05
                    value: 0.8
                    background: Rectangle {
                        x: tempSlider.leftPadding
                        y: tempSlider.topPadding + tempSlider.availableHeight / 2 - height / 2
                        width: tempSlider.availableWidth
                        height: 4
                        radius: 2
                        color: Theme.controlBg
                        Rectangle {
                            width: tempSlider.visualPosition * parent.width
                            height: parent.height
                            radius: 2
                            color: tempSlider.enabled ? Theme.accent : Theme.controlBorder
                        }
                    }
                    handle: Rectangle {
                        x: tempSlider.leftPadding + tempSlider.visualPosition * (tempSlider.availableWidth - width)
                        y: tempSlider.topPadding + tempSlider.availableHeight / 2 - height / 2
                        width: 12
                        height: 12
                        radius: 6
                        color: tempSlider.enabled ? Theme.textStrong : Theme.textDisabled
                    }
                }
                Text {
                    Layout.preferredWidth: 80
                    text: tempSwitch.checked ? tempSlider.value.toFixed(2) : "model default"
                    color: Theme.textMuted
                    font.family: Theme.monospaceFontFamily
                    font.pixelSize: Theme.smallSize
                }
            }
        }
    }
}
