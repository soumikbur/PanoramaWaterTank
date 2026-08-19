cmake_minimum_required(VERSION 3.21)

project(PanoramaWaterTank
    VERSION 1.0.0
    DESCRIPTION "Panorama Water Tank Monitor - Qt/QML industrial dashboard"
    LANGUAGES CXX
)

# --- Language configuration -------------------------------------------------

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# --- Build type ---------------------------------------------------------

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

option(PANORAMA_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

# --- Qt ----------------------------------------------------------------

find_package(Qt6 6.9 REQUIRED COMPONENTS
    Core
    Quick
    QuickControls2
    Svg
    Network
)

qt_standard_project_setup(REQUIRES 6.9)

# --- Executable target ---------------------------------------------------

qt_add_executable(PanoramaWaterTank
    src/main.cpp

    src/backend/Types.h

    src/logging/Logger.h
    src/logging/Logger.cpp

    src/settings/SettingsManager.h
    src/settings/SettingsManager.cpp

    src/utilities/TimeManager.h
    src/utilities/TimeManager.cpp
    src/utilities/HistoryExporter.h
    src/utilities/HistoryExporter.cpp

    src/calculations/VolumeCalculator.h
    src/calculations/VolumeCalculator.cpp

    src/models/TankModel.h
    src/models/TankModel.cpp

    src/repository/TankRepository.h
    src/repository/TankRepository.cpp

    src/network/ConnectionManager.h
    src/network/ConnectionManager.cpp
    src/network/ApiClient.h
    src/network/ApiClient.cpp

    src/state/ApplicationStateManager.h
    src/state/ApplicationStateManager.cpp

    src/controllers/DashboardController.h
    src/controllers/DashboardController.cpp
)

# --- QML module ----------------------------------------------------------
#
# All QML files are registered as ONE logical module ("PanoramaWaterTank").
# Files under qml/theme, qml/components, and qml/pages are all part of this
# same module, so any file can reference any other type in it after
# `import PanoramaWaterTank` - no relative path imports are required.
# Resource entries (icons/fonts/images) are added here once real assets
# exist (Phase 6, Milestone 6); none are populated yet in this skeleton.

# The theme token files declare `pragma Singleton` internally, which is
# normally sufficient on its own for qt_add_qml_module to register them as
# QML singletons. This explicit property is set in addition, as a defensive
# measure: it was required to produce a correctly-registered `singleton`
# entry in the generated qmldir during this milestone's build verification
# against a Qt 6.4.2 toolchain, where pragma-only auto-detection did not
# reliably take effect. It must be set before the qt_add_qml_module() call
# below, since that call consumes these source file properties.
set_source_files_properties(
    qml/theme/Colors.qml
    qml/theme/Typography.qml
    qml/theme/Metrics.qml
    qml/theme/Icons.qml
    qml/theme/Theme.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(PanoramaWaterTank
    URI PanoramaWaterTank
    VERSION 1.0

    QML_FILES
        # ------------------------------------------------------------------
        # Application
        # ------------------------------------------------------------------
        qml/App.qml

        # ------------------------------------------------------------------
        # Theme
        # ------------------------------------------------------------------
        qml/theme/Colors.qml
        qml/theme/Typography.qml
        qml/theme/Metrics.qml
        qml/theme/Icons.qml
        qml/theme/Theme.qml

        # ------------------------------------------------------------------
        # Core Components
        # ------------------------------------------------------------------
        qml/components/BaseCard.qml
        qml/components/Header.qml
        qml/components/FramelessResizeHandler.qml
        qml/components/Sidebar.qml

        qml/components/NavItem.qml
        qml/components/Footer.qml
        qml/components/WaterWave.qml
        qml/components/StatusBadge.qml
        qml/components/InfoRow.qml

        # ------------------------------------------------------------------
        # Dashboard Components
        # ------------------------------------------------------------------
        qml/components/SummaryCard.qml
        qml/components/RankIndicator.qml
        qml/components/TankVisualization.qml
        qml/components/TrendChart.qml
        qml/components/LevelTrendCard.qml
        qml/components/TankInfoCard.qml
        qml/components/DashboardLayout.qml
        qml/components/DashboardTopSection.qml
        qml/components/DashboardStatus.qml
        qml/components/DashboardSummaryGrid.qml

        # ------------------------------------------------------------------
        # Pages
        # ------------------------------------------------------------------
        qml/pages/PlaceholderPage.qml
        qml/pages/DashboardPage.qml
        qml/pages/WaterTankPage.qml
        qml/pages/DevicesPage.qml
        qml/pages/AlertsPage.qml
        qml/pages/HistoryPage.qml
        qml/pages/SettingsPage.qml
)

# --- Include paths ---------------------------------------------------------
#
# Every backend subdirectory is on the include path so internal headers can
# be included by their bare filename (e.g. #include "Logger.h") regardless
# of which layer is including them. This keeps cross-layer includes short
# once later milestones wire the layers together, while the *directory*
# each header lives in still encodes and enforces its layer (docs/05 §15).

target_include_directories(PanoramaWaterTank PRIVATE
    src
    src/backend
    src/logging
    src/settings
    src/utilities
    src/calculations
    src/models
    src/repository
    src/network
    src/state
    src/controllers
)

# --- Compiler warnings ---------------------------------------------------

if (MSVC)
    target_compile_options(PanoramaWaterTank PRIVATE /W4 /permissive-)
    if (PANORAMA_WARNINGS_AS_ERRORS)
        target_compile_options(PanoramaWaterTank PRIVATE /WX)
    endif()
else()
    target_compile_options(PanoramaWaterTank PRIVATE -Wall -Wextra -Wpedantic)
    if (PANORAMA_WARNINGS_AS_ERRORS)
        target_compile_options(PanoramaWaterTank PRIVATE -Werror)
    endif()
endif()

# --- Compile definitions --------------------------------------------------

target_compile_definitions(PanoramaWaterTank PRIVATE
    QT_DISABLE_DEPRECATED_BEFORE=0x060900
    $<$<CONFIG:Debug>:PANORAMA_DEBUG_BUILD>
    $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>
)

# --- Link libraries --------------------------------------------------------

target_link_libraries(PanoramaWaterTank
    PRIVATE
        Qt6::Core
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Svg
        Qt6::Network
)

# --- Platform target properties -------------------------------------------
#
# The executable's runtime output directory is deliberately set to a
# dedicated bin/ subdirectory rather than the build root. qt_add_qml_module
# creates a build-tree directory named after the module URI
# ("PanoramaWaterTank/") to hold generated QML artifacts; since the
# executable target is also named "PanoramaWaterTank", leaving it in the
# build root collides with that directory on the filesystem. Routing the
# binary into bin/ avoids the collision entirely.

set_target_properties(PanoramaWaterTank PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

# --- Install -------------------------------------------------------------

include(GNUInstallDirs)
install(TARGETS PanoramaWaterTank
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# --- Test Targets --------------------------------------------------------

add_executable(test_csv_export
    tests/test_csv_export.cpp
    src/utilities/HistoryExporter.h
    src/utilities/HistoryExporter.cpp
)

target_include_directories(test_csv_export PRIVATE
    src
    src/utilities
)

target_link_libraries(test_csv_export PRIVATE
    Qt6::Core
)

set_target_properties(test_csv_export PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_executable(test_e2e_integration
    tests/test_e2e_integration.cpp
    src/utilities/HistoryExporter.h
    src/utilities/HistoryExporter.cpp
    src/models/TankModel.h
    src/models/TankModel.cpp
    src/calculations/VolumeCalculator.h
    src/calculations/VolumeCalculator.cpp
)

target_include_directories(test_e2e_integration PRIVATE
    src
    src/backend
    src/logging
    src/settings
    src/utilities
    src/calculations
    src/models
    src/repository
    src/network
    src/state
    src/controllers
)

target_link_libraries(test_e2e_integration PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Quick
)

set_target_properties(test_e2e_integration PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

