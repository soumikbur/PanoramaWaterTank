# Panorama Water Tank Monitor

An industrial SCADA-style Qt 6 / QML desktop dashboard for monitoring a
cylindrical water tank instrumented with an ESP32-S3, connected through a
Ubidots REST API backend (sensor → ESP32-S3 → EC200U-CN 4G module → Ubidots).

## Status

This repository is currently at **Milestone 1: Project Skeleton**
(`docs/06-development-workflow-roadmap.md`, Section 4). The application
builds and launches to a blank, themed shell — no live tank data, network
calls, or engineering calculations are wired up yet. See the roadmap for
the full milestone sequence.

## Documentation

The complete requirements, architecture, UI/UX design system, engineering
model, implementation blueprint, and development roadmap for this project
live in `docs/`:

- `01-requirements-analysis.md`
- `02-software-architecture.md`
- `03-ui-ux-design-system.md`
- `04-engineering-model-workflow.md`
- `05-implementation-blueprint.md`
- `06-development-workflow-roadmap.md`

Read these before making an architectural change. This project follows a
planning-first process, and `05-implementation-blueprint.md` Section 15
defines the directory-level dependency rules enforced in code review
(Section 3 of the roadmap).

## Requirements

- Qt 6.9 or later — Core, Quick, QuickControls2, Svg, Network
- CMake 3.21 or later
- A C++20-capable compiler (MSVC 2022+, GCC 12+, or Clang 15+)
- Qt Creator 17+ is recommended but not required

## Building

### Windows (Qt Creator or command line)

```
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Debug
```

### Linux

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

The resulting executable is `PanoramaWaterTank` (`PanoramaWaterTank.exe` on
Windows) under `build/` (exact path depends on generator and configuration).

### Optional: treat warnings as errors

```
cmake -S . -B build -DPANORAMA_WARNINGS_AS_ERRORS=ON
```

## Project structure

```
src/            C++ backend, organized by layer
    backend/        Shared, dependency-free types (Types.h)
    logging/         Logger
    settings/         SettingsManager
    utilities/         TimeManager
    calculations/       VolumeCalculator
    models/              TankModel
    repository/           TankRepository
    network/               ApiClient, ConnectionManager
    state/                   ApplicationStateManager
    controllers/              DashboardController
qml/            QML presentation layer
    theme/           Colors, Typography, Metrics, Icons, Theme (design tokens)
    components/       Header, Sidebar, Footer
    pages/             DashboardPage
    assets/             Reserved for QML-local assets
resources/      Fonts, icons, images, SVG assets (Qt resource system)
config/         Reserved for deployment-time configuration
docs/           The full planning document set (Phases 1-6)
tests/          Reserved for the test suite (docs/05, Section 17)
scripts/        Reserved for build/CI helper scripts
```

Directory-level allowed/forbidden dependencies are specified in
`docs/05-implementation-blueprint.md`, Section 15, and are not repeated
here to avoid the two documents drifting out of sync.

## License

See `LICENSE`.
