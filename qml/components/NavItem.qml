import QtQuick
import QtQuick.Layouts
import PanoramaWaterTank

// Single sidebar navigation item: 48-52px height, 24px icon, 16px text.
// Modern SCADA-style styling with soft light blue active state (#EFF6FF),
// rounded rectangular clickable area (10-12px radius), and smooth hover states.
Rectangle {
    id: root

    property string label: ""
    property string iconGlyph: "dashboard" // dashboard | tank | devices | alerts | history | settings
    property bool active: false

    signal clicked()

    Layout.fillWidth: true
    Layout.preferredHeight: 50
    radius: 10

    color: active ? "#EFF6FF"
        : (hoverHandler.hovered ? "#F1F5F9" : "transparent")

    Behavior on color {
        ColorAnimation { duration: 150 }
    }

    HoverHandler {
        id: hoverHandler
        cursorShape: Qt.PointingHandCursor
    }

    TapHandler {
        onTapped: root.clicked()
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 14

        Item {
            id: iconArea
            Layout.preferredWidth: 24
            Layout.preferredHeight: 24
            Layout.alignment: Qt.AlignVCenter

            readonly property color tint: root.active ? Theme.colors.primary : "#64748B"
            readonly property real strokeWidth: 2.0

            // 1. Dashboard: 2x2 grid of rounded squares
            Grid {
                anchors.centerIn: parent
                visible: root.iconGlyph === "dashboard"
                columns: 2
                spacing: 3.5
                Repeater {
                    model: 4
                    delegate: Rectangle {
                        width: 8.5
                        height: 8.5
                        radius: 2
                        color: iconArea.tint
                    }
                }
            }

            // 2. Water Tank: Industrial vessel outline + top neck
            Item {
                anchors.centerIn: parent
                width: 20
                height: 22
                visible: root.iconGlyph === "tank"

                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 20
                    height: 18
                    radius: 4
                    color: "transparent"
                    border.width: iconArea.strokeWidth
                    border.color: iconArea.tint
                }
                Rectangle {
                    anchors.top: parent.top
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 11
                    height: 3.5
                    radius: 1.5
                    color: "transparent"
                    border.width: iconArea.strokeWidth
                    border.color: iconArea.tint
                }
            }

            // 3. Devices: Monitor screen outline + stand
            Item {
                anchors.centerIn: parent
                width: 22
                height: 20
                visible: root.iconGlyph === "devices"

                Rectangle {
                    anchors.top: parent.top
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 22
                    height: 14
                    radius: 2.5
                    color: "transparent"
                    border.width: iconArea.strokeWidth
                    border.color: iconArea.tint
                }
                Rectangle {
                    anchors.top: parent.top
                    anchors.topMargin: 14
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: iconArea.strokeWidth
                    height: 4
                    color: iconArea.tint
                }
                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 12
                    height: iconArea.strokeWidth
                    radius: 1
                    color: iconArea.tint
                }
            }

            // 4. Alerts: Notification bell shape
            Item {
                anchors.centerIn: parent
                width: 20
                height: 22
                visible: root.iconGlyph === "alerts"

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: 1
                    width: 16
                    height: 14
                    radius: 8
                    color: "transparent"
                    border.width: iconArea.strokeWidth
                    border.color: iconArea.tint
                }
                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 3
                    width: 19
                    height: iconArea.strokeWidth
                    radius: 1
                    color: iconArea.tint
                }
                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    width: 5
                    height: 4
                    radius: 2
                    color: iconArea.tint
                }
            }

            // 5. History: Clock circle with hour & minute hands
            Item {
                anchors.centerIn: parent
                width: 22
                height: 22
                visible: root.iconGlyph === "history"

                Rectangle {
                    anchors.fill: parent
                    radius: width / 2
                    color: "transparent"
                    border.width: iconArea.strokeWidth
                    border.color: iconArea.tint
                }
                Rectangle {
                    x: parent.width / 2 - iconArea.strokeWidth / 2
                    y: parent.height / 2 - 6.5
                    width: iconArea.strokeWidth
                    height: 7
                    radius: 1
                    color: iconArea.tint
                }
                Rectangle {
                    x: parent.width / 2 - iconArea.strokeWidth / 2
                    y: parent.height / 2 - iconArea.strokeWidth / 2
                    width: 6
                    height: iconArea.strokeWidth
                    radius: 1
                    color: iconArea.tint
                }
            }

            // 6. Settings: Center gear circle with radial notches
            Item {
                id: settingsIcon
                anchors.centerIn: parent
                width: 22
                height: 22
                visible: root.iconGlyph === "settings"

                Rectangle {
                    anchors.centerIn: parent
                    width: 13
                    height: 13
                    radius: width / 2
                    color: "transparent"
                    border.width: iconArea.strokeWidth
                    border.color: iconArea.tint
                }

                Repeater {
                    model: 6
                    delegate: Rectangle {
                        required property int index
                        readonly property real angle: index * (Math.PI / 3)
                        width: 2.5
                        height: 4.5
                        radius: 1.2
                        color: iconArea.tint
                        x: settingsIcon.width / 2 - width / 2 + Math.cos(angle) * (settingsIcon.width / 2 - 2.5)
                        y: settingsIcon.height / 2 - height / 2 + Math.sin(angle) * (settingsIcon.height / 2 - 2.5)
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            text: root.label
            color: root.active ? Theme.colors.primary : "#1E293B"
            font.family: Theme.typography.fontFamily
            font.pixelSize: 16
            font.weight: root.active ? Font.DemiBold : Font.Medium
        }
    }
}
