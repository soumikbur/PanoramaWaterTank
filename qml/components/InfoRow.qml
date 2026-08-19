import QtQuick
import QtQuick.Layouts
import PanoramaWaterTank

RowLayout {
    id: root

    property string label: ""
    property string value: ""
    property color valueColor: Theme.colors.textPrimary

    Layout.fillWidth: true
    Layout.preferredHeight: 30
    spacing: 8

    Text {
        text: root.label
        color: Theme.colors.textSecondary
        font.family: Theme.typography.fontFamily
        font.pixelSize: 13
        font.weight: Font.Medium
        Layout.preferredWidth: 120
    }

    Text {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        text: root.value
        color: root.valueColor
        font.family: Theme.typography.fontFamily
        font.pixelSize: 14
        font.weight: Font.Bold
        horizontalAlignment: Text.AlignRight
        elide: Text.ElideRight

        Behavior on color { ColorAnimation { duration: 250 } }
    }
}
