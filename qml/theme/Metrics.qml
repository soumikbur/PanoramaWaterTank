// Metrics.qml
pragma Singleton
import QtQuick

QtObject {

    // ====================================================================
    // Window
    // ====================================================================

    readonly property int windowDefaultWidth: 1536
    readonly property int windowDefaultHeight: 1024

    readonly property int windowMinimumWidth: 1280
    readonly property int windowMinimumHeight: 800

    readonly property int compactBreakpoint: 1200
    readonly property int desktopBreakpoint: 1600
    readonly property int ultrawideBreakpoint: 2400

    // ====================================================================
    // Spacing Scale (8pt Design System)
    // ====================================================================

    readonly property int space4: 4
    readonly property int space8: 8
    readonly property int space12: 12
    readonly property int space14: 14
    readonly property int space16: 16
    readonly property int space20: 20
    readonly property int space24: 24
    readonly property int space32: 32
    readonly property int space40: 40
    readonly property int space48: 48
    readonly property int space64: 64

    // Semantic spacing
    readonly property int spacingTiny: space4
    readonly property int spacingSmall: space8
    readonly property int spacingMedium: space16
    readonly property int spacingLarge: space24
    readonly property int spacingXLarge: space32
    readonly property int spacingXXLarge: space48

    // ====================================================================
    // Radii
    // ====================================================================

    readonly property int radiusSmall: 4
    readonly property int radiusMedium: 8
    readonly property int radiusLarge: 12
    readonly property int radiusXLarge: 14

    // ====================================================================
    // App Layout
    // ====================================================================

    readonly property int pageMargin: 14
    readonly property int contentMargin: 14
    readonly property int headerHeight: 40
    readonly property int footerHeight: 28
    readonly property int sectionSpacing: 14

    // ====================================================================
    // Sidebar
    // ====================================================================

    readonly property int sidebarDefaultWidth: 270
    readonly property int sidebarMinWidth: 250
    readonly property int sidebarMaxWidth: 290
    readonly property real sidebarWidthFraction: 0.18

    // ====================================================================
    // Cards & Layout
    // ====================================================================

    readonly property int cardPadding: 16
    readonly property int cardSpacing: 14
    readonly property int cardMinimumWidth: 260
    readonly property int cardPreferredWidth: 320
    readonly property int cardMaximumWidth: 1200
    readonly property int cardMinimumHeight: 100

    readonly property real tankCardWidthFraction: 0.30
    readonly property real trendCardWidthFraction: 0.35
    readonly property real infoCardWidthFraction: 0.35

    // ====================================================================
    // Tank Visualization
    // ====================================================================

    readonly property int tankMinimumWidth: 300
    readonly property int tankMaximumWidth: 520
    readonly property real tankAspectRatio: 0.42
    readonly property int tankTopPadding: 12
    readonly property int tankBottomPadding: 12
    readonly property int tankCardMinHeight: 280

    // ====================================================================
    // Trend Chart
    // ====================================================================

    readonly property int chartMinimumHeight: 180
    readonly property int chartPreferredHeight: 240
    readonly property int chartMargin: 12
    readonly property int chartLegendSpacing: 8
    readonly property int chartLineThickness: 3
    readonly property int chartPointRadius: 4

    // ====================================================================
    // Dashboard Components
    // ====================================================================

    readonly property int summaryCardHeight: 122
    readonly property int summaryIconBadgeSize: 38
    readonly property int summaryIconBadgeRadius: 10
    readonly property int infoRowSpacing: 6
    readonly property int dividerThickness: 1

    // ====================================================================
    // Rank Indicator
    // ====================================================================

    readonly property int rankIndicatorHeight: 124
    readonly property int rankBarHeight: 28
    readonly property int rankBarRadius: 8
    readonly property int rankSegmentGap: 3
    readonly property int rankMarkerWidth: 14
    readonly property int rankMarkerHeight: 9
    readonly property int rankMarkerGap: 4
    readonly property int rankMarkerAnimationDuration: 300

    // ====================================================================
    // Icons
    // ====================================================================

    readonly property int iconSmall: 16
    readonly property int iconMedium: 20
    readonly property int iconLarge: 24
    readonly property int iconXLarge: 32

    // ====================================================================
    // Animation
    // ====================================================================

    readonly property int animationFast: 120
    readonly property int animationNormal: 200
    readonly property int animationSlow: 350
}