// TankInfoCard.qml
import QtQuick
import QtQuick.Layouts
import PanoramaWaterTank

// Tank Information panel: identity, live readings, geometry, and connection/
// sensor health at a glance.
BaseCard {
    id: root

    backgroundColor: Theme.colors.surface
    borderColor: Theme.colors.divider
    padding: Theme.metrics.spacingLarge

    component Divider : Rectangle {
        Layout.fillWidth: true
        height: Theme.metrics.dividerThickness
        color: Theme.colors.divider
    }

    ColumnLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: Theme.metrics.infoRowSpacing

        Text {
            text: "Tank Information"
            color: Theme.colors.textPrimary
            font.family: Theme.typography.fontFamily
            font.pixelSize: Theme.typography.dashCardTitle.size
            font.weight: Theme.typography.dashCardTitle.weight
            Layout.bottomMargin: Theme.metrics.space12
        }

        InfoRow {
            Layout.fillWidth: true
            label: "Tank Name"
            value: TankModel.tankName
        }

        Divider {}

        InfoRow {
            Layout.fillWidth: true
            label: "Device"
            value: TankModel.deviceLabel
        }

        Divider {}

        InfoRow {
            Layout.fillWidth: true
            label: "Temperature"
            value: TankModel.hasTemperature ? TankModel.temperature.toFixed(2) + " \u00B0C" : "\u2014"
        }

        Divider {}

        InfoRow {
            Layout.fillWidth: true
            label: "Connection"
            value: TankModel.connectionState
            valueColor: TankModel.connectionState === "Connected" ? Theme.colors.success : Theme.colors.offline
        }

        Divider {}

        InfoRow {
            Layout.fillWidth: true
            label: "Last Updated"
            value: TankModel.lastUpdated
        }

        Divider {}

        InfoRow {
            Layout.fillWidth: true
            label: "Sensor Status"
            value: TankModel.sensorStatus
            valueColor: TankModel.sensorStatus === "Healthy" ? Theme.colors.success : Theme.colors.critical
        }

        Divider {}

        InfoRow {
            Layout.fillWidth: true
            label: "Tank Height"
            value: TankModel.tankHeightCm.toFixed(2) + " cm"
        }

        Divider {}

        InfoRow {
            Layout.fillWidth: true
            label: "Tank Capacity"
            value: (TankModel.capacityLiters >= 1000
                ? TankModel.capacityLiters.toLocaleString(Qt.locale(), 'f', 2) + " L"
                : TankModel.capacityLiters.toFixed(2) + " L")
        }

        Item { Layout.fillHeight: true }
    }
}