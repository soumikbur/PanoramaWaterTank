import QtQuick
import QtQuick.Layouts
import QtQuick.Shapes
import PanoramaWaterTank

// Full-width segmented water level rank bar with dynamic triangular pointer
// and percentage readout. Segments: LOW (0-25%), MODERATE (25-50%), GOOD (50-75%), HIGH (75-100%).
BaseCard {
    id: root

    backgroundColor: Theme.colors.surface
    borderColor: Theme.colors.divider
    padding: 14

    Layout.preferredHeight: Theme.metrics.rankIndicatorHeight

    readonly property var segments: [
        { label: "LOW", color: "#EF4444" },
        { label: "MODERATE", color: "#F97316" },
        { label: "GOOD", color: "#EAB308" },
        { label: "HIGH", color: "#22C55E" }
    ]

    ColumnLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 4

        // Section Title
        Text {
            text: "Water Level Indicator (Rank)"
            color: Theme.colors.textPrimary
            font.family: Theme.typography.fontFamily
            font.pixelSize: 14
            font.weight: Font.Bold
        }

        // Percentage Scale Labels Row (0%, 25%, 50%, 75%, 100%)
        RowLayout {
            Layout.fillWidth: true

            Repeater {
                model: ["0%", "25%", "50%", "75%", "100%"]

                delegate: Text {
                    required property int index
                    required property string modelData
                    Layout.fillWidth: true
                    horizontalAlignment: index === 0 ? Text.AlignLeft
                        : (index === 4 ? Text.AlignRight : Text.AlignHCenter)
                    text: modelData
                    color: Theme.colors.textSecondary
                    font.family: Theme.typography.fontFamily
                    font.pixelSize: 11
                    font.weight: Font.Medium
                }
            }
        }

        // Segmented Color Bar
        Item {
            id: barArea
            Layout.fillWidth: true
            Layout.preferredHeight: 26

            RowLayout {
                anchors.fill: parent
                spacing: 3

                Repeater {
                    model: root.segments

                    delegate: Rectangle {
                        required property int index
                        required property var modelData

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: modelData.color
                        radius: 6

                        Rectangle {
                            anchors.right: index < 3 ? parent.right : undefined
                            anchors.left: index > 0 ? parent.left : undefined
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            width: 6
                            color: parent.color
                            visible: index > 0 || index < 3
                        }

                        Text {
                            anchors.centerIn: parent
                            text: modelData.label
                            color: "#FFFFFF"
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 11
                            font.weight: Font.Bold
                        }
                    }
                }
            }
        }

        // Pointer Indicator (Triangle pointing UP toward the bar + Percentage Text)
        Item {
            id: pointerContainer
            Layout.fillWidth: true
            Layout.preferredHeight: 28

            readonly property real clampedPercentage: Math.max(0, Math.min(100, TankModel.fillPercentage))

            Item {
                id: movingPointer
                width: 64
                height: 28
                x: Math.max(0, Math.min(pointerContainer.width - width,
                   (pointerContainer.width * (pointerContainer.clampedPercentage / 100.0)) - (width / 2.0)))

                Behavior on x {
                    NumberAnimation {
                        duration: 300
                        easing.type: Easing.InOutQuad
                    }
                }

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 1

                    Shape {
                        Layout.alignment: Qt.AlignHCenter
                        width: 14
                        height: 8
                        antialiasing: true

                        ShapePath {
                            fillColor: Theme.colors.primary
                            strokeColor: "transparent"
                            PathSvg { path: "M7,0 L14,8 L0,8 Z" }
                        }
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: TankModel.fillPercentage.toFixed(2) + "%"
                        color: Theme.colors.primary
                        font.family: Theme.typography.fontFamily
                        font.pixelSize: 13
                        font.weight: Font.Bold
                    }
                }
            }
        }
    }
}