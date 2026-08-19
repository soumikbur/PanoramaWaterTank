import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import PanoramaWaterTank

// Custom Application Title Bar for Frameless Window
Rectangle {
    id: header

    height: 44
    color: "#171C28"

    readonly property bool isMaximized: Window.window ? Window.window.visibility === Window.Maximized : false

    // Window Dragging & Double-Click Maximize Area across empty title bar space
    MouseArea {
        anchors.left: parent.left
        anchors.right: controlsRow.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        onPressed: {
            if (Window.window) {
                Window.window.startSystemMove()
            }
        }
        onDoubleClicked: {
            if (Window.window) {
                if (header.isMaximized) {
                    Window.window.showNormal()
                } else {
                    Window.window.showMaximized()
                }
            }
        }

        // Left-aligned title text
        Text {
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            text: "Panorama Water Tank Monitor"
            color: "#FFFFFF"
            font.family: Theme.typography.fontFamily
            font.pixelSize: 15
            font.weight: Font.DemiBold
        }
    }

    // Right-aligned Custom Window Control Buttons
    RowLayout {
        id: controlsRow
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        spacing: 0

        // Minimize Button
        Rectangle {
            Layout.preferredWidth: 44
            Layout.fillHeight: true
            color: minMouse.containsMouse ? (minMouse.pressed ? "#313C52" : "#262F40") : "transparent"

            Text {
                anchors.centerIn: parent
                text: "—"
                color: minMouse.containsMouse ? "#FFFFFF" : "#9CA3AF"
                font.pixelSize: 14
                font.weight: Font.Bold
            }

            MouseArea {
                id: minMouse
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    if (Window.window) {
                        Window.window.showMinimized()
                    }
                }
            }
        }

        // Maximize / Restore Button
        Rectangle {
            Layout.preferredWidth: 44
            Layout.fillHeight: true
            color: maxMouse.containsMouse ? (maxMouse.pressed ? "#313C52" : "#262F40") : "transparent"

            // Icon for Normal state (single square) vs Maximized state (restore double square)
            Item {
                anchors.centerIn: parent
                width: 12
                height: 12

                // Square icon (shown when window is normal)
                Rectangle {
                    anchors.fill: parent
                    visible: !header.isMaximized
                    color: "transparent"
                    border.width: 1.5
                    border.color: maxMouse.containsMouse ? "#FFFFFF" : "#9CA3AF"
                }

                // Restore double square icon (shown when window is maximized)
                Item {
                    anchors.fill: parent
                    visible: header.isMaximized

                    Rectangle {
                        x: 2; y: 0
                        width: 9; height: 9
                        color: "transparent"
                        border.width: 1.5
                        border.color: maxMouse.containsMouse ? "#FFFFFF" : "#9CA3AF"
                    }
                    Rectangle {
                        x: 0; y: 2
                        width: 9; height: 9
                        color: header.color
                        border.width: 1.5
                        border.color: maxMouse.containsMouse ? "#FFFFFF" : "#9CA3AF"
                    }
                }
            }

            MouseArea {
                id: maxMouse
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    if (Window.window) {
                        if (header.isMaximized) {
                            Window.window.showNormal()
                        } else {
                            Window.window.showMaximized()
                        }
                    }
                }
            }
        }

        // Close Button
        Rectangle {
            Layout.preferredWidth: 44
            Layout.fillHeight: true
            color: closeMouse.containsMouse ? (closeMouse.pressed ? "#B91C1C" : "#EF2020") : "transparent"

            Text {
                anchors.centerIn: parent
                text: "✕"
                color: closeMouse.containsMouse ? "#FFFFFF" : "#9CA3AF"
                font.pixelSize: 14
                font.weight: Font.Bold
            }

            MouseArea {
                id: closeMouse
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    if (Window.window) {
                        Window.window.close()
                    }
                }
            }
        }
    }
}
