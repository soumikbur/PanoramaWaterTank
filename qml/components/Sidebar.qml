import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Shapes
import PanoramaWaterTank

// Modern Industrial IoT / SCADA Left Sidebar Navigation Panel.
// Fixed 270px width, crisp white surface (#FFFFFF), subtle vertical divider (#E2E8F0),
// compact branding lockup with water-drop badge, primary navigation items,
// SYSTEM section, compact operational status card, and copyright footer.
Rectangle {
    id: root

    property int currentIndex: 0

    readonly property var primaryNavItems: [
        { label: "Dashboard", icon: "dashboard", navIndex: 0 },
        { label: "Water Tank", icon: "tank", navIndex: 1 },
        { label: "Devices", icon: "devices", navIndex: 2 },
        { label: "Alerts", icon: "alerts", navIndex: 3 },
        { label: "History", icon: "history", navIndex: 4 }
    ]

    readonly property var systemNavItems: [
        { label: "Settings", icon: "settings", navIndex: 5 }
    ]

    width: Theme.metrics.sidebarDefaultWidth
    Layout.preferredWidth: Theme.metrics.sidebarDefaultWidth
    Layout.minimumWidth: Theme.metrics.sidebarMinWidth
    Layout.maximumWidth: Theme.metrics.sidebarMaxWidth
    Layout.fillHeight: true
    color: "#FFFFFF"

    // Right border vertical divider
    Rectangle {
        anchors.right: parent.right
        width: 1
        height: parent.height
        color: Theme.colors.divider
        z: 2
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.topMargin: 16
        anchors.bottomMargin: 16
        spacing: 0

        // ====================================================================
        // 1. Branding Header Area
        // ====================================================================
        Item {
            id: brandingArea
            Layout.fillWidth: true
            Layout.preferredHeight: 74

            MouseArea {
                anchors.fill: parent
                onPressed: {
                    if (Window.window) {
                        Window.window.startSystemMove();
                    }
                }
            }

            RowLayout {
                anchors.fill: parent
                spacing: 12

                // Water-drop logo in rounded-square container (46x46 px)
                Rectangle {
                    Layout.preferredWidth: 46
                    Layout.preferredHeight: 46
                    Layout.alignment: Qt.AlignVCenter
                    radius: 12
                    color: Theme.colors.primarySurface
                    border.width: 1
                    border.color: "#DBEAFE"

                    Shape {
                        anchors.centerIn: parent
                        width: 22
                        height: 26
                        antialiasing: true

                        ShapePath {
                            fillColor: Theme.colors.primary
                            strokeColor: "transparent"
                            PathSvg {
                                path: "M11,1.5 C11,1.5 2.5,12 2.5,17 C2.5,21.7 6.3,25.5 11,25.5 C15.7,25.5 19.5,21.7 19.5,17 C19.5,12 11,1.5 11,1.5 Z"
                            }
                        }
                    }
                }

                // Brand text
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 1

                    Text {
                        text: "Panorama"
                        color: Theme.colors.textPrimary
                        font.family: Theme.typography.fontFamily
                        font.pixelSize: 22
                        font.weight: Font.Bold
                    }

                    Text {
                        text: "Water Tank Monitor"
                        color: Theme.colors.textSecondary
                        font.family: Theme.typography.fontFamily
                        font.pixelSize: 13
                        font.weight: Font.Medium
                    }
                }
            }
        }

        // Divider below branding
        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: 4
            Layout.bottomMargin: 12
            height: 1
            color: Theme.colors.divider
        }

        // ====================================================================
        // 2. Primary Navigation Menu
        // ====================================================================
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Repeater {
                model: root.primaryNavItems

                delegate: NavItem {
                    required property var modelData

                    Layout.fillWidth: true
                    label: modelData.label
                    iconGlyph: modelData.icon
                    active: root.currentIndex === modelData.navIndex
                    onClicked: root.currentIndex = modelData.navIndex
                }
            }
        }

        // ====================================================================
        // 3. System Navigation Section
        // ====================================================================
        Text {
            text: "SYSTEM"
            color: "#94A3B8"
            font.family: Theme.typography.fontFamily
            font.pixelSize: 11
            font.weight: Font.Bold
            font.letterSpacing: 1.2
            Layout.topMargin: 18
            Layout.bottomMargin: 6
            Layout.leftMargin: 12
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Repeater {
                model: root.systemNavItems

                delegate: NavItem {
                    required property var modelData

                    Layout.fillWidth: true
                    label: modelData.label
                    iconGlyph: modelData.icon
                    active: root.currentIndex === modelData.navIndex
                    onClicked: root.currentIndex = modelData.navIndex
                }
            }
        }

        // Vertical Spacer
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 16
        }

        // ====================================================================
        // 4. System Status Operational Health Card
        // ====================================================================
        Rectangle {
            id: statusCard
            Layout.fillWidth: true
            Layout.preferredHeight: 68
            radius: 10
            color: "#FFFFFF"
            border.color: Theme.colors.divider
            border.width: 1

            readonly property bool isOperational: TankModel.connectionState === "Connected"

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                anchors.topMargin: 10
                anchors.bottomMargin: 10
                spacing: 3

                RowLayout {
                    spacing: 8

                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: statusCard.isOperational ? Theme.colors.success : Theme.colors.warning

                        Rectangle {
                            anchors.centerIn: parent
                            width: 14
                            height: 14
                            radius: 7
                            color: statusCard.isOperational ? Theme.colors.success : Theme.colors.warning
                            opacity: 0.25
                            visible: statusCard.isOperational
                        }
                    }

                    Text {
                        text: "System Status"
                        color: Theme.colors.textSecondary
                        font.family: Theme.typography.fontFamily
                        font.pixelSize: 12
                        font.weight: Font.Medium
                    }
                }

                Text {
                    text: statusCard.isOperational ? "All Systems Operational" : "Connecting..."
                    color: statusCard.isOperational ? Theme.colors.successText : Theme.colors.warning
                    font.family: Theme.typography.fontFamily
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }
            }
        }

        // ====================================================================
        // 5. Copyright Footer
        // ====================================================================
        ColumnLayout {
            Layout.fillWidth: true
            Layout.topMargin: 12
            Layout.leftMargin: 12
            spacing: 2

            Text {
                text: "\u00A9 2026 Panorama"
                color: "#94A3B8"
                font.family: Theme.typography.fontFamily
                font.pixelSize: 11
            }

            Text {
                text: "All rights reserved."
                color: "#94A3B8"
                font.family: Theme.typography.fontFamily
                font.pixelSize: 11
            }
        }
    }
}
