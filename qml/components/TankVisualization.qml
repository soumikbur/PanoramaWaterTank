import QtQuick
import QtQuick.Layouts
import QtQuick.Shapes
import PanoramaWaterTank

// Water Level card: vertical cylindrical tank visualization with subtle fluid wave animation,
// physically accurate 0-100% scale (0% at bottom, 100% at top), numeric level percentage,
// Water Height and Tank Volume sub-cards, status badge, and bottom capacity summary bar.
BaseCard {
    id: root

    Layout.minimumWidth: Theme.metrics.tankMinimumWidth
    Layout.minimumHeight: Theme.metrics.tankCardMinHeight

    readonly property int vesselPreferredWidth: 140
    readonly property int rulerPreferredWidth: 42
    readonly property int vesselBorderWidth: 2
    readonly property int topRadius: 24
    readonly property int bottomRadius: 20

    readonly property bool isOverflow: TankModel.alarmLevel === "Overflow" || TankModel.fillPercentage >= 100.0 || (TankModel.tankHeightCm > 0 && TankModel.waterHeightCm >= TankModel.tankHeightCm)

    property color waterColorTop: isOverflow ? "#EF4444" : "#0284C7"
    property color waterColorBottom: isOverflow ? "#B91C1C" : "#1D4ED8"

    // Continuous smooth subtle fluid wave animation
    property real wavePhase: 0.0

    NumberAnimation on wavePhase {
        from: 0.0
        to: Math.PI * 2.0
        duration: 4000
        loops: Animation.Infinite
        running: root.visible
    }

    onWavePhaseChanged: {
        if (tankCanvas && root.visible) {
            tankCanvas.requestPaint();
        }
    }

    Behavior on waterColorTop {
        ColorAnimation { duration: 250; easing.type: Easing.InOutQuad }
    }
    Behavior on waterColorBottom {
        ColorAnimation { duration: 250; easing.type: Easing.InOutQuad }
    }

    onWaterColorTopChanged: { if (tankCanvas) tankCanvas.requestPaint(); }
    onWaterColorBottomChanged: { if (tankCanvas) tankCanvas.requestPaint(); }

    ColumnLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 10

        // Card Title
        Text {
            text: "Water Level"
            color: Theme.colors.textPrimary
            font.family: Theme.typography.fontFamily
            font.pixelSize: Theme.typography.dashCardTitle.size
            font.weight: Theme.typography.dashCardTitle.weight
        }

        // RowLayout: Left side (Tank + Scale) | Right side (Current Level, Height, Volume & Status)
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            // Tank + Scale container
            RowLayout {
                Layout.fillHeight: true
                spacing: 6

                Item {
                    id: tankContainer
                    Layout.preferredWidth: root.vesselPreferredWidth
                    Layout.fillHeight: true
                    Layout.topMargin: 2
                    Layout.bottomMargin: 2

                    // Top Pipe / Nozzle Lid
                    Item {
                        id: topNeckCap
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top
                        width: 36
                        height: 12
                        z: 5

                        // Lid cap
                        Rectangle {
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.top: parent.top
                            width: 36
                            height: 4
                            radius: 2
                            color: Theme.colors.surface
                            border.width: 1.5
                            border.color: Theme.colors.tankOutline
                        }
                        // Pipe collar
                        Rectangle {
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.top: parent.top
                            anchors.topMargin: 3
                            width: 24
                            height: 9
                            radius: 2
                            color: Theme.colors.surface
                            border.width: 1.5
                            border.color: Theme.colors.tankOutline
                        }
                    }

                    // Canvas Tank Body with Rounded-Corner Water Clipping Mask & Fluid Waves
                    Canvas {
                        id: tankCanvas
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: topNeckCap.bottom
                        anchors.topMargin: -2
                        anchors.bottom: parent.bottom
                        antialiasing: true

                        property real fillPercentage: Math.max(0, Math.min(100, TankModel.fillPercentage))
                        property color surfaceColor: "#FFFFFF"
                        property color borderColor: Theme.colors.tankOutline
                        property real topR: root.topRadius
                        property real botR: root.bottomRadius

                        onFillPercentageChanged: requestPaint()
                        onWidthChanged: requestPaint()
                        onHeightChanged: requestPaint()

                        onPaint: {
                            var ctx = getContext("2d");
                            ctx.clearRect(0, 0, width, height);

                            var w = width;
                            var h = height;
                            var tr = topR;
                            var br = botR;

                            // 1. Vessel Body Outline Path
                            ctx.save();
                            ctx.beginPath();
                            ctx.moveTo(tr, 0);
                            ctx.lineTo(w - tr, 0);
                            ctx.quadraticCurveTo(w, 0, w, tr);
                            ctx.lineTo(w, h - br);
                            ctx.quadraticCurveTo(w, h, w - br, h);
                            ctx.lineTo(br, h);
                            ctx.quadraticCurveTo(0, h, 0, h - br);
                            ctx.lineTo(0, tr);
                            ctx.quadraticCurveTo(0, 0, tr, 0);
                            ctx.closePath();

                            ctx.fillStyle = surfaceColor;
                            ctx.fill();

                            // 2. CLIP water fill to inner tank body
                            ctx.clip();

                            var waterH = h * (fillPercentage / 100.0);
                            if (waterH > 0) {
                                var waterTopY = h - waterH;

                                // Water Gradient Fill
                                var grad = ctx.createLinearGradient(0, waterTopY, 0, h);
                                grad.addColorStop(0, root.waterColorTop);
                                grad.addColorStop(1, root.waterColorBottom);
                                ctx.fillStyle = grad;

                                // Dual smooth animated sine waves for fluid motion
                                var phase1 = root.wavePhase;
                                var phase2 = root.wavePhase * 1.6 + 1.2;
                                var amp1 = 2.8;
                                var amp2 = 1.6;

                                ctx.beginPath();
                                ctx.moveTo(0, h);
                                ctx.lineTo(0, waterTopY);

                                for (var x = 0; x <= w; x += 4) {
                                    var waveY = waterTopY
                                        + Math.sin((x / w) * Math.PI * 2.0 + phase1) * amp1
                                        + Math.sin((x / w) * Math.PI * 4.0 + phase2) * amp2;
                                    ctx.lineTo(x, waveY);
                                }

                                ctx.lineTo(w, h);
                                ctx.closePath();
                                ctx.fill();

                                // Semi-transparent shimmering wave crest highlight
                                ctx.beginPath();
                                for (var x2 = 0; x2 <= w; x2 += 4) {
                                    var waveY2 = waterTopY
                                        + Math.sin((x2 / w) * Math.PI * 2.0 + phase1) * amp1
                                        + Math.sin((x2 / w) * Math.PI * 4.0 + phase2) * amp2;
                                    if (x2 === 0) ctx.moveTo(x2, waveY2);
                                    else ctx.lineTo(x2, waveY2);
                                }
                                ctx.lineWidth = 2.0;
                                ctx.strokeStyle = "rgba(255, 255, 255, 0.40)";
                                ctx.stroke();
                            }

                            ctx.restore(); // End clipping

                            // 3. Draw Outer Tank Outline Overlay
                            ctx.beginPath();
                            ctx.moveTo(tr, 1);
                            ctx.lineTo(w - tr, 1);
                            ctx.quadraticCurveTo(w - 1, 1, w - 1, tr);
                            ctx.lineTo(w - 1, h - br);
                            ctx.quadraticCurveTo(w - 1, h - 1, w - br, h - 1);
                            ctx.lineTo(br, h - 1);
                            ctx.quadraticCurveTo(1, h - 1, 1, h - br);
                            ctx.lineTo(1, tr);
                            ctx.quadraticCurveTo(1, 1, tr, 1);
                            ctx.closePath();

                            ctx.lineWidth = root.vesselBorderWidth;
                            ctx.strokeStyle = borderColor;
                            ctx.stroke();
                        }
                    }
                }

                // Vertical Scale (0% at bottom to 100% at top)
                Item {
                    id: rulerColumn
                    Layout.preferredWidth: root.rulerPreferredWidth
                    Layout.fillHeight: true
                    Layout.topMargin: 16
                    Layout.bottomMargin: 8

                    Rectangle {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: 1
                        color: Theme.colors.dividerStrong
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        Repeater {
                            model: 21

                            delegate: RowLayout {
                                required property int index
                                readonly property int percent: 100 - index * 5
                                readonly property bool isMajor: percent % 25 === 0

                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                spacing: 3

                                Rectangle {
                                    Layout.alignment: Qt.AlignVCenter
                                    width: isMajor ? 7 : 3
                                    height: 1
                                    color: Theme.colors.dividerStrong
                                }
                                Text {
                                    visible: isMajor
                                    text: percent + "%"
                                    color: Theme.colors.textSecondary
                                    font.family: Theme.typography.fontFamily
                                    font.pixelSize: 10
                                }
                            }
                        }
                    }
                }
            }

            // Right column: Values, Height & Volume Cards, & Status panel
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 4

                Text {
                    text: "Current Level"
                    color: Theme.colors.textSecondary
                    font.family: Theme.typography.fontFamily
                    font.pixelSize: 14
                    font.weight: Font.Medium
                }

                Text {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 52
                    text: TankModel.fillPercentage.toFixed(2) + "%"
                    color: Theme.colors.primary
                    font.family: Theme.typography.fontFamily
                    font.pixelSize: 48
                    font.weight: Font.Bold
                    fontSizeMode: Text.Fit
                    minimumPixelSize: 32
                    verticalAlignment: Text.AlignVCenter
                    wrapMode: Text.NoWrap
                }

                Item {
                    Layout.fillWidth: true
                    height: 2
                }

                // 1. Water Height Card Block
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 70
                    color: "#F8FAFC"
                    border.color: Theme.colors.divider
                    border.width: 1
                    radius: 8

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 1

                        RowLayout {
                            spacing: 6
                            Shape {
                                width: 14; height: 14
                                antialiasing: true
                                ShapePath {
                                    fillColor: "transparent"
                                    strokeColor: Theme.colors.primary
                                    strokeWidth: 2
                                    capStyle: ShapePath.RoundCap
                                    joinStyle: ShapePath.RoundJoin
                                    PathSvg { path: "M2,1 L2,13 M2,3 L7,3 M2,7 L10,7 M2,11 L7,11 M2,13 L10,13" }
                                }
                            }
                            Text {
                                text: "Water Height"
                                color: Theme.colors.textSecondary
                                font.family: Theme.typography.fontFamily
                                font.pixelSize: 12
                                font.weight: Font.Medium
                            }
                        }

                        Text {
                            text: TankModel.waterHeightCm.toFixed(2) + " cm"
                            color: Theme.colors.textPrimary
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 18
                            font.weight: Font.Bold
                        }

                        Text {
                            text: "of " + TankModel.tankHeightCm.toFixed(2) + " cm"
                            color: Theme.colors.textSecondary
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 11
                        }
                    }
                }

                // 2. Tank Volume Card Block
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 70
                    color: "#F8FAFC"
                    border.color: Theme.colors.divider
                    border.width: 1
                    radius: 8

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 1

                        RowLayout {
                            spacing: 6
                            Shape {
                                width: 14; height: 14
                                antialiasing: true
                                ShapePath {
                                    fillColor: "transparent"
                                    strokeColor: Theme.colors.primary
                                    strokeWidth: 2
                                    capStyle: ShapePath.RoundCap
                                    joinStyle: ShapePath.RoundJoin
                                    PathSvg { path: "M2,2 C2,1 10,1 10,2 L10,12 C10,13 2,13 2,12 Z M4,1 L8,1" }
                                }
                            }
                            Text {
                                text: "Tank Volume"
                                color: Theme.colors.textSecondary
                                font.family: Theme.typography.fontFamily
                                font.pixelSize: 12
                                font.weight: Font.Medium
                            }
                        }

                        Text {
                            text: TankModel.capacityLiters >= 1000
                                ? TankModel.waterVolumeLiters.toLocaleString(Qt.locale(), 'f', 2) + " L"
                                : TankModel.waterVolumeLiters.toFixed(2) + " L"
                            color: Theme.colors.textPrimary
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 18
                            font.weight: Font.Bold
                        }

                        Text {
                            text: "of " + (TankModel.capacityLiters >= 1000
                                ? TankModel.capacityLiters.toLocaleString(Qt.locale(), 'f', 2) + " L"
                                : TankModel.capacityLiters.toFixed(2) + " L")
                            color: Theme.colors.textSecondary
                            font.family: Theme.typography.fontFamily
                            font.pixelSize: 11
                        }
                    }
                }

                Text {
                    text: "Status"
                    color: Theme.colors.textSecondary
                    font.family: Theme.typography.fontFamily
                    font.pixelSize: 11
                    Layout.topMargin: 2
                }

                StatusBadge {
                    Layout.fillWidth: true
                    severity: {
                        if (TankModel.connectionState === "Offline") return "Offline";
                        if (TankModel.alarmLevel === "Overflow") return "Critical";
                        if (TankModel.alarmLevel === "Critical") return "Critical";
                        if (TankModel.alarmLevel === "Warning") return "Warning";
                        if (TankModel.connectionState === "Connected") return "Normal";
                        return "Waiting";
                    }
                    label: {
                        if (TankModel.connectionState === "Offline") return "Sensor Offline";
                        if (TankModel.alarmLevel === "Overflow") return "OVERFLOW ALERT";
                        if (TankModel.alarmLevel === "Critical") return "Low Critical Alert";
                        if (TankModel.alarmLevel === "Warning") return "Low Warning Alert";
                        if (TankModel.connectionState === "Connected") return "Receiving Data";
                        return "Waiting for Live Data";
                    }
                }

                Text {
                    Layout.topMargin: 1
                    text: "Last Update: " + TankModel.lastUpdated
                    color: Theme.colors.textSecondary
                    font.family: Theme.typography.fontFamily
                    font.pixelSize: 11
                }
            }
        }

        // Bottom Full-width Light Blue Tank Capacity Banner
        Rectangle {
            Layout.fillWidth: true
            height: 30
            radius: 6
            color: Theme.colors.primarySurface
            border.color: "#DBEAFE"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14

                Text {
                    text: "Tank Height: " + TankModel.tankHeightCm.toFixed(2) + " cm"
                    color: "#1E40AF"
                    font.family: Theme.typography.fontFamily
                    font.pixelSize: 12
                    font.weight: Font.Bold
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: "Tank Capacity: " + (TankModel.capacityLiters >= 1000
                        ? TankModel.capacityLiters.toLocaleString(Qt.locale(), 'f', 2) + " L"
                        : TankModel.capacityLiters.toFixed(2) + " L")
                    color: "#1E40AF"
                    font.family: Theme.typography.fontFamily
                    font.pixelSize: 12
                    font.weight: Font.Bold
                }
            }
        }
    }
}
