pragma Singleton
import QtQuick

// Color design tokens for Panorama Water Tank Monitoring Dashboard.
// Clean industrial SCADA/HMI theme with primary blue (#1479E8),
// light-gray background (#F8FAFC), crisp white card surfaces (#FFFFFF),
// subtle light-gray borders (#E2E8F0), and distinct operational status tones.
QtObject {
    readonly property color headerBackground: "#171C28"

    readonly property color primary: "#1479E8"
    readonly property color primaryPressed: "#0F62BD"
    readonly property color primarySurface: "#EFF6FF"

    readonly property color water: "#1479E8"
    readonly property color waterTop: "#0284C7"
    readonly property color waterBottom: "#1D4ED8"

    readonly property color background: "#F8FAFC"
    readonly property color surface: "#FFFFFF"
    readonly property color surfaceAlt: "#F1F5F9"

    readonly property color divider: "#E2E8F0"
    readonly property color dividerStrong: "#CBD5E1"
    readonly property color border: "#E2E8F0"

    readonly property color success: "#10B981"
    readonly property color successText: "#059669"
    readonly property color successSurface: "#ECFDF5"

    readonly property color warning: "#F97316"
    readonly property color warningSurface: "#FFF7ED"

    readonly property color critical: "#EF4444"
    readonly property color criticalSurface: "#FEF2F2"

    readonly property color overflow: "#DC2626"
    readonly property color sensorError: "#8B5CF6"

    readonly property color offline: "#64748B"
    readonly property color offlineSurface: "#F1F5F9"

    readonly property color disabled: "#CBD5E1"

    readonly property color textPrimary: "#172033"
    readonly property color textSecondary: "#64748B"
    readonly property color textOnPrimary: "#FFFFFF"

    readonly property color tankOutline: "#94A3B8"
    readonly property color waterHighlight: "#60A5FA"
    readonly property color sidebarInactive: "#64748B"
}
