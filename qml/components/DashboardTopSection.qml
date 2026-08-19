import QtQuick
import QtQuick.Layouts
import PanoramaWaterTank

// Responsive 3-column top row: Tank Visualization / Level Trend / Tank
// Information. Collapses to a single stacked column when `compact` is
// true. Owns the width-fraction math so no sibling file needs to know
// how the three cards divide the available width.
GridLayout {
    id: topRow

    // Set externally by whoever owns the page-wide breakpoint
    // (DashboardLayout), not re-derived from this item's own width -
    // topRow's width is narrower than the full page width by the page
    // margins, so self-deriving would shift the breakpoint threshold.
    property bool compact: false

    columns: compact ? 1 : 3
    columnSpacing: Theme.metrics.cardSpacing
    rowSpacing: Theme.metrics.cardSpacing

    readonly property real cardAreaWidth: width - (compact ? 0 : columnSpacing * 2)

    TankVisualization {
        Layout.fillWidth: true
        Layout.preferredWidth: topRow.compact
            ? -1 : topRow.cardAreaWidth * Theme.metrics.tankCardWidthFraction
        Layout.fillHeight: true
    }

    LevelTrendCard {
        Layout.fillWidth: true
        Layout.preferredWidth: topRow.compact
            ? -1 : topRow.cardAreaWidth * Theme.metrics.trendCardWidthFraction
        Layout.fillHeight: true
        Layout.minimumHeight: Theme.metrics.tankCardMinHeight
    }

    TankInfoCard {
        Layout.fillWidth: true
        Layout.preferredWidth: topRow.compact
            ? -1 : topRow.cardAreaWidth * Theme.metrics.infoCardWidthFraction
        Layout.fillHeight: true
        Layout.minimumHeight: Theme.metrics.tankCardMinHeight
    }
}