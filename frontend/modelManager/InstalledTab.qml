import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// The models Ollama has: size, parameters and quantization, with Use and Remove. Remove asks
// once more on the row before anything is deleted.
ColumnLayout {
    id: root
    required property ModelManagerWrap manager
    property string confirmRemove

    spacing: Theme.spaceMd

    Text {
        Layout.fillWidth: true
        text: root.manager.installedSummary
        color: Theme.text
        wrapMode: Text.Wrap
        font.family: Theme.monospaceFontFamily
        font.pixelSize: Theme.smallSize
    }

    ListView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        spacing: Theme.spaceXs
        model: root.manager.installed
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {}
        delegate: ModelRow {
            required property var model
            readonly property bool asking: root.confirmRemove === model.name
            width: ListView.view.width
            title: model.name
            note: asking ? "Delete " + model.name + " from Ollama?" : model.note
            details: model.details
            current: model.current
            primaryText: asking ? "Keep" : model.current ? "" : "Use"
            primaryAccent: !asking
            secondaryText: asking ? "Delete" : "Remove"
            onPrimaryClicked: asking ? root.confirmRemove = "" : root.manager.use(model.name)
            onSecondaryClicked: {
                if (asking) {
                    root.manager.remove(model.name);
                    root.confirmRemove = "";
                } else {
                    root.confirmRemove = model.name;
                }
            }
        }
    }
}
