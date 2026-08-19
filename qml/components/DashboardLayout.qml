import QtQuick
import QtQuick.Layouts
import PanoramaWaterTank

// Dashboard page chrome: background, scroll container, and the
// responsive breakpoint state every section needs - computed once
// here instead of re-derived per section.
Item {
    id: root

    default property alias content: mainColumn.data

    readonly property bool compact: width < Theme.metrics.compactBreakpoint
    // This is a 2-tier layout today (compact/tablet share one
    // boundary; a genuine 3-tier system would need real design input
    // this refactor doesn't include). desktop/tablet exist so
    // consuming components have the vocabulary without inventing an
    // undesigned third visual state.
    readonly property bool tablet: compact
    readonly property bool desktop: !compact
    readonly property real availableWidth: width

    Rectangle {
        anchors.fill: parent
        color: Theme.colors.background
    }

    ColumnLayout {
        id: mainColumn
        anchors.fill: parent
        anchors.margins: Theme.metrics.contentMargin
        spacing: Theme.metrics.cardSpacing
    }
}
