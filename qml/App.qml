import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PanoramaWaterTank

// Application shell. Custom TitleBar (Header), Sidebar, and Footer surround
// StackLayout. Frameless window with custom title bar and 8-direction edge resizer.
ApplicationWindow {
    id: window

    flags: Qt.FramelessWindowHint | Qt.Window

    width: Theme.metrics.windowDefaultWidth
    height: Theme.metrics.windowDefaultHeight
    minimumWidth: Theme.metrics.windowMinimumWidth
    minimumHeight: Theme.metrics.windowMinimumHeight

    visible: true
    title: qsTr("Panorama - Water Tank Monitor")
    color: Theme.colors.background

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Header {
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Sidebar {
                id: sidebar
                Layout.fillHeight: true
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: sidebar.currentIndex

                    DashboardPage {}
                    WaterTankPage {}
                    DevicesPage {}
                    AlertsPage {}
                    HistoryPage {}
                    SettingsPage {}
                }

                Footer {
                    Layout.fillWidth: true
                }
            }
        }
    }

    // 8-Direction Mouse Edge & Corner Window Resizer
    FramelessResizeHandler {}
}
