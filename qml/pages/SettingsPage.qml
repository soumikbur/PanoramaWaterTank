import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PanoramaWaterTank

Item {
    id: settingsPage

    readonly property int maxContentWidth: 920

    // Test / Connection State properties
    property string connectionStatusState: Settings.apiToken.length > 0 ? (TankModel.connectionState === "Connected" ? "Connected" : "Connecting") : "Not Configured"
    property string lastSuccessfulUpdate: TankModel.lastUpdated !== "" ? TankModel.lastUpdated : "--"
    property string lastErrorMessage: "--"

    property string testWaterLevel: TankModel.fillPercentage > 0 ? TankModel.fillPercentage.toFixed(2) + "%" : "--"
    property string testPressure: "--"
    property string testTemperature: TankModel.hasTemperature ? TankModel.temperature.toFixed(1) + " °C" : "--"
    property string testSensorStatus: TankModel.sensorStatus !== "" ? TankModel.sensorStatus : "--"

    property string statusMessage: ""
    property bool statusIsError: false

    // Application defaults
    readonly property var defaults: ({
        apiBaseUrl: "https://industrial.api.ubidots.com",
        deviceLabel: "wli",
        refreshIntervalMs: 5000,
        levelVariable: "waterlevel",
        pressureVariable: "pressure",
        temperatureVariable: "",
        statusVariable: "sensorstatus"
    })

    // Refresh interval mapping
    readonly property var refreshIntervals: [1000, 5000, 10000, 30000, 60000]

    function getRefreshIndex(ms) {
        for (var i = 0; i < refreshIntervals.length; i++) {
            if (refreshIntervals[i] === ms) return i;
        }
        return 1; // Default to 5 seconds
    }

    function saveConfiguration() {
        // Validation
        var host = apiHostField.text.trim()
        var token = apiTokenField.text.trim()
        var device = deviceLabelField.text.trim()
        var levelVar = levelVarField.text.trim()
        var pressureVar = pressureVarField.text.trim()
        var statusVar = statusVarField.text.trim()

        if (host === "") {
            showStatus("API Host cannot be empty.", true)
            return
        }
        if (token === "") {
            showStatus("API Token cannot be empty. Please enter your Ubidots API Token.", true)
            return
        }
        if (device === "") {
            showStatus("Device API Label cannot be empty.", true)
            return
        }
        if (levelVar === "") {
            showStatus("Water Level API Label cannot be empty.", true)
            return
        }
        if (pressureVar === "") {
            showStatus("Pressure API Label cannot be empty.", true)
            return
        }
        if (statusVar === "") {
            showStatus("Sensor Status API Label cannot be empty.", true)
            return
        }

        // Apply to Settings singleton
        Settings.apiBaseUrl = host
        Settings.apiToken = token
        Settings.deviceLabel = device
        Settings.refreshIntervalMs = refreshIntervals[refreshCombo.currentIndex]

        Settings.levelVariable = levelVar
        Settings.pressureVariable = pressureVar
        Settings.temperatureVariable = tempVarField.text.trim()
        Settings.statusVariable = statusVar

        showStatus("Configuration saved successfully. Ubidots client restarted.", false)
    }

    function testConnection() {
        var host = apiHostField.text.trim()
        var token = apiTokenField.text.trim()
        var device = deviceLabelField.text.trim()

        if (host === "") {
            showStatus("Test Failed: API Host is required.", true)
            return
        }
        if (token === "") {
            showStatus("Test Failed: API Token is required.", true)
            return
        }
        if (device === "") {
            showStatus("Test Failed: Device API Label is required.", true)
            return
        }

        connectionStatusState = "Connecting"
        showStatus("Testing Ubidots connection...", false)

        var deviceKey = device.indexOf("~") === 0 ? device : ("~" + device)
        var url = host
        if (!url.endsWith("/")) url += "/"
        url += "api/v2.0/devices/" + deviceKey + "/_/values/last"

        var xhr = new XMLHttpRequest()
        xhr.open("GET", url)
        xhr.setRequestHeader("X-Auth-Token", token)
        xhr.setRequestHeader("Content-Type", "application/json")

        xhr.onreadystatechange = function() {
            if (xhr.readyState === XMLHttpRequest.DONE) {
                if (xhr.status === 200 || xhr.status === 201) {
                    try {
                        var json = JSON.parse(xhr.responseText)
                        var foundLevel = "--"
                        var foundPressure = "--"
                        var foundTemp = "--"
                        var foundStatus = "--"

                        var targetLevel = levelVarField.text.trim().toLowerCase()
                        var targetPressure = pressureVarField.text.trim().toLowerCase()
                        var targetTemp = tempVarField.text.trim().toLowerCase()
                        var targetStatus = statusVarField.text.trim().toLowerCase()

                        var processPair = function(rawLabel, rawVal) {
                            var label = (rawLabel || "").trim().toLowerCase()
                            var val = rawVal
                            if (val && typeof val === "object") {
                                if (val.value !== undefined) val = val.value
                                else if (val.last_value && val.last_value.value !== undefined) val = val.last_value.value
                            }
                            if (val !== null && val !== undefined) {
                                if (label === targetLevel) foundLevel = parseFloat(val).toFixed(2) + "%"
                                else if (label === targetPressure) foundPressure = parseFloat(val).toFixed(2)
                                else if (targetTemp !== "" && label === targetTemp) foundTemp = parseFloat(val).toFixed(1) + " °C"
                                else if (label === targetStatus) foundStatus = (val === 1 || val === "1") ? "Healthy" : ((val === 0 || val === "0") ? "Fault" : val)
                            }
                        }

                        if (Array.isArray(json)) {
                            for (var i = 0; i < json.length; i++) {
                                var item = json[i]
                                var lbl = item.label || (item.variable ? item.variable.label : "")
                                var v = item.value !== undefined ? item.value : item.last_value
                                processPair(lbl, v)
                            }
                        } else if (typeof json === "object") {
                            if (json.results && Array.isArray(json.results)) {
                                for (var j = 0; j < json.results.length; j++) {
                                    var r = json.results[j]
                                    processPair(r.label || "", r.last_value)
                                }
                            } else {
                                for (var key in json) {
                                    if (json.hasOwnProperty(key)) {
                                        processPair(key, json[key])
                                    }
                                }
                            }
                        }

                        testWaterLevel = foundLevel
                        testPressure = foundPressure
                        testTemperature = foundTemp
                        testSensorStatus = foundStatus

                        connectionStatusState = "Connected"
                        lastErrorMessage = "None"
                        var now = new Date()
                        lastSuccessfulUpdate = now.toLocaleTimeString(Qt.locale(), "hh:mm:ss AP")

                        showStatus("Connection Successful! Validated device '" + device + "' on Ubidots.", false)
                    } catch (e) {
                        connectionStatusState = "Failed"
                        lastErrorMessage = "JSON Parse Error"
                        showStatus("Test Failed: Invalid JSON payload returned from Ubidots.", true)
                    }
                } else if (xhr.status === 401) {
                    connectionStatusState = "Failed"
                    lastErrorMessage = "HTTP 401 Unauthorized"
                    showStatus("Authentication failed. Check the Ubidots token.", true)
                } else if (xhr.status === 404) {
                    connectionStatusState = "Failed"
                    lastErrorMessage = "HTTP 404 Not Found"
                    showStatus("Device or endpoint not found. Verify the device API label and API URL.", true)
                } else if (xhr.status === 429) {
                    connectionStatusState = "Failed"
                    lastErrorMessage = "HTTP 429 Rate Limited"
                    showStatus("Ubidots rate limit exceeded. Increase the refresh interval.", true)
                } else {
                    connectionStatusState = "Failed"
                    lastErrorMessage = "Ubidots server cannot be reached."
                    showStatus("Ubidots server cannot be reached.", true)
                }
            }
        }
        xhr.send()
    }


    function restoreDefaults() {
        apiHostField.text = defaults.apiBaseUrl
        deviceLabelField.text = defaults.deviceLabel
        refreshCombo.currentIndex = getRefreshIndex(defaults.refreshIntervalMs)
        levelVarField.text = defaults.levelVariable
        pressureVarField.text = defaults.pressureVariable
        tempVarField.text = defaults.temperatureVariable
        statusVarField.text = defaults.statusVariable
        showStatus("Restored default fields. Click Save Configuration to apply.", false)
    }

    function showStatus(msg, isErr) {
        statusMessage = msg
        statusIsError = isErr
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.colors.background
    }

    Flickable {
        id: flickable
        anchors.fill: parent
        contentWidth: width
        contentHeight: mainColumn.implicitHeight + Theme.metrics.spacingLarge * 2
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        ColumnLayout {
            id: mainColumn
            anchors.top: parent.top
            anchors.topMargin: Theme.metrics.spacingLarge
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(flickable.width - Theme.metrics.spacingLarge * 2, settingsPage.maxContentWidth)
            spacing: Theme.metrics.spacingMedium

            // Page Header
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.metrics.space4

                Text {
                    text: "Settings"
                    color: Theme.colors.textPrimary
                    font.family: Theme.typography.fontFamily
                    font.pixelSize: 28
                    font.weight: Font.Bold
                }
                Text {
                    text: "Configure the connection between the dashboard and your Ubidots device."
                    color: Theme.colors.textSecondary
                    font.family: Theme.typography.fontFamily
                    font.pixelSize: 14
                }
            }

            // =============================================================
            // Card 1: UBIDOTS CONNECTION
            // =============================================================
            BaseCard {
                Layout.fillWidth: true
                backgroundColor: Theme.colors.surface
                borderColor: Theme.colors.divider
                padding: Theme.metrics.spacingLarge

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.metrics.spacingMedium

                    ColumnLayout {
                        spacing: Theme.metrics.space4
                        Text {
                            text: "Ubidots Connection"
                            color: Theme.colors.textPrimary
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 17
                            font.weight: Font.DemiBold
                        }
                        Text {
                            text: "Configure the API connection used to retrieve live water-tank telemetry."
                            color: Theme.colors.textSecondary
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 13
                        }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: Theme.metrics.spacingMedium
                        rowSpacing: Theme.metrics.spacingMedium

                        Label {
                            text: "API Host"
                            Layout.preferredWidth: 160
                            color: Theme.colors.textPrimary
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 14
                        }
                        TextField {
                            id: apiHostField
                            Layout.fillWidth: true
                            text: Settings.apiBaseUrl
                            placeholderText: "https://industrial.api.ubidots.com"
                        }

                        Label {
                            text: "API Token"
                            Layout.preferredWidth: 160
                            color: Theme.colors.textPrimary
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 14
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Theme.metrics.space4

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.metrics.spacingSmall

                                TextField {
                                    id: apiTokenField
                                    Layout.fillWidth: true
                                    text: Settings.apiToken
                                    placeholderText: "Enter Ubidots API Token"
                                    echoMode: tokenVisibilityButton.checked ? TextInput.Normal : TextInput.Password
                                }
                                Button {
                                    id: tokenVisibilityButton
                                    checkable: true
                                    text: checked ? "Hide" : "Show"
                                }
                            }
                            Text {
                                text: "Used to authenticate requests to the Ubidots API."
                                color: Theme.colors.textSecondary
                                font.family: Theme.typography.fontFamily
                                font.pixelSize: 12
                            }
                        }

                        Label {
                            text: "Device API Label"
                            Layout.preferredWidth: 160
                            color: Theme.colors.textPrimary
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 14
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Theme.metrics.space4

                            TextField {
                                id: deviceLabelField
                                Layout.fillWidth: true
                                text: Settings.deviceLabel
                                placeholderText: "wli"
                            }
                            Text {
                                text: "Ubidots device API label (e.g. wli)."
                                color: Theme.colors.textSecondary
                                font.family: Theme.typography.fontFamily
                                font.pixelSize: 12
                            }
                        }

                        Label {
                            text: "Data Refresh Interval"
                            Layout.preferredWidth: 160
                            color: Theme.colors.textPrimary
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 14
                        }
                        ComboBox {
                            id: refreshCombo
                            Layout.fillWidth: true
                            model: ["1 second", "5 seconds", "10 seconds", "30 seconds", "60 seconds"]
                            currentIndex: settingsPage.getRefreshIndex(Settings.refreshIntervalMs)
                        }
                    }
                }
            }

            // =============================================================
            // Card 2: DEVICE VARIABLES
            // =============================================================
            BaseCard {
                Layout.fillWidth: true
                backgroundColor: Theme.colors.surface
                borderColor: Theme.colors.divider
                padding: Theme.metrics.spacingLarge

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.metrics.spacingMedium

                    ColumnLayout {
                        spacing: Theme.metrics.space4
                        Text {
                            text: "Device Variables"
                            color: Theme.colors.textPrimary
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 17
                            font.weight: Font.DemiBold
                        }
                        Text {
                            text: "Map Ubidots variables to dashboard measurements."
                            color: Theme.colors.textSecondary
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 13
                        }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: Theme.metrics.spacingMedium
                        rowSpacing: Theme.metrics.spacingMedium

                        Label {
                            text: "Water Level"
                            Layout.preferredWidth: 160
                            color: Theme.colors.textPrimary
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 14
                        }
                        TextField {
                            id: levelVarField
                            Layout.fillWidth: true
                            text: Settings.levelVariable
                            placeholderText: "waterlevel"
                        }

                        Label {
                            text: "Pressure"
                            Layout.preferredWidth: 160
                            color: Theme.colors.textPrimary
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 14
                        }
                        TextField {
                            id: pressureVarField
                            Layout.fillWidth: true
                            text: Settings.pressureVariable
                            placeholderText: "pressure"
                        }

                        Label {
                            text: "Temperature"
                            Layout.preferredWidth: 160
                            color: Theme.colors.textPrimary
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 14
                        }
                        TextField {
                            id: tempVarField
                            Layout.fillWidth: true
                            text: Settings.temperatureVariable
                            placeholderText: "Optional (leave empty if not configured)"
                        }

                        Label {
                            text: "Sensor Status"
                            Layout.preferredWidth: 160
                            color: Theme.colors.textPrimary
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 14
                        }
                        TextField {
                            id: statusVarField
                            Layout.fillWidth: true
                            text: Settings.statusVariable
                            placeholderText: "sensorstatus"
                        }
                    }
                }
            }

            // =============================================================
            // Card 3 & 4: STATUS & LATEST DEVICE DATA PREVIEW (2 Columns)
            // =============================================================
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.metrics.cardSpacing

                // Connection Status Card
                BaseCard {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    backgroundColor: Theme.colors.surface
                    borderColor: Theme.colors.divider
                    padding: Theme.metrics.spacingLarge

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.metrics.spacingSmall

                        Text {
                            text: "Connection Status"
                            color: Theme.colors.textPrimary
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 16
                            font.weight: Font.DemiBold
                            Layout.bottomMargin: Theme.metrics.space4
                        }

                        RowLayout {
                            spacing: Theme.metrics.space8
                            Rectangle {
                                width: 10
                                height: 10
                                radius: 5
                                color: {
                                    if (settingsPage.connectionStatusState === "Connected") return Theme.colors.success;
                                    if (settingsPage.connectionStatusState === "Connecting") return Theme.colors.primary;
                                    if (settingsPage.connectionStatusState === "Failed") return Theme.colors.critical;
                                    return Theme.colors.textMuted;
                                }
                            }
                            Text {
                                text: settingsPage.connectionStatusState
                                color: Theme.colors.textPrimary
                                font.family: Theme.typography.fontFamily
                                font.pixelSize: 14
                                font.weight: Font.DemiBold
                            }
                        }

                        Item { Layout.preferredHeight: Theme.metrics.space4 }

                        Text {
                            text: "Last Successful Update: " + settingsPage.lastSuccessfulUpdate
                            color: Theme.colors.textSecondary
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 13
                        }
                        Text {
                            text: "Last Error: " + settingsPage.lastErrorMessage
                            color: settingsPage.lastErrorMessage !== "None" && settingsPage.lastErrorMessage !== "--" ? Theme.colors.critical : Theme.colors.textSecondary
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 13
                        }
                    }
                }

                // Latest Device Data Card
                BaseCard {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    backgroundColor: Theme.colors.surface
                    borderColor: Theme.colors.divider
                    padding: Theme.metrics.spacingLarge

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.metrics.spacingSmall

                        Text {
                            text: "Latest Device Data"
                            color: Theme.colors.textPrimary
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 16
                            font.weight: Font.DemiBold
                            Layout.bottomMargin: Theme.metrics.space4
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            rowSpacing: Theme.metrics.space4

                            Text { text: "Water Level:"; color: Theme.colors.textSecondary; font.pixelSize: 13 }
                            Text { text: TankModel.fillPercentage > 0 ? TankModel.fillPercentage.toFixed(2) + "%" : settingsPage.testWaterLevel; color: Theme.colors.primary; font.pixelSize: 13; font.weight: Font.Bold }

                            Text { text: "Pressure:"; color: Theme.colors.textSecondary; font.pixelSize: 13 }
                            Text { text: TankModel.hasPressure ? TankModel.pressure.toFixed(2) : settingsPage.testPressure; color: Theme.colors.textPrimary; font.pixelSize: 13 }

                            Text { text: "Temperature:"; color: Theme.colors.textSecondary; font.pixelSize: 13 }
                            Text { text: TankModel.hasTemperature ? TankModel.temperature.toFixed(1) + " \u00B0C" : settingsPage.testTemperature; color: Theme.colors.textPrimary; font.pixelSize: 13 }

                            Text { text: "Sensor Status:"; color: Theme.colors.textSecondary; font.pixelSize: 13 }
                            Text {
                                text: TankModel.sensorStatus.length > 0 && TankModel.sensorStatus !== "--" ? TankModel.sensorStatus : settingsPage.testSensorStatus
                                color: (TankModel.sensorStatus === "Healthy" || settingsPage.testSensorStatus === "Healthy") ? Theme.colors.success : ((TankModel.sensorStatus === "Fault" || settingsPage.testSensorStatus === "Fault") ? Theme.colors.critical : Theme.colors.textPrimary)
                                font.pixelSize: 13
                                font.weight: Font.DemiBold
                            }

                        }
                    }
                }
            }

            // =============================================================
            // ACTIONS & STATUS FEEDBACK
            // =============================================================
            BaseCard {
                Layout.fillWidth: true
                backgroundColor: Theme.colors.surface
                borderColor: Theme.colors.divider
                padding: Theme.metrics.spacingLarge

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.metrics.spacingMedium

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.metrics.spacingMedium

                        Button {
                            text: "Save Configuration"
                            highlighted: true
                            onClicked: settingsPage.saveConfiguration()
                        }

                        Button {
                            text: "Test Connection"
                            onClicked: settingsPage.testConnection()
                        }

                        Button {
                            text: "Restore Defaults"
                            onClicked: settingsPage.restoreDefaults()
                        }

                        Item { Layout.fillWidth: true }
                    }

                    Rectangle {
                        visible: settingsPage.statusMessage.length > 0
                        Layout.fillWidth: true
                        implicitHeight: statusText.implicitHeight + 16
                        radius: Theme.metrics.radiusSmall
                        color: settingsPage.statusIsError ? "#FEE2E2" : "#E0F2FE"
                        border.width: 1
                        border.color: settingsPage.statusIsError ? "#FCA5A5" : "#BAE6FD"

                        Text {
                            id: statusText
                            anchors.fill: parent
                            anchors.margins: 8
                            text: settingsPage.statusMessage
                            color: settingsPage.statusIsError ? Theme.colors.critical : Theme.colors.primary
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 13
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: Theme.metrics.spacingLarge }
        }
    }
}