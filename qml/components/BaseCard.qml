import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import "../theme"

Rectangle {
    id: root

    // ------------------------------------------------------------------
    // Public API
    // ------------------------------------------------------------------

    default property alias content: contentLayout.data

    property color backgroundColor: Theme.surface
    property color borderColor: Theme.border
    property color shadowColor: "#40000000"

    property int cardRadius: Metrics.radiusLarge
    property int padding: Metrics.cardPadding

    property bool elevated: true
    property bool showBorder: true

    // ------------------------------------------------------------------
    // Layout
    // ------------------------------------------------------------------

    Layout.fillWidth: true
    Layout.fillHeight: true

    Layout.minimumWidth: Metrics.cardMinimumWidth
    Layout.minimumHeight: Metrics.cardMinimumHeight

    // Derived from contentLayout's own content-driven implicit size, so
    // a card with no explicit Layout.preferredWidth/Height (every
    // SettingsPage.qml section) sizes itself to fit its actual content
    // instead of collapsing to Layout.minimumWidth/Height. Strictly
    // downward (content -> contentLayout -> root, never the reverse),
    // so it introduces no binding loop, and it's only consulted when a
    // consumer hasn't already set Layout.preferredWidth/Height
    // explicitly (every dashboard card does, and is unaffected).
    implicitWidth: contentLayout.implicitWidth + padding * 2
    implicitHeight: contentLayout.implicitHeight + padding * 2

    color: backgroundColor

    radius: cardRadius

    border.width: showBorder ? 1 : 0
    border.color: borderColor

    clip: true

    antialiasing: true

    // ------------------------------------------------------------------
    // Shadow
    // ------------------------------------------------------------------

    layer.enabled: elevated

    layer.effect: MultiEffect {

        shadowEnabled: true

        shadowColor: root.shadowColor

        shadowBlur: 0.45

        shadowVerticalOffset: 4

        shadowHorizontalOffset: 0
    }

    // ------------------------------------------------------------------
    // Hover Effect
    // ------------------------------------------------------------------

    Behavior on scale {
        NumberAnimation {
            duration: Metrics.animationFast
        }
    }

    Behavior on border.color {
        ColorAnimation {
            duration: Metrics.animationFast
        }
    }

    HoverHandler {

        acceptedDevices: PointerDevice.Mouse

        onHoveredChanged: {

            if (hovered) {
                root.scale = 1.01
            } else {
                root.scale = 1.0
            }
        }
    }

    // ------------------------------------------------------------------
    // Content Container
    // ------------------------------------------------------------------

    ColumnLayout {

        id: contentLayout

        anchors.fill: parent

        anchors.margins: root.padding

        spacing: Metrics.spacingMedium
    }
}