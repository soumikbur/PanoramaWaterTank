import QtQuick
import QtQuick.Layouts
import PanoramaWaterTank

// Page assembler only. Layout chrome and responsive state live in
// DashboardLayout; section composition lives in DashboardTopSection /
// DashboardStatus / DashboardSummaryGrid. This file places sections
// and nothing else - no business logic, no layout math.
Item {
    id: dashboardPage

    DashboardLayout {
        id: layout
        anchors.fill: parent

        DashboardTopSection {
            Layout.fillWidth: true
            Layout.fillHeight: true
            compact: layout.compact
        }

        DashboardStatus {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.metrics.rankIndicatorHeight
        }

        DashboardSummaryGrid {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.metrics.summaryCardHeight
            compact: layout.compact
        }
    }
}
