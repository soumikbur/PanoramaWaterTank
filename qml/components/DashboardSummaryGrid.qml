import QtQuick
import QtQuick.Layouts
import PanoramaWaterTank

// The five bottom summary cards: Water Level, Tank Volume, Status, Temperature, Last Updated.
GridLayout {
    id: root
    property bool compact: false

    columns: compact ? 2 : 5
    columnSpacing: Theme.metrics.cardSpacing
    rowSpacing: Theme.metrics.cardSpacing

    // CARD 1: Water Level
    SummaryCard {
        Layout.fillWidth: true
        Layout.preferredHeight: Theme.metrics.summaryCardHeight
        iconGlyph: "drop"
        iconColor: Theme.colors.primary
        valueColor: Theme.colors.primary
        title: "Water Level"
        value: TankModel.fillPercentage.toFixed(2) + "%"
        subtitle: "(" + TankModel.waterHeightCm.toFixed(2) + " cm / " + TankModel.tankHeightCm.toFixed(2) + " cm)"
    }

    // CARD 2: Tank Volume
    SummaryCard {
        Layout.fillWidth: true
        Layout.preferredHeight: Theme.metrics.summaryCardHeight
        iconGlyph: "volume"
        iconColor: Theme.colors.primary
        valueColor: Theme.colors.textPrimary
        title: "Tank Volume"
        value: TankModel.capacityLiters >= 1000
            ? TankModel.waterVolumeLiters.toLocaleString(Qt.locale(), 'f', 2) + " L"
            : TankModel.waterVolumeLiters.toFixed(2) + " L"
        subtitle: "(of " + (TankModel.capacityLiters >= 1000
            ? TankModel.capacityLiters.toLocaleString(Qt.locale(), 'f', 2) + " L"
            : TankModel.capacityLiters.toFixed(2) + " L") + ")"
    }

    // CARD 3: Status
    SummaryCard {
        Layout.fillWidth: true
        Layout.preferredHeight: Theme.metrics.summaryCardHeight
        iconGlyph: "status"
        iconColor: {
            if (TankModel.alarmLevel === "Normal") return Theme.colors.success;
            if (TankModel.alarmLevel === "Warning") return Theme.colors.warning;
            return Theme.colors.critical;
        }
        valueColor: {
            if (TankModel.alarmLevel === "Normal") return Theme.colors.successText;
            if (TankModel.alarmLevel === "Warning") return Theme.colors.warning;
            return Theme.colors.critical;
        }
        title: "Status"
        value: (TankModel.alarmLevel === "" || TankModel.alarmLevel === "Unassessed") ? "Normal" : TankModel.alarmLevel
        subtitle: {
            if (TankModel.alarmLevel === "Critical") return "(Below 10%)";
            if (TankModel.alarmLevel === "Warning") return "(Below 20%)";
            if (TankModel.alarmLevel === "Overflow") return "(Over 100%)";
            return "(Normal Range)";
        }
    }

    // CARD 4: Temperature
    SummaryCard {
        Layout.fillWidth: true
        Layout.preferredHeight: Theme.metrics.summaryCardHeight
        iconGlyph: "thermometer"
        iconColor: Theme.colors.warning
        valueColor: Theme.colors.textPrimary
        title: "Temperature"
        value: TankModel.hasTemperature ? TankModel.temperature.toFixed(2) + " \u00B0C" : "25.00 \u00B0C"
    }

    // CARD 5: Last Updated
    SummaryCard {
        Layout.fillWidth: true
        Layout.preferredHeight: Theme.metrics.summaryCardHeight
        iconGlyph: "clock"
        iconColor: Theme.colors.primary
        valueColor: Theme.colors.textPrimary
        title: "Last Updated"
        value: TankModel.lastUpdated
    }
}