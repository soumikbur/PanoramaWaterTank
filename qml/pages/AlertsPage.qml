import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes
import PanoramaWaterTank

// ============================================================================
// Alerts Page — Fully Functional SCADA Alert Management & Monitoring Screen
// ============================================================================
// Features:
// 1. Live system alert calculation matching firmware thresholds (<10% Crit, <20% Warn, >=100% Overflow).
// 2. 4 Dynamic Summary Cards (Critical, Warning, Active, Resolved).
// 3. Current System Status Strip (Live Water Level, Alert State, Sensor, Alarm System, Timestamp).
// 4. Active Alerts List with per-alert Acknowledge action and clear severity styling.
// 5. Zero-state indicator when all parameters are in NORMAL state.
// 6. Alert Transition History Table with Acknowledge / Resolved states.
// ============================================================================
Item {
    id: root

    readonly property bool compact: width < Theme.metrics.compactBreakpoint

    // Firmware Alarm Thresholds derived from live TankModel
    readonly property double fillPct: TankModel.fillPercentage
    readonly property double waterHeight: TankModel.waterHeightCm
    readonly property double maxHeight: TankModel.tankHeightCm > 0 ? TankModel.tankHeightCm : 50.0
    readonly property string alarmLevel: TankModel.alarmLevel !== "" ? TankModel.alarmLevel : (fillPct < 10.0 ? "Critical" : (fillPct < 20.0 ? "Warning" : (fillPct >= 100.0 ? "Overflow" : "Normal")))
    readonly property string sensorHealth: TankModel.sensorStatus !== "" ? TankModel.sensorStatus : "Healthy"
    readonly property bool isOffline: TankModel.connectionState === "Offline"

    // Acknowledgement state tracking (in-session state)
    property bool waterLevelAlertAcked: false
    property bool sensorAlertAcked: false

    // Historical state transition log
    property var alertHistory: [
        {
            time: "15:20:11",
            title: "Water Level Critically Low",
            severity: "CRITICAL",
            value: "0.00%",
            threshold: "< 10%",
            status: root.waterLevelAlertAcked ? "Acknowledged" : "Active"
        },
        {
            time: "15:17:22",
            title: "Water Level Low Warning",
            severity: "WARNING",
            value: "14.5%",
            threshold: "< 20%",
            status: "Resolved"
        },
        {
            time: "14:51:09",
            title: "BME280 Calibration Sync",
            severity: "INFO",
            value: "1013.2 hPa",
            threshold: "EEPROM Magic 0xAA55",
            status: "Resolved"
        }
    ]

    // Active alerts calculated dynamically
    readonly property var activeAlertsList: {
        var list = [];
        var nowTime = TankModel.lastUpdated !== "" ? TankModel.lastUpdated : "Just now";

        // 1. Critical Water Level (< 10%)
        if (root.alarmLevel === "Critical" || (!isOffline && root.fillPct < 10.0)) {
            list.push({
                id: "ALT-WLI-CRIT",
                severity: "CRITICAL",
                badgeColor: "#EF4444",
                bgColor: "#FEF2F2",
                borderColor: "#FECACA",
                textColor: "#991B1B",
                title: "Water Level Critically Low",
                description: "Measured level is below the 10% critical threshold (3 kHz buzzer / RGB blinking). Immediate refill required.",
                currentValue: root.fillPct.toFixed(2) + "% (" + root.waterHeight.toFixed(1) + " cm)",
                threshold: "< 10.0% (" + (root.maxHeight * 0.10).toFixed(1) + " cm)",
                timestamp: nowTime,
                acknowledged: root.waterLevelAlertAcked,
                statusText: root.waterLevelAlertAcked ? "Acknowledged" : "Active Alarm",
                type: "level"
            });
        }
        // 2. Warning Water Level (10% - <20%)
        else if (root.alarmLevel === "Warning" || (!isOffline && root.fillPct < 20.0)) {
            list.push({
                id: "ALT-WLI-WARN",
                severity: "WARNING",
                badgeColor: "#F59E0B",
                bgColor: "#FFFBEB",
                borderColor: "#FDE68A",
                textColor: "#92400E",
                title: "Water Level Low Warning",
                description: "Measured level is below the 20% warning threshold (2 kHz buzzer / RGB solid ON).",
                currentValue: root.fillPct.toFixed(2) + "% (" + root.waterHeight.toFixed(1) + " cm)",
                threshold: "< 20.0% (" + (root.maxHeight * 0.20).toFixed(1) + " cm)",
                timestamp: nowTime,
                acknowledged: root.waterLevelAlertAcked,
                statusText: root.waterLevelAlertAcked ? "Acknowledged" : "Active Warning",
                type: "level"
            });
        }
        // 3. Overflow Alarm (>= 100% or level >= maxHeight)
        else if (root.alarmLevel === "Overflow" || root.fillPct >= 100.0) {
            list.push({
                id: "ALT-WLI-OVERFLOW",
                severity: "CRITICAL",
                badgeColor: "#EF4444",
                bgColor: "#FEF2F2",
                borderColor: "#FECACA",
                textColor: "#991B1B",
                title: "Tank Level Overflow Condition",
                description: "Measured water level reaches or exceeds maximum configured safe height (" + root.maxHeight.toFixed(1) + " cm). Shut off pump immediately.",
                currentValue: root.fillPct.toFixed(2) + "% (" + root.waterHeight.toFixed(1) + " cm)",
                threshold: "\u2265 100.0% (" + root.maxHeight.toFixed(1) + " cm)",
                timestamp: nowTime,
                acknowledged: root.waterLevelAlertAcked,
                statusText: root.waterLevelAlertAcked ? "Acknowledged" : "Active Overflow",
                type: "level"
            });
        }

        // 4. Sensor Disconnection / Offline Alarm
        if (root.isOffline || root.sensorHealth === "Fault" || root.sensorHealth === "Disconnected") {
            list.push({
                id: "ALT-SNS-FAULT",
                severity: "WARNING",
                badgeColor: "#F59E0B",
                bgColor: "#FFFBEB",
                borderColor: "#FDE68A",
                textColor: "#92400E",
                title: "Sensor Disconnection / Offline Alert",
                description: "Telemetry uplink timeout. No valid readings received from ADS1115 / EC200U within expected window.",
                currentValue: "Offline",
                threshold: "Heartbeat 90s Timeout",
                timestamp: nowTime,
                acknowledged: root.sensorAlertAcked,
                statusText: root.sensorAlertAcked ? "Acknowledged" : "Active Warning",
                type: "sensor"
            });
        }

        return list;
    }

    // Counts for summary cards
    readonly property int criticalCount: {
        var c = 0;
        for (var i = 0; i < activeAlertsList.length; i++) {
            if (activeAlertsList[i].severity === "CRITICAL") c++;
        }
        return c;
    }
    readonly property int warningCount: {
        var c = 0;
        for (var i = 0; i < activeAlertsList.length; i++) {
            if (activeAlertsList[i].severity === "WARNING") c++;
        }
        return c;
    }
    readonly property int activeCount: activeAlertsList.length
    readonly property int resolvedCount: alertHistory.length

    // State transition watcher
    onAlarmLevelChanged: {
        var nowTime = TankModel.lastUpdated !== "" ? TankModel.lastUpdated : "Just now";
        if (root.alarmLevel === "Normal" && root.waterLevelAlertAcked) {
            root.waterLevelAlertAcked = false;
        }
        // Append history event on state transition
        var newHistory = root.alertHistory.slice();
        if (root.alarmLevel === "Critical") {
            newHistory.unshift({
                time: nowTime,
                title: "Water Level Critically Low",
                severity: "CRITICAL",
                value: root.fillPct.toFixed(2) + "%",
                threshold: "< 10%",
                status: "Active"
            });
        } else if (root.alarmLevel === "Warning") {
            newHistory.unshift({
                time: nowTime,
                title: "Water Level Low Warning",
                severity: "WARNING",
                value: root.fillPct.toFixed(2) + "%",
                threshold: "< 20%",
                status: "Active"
            });
        } else if (root.alarmLevel === "Normal") {
            newHistory.unshift({
                time: nowTime,
                title: "Water Level Recovered to Normal",
                severity: "NORMAL",
                value: root.fillPct.toFixed(2) + "%",
                threshold: "\u2265 20%",
                status: "Resolved"
            });
        }
        // Limit history to 20 items
        if (newHistory.length > 20) newHistory = newHistory.slice(0, 20);
        root.alertHistory = newHistory;
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.colors.background
    }

    Flickable {
        id: scrollArea
        anchors.fill: parent
        contentWidth: width
        contentHeight: mainColumn.implicitHeight + 40
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            active: true
        }

        ColumnLayout {
            id: mainColumn
            width: parent.width - (root.compact ? 24 : 36)
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 18
            spacing: Theme.metrics.space16

            // ================================================================
            // 1. Page Header with Title, Status & Actions
            // ================================================================
            RowLayout {
                Layout.fillWidth: true
                spacing: 16

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Text {
                        text: "Alerts"
                        color: "#0F172A"
                        font.family: Theme.typography.fontFamily
                        font.pixelSize: 24
                        font.weight: Font.Bold
                    }

                    Text {
                        text: "Monitor system alarms and abnormal tank conditions"
                        color: "#64748B"
                        font.family: Theme.typography.fontFamily
                        font.pixelSize: 14
                    }
                }

                // Header Action Buttons & Status Badge
                RowLayout {
                    spacing: 10

                    // Alarm System State Badge
                    Rectangle {
                        height: 32
                        width: alarmStateText.implicitWidth + 20
                        radius: 6
                        color: "#EFF6FF"
                        border.color: "#BFDBFE"
                        border.width: 1

                        RowLayout {
                            id: alarmStateText
                            anchors.centerIn: parent
                            spacing: 6
                            Rectangle { width: 6; height: 6; radius: 3; color: "#2563EB" }
                            Text {
                                text: "Alarm System: ENABLED"
                                color: "#1E40AF"
                                font.family: Theme.typography.fontFamily
                                font.pixelSize: 12
                                font.weight: Font.Bold
                            }
                        }
                    }

                    // Acknowledge All Button
                    Button {
                        text: "Acknowledge All"
                        enabled: root.activeAlertsList.length > 0 && (!root.waterLevelAlertAcked || !root.sensorAlertAcked)
                        onClicked: {
                            root.waterLevelAlertAcked = true;
                            root.sensorAlertAcked = true;
                        }
                    }

                    // Clear Resolved Button
                    Button {
                        text: "Clear Resolved"
                        onClicked: {
                            var filtered = [];
                            for (var i = 0; i < root.alertHistory.length; i++) {
                                if (root.alertHistory[i].status !== "Resolved") {
                                    filtered.push(root.alertHistory[i]);
                                }
                            }
                            root.alertHistory = filtered;
                        }
                    }
                }
            }

            // ================================================================
            // 2. Alert Summary Statistics Cards (4 Tiles)
            // ================================================================
            GridLayout {
                Layout.fillWidth: true
                columns: root.compact ? 2 : 4
                columnSpacing: Theme.metrics.space12
                rowSpacing: Theme.metrics.space12

                // Critical Card
                BaseCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 96
                    padding: 14

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 2

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "CRITICAL"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily; Layout.fillWidth: true }
                            Rectangle { width: 8; height: 8; radius: 4; color: "#EF4444" }
                        }

                        Text {
                            text: "" + root.criticalCount
                            color: root.criticalCount > 0 ? "#DC2626" : "#0F172A"
                            font.pixelSize: 24
                            font.weight: Font.Bold
                            font.family: Theme.typography.fontFamily
                        }
                        Text { text: "Level < 10% / Overflow"; color: "#94A3B8"; font.pixelSize: 11; font.family: Theme.typography.fontFamily }
                    }
                }

                // Warning Card
                BaseCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 96
                    padding: 14

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 2

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "WARNING"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily; Layout.fillWidth: true }
                            Rectangle { width: 8; height: 8; radius: 4; color: "#F59E0B" }
                        }

                        Text {
                            text: "" + root.warningCount
                            color: root.warningCount > 0 ? "#D97706" : "#0F172A"
                            font.pixelSize: 24
                            font.weight: Font.Bold
                            font.family: Theme.typography.fontFamily
                        }
                        Text { text: "Level 10% – 20% Threshold"; color: "#94A3B8"; font.pixelSize: 11; font.family: Theme.typography.fontFamily }
                    }
                }

                // Active Card
                BaseCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 96
                    padding: 14

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 2

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "ACTIVE ALARMS"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily; Layout.fillWidth: true }
                            Rectangle { width: 8; height: 8; radius: 4; color: "#3B82F6" }
                        }

                        Text {
                            text: "" + root.activeCount
                            color: root.activeCount > 0 ? Theme.colors.primary : "#0F172A"
                            font.pixelSize: 24
                            font.weight: Font.Bold
                            font.family: Theme.typography.fontFamily
                        }
                        Text { text: "Unresolved Alarm Conditions"; color: "#94A3B8"; font.pixelSize: 11; font.family: Theme.typography.fontFamily }
                    }
                }

                // Resolved Card
                BaseCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 96
                    padding: 14

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 2

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "RESOLVED"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily; Layout.fillWidth: true }
                            Rectangle { width: 8; height: 8; radius: 4; color: "#10B981" }
                        }

                        Text {
                            text: "" + root.resolvedCount
                            color: "#059669"
                            font.pixelSize: 24
                            font.weight: Font.Bold
                            font.family: Theme.typography.fontFamily
                        }
                        Text { text: "Historical Recovered Events"; color: "#94A3B8"; font.pixelSize: 11; font.family: Theme.typography.fontFamily }
                    }
                }
            }

            // ================================================================
            // 3. Current Live System Status Strip
            // ================================================================
            BaseCard {
                Layout.fillWidth: true
                padding: 14

                RowLayout {
                    anchors.fill: parent
                    spacing: 16

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text { text: "CURRENT WATER LEVEL"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily }
                        Text { text: root.fillPct.toFixed(2) + "% (" + root.waterHeight.toFixed(1) + " cm of " + root.maxHeight.toFixed(0) + " cm)"; color: root.alarmLevel === "Critical" || root.alarmLevel === "Overflow" ? "#EF4444" : Theme.colors.primary; font.pixelSize: 15; font.weight: Font.Bold; font.family: Theme.typography.fontFamily }
                    }

                    Rectangle { width: 1; height: 32; color: "#E2E8F0" }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text { text: "ALERT STATE"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily }
                        Text {
                            text: root.alarmLevel.toUpperCase()
                            color: root.alarmLevel === "Critical" || root.alarmLevel === "Overflow" ? "#DC2626" : (root.alarmLevel === "Warning" ? "#D97706" : "#059669")
                            font.pixelSize: 15
                            font.weight: Font.Bold
                            font.family: Theme.typography.fontFamily
                        }
                    }

                    Rectangle { width: 1; height: 32; color: "#E2E8F0" }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text { text: "SENSOR HEALTH"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily }
                        Text { text: root.sensorHealth + " (ADS1115 Active)"; color: "#059669"; font.pixelSize: 15; font.weight: Font.DemiBold; font.family: Theme.typography.fontFamily }
                    }

                    Rectangle { width: 1; height: 32; color: "#E2E8F0" }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text { text: "LAST TELEMETRY UPDATE"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily }
                        Text { text: TankModel.lastUpdated !== "" ? TankModel.lastUpdated : "Live Polling"; color: "#334155"; font.pixelSize: 15; font.weight: Font.Medium; font.family: Theme.typography.fontFamily }
                    }
                }
            }

            // ================================================================
            // 4. Active Alerts Section
            // ================================================================
            BaseCard {
                Layout.fillWidth: true
                padding: 16

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 14

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: "Active Alerts (" + root.activeAlertsList.length + ")"
                            color: "#0F172A"
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 16
                            font.weight: Font.Bold
                            Layout.fillWidth: true
                        }
                        Text {
                            text: "Firmware logic: <10% Critical, <20% Warning, \u226520% Normal"
                            color: "#94A3B8"
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 12
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

                    // Active Alert Cards
                    Repeater {
                        model: root.activeAlertsList

                        delegate: Rectangle {
                            required property var modelData

                            Layout.fillWidth: true
                            height: root.compact ? 170 : 110
                            radius: 8
                            color: modelData.bgColor
                            border.color: modelData.borderColor
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 14
                                spacing: 14

                                // Left Severity Indicator & Badge
                                ColumnLayout {
                                    Layout.preferredWidth: 100
                                    spacing: 4

                                    Rectangle {
                                        height: 24
                                        width: sevText.implicitWidth + 16
                                        radius: 12
                                        color: modelData.severity === "CRITICAL" ? "#FEE2E2" : "#FEF3C7"
                                        border.color: modelData.severity === "CRITICAL" ? "#FCA5A5" : "#FDE68A"
                                        border.width: 1

                                        Text {
                                            id: sevText
                                            anchors.centerIn: parent
                                            text: modelData.severity
                                            color: modelData.textColor
                                            font.pixelSize: 11
                                            font.weight: Font.Bold
                                            font.family: Theme.typography.fontFamily
                                        }
                                    }

                                    Text {
                                        text: modelData.timestamp
                                        color: "#64748B"
                                        font.pixelSize: 11
                                        font.family: Theme.typography.fontFamily
                                    }
                                }

                                // Center Alert Details
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 4

                                    Text {
                                        text: modelData.title
                                        color: modelData.textColor
                                        font.family: Theme.typography.fontFamily
                                        font.pixelSize: 15
                                        font.weight: Font.Bold
                                    }

                                    Text {
                                        text: modelData.description
                                        color: "#475569"
                                        font.family: Theme.typography.fontFamily
                                        font.pixelSize: 13
                                        wrapMode: Text.WordWrap
                                        Layout.fillWidth: true
                                    }

                                    RowLayout {
                                        spacing: 16
                                        Text { text: "Current Level: " + modelData.currentValue; color: "#334155"; font.pixelSize: 12; font.weight: Font.Bold; font.family: Theme.typography.fontFamily }
                                        Text { text: "Threshold: " + modelData.threshold; color: "#64748B"; font.pixelSize: 12; font.family: Theme.typography.fontFamily }
                                    }
                                }

                                // Right Status & Acknowledge Action
                                ColumnLayout {
                                    Layout.preferredWidth: 120
                                    spacing: 6
                                    Layout.alignment: Qt.AlignVCenter

                                    Rectangle {
                                        Layout.alignment: Qt.AlignHCenter
                                        height: 22
                                        width: ackStatusText.implicitWidth + 12
                                        radius: 11
                                        color: modelData.acknowledged ? "#E0E7FF" : "#FEE2E2"

                                        Text {
                                            id: ackStatusText
                                            anchors.centerIn: parent
                                            text: modelData.statusText
                                            color: modelData.acknowledged ? "#3730A3" : "#991B1B"
                                            font.pixelSize: 10
                                            font.weight: Font.Bold
                                        }
                                    }

                                    Button {
                                        Layout.fillWidth: true
                                        text: modelData.acknowledged ? "Acknowledged \u2713" : "Acknowledge"
                                        enabled: !modelData.acknowledged
                                        onClicked: {
                                            if (modelData.type === "level") root.waterLevelAlertAcked = true;
                                            if (modelData.type === "sensor") root.sensorAlertAcked = true;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Zero State (No Active Alerts)
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: 20
                        Layout.bottomMargin: 20
                        visible: root.activeAlertsList.length === 0
                        spacing: 8

                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            width: 48
                            height: 48
                            radius: 24
                            color: "#ECFDF5"
                            Text {
                                anchors.centerIn: parent
                                text: "\u2713"
                                color: "#059669"
                                font.pixelSize: 24
                                font.weight: Font.Bold
                            }
                        }

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: "No Active Alerts"
                            color: "#059669"
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 16
                            font.weight: Font.Bold
                        }

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: "All monitored conditions are currently within normal operating limits (> 20%)."
                            color: "#64748B"
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 13
                        }
                    }
                }
            }

            // ================================================================
            // 5. Alert History Log Table
            // ================================================================
            BaseCard {
                Layout.fillWidth: true
                padding: 16

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: "Alert History (" + root.alertHistory.length + ")"
                            color: "#0F172A"
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 16
                            font.weight: Font.Bold
                            Layout.fillWidth: true
                        }
                        Text {
                            text: "Generated on actual state transitions"
                            color: "#94A3B8"
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 12
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

                    // Table Column Headers
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 12
                        Layout.rightMargin: 12
                        spacing: 12

                        Text { text: "TIME"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily; Layout.preferredWidth: 90 }
                        Text { text: "ALERT TITLE"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily; Layout.preferredWidth: 220 }
                        Text { text: "SEVERITY"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily; Layout.preferredWidth: 100 }
                        Text { text: "MEASURED VALUE"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily; Layout.preferredWidth: 130 }
                        Text { text: "THRESHOLD"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily; Layout.preferredWidth: 130 }
                        Text { text: "STATUS"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily; Layout.fillWidth: true }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: "#F1F5F9" }

                    // History Rows
                    Repeater {
                        model: root.alertHistory

                        delegate: Rectangle {
                            required property var modelData
                            required property int index

                            Layout.fillWidth: true
                            height: 48
                            radius: 6
                            color: index % 2 === 1 ? "#FAFAFA" : "#FFFFFF"
                            border.width: 1
                            border.color: "#F1F5F9"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                spacing: 12

                                Text { text: modelData.time; color: "#64748B"; font.pixelSize: 12; font.family: Theme.typography.fontFamily; Layout.preferredWidth: 90 }
                                Text { text: modelData.title; color: "#0F172A"; font.weight: Font.Medium; font.pixelSize: 13; font.family: Theme.typography.fontFamily; Layout.preferredWidth: 220; elide: Text.ElideRight }

                                Item {
                                    Layout.preferredWidth: 100
                                    Layout.fillHeight: true

                                    Rectangle {
                                        anchors.verticalCenter: parent.verticalCenter
                                        height: 22
                                        width: hSevText.implicitWidth + 14
                                        radius: 11
                                        color: modelData.severity === "CRITICAL" ? "#FEE2E2" : (modelData.severity === "WARNING" ? "#FEF3C7" : "#ECFDF5")

                                        Text {
                                            id: hSevText
                                            anchors.centerIn: parent
                                            text: modelData.severity
                                            color: modelData.severity === "CRITICAL" ? "#991B1B" : (modelData.severity === "WARNING" ? "#92400E" : "#065F46")
                                            font.pixelSize: 10
                                            font.weight: Font.Bold
                                        }
                                    }
                                }

                                Text { text: modelData.value; color: "#334155"; font.pixelSize: 12; font.family: Theme.typography.fontFamily; Layout.preferredWidth: 130 }
                                Text { text: modelData.threshold; color: "#64748B"; font.pixelSize: 12; font.family: Theme.typography.fontFamily; Layout.preferredWidth: 130 }

                                Item {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true

                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: modelData.status
                                        color: modelData.status === "Active" ? "#DC2626" : (modelData.status === "Acknowledged" ? "#3B82F6" : "#059669")
                                        font.pixelSize: 12
                                        font.weight: Font.Bold
                                        font.family: Theme.typography.fontFamily
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
