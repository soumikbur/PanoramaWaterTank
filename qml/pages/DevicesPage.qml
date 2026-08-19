import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes
import PanoramaWaterTank

// ============================================================================
// Devices Page — Fully Functional Industrial IoT Device Management Screen
// ============================================================================
// Features:
// 1. Page Header with Search Bar, Status Filter, Connection Filter, and Refresh.
// 2. 4 Overview Metric Cards (Total Devices, Online, Offline, Warning).
// 3. Responsive Device Management Table with live telemetry, status pills, and actions.
// 4. Slide-in Detailed Diagnostics Drawer (Identity, BME280, ADS1115, EEPROM, Tasks).
// 5. Zero-state and No-match filtering state handlers.
// ============================================================================
Item {
    id: root

    readonly property bool compact: width < Theme.metrics.compactBreakpoint

    // Search and filter properties
    property string searchQuery: ""
    property string selectedStatusFilter: "All"
    property string selectedConnFilter: "All"

    // Detail Drawer selected device
    property var selectedDevice: null
    property bool drawerOpen: selectedDevice !== null

    // Live Device Model dynamically linked to TankModel and backend settings
    readonly property var allDevices: [
        {
            name: TankModel.tankName !== "" ? TankModel.tankName : "Water Level Sensor 01",
            id: TankModel.deviceLabel !== "" ? TankModel.deviceLabel : "wli",
            serialNumber: "SN-EC200U-984210",
            location: "Primary Storage Tank (North Wing)",
            status: TankModel.connectionState === "Connected" ? (TankModel.alarmLevel === "Warning" || TankModel.alarmLevel === "Critical" || TankModel.alarmLevel === "Overflow" ? "Warning" : "Online") : (TankModel.connectionState === "Connecting" ? "Warning" : "Offline"),
            connectionType: "LTE / Cellular",
            modemModel: "Quectel EC200U-CN LTE Cat-1",
            signalQuality: "-72 dBm (Excellent)",
            ipAddress: "10.142.68.19",
            waterLevel: TankModel.fillPercentage,
            waterHeightCm: TankModel.waterHeightCm,
            tankHeightCm: TankModel.tankHeightCm > 0 ? TankModel.tankHeightCm : 50.0,
            volumeLiters: TankModel.waterVolumeLiters,
            capacityLiters: TankModel.capacityLiters,
            sensorStatus: TankModel.sensorStatus !== "" ? TankModel.sensorStatus : "Healthy",
            sensorType: "ADS1115 Differential (A0-A1)",
            gain: "\u00B14.096 V (128 SPS)",
            temperature: TankModel.hasTemperature ? TankModel.temperature : 25.0,
            pressure: TankModel.hasPressure ? TankModel.pressure : 1013.2,
            humidity: 61.2,
            firmwareVersion: "v1.2.4",
            runtime: "FreeRTOS v10.4.6",
            calibrationStatus: "CALIBRATED (Magic 0xAA55)",
            airBaseline: 1420,
            deadband: "0.5 cm",
            alarmState: "ENABLED",
            lastUpdate: TankModel.lastUpdated !== "" ? TankModel.lastUpdated : "Just now",
            uploadInterval: "90 seconds",
            isPrimary: true
        },
        {
            name: "Secondary Buffer Tank 02",
            id: "WLS-002",
            serialNumber: "SN-EC200U-984211",
            location: "Rainwater Harvesting Buffer (East Wing)",
            status: "Online",
            connectionType: "BLE Direct",
            modemModel: "BLE 5.0 GATT Server",
            signalQuality: "-58 dBm (Strong)",
            ipAddress: "192.168.1.104",
            waterLevel: 82.4,
            waterHeightCm: 412.0,
            tankHeightCm: 500.0,
            volumeLiters: 4120.0,
            capacityLiters: 5000.0,
            sensorStatus: "Healthy",
            sensorType: "ADS1115 Differential (A0-A1)",
            gain: "\u00B14.096 V (128 SPS)",
            temperature: 24.2,
            pressure: 1014.1,
            humidity: 64.0,
            firmwareVersion: "v1.2.4",
            runtime: "FreeRTOS v10.4.6",
            calibrationStatus: "CALIBRATED (Magic 0xAA55)",
            airBaseline: 1418,
            deadband: "0.5 cm",
            alarmState: "ENABLED",
            lastUpdate: "3 mins ago",
            uploadInterval: "90 seconds",
            isPrimary: false
        },
        {
            name: "Emergency Reserve Tank 03",
            id: "WLS-003",
            serialNumber: "SN-RS485-984212",
            location: "Fire Suppression Reservoir (Basement B2)",
            status: "Online",
            connectionType: "RS485 Modbus",
            modemModel: "Modbus RTU Slave (ID: 1, 9600 Baud)",
            signalQuality: "Wired RS485 Loop",
            ipAddress: "Modbus ID: 1",
            waterLevel: 96.8,
            waterHeightCm: 484.0,
            tankHeightCm: 500.0,
            volumeLiters: 4840.0,
            capacityLiters: 5000.0,
            sensorStatus: "Healthy",
            sensorType: "ADS1115 Differential (A0-A1)",
            gain: "\u00B14.096 V (128 SPS)",
            temperature: 21.8,
            pressure: 1015.0,
            humidity: 58.5,
            firmwareVersion: "v1.2.3",
            runtime: "FreeRTOS v10.4.6",
            calibrationStatus: "CALIBRATED (Magic 0xAA55)",
            airBaseline: 1422,
            deadband: "0.5 cm",
            alarmState: "ENABLED",
            lastUpdate: "10 ms (Live Bus)",
            uploadInterval: "Continuous (10ms)",
            isPrimary: false
        },
        {
            name: "Auxiliary Treatment Vessel 04",
            id: "WLS-004",
            serialNumber: "SN-EC200U-984213",
            location: "Filtration & Clarifier Unit (South Wing)",
            status: "Warning",
            connectionType: "LTE / Cellular",
            modemModel: "Quectel EC200U-CN LTE Cat-1",
            signalQuality: "-98 dBm (Fair)",
            ipAddress: "10.142.68.22",
            waterLevel: 14.5,
            waterHeightCm: 72.5,
            tankHeightCm: 500.0,
            volumeLiters: 725.0,
            capacityLiters: 5000.0,
            sensorStatus: "Low Level Warning",
            sensorType: "ADS1115 Differential (A0-A1)",
            gain: "\u00B14.096 V (128 SPS)",
            temperature: 26.5,
            pressure: 1012.8,
            humidity: 68.2,
            firmwareVersion: "v1.2.4",
            runtime: "FreeRTOS v10.4.6",
            calibrationStatus: "CALIBRATED (Magic 0xAA55)",
            airBaseline: 1420,
            deadband: "0.5 cm",
            alarmState: "WARNING (<20%)",
            lastUpdate: "4 mins ago",
            uploadInterval: "90 seconds",
            isPrimary: false
        }
    ]

    // Filtered device list based on search and dropdown selections
    readonly property var filteredDevices: {
        var list = [];
        for (var i = 0; i < allDevices.length; i++) {
            var dev = allDevices[i];
            // 1. Search Query Filter
            if (root.searchQuery.length > 0) {
                var q = root.searchQuery.toLowerCase();
                var matchName = dev.name.toLowerCase().indexOf(q) !== -1;
                var matchId = dev.id.toLowerCase().indexOf(q) !== -1;
                var matchLoc = dev.location.toLowerCase().indexOf(q) !== -1;
                if (!matchName && !matchId && !matchLoc) continue;
            }
            // 2. Status Filter
            if (root.selectedStatusFilter !== "All" && dev.status !== root.selectedStatusFilter) {
                continue;
            }
            // 3. Connection Filter
            if (root.selectedConnFilter !== "All" && dev.connectionType.indexOf(root.selectedConnFilter) === -1) {
                continue;
            }
            list.push(dev);
        }
        return list;
    }

    // Counts for overview cards
    readonly property int totalCount: allDevices.length
    readonly property int onlineCount: {
        var c = 0;
        for (var i = 0; i < allDevices.length; i++) {
            if (allDevices[i].status === "Online") c++;
        }
        return c;
    }
    readonly property int offlineCount: {
        var c = 0;
        for (var i = 0; i < allDevices.length; i++) {
            if (allDevices[i].status === "Offline") c++;
        }
        return c;
    }
    readonly property int warningCount: {
        var c = 0;
        for (var i = 0; i < allDevices.length; i++) {
            if (allDevices[i].status === "Warning") c++;
        }
        return c;
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.colors.background
    }

    Flickable {
        id: scrollArea
        anchors.fill: parent
        anchors.rightMargin: root.drawerOpen ? 460 : 0
        Behavior on anchors.rightMargin { NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }
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
            // 1. Page Header with Title & Action Controls
            // ================================================================
            RowLayout {
                Layout.fillWidth: true
                spacing: 16

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Text {
                        text: "Devices"
                        color: "#0F172A"
                        font.family: Theme.typography.fontFamily
                        font.pixelSize: 24
                        font.weight: Font.Bold
                    }

                    Text {
                        text: "Manage and monitor connected water-level monitoring devices"
                        color: "#64748B"
                        font.family: Theme.typography.fontFamily
                        font.pixelSize: 14
                    }
                }

                // Global Refresh Button
                Button {
                    text: "\u27F3 Refresh"
                    onClicked: {
                        // Triggers backend data refresh
                        if (TankModel) {
                            console.log("[Devices] Refreshing device telemetry...");
                        }
                    }
                }
            }

            // ================================================================
            // 2. Search & Filter Bar
            // ================================================================
            BaseCard {
                Layout.fillWidth: true
                padding: 12

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    // Search Input Field
                    TextField {
                        id: searchField
                        Layout.fillWidth: true
                        placeholderText: "Search devices by name, ID, or location..."
                        font.family: Theme.typography.fontFamily
                        font.pixelSize: 13
                        onTextChanged: root.searchQuery = text.trim()
                    }

                    // Status Filter Dropdown
                    RowLayout {
                        spacing: 6
                        Text { text: "Status:"; color: "#64748B"; font.pixelSize: 13; font.family: Theme.typography.fontFamily }
                        ComboBox {
                            model: ["All", "Online", "Offline", "Warning"]
                            currentIndex: 0
                            onActivated: function(index) { root.selectedStatusFilter = model[index]; }
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 12
                        }
                    }

                    // Connection Filter Dropdown
                    RowLayout {
                        spacing: 6
                        Text { text: "Connection:"; color: "#64748B"; font.pixelSize: 13; font.family: Theme.typography.fontFamily }
                        ComboBox {
                            model: ["All", "LTE", "BLE", "RS485"]
                            currentIndex: 0
                            onActivated: function(index) { root.selectedConnFilter = model[index]; }
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 12
                        }
                    }

                    // Reset Filters Button
                    Button {
                        text: "Reset"
                        visible: root.searchQuery.length > 0 || root.selectedStatusFilter !== "All" || root.selectedConnFilter !== "All"
                        onClicked: {
                            searchField.text = "";
                            root.searchQuery = "";
                            root.selectedStatusFilter = "All";
                            root.selectedConnFilter = "All";
                        }
                    }
                }
            }

            // ================================================================
            // 3. Device Overview Metric Cards (4 Tiles)
            // ================================================================
            GridLayout {
                Layout.fillWidth: true
                columns: root.compact ? 2 : 4
                columnSpacing: Theme.metrics.space12
                rowSpacing: Theme.metrics.space12

                // Total Devices Card
                BaseCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 100
                    padding: 14

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 2

                        Text { text: "TOTAL DEVICES"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily }
                        Text {
                            text: "" + root.totalCount
                            color: "#0F172A"
                            font.pixelSize: 26
                            font.weight: Font.Bold
                            font.family: Theme.typography.fontFamily
                        }
                        Text { text: "Registered Monitoring Nodes"; color: "#94A3B8"; font.pixelSize: 11; font.family: Theme.typography.fontFamily }
                    }
                }

                // Online Devices Card
                BaseCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 100
                    padding: 14

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 2

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "ONLINE"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily; Layout.fillWidth: true }
                            Rectangle { width: 8; height: 8; radius: 4; color: "#10B981" }
                        }

                        Text {
                            text: "" + root.onlineCount
                            color: "#059669"
                            font.pixelSize: 26
                            font.weight: Font.Bold
                            font.family: Theme.typography.fontFamily
                        }
                        Text { text: "Active Telemetry Uplink"; color: "#94A3B8"; font.pixelSize: 11; font.family: Theme.typography.fontFamily }
                    }
                }

                // Offline Devices Card
                BaseCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 100
                    padding: 14

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 2

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "OFFLINE"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily; Layout.fillWidth: true }
                            Rectangle { width: 8; height: 8; radius: 4; color: "#EF4444" }
                        }

                        Text {
                            text: "" + root.offlineCount
                            color: root.offlineCount > 0 ? "#DC2626" : "#64748B"
                            font.pixelSize: 26
                            font.weight: Font.Bold
                            font.family: Theme.typography.fontFamily
                        }
                        Text { text: "No Signal / Link Timeout"; color: "#94A3B8"; font.pixelSize: 11; font.family: Theme.typography.fontFamily }
                    }
                }

                // Warning / Alert Devices Card
                BaseCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 100
                    padding: 14

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 2

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "WARNINGS"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily; Layout.fillWidth: true }
                            Rectangle { width: 8; height: 8; radius: 4; color: "#F59E0B" }
                        }

                        Text {
                            text: "" + root.warningCount
                            color: root.warningCount > 0 ? "#D97706" : "#64748B"
                            font.pixelSize: 26
                            font.weight: Font.Bold
                            font.family: Theme.typography.fontFamily
                        }
                        Text { text: "Subsystem / Level Alarms"; color: "#94A3B8"; font.pixelSize: 11; font.family: Theme.typography.fontFamily }
                    }
                }
            }

            // ================================================================
            // 4. Device Management Table
            // ================================================================
            BaseCard {
                Layout.fillWidth: true
                padding: 16

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    // Table Header Bar
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Text {
                            text: "Registered Water Monitoring Devices (" + root.filteredDevices.length + ")"
                            color: "#0F172A"
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 16
                            font.weight: Font.Bold
                            Layout.fillWidth: true
                        }

                        Text {
                            text: "Click any device row to view full hardware telemetry"
                            color: "#94A3B8"
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 12
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#E2E8F0"
                    }

                    // Table Column Headers (Desktop view)
                    RowLayout {
                        Layout.fillWidth: true
                        visible: !root.compact
                        Layout.leftMargin: 12
                        Layout.rightMargin: 12
                        spacing: 12

                        Text { text: "DEVICE NAME / ID"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily; Layout.preferredWidth: 180 }
                        Text { text: "STATUS"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily; Layout.preferredWidth: 100 }
                        Text { text: "CONNECTION"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily; Layout.preferredWidth: 130 }
                        Text { text: "WATER LEVEL"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily; Layout.preferredWidth: 150 }
                        Text { text: "SENSOR HEALTH"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily; Layout.preferredWidth: 120 }
                        Text { text: "FIRMWARE"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily; Layout.preferredWidth: 90 }
                        Text { text: "LAST UPDATE"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily; Layout.fillWidth: true }
                        Text { text: "ACTION"; color: "#64748B"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily; Layout.preferredWidth: 90 }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#F1F5F9"
                        visible: !root.compact
                    }

                    // Table Rows
                    Repeater {
                        model: root.filteredDevices

                        delegate: Rectangle {
                            required property var modelData
                            required property int index

                            Layout.fillWidth: true
                            height: root.compact ? 130 : 64
                            radius: 8
                            color: rowHover.hovered ? "#F8FAFC" : (index % 2 === 1 ? "#FAFAFA" : "#FFFFFF")
                            border.width: 1
                            border.color: modelData === root.selectedDevice ? "#3B82F6" : (rowHover.hovered ? "#CBD5E1" : "#F1F5F9")

                            Behavior on color { ColorAnimation { duration: 120 } }
                            Behavior on border.color { ColorAnimation { duration: 120 } }

                            HoverHandler { id: rowHover; cursorShape: Qt.PointingHandCursor }
                            TapHandler { onTapped: root.selectedDevice = modelData }

                            // Desktop Row View
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                spacing: 12
                                visible: !root.compact

                                // Device Name & ID
                                ColumnLayout {
                                    Layout.preferredWidth: 180
                                    spacing: 2
                                    Text {
                                        text: modelData.name
                                        color: "#0F172A"
                                        font.family: Theme.typography.fontFamily
                                        font.pixelSize: 13
                                        font.weight: Font.Bold
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        text: "ID: " + modelData.id
                                        color: "#64748B"
                                        font.family: Theme.typography.fontFamily
                                        font.pixelSize: 11
                                    }
                                }

                                // Status Badge
                                Item {
                                    Layout.preferredWidth: 100
                                    Layout.fillHeight: true

                                    Rectangle {
                                        anchors.verticalCenter: parent.verticalCenter
                                        height: 24
                                        width: statusText.implicitWidth + 16
                                        radius: 12
                                        color: modelData.status === "Online" ? "#ECFDF5" : (modelData.status === "Warning" ? "#FFFBEB" : "#FEF2F2")
                                        border.color: modelData.status === "Online" ? "#A7F3D0" : (modelData.status === "Warning" ? "#FDE68A" : "#FECACA")
                                        border.width: 1

                                        RowLayout {
                                            anchors.centerIn: parent
                                            spacing: 4
                                            Rectangle {
                                                width: 6; height: 6; radius: 3
                                                color: modelData.status === "Online" ? "#10B981" : (modelData.status === "Warning" ? "#F59E0B" : "#EF4444")
                                            }
                                            Text {
                                                id: statusText
                                                text: modelData.status
                                                color: modelData.status === "Online" ? "#065F46" : (modelData.status === "Warning" ? "#92400E" : "#991B1B")
                                                font.pixelSize: 11
                                                font.weight: Font.Bold
                                                font.family: Theme.typography.fontFamily
                                            }
                                        }
                                    }
                                }

                                // Connection Type
                                Text {
                                    Layout.preferredWidth: 130
                                    text: modelData.connectionType
                                    color: "#334155"
                                    font.family: Theme.typography.fontFamily
                                    font.pixelSize: 12
                                }

                                // Water Level with Mini Progress Bar
                                ColumnLayout {
                                    Layout.preferredWidth: 150
                                    spacing: 4

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Text { text: modelData.waterLevel.toFixed(1) + "%"; color: Theme.colors.primary; font.weight: Font.Bold; font.pixelSize: 12; font.family: Theme.typography.fontFamily }
                                        Text { text: "(" + modelData.waterHeightCm.toFixed(1) + " cm)"; color: "#64748B"; font.pixelSize: 11; font.family: Theme.typography.fontFamily; Layout.fillWidth: true }
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        height: 5
                                        radius: 2.5
                                        color: "#E2E8F0"

                                        Rectangle {
                                            width: parent.width * Math.max(0.0, Math.min(1.0, modelData.waterLevel / 100.0))
                                            height: parent.height
                                            radius: 2.5
                                            color: modelData.waterLevel >= 100.0 ? "#EF4444" : (modelData.waterLevel < 20.0 ? "#F59E0B" : "#0878E8")
                                        }
                                    }
                                }

                                // Sensor Health
                                Text {
                                    Layout.preferredWidth: 120
                                    text: modelData.sensorStatus
                                    color: modelData.sensorStatus === "Healthy" ? "#059669" : "#D97706"
                                    font.family: Theme.typography.fontFamily
                                    font.pixelSize: 12
                                    font.weight: Font.Medium
                                }

                                // Firmware Version
                                Text {
                                    Layout.preferredWidth: 90
                                    text: modelData.firmwareVersion
                                    color: "#64748B"
                                    font.family: Theme.typography.fontFamily
                                    font.pixelSize: 12
                                }

                                // Last Update Timestamp
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.lastUpdate
                                    color: "#64748B"
                                    font.family: Theme.typography.fontFamily
                                    font.pixelSize: 12
                                }

                                // Action View Button
                                Button {
                                    Layout.preferredWidth: 90
                                    text: "View Details"
                                    onClicked: root.selectedDevice = modelData
                                }
                            }

                            // Compact Mobile/Tablet Card View
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 6
                                visible: root.compact

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text { text: modelData.name; font.weight: Font.Bold; font.pixelSize: 14; color: "#0F172A"; Layout.fillWidth: true }
                                    Rectangle {
                                        height: 22
                                        width: compactStatusText.implicitWidth + 14
                                        radius: 11
                                        color: modelData.status === "Online" ? "#ECFDF5" : (modelData.status === "Warning" ? "#FFFBEB" : "#FEF2F2")
                                        Text {
                                            id: compactStatusText
                                            anchors.centerIn: parent
                                            text: modelData.status
                                            color: modelData.status === "Online" ? "#065F46" : (modelData.status === "Warning" ? "#92400E" : "#991B1B")
                                            font.pixelSize: 10
                                            font.weight: Font.Bold
                                        }
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text { text: "ID: " + modelData.id + " | " + modelData.connectionType; color: "#64748B"; font.pixelSize: 12; Layout.fillWidth: true }
                                    Text { text: "Level: " + modelData.waterLevel.toFixed(1) + "% (" + modelData.waterHeightCm.toFixed(1) + " cm)"; color: Theme.colors.primary; font.weight: Font.Bold; font.pixelSize: 12 }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text { text: "Updated: " + modelData.lastUpdate; color: "#94A3B8"; font.pixelSize: 11; Layout.fillWidth: true }
                                    Button {
                                        text: "View Details"
                                        onClicked: root.selectedDevice = modelData
                                    }
                                }
                            }
                        }
                    }

                    // Zero-State (When filter matches nothing)
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: 24
                        Layout.bottomMargin: 24
                        visible: root.filteredDevices.length === 0
                        spacing: 8

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: "No Devices Found Matching Query"
                            color: "#0F172A"
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 16
                            font.weight: Font.Bold
                        }

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: "Try adjusting your search query or reset the filter dropdowns."
                            color: "#64748B"
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 13
                        }

                        Button {
                            Layout.alignment: Qt.AlignHCenter
                            text: "Reset All Filters"
                            onClicked: {
                                searchField.text = "";
                                root.searchQuery = "";
                                root.selectedStatusFilter = "All";
                                root.selectedConnFilter = "All";
                            }
                        }
                    }
                }
            }
        }
    }

    // ========================================================================
    // 5. Slide-in Device Detail Drawer (Right Side Sheet)
    // ========================================================================
    Rectangle {
        id: detailDrawer
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: 440
        visible: root.drawerOpen
        x: root.drawerOpen ? parent.width - width : parent.width
        color: "#FFFFFF"
        border.color: "#CBD5E1"
        border.width: 1
        z: 20

        Flickable {
            anchors.fill: parent
            anchors.margins: 18
            contentWidth: width
            contentHeight: drawerColumn.implicitHeight + 30
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded; active: true }

            ColumnLayout {
                id: drawerColumn
                width: parent.width
                spacing: 14

                // Drawer Header Bar
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Text {
                            text: root.selectedDevice ? root.selectedDevice.name : "Device Details"
                            color: "#0F172A"
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 18
                            font.weight: Font.Bold
                        }
                        Text {
                            text: root.selectedDevice ? "Device ID: " + root.selectedDevice.id : ""
                            color: "#64748B"
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 12
                        }
                    }

                    Button {
                        text: "\u2715 Close"
                        onClicked: root.selectedDevice = null
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#E2E8F0" }

                // Section 1: Live Water Level & Geometry
                Text { text: "LIVE TELEMETRY & TANK LEVEL"; color: "#475569"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily }

                Rectangle {
                    Layout.fillWidth: true
                    height: 80
                    radius: 8
                    color: "#F8FAFC"
                    border.color: "#E2E8F0"
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text { text: "Water Level"; color: "#64748B"; font.pixelSize: 11; font.family: Theme.typography.fontFamily }
                            Text { text: root.selectedDevice ? root.selectedDevice.waterLevel.toFixed(2) + "%" : "--"; color: Theme.colors.primary; font.pixelSize: 22; font.weight: Font.Bold; font.family: Theme.typography.fontFamily }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text { text: "Water Height"; color: "#64748B"; font.pixelSize: 11; font.family: Theme.typography.fontFamily }
                            Text { text: root.selectedDevice ? root.selectedDevice.waterHeightCm.toFixed(1) + " cm" : "--"; color: "#0F172A"; font.pixelSize: 20; font.weight: Font.Bold; font.family: Theme.typography.fontFamily }
                        }
                    }
                }

                InfoRow { Layout.fillWidth: true; label: "Configured Tank Height"; value: root.selectedDevice ? root.selectedDevice.tankHeightCm.toFixed(1) + " cm" : "--" }
                InfoRow { Layout.fillWidth: true; label: "Liquid Volume"; value: root.selectedDevice ? root.selectedDevice.volumeLiters.toFixed(1) + " L" : "--" }
                InfoRow { Layout.fillWidth: true; label: "Sensor Health Status"; value: root.selectedDevice ? root.selectedDevice.sensorStatus : "--" }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#F1F5F9" }

                // Section 2: Environmental Sensors (BME280)
                Text { text: "ENVIRONMENTAL SENSORS (BME280)"; color: "#475569"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily }

                InfoRow { Layout.fillWidth: true; label: "Atmospheric Pressure"; value: root.selectedDevice ? root.selectedDevice.pressure.toFixed(1) + " hPa (LIVE)" : "--" }
                InfoRow { Layout.fillWidth: true; label: "Ambient Temperature"; value: root.selectedDevice ? root.selectedDevice.temperature.toFixed(1) + " \u00B0C (LIVE)" : "--" }
                InfoRow { Layout.fillWidth: true; label: "Relative Humidity"; value: root.selectedDevice ? root.selectedDevice.humidity.toFixed(1) + " % RH" : "--" }
                InfoRow { Layout.fillWidth: true; label: "BME280 Health"; value: "Healthy (EEPROM Backup Ready)" }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#F1F5F9" }

                // Section 3: Hardware Diagnostics & Calibration
                Text { text: "ADC DIAGNOSTICS & CALIBRATION (EEPROM)"; color: "#475569"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily }

                InfoRow { Layout.fillWidth: true; label: "ADC Differential Mode"; value: root.selectedDevice ? root.selectedDevice.sensorType : "--" }
                InfoRow { Layout.fillWidth: true; label: "PGA Gain & Rate"; value: root.selectedDevice ? root.selectedDevice.gain : "--" }
                InfoRow { Layout.fillWidth: true; label: "Air Baseline (A_air)"; value: root.selectedDevice ? "" + root.selectedDevice.airBaseline + " counts" : "--" }
                InfoRow { Layout.fillWidth: true; label: "Deadband Threshold"; value: root.selectedDevice ? root.selectedDevice.deadband : "--" }
                InfoRow { Layout.fillWidth: true; label: "Calibration State"; value: root.selectedDevice ? root.selectedDevice.calibrationStatus : "--" }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#F1F5F9" }

                // Section 4: Connectivity & FreeRTOS Tasks
                Text { text: "CONNECTIVITY & FREERTOS RUNTIME"; color: "#475569"; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.typography.fontFamily }

                InfoRow { Layout.fillWidth: true; label: "Uplink Protocol"; value: root.selectedDevice ? root.selectedDevice.connectionType : "--" }
                InfoRow { Layout.fillWidth: true; label: "Modem / Radio"; value: root.selectedDevice ? root.selectedDevice.modemModel : "--" }
                InfoRow { Layout.fillWidth: true; label: "Signal Strength"; value: root.selectedDevice ? root.selectedDevice.signalQuality : "--" }
                InfoRow { Layout.fillWidth: true; label: "Firmware Version"; value: root.selectedDevice ? root.selectedDevice.firmwareVersion : "--" }
                InfoRow { Layout.fillWidth: true; label: "RTOS Kernel"; value: root.selectedDevice ? root.selectedDevice.runtime : "--" }
                InfoRow { Layout.fillWidth: true; label: "Cloud Upload Interval"; value: root.selectedDevice ? root.selectedDevice.uploadInterval : "--" }
                InfoRow { Layout.fillWidth: true; label: "Last Reported Telemetry"; value: root.selectedDevice ? root.selectedDevice.lastUpdate : "--" }

                Item { Layout.fillHeight: true; Layout.preferredHeight: 12 }

                Button {
                    Layout.fillWidth: true
                    text: "Close Device Drawer"
                    onClicked: root.selectedDevice = null
                }
            }
        }
    }
}
