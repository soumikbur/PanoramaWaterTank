import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PanoramaWaterTank

// "Level Trend" card component with selectable time ranges: 12 Hours, 24 Hours, 7 Days.
// Binds directly to TankModel.trendHistory (populated via C++ ApiClient with UTC Unix epoch milliseconds).
// Embeds TrendChart for grid, curve, and area fill, with 0-100% Y-axis, dynamic X-axis time labels,
// and state overlays ("Loading level history...", "No water level history available").
BaseCard {
    id: root

    Layout.minimumHeight: Theme.metrics.tankCardMinHeight

    readonly property var rawHistory: TankModel.trendHistory
    readonly property real livePercentage: TankModel.fillPercentage
    property var numericValues: []
    property var timeLabels: []
    property bool hasData: numericValues.length > 0

    // Time range selection: "12 Hours", "24 Hours", "7 Days"
    readonly property var rangeModel: ["12 Hours", "24 Hours", "7 Days"]
    readonly property var rangeHours: [12, 24, 168]

    readonly property int selectedRangeIndex: {
        var h = TankModel.historyRangeHours;
        if (h === 24) return 1;
        if (h === 168) return 2;
        return 0; // default 12 Hours
    }

    // Format a millisecond timestamp to local 12-hour AM/PM string (e.g., "8:00 PM")
    function formatTime12h(date) {
        var hours = date.getHours();
        var minutes = date.getMinutes();
        var ampm = hours >= 12 ? "PM" : "AM";
        var h12 = hours % 12;
        if (h12 === 0) h12 = 12;
        var mm = minutes < 10 ? "0" + minutes : "" + minutes;
        return h12 + ":" + mm + " " + ampm;
    }

    // Format a millisecond timestamp for 7-day view (e.g., "Aug 17")
    function formatDate7d(date) {
        var months = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];
        return months[date.getMonth()] + " " + date.getDate();
    }

    // Generate N non-overlapping labels between startMs and endMs
    function generateLabels(startMs, endMs, rangeIdx) {
        var labels = [];
        var count = rangeIdx === 2 ? 7 : 6;
        var step = count > 1 ? (endMs - startMs) / (count - 1) : 0;
        for (var k = 0; k < count; k++) {
            var t = new Date(startMs + k * step);
            if (rangeIdx === 2) {
                labels.push(formatDate7d(t));
            } else {
                labels.push(formatTime12h(t));
            }
        }
        return labels;
    }

    onRawHistoryChanged: updateHistoryData()
    onLivePercentageChanged: updateHistoryData()
    onSelectedRangeIndexChanged: updateHistoryData()
    Component.onCompleted: updateHistoryData()

    function updateHistoryData() {
        var maxHeightCm = TankModel.tankHeightCm > 0 ? TankModel.tankHeightCm : 50.0;
        var hours = TankModel.historyRangeHours > 0 ? TankModel.historyRangeHours : 12;
        var durationMs = hours * 3600 * 1000;
        var now = new Date().getTime();

        if (!rawHistory || rawHistory.length === 0) {
            numericValues = [];
            hasData = false;
            timeLabels = generateLabels(now - durationMs, now, selectedRangeIndex);
            return;
        }

        // Step 1: Parse and normalize all points: waterLevel (cm) -> levelPercentage (0-100%)
        var points = [];
        for (var i = 0; i < rawHistory.length; i++) {
            var item = rawHistory[i];
            if (!item || item.value === undefined || isNaN(item.value)) continue;

            var waterLevelCm = parseFloat(item.value);
            if (isNaN(waterLevelCm)) continue;

            // If waterLevelCm is reported in meters (<= 0.50 m when maxHeightCm >= 10.0 and waterLevelCm > 0), convert to cm:
            if (waterLevelCm <= (maxHeightCm / 100.0) && maxHeightCm >= 10.0 && waterLevelCm > 0) {
                waterLevelCm *= 100.0;
            }

            // Convert physical water level (cm) to percentage (0-100%) against configured maxHeightCm
            var levelPercentage = maxHeightCm > 0 ? (waterLevelCm / maxHeightCm) * 100.0 : 0.0;
            levelPercentage = Math.max(0.0, Math.min(100.0, levelPercentage));

            var ts = 0;
            if (item.timestampMs !== undefined) {
                ts = Number(item.timestampMs);
            } else if (item.timestamp !== undefined) {
                ts = Number(item.timestamp);
            }

            // Normalization check: if Unix seconds (< 10^11), convert to milliseconds
            if (ts > 0 && ts < 1e11) {
                ts *= 1000;
            }

            if (ts > 0 && !isNaN(ts)) {
                points.push({
                    timestampMs: ts,
                    value: levelPercentage,
                    waterLevelCm: waterLevelCm
                });
            }
        }

        if (points.length === 0) {
            numericValues = [];
            hasData = false;
            timeLabels = generateLabels(now - durationMs, now, selectedRangeIndex);
            return;
        }

        // Step 2: Sort chronologically ascending (oldest -> newest)
        points.sort(function(a, b) { return a.timestampMs - b.timestampMs; });

        // Step 3: Determine selected time window bounds in milliseconds (now - durationMs -> now)
        var latestTs = points[points.length - 1].timestampMs;
        var plotEndMs = Math.max(now, latestTs);
        var plotStartMs = plotEndMs - durationMs;

        // Step 4: Filter points strictly within the active plotting window
        var inWindowPoints = [];
        for (var j = 0; j < points.length; j++) {
            if (points[j].timestampMs >= plotStartMs && points[j].timestampMs <= plotEndMs) {
                inWindowPoints.push(points[j]);
            }
        }

        if (inWindowPoints.length === 0) {
            inWindowPoints = points;
            plotEndMs = latestTs;
            plotStartMs = Math.max(0, latestTs - durationMs);
        }

        // Step 5: Map each point's temporal position proportionally across the full window
        var timeSpanMs = plotEndMs - plotStartMs;
        var chartPts = [];
        for (var k = 0; k < inWindowPoints.length; k++) {
            var pt = inWindowPoints[k];
            var xRatio = timeSpanMs > 0 ? (pt.timestampMs - plotStartMs) / timeSpanMs : (inWindowPoints.length > 1 ? k / (inWindowPoints.length - 1) : 0.5);
            xRatio = Math.max(0.0, Math.min(1.0, xRatio));
            chartPts.push({
                xRatio: xRatio,
                value: pt.value,
                timestampMs: pt.timestampMs
            });
        }

        numericValues = chartPts;
        hasData = true;

        // Step 6: Generate non-overlapping X-axis labels matching the window
        timeLabels = generateLabels(plotStartMs, plotEndMs, selectedRangeIndex);

        var validCount = inWindowPoints.length;
        var invalidCount = (rawHistory ? rawHistory.length : 0) - points.length;
        var firstPt = inWindowPoints.length > 0 ? inWindowPoints[0] : null;
        var lastPt = inWindowPoints.length > 0 ? inWindowPoints[inWindowPoints.length - 1] : null;

        console.log("========== TREND CHART DEBUG ==========");
        console.log("History count: " + (rawHistory ? rawHistory.length : 0));
        console.log("Valid points: " + validCount);
        console.log("Invalid points: " + invalidCount);
        if (firstPt && lastPt) {
            console.log("First timestamp: " + firstPt.timestampMs + " (" + new Date(firstPt.timestampMs).toISOString() + ")");
            console.log("Last timestamp: " + lastPt.timestampMs + " (" + new Date(lastPt.timestampMs).toISOString() + ")");
            console.log("First water level: " + firstPt.value.toFixed(2) + "%");
            console.log("Last water level: " + lastPt.value.toFixed(2) + "%");
            console.log("Timestamp range: " + (lastPt.timestampMs - firstPt.timestampMs) + " ms");
        }
        console.log("Timestamp unit: epoch milliseconds");
        console.log("Selected interval: " + root.rangeModel[selectedRangeIndex]);
        console.log("Interval milliseconds: " + durationMs + " ms");
        console.log("X axis minimum: " + plotStartMs + " (" + new Date(plotStartMs).toISOString() + ")");
        console.log("X axis maximum: " + plotEndMs + " (" + new Date(plotEndMs).toISOString() + ")");
        console.log("Y axis minimum: 0%");
        console.log("Y axis maximum: 100%");
        console.log("Series count: " + chartPts.length);
        console.log("Chart width: " + chartContainer.width);
        console.log("Chart height: " + chartContainer.height);
        console.log("========================================");
    }

    ColumnLayout {
        id: innerColumn
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: Theme.metrics.spacingSmall

        // Card Header Row
        RowLayout {
            Layout.fillWidth: true

            Text {
                Layout.fillWidth: true
                text: "Level Trend (" + root.rangeModel[root.selectedRangeIndex] + ")"
                color: Theme.colors.textPrimary
                font.family: Theme.typography.fontFamily
                font.pixelSize: Theme.typography.dashCardTitle.size
                font.weight: Theme.typography.dashCardTitle.weight
            }

            ComboBox {
                id: rangeCombo
                model: root.rangeModel
                currentIndex: root.selectedRangeIndex
                onActivated: function(index) {
                    TankModel.historyRangeHours = root.rangeHours[index];
                }
                font.family: Theme.typography.fontFamily
                font.pixelSize: Theme.typography.dashLabel.size
            }
        }

        // Chart Plot Row with Y-Axis & TrendChart
        RowLayout {
            id: chartPlotRow
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: Theme.metrics.space8
            spacing: 4

            // Y-Axis Title & Labels Column (strictly fixed width: 48px)
            Item {
                id: yAxisArea
                Layout.fillHeight: true
                Layout.preferredWidth: 48
                Layout.minimumWidth: 48
                Layout.maximumWidth: 48
                implicitWidth: 48

                Text {
                    text: "Water Level (%)"
                    color: Theme.colors.textSecondary
                    font.family: Theme.typography.fontFamily
                    font.pixelSize: 9
                    rotation: -90
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: -18
                }

                Item {
                    id: yAxisLabelsContainer
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    anchors.rightMargin: 4
                    width: 32

                    Repeater {
                        model: ["100%", "75%", "50%", "25%", "0%"]
                        delegate: Text {
                            required property int index
                            required property string modelData
                            anchors.right: parent.right
                            y: {
                                var topPad = 4;
                                var botPad = 6;
                                var usableH = Math.max(1, yAxisLabelsContainer.height - topPad - botPad);
                                return Math.round(topPad + usableH * (index / 4.0) - implicitHeight / 2);
                            }
                            text: modelData
                            color: Theme.colors.textSecondary
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 10
                        }
                    }
                }
            }

            // Chart Container Area
            Item {
                id: chartContainer
                Layout.fillWidth: true
                Layout.fillHeight: true

                TrendChart {
                    id: trendChart
                    anchors.fill: parent
                    values: root.numericValues
                    lineColor: Theme.colors.primary
                    gridColor: Theme.colors.divider
                }

                // Active Tooltip Badge Pill at latest data point (clamped to prevent right/bottom clipping)
                Rectangle {
                    id: tooltipBadge
                    visible: root.hasData && trendChart.points.length > 0
                    width: tooltipText.implicitWidth + 14
                    height: 22
                    radius: 4
                    color: Theme.colors.primary
                    z: 10
                    x: {
                        if (trendChart.points.length <= 0) return 0;
                        var lastPt = trendChart.points[trendChart.points.length - 1];
                        var candidateX = lastPt.x - width - 8;
                        if (candidateX < 0) candidateX = lastPt.x + 8;
                        return Math.max(0, Math.min(parent.width - width - 4, candidateX));
                    }
                    y: {
                        if (trendChart.points.length <= 0) return 0;
                        var lastPt = trendChart.points[trendChart.points.length - 1];
                        var candidateY = lastPt.y - height - 6;
                        if (candidateY < 0) candidateY = lastPt.y + 6;
                        return Math.max(0, Math.min(parent.height - height, candidateY));
                    }

                    Text {
                        id: tooltipText
                        anchors.centerIn: parent
                        text: {
                            if (trendChart.points.length > 0) {
                                var lastItem = root.numericValues[root.numericValues.length - 1];
                                var val = (typeof lastItem === 'object' && lastItem !== null) ? lastItem.value : parseFloat(lastItem);
                                return (!isNaN(val) ? val.toFixed(2) : TankModel.fillPercentage.toFixed(2)) + "%";
                            }
                            return TankModel.fillPercentage.toFixed(2) + "%";
                        }
                        color: "#FFFFFF"
                        font.family: Theme.typography.fontFamily
                        font.pixelSize: 11
                        font.weight: Font.Bold
                    }
                }

                // Outer Ring Marker Dot at latest data point
                Rectangle {
                    id: latestDot
                    visible: root.hasData && trendChart.points.length > 0
                    width: 10
                    height: 10
                    radius: 5
                    color: "#FFFFFF"
                    border.width: 2.5
                    border.color: Theme.colors.primary
                    z: 11
                    x: trendChart.points.length > 0 ? Math.max(0, Math.min(parent.width - 10, trendChart.points[trendChart.points.length - 1].x - 5)) : 0
                    y: trendChart.points.length > 0 ? Math.max(0, Math.min(parent.height - 10, trendChart.points[trendChart.points.length - 1].y - 5)) : 0
                }

                // Overlay Text for Empty / Loading / Error States
                Text {
                    anchors.centerIn: parent
                    visible: !root.hasData
                    text: {
                        if (TankModel.connectionState === "Connecting") return "Loading level history...";
                        if (TankModel.connectionState === "Offline") return "Unable to load level history";
                        return "No water level history available";
                    }
                    color: Theme.colors.textSecondary
                    font.family: Theme.typography.fontFamily
                    font.pixelSize: 13
                }
            }
        }

        // X-Axis Time Labels Row & "Time" label
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            RowLayout {
                Layout.fillWidth: true
                spacing: 4

                // Left offset matching yAxisArea exactly
                Item {
                    Layout.preferredWidth: yAxisArea.width
                }

                // Time labels container directly beneath chartContainer
                Item {
                    id: timeLabelsContainer
                    Layout.fillWidth: true
                    Layout.preferredHeight: 16

                    Repeater {
                        model: root.timeLabels
                        delegate: Text {
                            required property int index
                            required property string modelData
                            text: modelData
                            color: Theme.colors.textSecondary
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 10
                            x: {
                                if (root.timeLabels.length <= 1) return 0;
                                var totalW = timeLabelsContainer.width;
                                if (totalW <= 0) return 0;
                                if (index === 0) return 0;
                                if (index === root.timeLabels.length - 1) return Math.max(0, totalW - implicitWidth);
                                var centerPos = (index / (root.timeLabels.length - 1)) * totalW;
                                return Math.max(0, Math.min(totalW - implicitWidth, centerPos - implicitWidth / 2));
                            }
                        }
                    }
                }
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "Time"
                color: Theme.colors.textSecondary
                font.family: Theme.typography.fontFamily
                font.pixelSize: 10
            }
        }

        // Card Footer Timestamp
        Text {
            Layout.topMargin: 2
            text: "Last Update: " + TankModel.lastUpdated
            color: Theme.colors.textSecondary
            font.family: Theme.typography.fontFamily
            font.pixelSize: Theme.typography.dashLabel.size
        }
    }
}