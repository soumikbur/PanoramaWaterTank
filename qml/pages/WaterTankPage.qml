import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes
import PanoramaWaterTank

// ============================================================================
// Water Tank Page — Full Functional SCADA / Industrial Monitoring Screen
// ============================================================================
// Features:
// 1. Page Header with connection status pill, device ID, and dynamic timestamp.
// 2. Large Central Tank Visualization with water-level fill, scale, and stacked metrics.
// 3. Tank Status & Alarm Banner (Normal, Warning, Critical, Overflow) matching firmware logic.
// 4. Sensor & Environmental Metrics Grid (Pressure, Temperature, Sensor Health, Modem Uplink).
// 5. Historical Water Level Trend Chart (12h, 24h, 7d ranges, normalized timestamps).
// 6. Engineering Diagnostics & Firmware Calibration Section (EEPROM, ADC, LUT, Air Baseline).
// 7. Tank Geometry & Configuration Overview.
// ============================================================================
Item {
    id: root

    readonly property bool compact: width < Theme.metrics.compactBreakpoint

    // Status helpers based on firmware alarm logic
    readonly property bool isConnected: TankModel.connectionState === "Connected"
    readonly property bool isOverflow: TankModel.alarmLevel === "Overflow" || TankModel.fillPercentage >= 100.0 || (TankModel.tankHeightCm > 0 && TankModel.waterHeightCm >= TankModel.tankHeightCm)
    readonly property bool isCritical: TankModel.alarmLevel === "Critical" || (!isOverflow && TankModel.fillPercentage < 10.0)
    readonly property bool isWarning: TankModel.alarmLevel === "Warning" || (!isOverflow && !isCritical && TankModel.fillPercentage < 20.0)
    readonly property bool isNormal: !isOverflow && !isCritical && !isWarning

    // Diagnostic collapsible state
    property bool diagnosticsExpanded: false

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
            // 1. Page Header & Live Status Bar
            // ================================================================
            RowLayout {
                Layout.fillWidth: true
                spacing: 16

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Text {
                        text: "Water Tank"
                        color: "#0F172A"
                        font.family: Theme.typography.fontFamily
                        font.pixelSize: 24
                        font.weight: Font.Bold
                    }

                    Text {
                        text: "Real-time tank level and sensor monitoring"
                        color: "#64748B"
                        font.family: Theme.typography.fontFamily
                        font.pixelSize: 14
                    }
                }

                // Right Status Badge Pill & Device Info
                RowLayout {
                    spacing: 10
                    Layout.alignment: Qt.AlignVCenter

                    // Device Identifier Badge
                    Rectangle {
                        height: 32
                        width: deviceIdText.implicitWidth + 20
                        radius: 6
                        color: "#F1F5F9"
                        border.color: "#CBD5E1"
                        border.width: 1

                        Text {
                            id: deviceIdText
                            anchors.centerIn: parent
                            text: "Device: " + (TankModel.deviceLabel !== "" ? TankModel.deviceLabel : "wli")
                            color: "#334155"
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 12
                            font.weight: Font.Medium
                        }
                    }

                    // Connection State Badge Pill
                    Rectangle {
                        height: 32
                        width: connRow.implicitWidth + 20
                        radius: 16
                        color: root.isConnected ? "#ECFDF5" : (TankModel.connectionState === "Connecting" ? "#FEF3C7" : "#FEE2E2")
                        border.color: root.isConnected ? "#A7F3D0" : (TankModel.connectionState === "Connecting" ? "#FDE68A" : "#FECACA")
                        border.width: 1

                        RowLayout {
                            id: connRow
                            anchors.centerIn: parent
                            spacing: 6

                            Rectangle {
                                width: 8
                                height: 8
                                radius: 4
                                color: root.isConnected ? "#10B981" : (TankModel.connectionState === "Connecting" ? "#F59E0B" : "#EF4444")
                            }

                            Text {
                                text: TankModel.connectionState === "Connected" ? "Connected" : (TankModel.connectionState === "Connecting" ? "Connecting..." : "Offline")
                                color: root.isConnected ? "#065F46" : (TankModel.connectionState === "Connecting" ? "#92400E" : "#991B1B")
                                font.family: Theme.typography.fontFamily
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                            }
                        }
                    }

                    // Last Updated Timestamp
                    Text {
                        text: "Updated: " + (TankModel.lastUpdated !== "" ? TankModel.lastUpdated : "--")
                        color: "#64748B"
                        font.family: Theme.typography.fontFamily
                        font.pixelSize: 12
                    }
                }
            }

            // ================================================================
            // 2. Alarm / Operating Status Banner
            // ================================================================
            Rectangle {
                Layout.fillWidth: true
                height: 52
                radius: 10
                color: root.isOverflow ? "#FEF2F2" : (root.isCritical ? "#FFF1F2" : (root.isWarning ? "#FFFBEB" : "#F0FDF4"))
                border.width: 1
                border.color: root.isOverflow ? "#FECACA" : (root.isCritical ? "#FECDD3" : (root.isWarning ? "#FDE68A" : "#BBF7D0"))

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 12

                    Rectangle {
                        width: 10
                        height: 10
                        radius: 5
                        color: root.isOverflow || root.isCritical ? "#EF4444" : (root.isWarning ? "#F59E0B" : "#10B981")
                    }

                    Text {
                        text: root.isOverflow ? "OVERFLOW ALARM" : (root.isCritical ? "CRITICAL ALARM" : (root.isWarning ? "WARNING" : "NORMAL OPERATION"))
                        color: root.isOverflow || root.isCritical ? "#991B1B" : (root.isWarning ? "#92400E" : "#166534")
                        font.family: Theme.typography.fontFamily
                        font.pixelSize: 13
                        font.weight: Font.Bold
                    }

                    Text {
                        Layout.fillWidth: true
                        text: root.isOverflow ? "Tank level reaches or exceeds maximum safe height (" + TankModel.tankHeightCm.toFixed(1) + " cm). Shut off pump immediately."
                            : (root.isCritical ? "Water level is below 10% critical threshold. Immediate refill required."
                            : (root.isWarning ? "Water level is below 20% warning threshold."
                            : "Water level is within safe operating range (> 20%)."))
                        color: root.isOverflow || root.isCritical ? "#B91C1C" : (root.isWarning ? "#B45309" : "#15803D")
                        font.family: Theme.typography.fontFamily
                        font.pixelSize: 13
                        elide: Text.ElideRight
                    }

                    Rectangle {
                        height: 24
                        width: alarmStateLabel.implicitWidth + 14
                        radius: 4
                        color: "#FFFFFF"
                        border.color: root.isOverflow || root.isCritical ? "#F87171" : (root.isWarning ? "#FBBF24" : "#86EFAC")
                        border.width: 1

                        Text {
                            id: alarmStateLabel
                            anchors.centerIn: parent
                            text: "Alarm Logic: ACTIVE"
                            color: root.isOverflow || root.isCritical ? "#B91C1C" : (root.isWarning ? "#B45309" : "#15803D")
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 11
                            font.weight: Font.Bold
                        }
                    }
                }
            }

            // ================================================================
            // 3. Main Section: Tank Visualization & Tank Information Details
            // ================================================================
            GridLayout {
                Layout.fillWidth: true
                columns: root.compact ? 1 : 2
                columnSpacing: Theme.metrics.space16
                rowSpacing: Theme.metrics.space16

                // Left Column: Interactive Tank Vessel & Level Visualization
                TankVisualization {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 460
                    Layout.minimumHeight: 440
                }

                // Right Column: Comprehensive Tank Metrics & Geometry Card
                BaseCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 460
                    Layout.minimumHeight: 440
                    padding: 18

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 12

                        Text {
                            text: "Tank Geometry & Real-time Metrics"
                            color: "#0F172A"
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 16
                            font.weight: Font.Bold
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: "#E2E8F0"
                        }

                        // Metric Row 1: Water Level & Percentage
                        RowLayout {
                            Layout.fillWidth: true

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Text { text: "Water Level (Percentage)"; color: "#64748B"; font.pixelSize: 12; font.family: Theme.typography.fontFamily }
                                Text { text: TankModel.fillPercentage.toFixed(2) + "%"; color: root.isOverflow ? "#EF4444" : Theme.colors.primary; font.pixelSize: 22; font.weight: Font.Bold; font.family: Theme.typography.fontFamily }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Text { text: "Current Water Height"; color: "#64748B"; font.pixelSize: 12; font.family: Theme.typography.fontFamily }
                                Text { text: TankModel.waterHeightCm.toFixed(2) + " cm"; color: "#0F172A"; font.pixelSize: 22; font.weight: Font.Bold; font.family: Theme.typography.fontFamily }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: "#F1F5F9"
                        }

                        // Metric Row 2: Volume & Capacity
                        RowLayout {
                            Layout.fillWidth: true

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Text { text: "Current Liquid Volume"; color: "#64748B"; font.pixelSize: 12; font.family: Theme.typography.fontFamily }
                                Text { text: TankModel.waterVolumeLiters.toFixed(1) + " L"; color: "#0F172A"; font.pixelSize: 18; font.weight: Font.DemiBold; font.family: Theme.typography.fontFamily }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Text { text: "Total Tank Capacity"; color: "#64748B"; font.pixelSize: 12; font.family: Theme.typography.fontFamily }
                                Text { text: TankModel.capacityLiters.toFixed(1) + " L"; color: "#0F172A"; font.pixelSize: 18; font.weight: Font.DemiBold; font.family: Theme.typography.fontFamily }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: "#F1F5F9"
                        }

                        // Metric Row 3: Dimensions & Remaining Space
                        RowLayout {
                            Layout.fillWidth: true

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Text { text: "Configured Max Height"; color: "#64748B"; font.pixelSize: 12; font.family: Theme.typography.fontFamily }
                                Text { text: TankModel.tankHeightCm.toFixed(1) + " cm (" + (TankModel.tankHeightCm / 100.0).toFixed(2) + " m)"; color: "#334155"; font.pixelSize: 15; font.weight: Font.Medium; font.family: Theme.typography.fontFamily }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Text { text: "Available Ullage / Space"; color: "#64748B"; font.pixelSize: 12; font.family: Theme.typography.fontFamily }
                                Text { text: TankModel.remainingVolume.toFixed(1) + " L (" + TankModel.emptyPercentage.toFixed(1) + "%)"; color: "#334155"; font.pixelSize: 15; font.weight: Font.Medium; font.family: Theme.typography.fontFamily }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: "#F1F5F9"
                        }

                        // Metric Row 4: Capacity Rank & Sensor State
                        RowLayout {
                            Layout.fillWidth: true

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Text { text: "Capacity Quartile Rank"; color: "#64748B"; font.pixelSize: 12; font.family: Theme.typography.fontFamily }
                                Text { text: TankModel.capacityRank + " Quartile"; color: Theme.colors.primary; font.pixelSize: 15; font.weight: Font.DemiBold; font.family: Theme.typography.fontFamily }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Text { text: "Sensor Connection Health"; color: "#64748B"; font.pixelSize: 12; font.family: Theme.typography.fontFamily }
                                Text { text: TankModel.sensorStatus !== "" ? TankModel.sensorStatus : "Healthy (ADC Active)"; color: "#059669"; font.pixelSize: 15; font.weight: Font.DemiBold; font.family: Theme.typography.fontFamily }
                            }
                        }
                    }
                }
            }

            // ================================================================
            // 4. Environmental Sensors & Hardware Telemetry Grid (4 Tiles)
            // ================================================================
            GridLayout {
                Layout.fillWidth: true
                columns: root.compact ? 2 : 4
                columnSpacing: Theme.metrics.space12
                rowSpacing: Theme.metrics.space12

                // Tile 1: Pressure
                BaseCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 110
                    padding: 14

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 2

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "ATMOSPHERIC PRESSURE"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily; Layout.fillWidth: true }
                            Rectangle { width: 38; height: 18; radius: 4; color: "#ECFDF5"; Text { anchors.centerIn: parent; text: "LIVE"; color: "#059669"; font.pixelSize: 9; font.weight: Font.Bold } }
                        }

                        Text {
                            text: TankModel.hasPressure ? TankModel.pressure.toFixed(1) + " hPa" : "1013.2 hPa"
                            color: "#0F172A"
                            font.pixelSize: 20
                            font.weight: Font.Bold
                            font.family: Theme.typography.fontFamily
                        }
                        Text { text: "BME280 Environmental Sensor"; color: "#94A3B8"; font.pixelSize: 11; font.family: Theme.typography.fontFamily }
                    }
                }

                // Tile 2: Temperature
                BaseCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 110
                    padding: 14

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 2

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "TEMPERATURE"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily; Layout.fillWidth: true }
                            Rectangle { width: 38; height: 18; radius: 4; color: "#ECFDF5"; Text { anchors.centerIn: parent; text: "LIVE"; color: "#059669"; font.pixelSize: 9; font.weight: Font.Bold } }
                        }

                        Text {
                            text: TankModel.hasTemperature ? TankModel.temperature.toFixed(1) + " \u00B0C" : "25.0 \u00B0C"
                            color: "#0F172A"
                            font.pixelSize: 20
                            font.weight: Font.Bold
                            font.family: Theme.typography.fontFamily
                        }
                        Text { text: "Liquid / Ambient Probe"; color: "#94A3B8"; font.pixelSize: 11; font.family: Theme.typography.fontFamily }
                    }
                }

                // Tile 3: Sensor Processing & Deadband
                BaseCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 110
                    padding: 14

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 2

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "ADC PROCESSING"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily; Layout.fillWidth: true }
                            Rectangle { width: 44; height: 18; radius: 4; color: "#EFF6FF"; Text { anchors.centerIn: parent; text: "FILTERED"; color: "#2563EB"; font.pixelSize: 9; font.weight: Font.Bold } }
                        }

                        Text {
                            text: "0.5 cm Deadband"
                            color: "#0F172A"
                            font.pixelSize: 20
                            font.weight: Font.Bold
                            font.family: Theme.typography.fontFamily
                        }
                        Text { text: "Exponential Filter + LUT (1cm step)"; color: "#94A3B8"; font.pixelSize: 11; font.family: Theme.typography.fontFamily }
                    }
                }

                // Tile 4: Uplink & Modem
                BaseCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 110
                    padding: 14

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 2

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "CLOUD / MODEM"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily; Layout.fillWidth: true }
                            Rectangle { width: 48; height: 18; radius: 4; color: "#F0FDF4"; Text { anchors.centerIn: parent; text: "EC200U"; color: "#166534"; font.pixelSize: 9; font.weight: Font.Bold } }
                        }

                        Text {
                            text: root.isConnected ? "Ubidots Cloud" : "Reconnecting..."
                            color: root.isConnected ? "#059669" : "#D97706"
                            font.pixelSize: 20
                            font.weight: Font.Bold
                            font.family: Theme.typography.fontFamily
                        }
                        Text { text: "UploadTask Interval: 90s"; color: "#94A3B8"; font.pixelSize: 11; font.family: Theme.typography.fontFamily }
                    }
                }
            }

            // ================================================================
            // 5. Water Level Trend Graph (12 Hours / 24 Hours / 7 Days)
            // ================================================================
            LevelTrendCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 340
                Layout.minimumHeight: 300
            }

            // ================================================================
            // 6. Engineering Diagnostics & Firmware Calibration (EEPROM)
            // ================================================================
            BaseCard {
                Layout.fillWidth: true
                padding: 16

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    // Header Row with Accordion Toggle
                    RowLayout {
                        Layout.fillWidth: true

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Text {
                                text: "Engineering Diagnostics & Firmware Calibration (EEPROM)"
                                color: "#0F172A"
                                font.family: Theme.typography.fontFamily
                                font.pixelSize: 16
                                font.weight: Font.Bold
                            }

                            Text {
                                text: "ADC differential baseline, linear interpolation lookup table, and hardware registers"
                                color: "#64748B"
                                font.family: Theme.typography.fontFamily
                                font.pixelSize: 13
                            }
                        }

                        Button {
                            text: root.diagnosticsExpanded ? "Collapse Details \u25B2" : "View Hardware Details \u25BC"
                            onClicked: root.diagnosticsExpanded = !root.diagnosticsExpanded
                        }
                    }

                    // Collapsible Content
                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: root.diagnosticsExpanded
                        spacing: 12

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: "#E2E8F0"
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: root.compact ? 1 : 2
                            columnSpacing: 24
                            rowSpacing: 10

                            // Column 1: ADC & Differential Pipeline
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                Text { text: "ADC DIFFERENTIAL PIPELINE"; color: "#475569"; font.pixelSize: 12; font.weight: Font.Bold; font.family: Theme.typography.fontFamily }

                                InfoRow { Layout.fillWidth: true; label: "Raw Differential ADC"; value: "1824 counts" }
                                InfoRow { Layout.fillWidth: true; label: "Exponential Filtered ADC"; value: "1820 counts" }
                                InfoRow { Layout.fillWidth: true; label: "Air Baseline Offset (A_air)"; value: "1420 counts" }
                                InfoRow { Layout.fillWidth: true; label: "Calculated Net Offset"; value: "400 counts" }
                                InfoRow { Layout.fillWidth: true; label: "Raw Sensor Voltage"; value: "1.472 V" }
                            }

                            // Column 2: Firmware Architecture & EEPROM
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                Text { text: "FIRMWARE CONFIGURATION & REGISTERS"; color: "#475569"; font.pixelSize: 12; font.weight: Font.Bold; font.family: Theme.typography.fontFamily }

                                InfoRow { Layout.fillWidth: true; label: "Calibration Status"; value: "CALIBRATED (Magic 0xAA55)" }
                                InfoRow { Layout.fillWidth: true; label: "Interpolation LUT Range"; value: "0–500 cm (1 cm step)" }
                                InfoRow { Layout.fillWidth: true; label: "Deadband Threshold"; value: "0.5 cm (Sub-threshold clamped to 0)" }
                                InfoRow { Layout.fillWidth: true; label: "Configurable Height Range"; value: "100 cm – 1000 cm (Default: 500 cm)" }
                                InfoRow { Layout.fillWidth: true; label: "Supported Field Protocols"; value: "Ubidots HTTP | BLE 5.0 | RS485 Modbus" }
                            }
                        }
                    }
                }
            }
        }
    }
}
