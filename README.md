# Panorama Water Tank Monitor

A Qt/QML-based industrial-style water tank monitoring dashboard for visualizing real-time tank level, water volume, environmental conditions, sensor health, connectivity, alerts, and historical level trends.

The system is designed around an embedded water-level sensing device with an ESP32-class controller, ADS1115 ADC, BME280 environmental sensor, RS485/Modbus RTU, BLE, EC200U LTE connectivity, EEPROM-based configuration storage, FreeRTOS task scheduling, and an Ubidots cloud data pipeline.

The desktop dashboard provides a SCADA/HMI-style interface for monitoring the connected tank and presenting sensor information in a clear, operator-oriented format.

> **Note:** Hardware/firmware behavior described below is based on the project's firmware flow documentation. The dashboard implementation may contain additional application-layer functionality beyond the firmware document.

---

## Features

### Dashboard

The main dashboard provides a consolidated view of the water tank system:

* Real-time water level percentage
* Current water height in centimeters
* Calculated tank volume
* Tank capacity
* Tank height
* Temperature
* Sensor health
* Device connection status
* Last-update timestamp
* Water-level status/rank
* Historical water-level trend
* System operational status
* Animated water-tank visualization

The dashboard is designed to resemble an industrial monitoring/HMI application rather than a generic consumer dashboard.

---

## Water Level Monitoring

The water level is represented using both physical height and percentage.

For the current tank configuration:

```text
Tank Height    : 50.00 cm
Tank Capacity  : 20.00 L
```

The water-level percentage is calculated from the physical water height:

```text
Level (%) = (Water Height / Tank Height) × 100
```

For example:

```text
Water Height = 27.58 cm
Tank Height  = 50.00 cm

Level = (27.58 / 50.00) × 100
      = 55.16%
```

The dashboard therefore treats `27.58 cm` and `55.16%` as different representations of the same physical level.

The tank scale is oriented correctly:

```text
100%  ─────────── Tank maximum
 75%
 50%  ─────────── Current region
 25%
  0%  ─────────── Tank bottom
```

The 0% position is always at the bottom of the tank and 100% is always at the top.

---

## Tank Volume

The tank configuration used by the dashboard is:

```text
Tank Height   : 50.00 cm
Tank Capacity : 20.00 L
```

The dashboard displays the calculated volume alongside the maximum capacity.

Example:

```text
Current Volume : 11.03 L
Tank Capacity  : 20.00 L
```

Volume and level are kept consistent with the configured tank geometry.

---

# System Architecture

The overall system consists of four major layers:

```text
┌─────────────────────────────────────────────┐
│              Qt/QML Dashboard               │
│                                             │
│  Dashboard │ Water Tank │ Devices │ Alerts │
│  History   │ Settings                     │
└──────────────────────┬──────────────────────┘
                       │
                       │ Application Data
                       ▼
┌─────────────────────────────────────────────┐
│            Qt/C++ Backend Layer             │
│                                             │
│ ApiClient │ TankRepository │ TankModel      │
│ ConnectionManager │ Data Processing        │
└──────────────────────┬──────────────────────┘
                       │
                       │ HTTP / Cloud API
                       ▼
┌─────────────────────────────────────────────┐
│                 Ubidots                     │
│                                             │
│ Device Variables / Historical Data          │
└──────────────────────┬──────────────────────┘
                       │
                       │ LTE / Internet
                       ▼
┌─────────────────────────────────────────────┐
│          Embedded Water Level Device        │
│                                             │
│ ESP32-class Controller                      │
│ ADS1115 │ BME280 │ RS485 │ BLE │ EC200U     │
└─────────────────────────────────────────────┘
```

---

# Embedded Firmware Architecture

The embedded firmware uses FreeRTOS tasks distributed across the controller cores.

The documented firmware initialization sequence is:

```text
Power ON
   │
   ▼
Hardware Initialization
   │
   ├── Serial Console
   ├── I2C
   ├── EEPROM
   ├── BLE
   ├── BME280
   ├── ADS1115
   ├── RS485
   ├── EC200U LTE
   ├── Buzzer
   └── RGB LED
   │
   ▼
Load Configuration from EEPROM
   │
   ▼
Self Test / Calibration
   │
   ▼
Initialize FreeRTOS Tasks
   │
   ▼
Main Scheduler
```

The firmware flow documentation specifies this initialization and task architecture.

---

# Hardware Components

## ADS1115 ADC

The ADS1115 is used for differential analog measurement.

Configuration documented by the firmware:

```text
Interface       : I2C
Address         : 0x48
Mode            : Differential A0-A1
Resolution      : 16-bit
Gain            : ±4.096 V
Data Rate       : 128 SPS
```

The sensor data processing pipeline is:

```text
ADC
 │
 ▼
10-Sample Average
 │
 ▼
Exponential Filter
 │
 ▼
Offset Calculation
 │
 ▼
Lookup Table
 │
 ▼
Linear Interpolation
 │
 ▼
Water Level
```

The firmware uses an exponential filter with:

```text
alpha = 0.1
```

and applies a deadband below:

```text
0.5 cm
```

to suppress small fluctuations.

---

# BME280 Environmental Sensor

The BME280 provides:

* Temperature
* Atmospheric pressure
* Relative humidity

The documented firmware reads BME280 data periodically and maintains EEPROM-backed data as a fallback.

The firmware also includes a dedicated retry mechanism if the BME280 becomes unavailable.

Dashboard representation:

```text
Temperature : 25.00 °C
Pressure    : Available from backend
Humidity    : Available from backend
```

---

# EC200U LTE Modem

The EC200U is used as the cellular communication interface.

Documented interface:

```text
Interface : UART
Pins      : 7, 8
Protocol  : AT Commands
Network   : TCP/IP
```

The firmware checks modem connectivity before attempting cloud uploads and retries when the connection is unavailable.

---

# RS485 / Modbus RTU

The device exposes sensor data through RS485 using Modbus RTU.

Documented configuration:

```text
Interface : RS485
Baud Rate : 9600
Mode      : Modbus RTU Slave
Slave ID  : 1
```

The firmware supports the Read Holding Registers function:

```text
Function Code: 0x03
```

Documented registers:

| Register | Data                    |
| -------: | ----------------------- |
|        0 | Water level (`cm × 10`) |
|        1 | Pressure (`raw × 10`)   |
|        2 | Sensor status           |

The firmware calculates the Modbus CRC before transmitting the response.

---

# Bluetooth Low Energy

The embedded device exposes a BLE server:

```text
Water Level Sensor
```

The documented BLE command interface includes:

```text
ALARM ON/OFF
SET_AIR=xxxx
SHOW_AIR
SHOW_LEVEL
SHOW_BME
SHOW_VOLTAGE
SET_MAX_HEIGHT=xxx
HELP
```

The device periodically sends information including:

* Raw voltage
* BME pressure
* Sensor status
* Alarm state

The documented BLE update interval is approximately 2 seconds.

---

# Water-Level Calibration

The firmware stores calibration information in EEPROM.

Important parameters include:

```text
A_air
Calibration Status
Alarm State
Maximum Height
BME Data
Calibration Magic
```

The documented calibration magic value is:

```text
0xAA55
```

If the device has not been calibrated, the firmware performs an automatic air-baseline calibration.

Documented calibration procedure:

```text
Calibration Duration : 5 seconds
Samples               : 50
Measurement Mode      : Differential
```

The resulting air baseline is stored as the reference for subsequent water-level calculations.

---

# Water-Level Calculation

The documented firmware calculates water level using the following pipeline:

```text
filteredADC
     │
     ▼
offset = filteredADC - A_air
     │
     ▼
Lookup Table
0–500 cm
1 cm step
     │
     ▼
Linear Interpolation
     │
     ▼
Deadband
< 0.5 cm → 0
     │
     ▼
Water Height
```

The lookup table is documented for a default maximum height of:

```text
500 cm
```

with configurable maximum height support between:

```text
100–1000 cm
```

The dashboard's current tank configuration is independently represented as:

```text
50.00 cm
```

and must not be confused with the firmware's documented default maximum-height configuration.

---

# FreeRTOS Task Architecture

The firmware uses multiple FreeRTOS tasks.

| Task         |   Core | Priority | Stack | Interval |
| ------------ | -----: | -------: | ----: | -------: |
| SensorTask   | Core 0 |        3 |  8 KB |    50 ms |
| ModbusTask   | Core 1 |        1 |  4 KB |    10 ms |
| AlertTask    | Core 1 |        1 |  4 KB |   100 ms |
| BLETask      | Core 1 |        1 |  4 KB |   100 ms |
| UploadTask   | Core 1 |        1 |  8 KB |     90 s |
| BMERetryTask | Core 1 |        1 |  4 KB |      5 s |

These values are taken from the documented firmware flow.

---

# SensorTask

The SensorTask is responsible for the primary measurement pipeline.

Its documented responsibilities include:

1. Read ADC.
2. Average 10 samples.
3. Apply exponential filtering.
4. Validate sensor connection.
5. Read BME280 periodically.
6. Perform periodic air-baseline recalibration.
7. Calculate water level.
8. Validate the calculated level.
9. Update shared data.
10. Print diagnostic information to the serial console.

Sensor disconnection is detected when the ADC reading falls outside the documented valid range:

```text
ADC < 100
OR
ADC > 32767
```

The firmware also documents a safety condition where five consecutive bad readings result in:

```text
Sensor Status = 0
```

---

# Alert System

The firmware contains three water-level states.

## Critical

Triggered when:

```text
Level < 10% of maximum height
```

Actions:

```text
Buzzer : 3 kHz / 100 ms, 3 times
RGB    : Blinking
```

## Warning

Triggered when:

```text
Level < 20% of maximum height
```

Actions:

```text
Buzzer : 2 kHz / 200 ms
RGB    : Solid ON
```

## Normal

Triggered when:

```text
Level > 20% of maximum height
```

Actions:

```text
Buzzer : OFF
RGB    : OFF
```

These thresholds are defined by the documented firmware flow.

---

# Cloud Data Pipeline

The embedded system periodically builds a JSON payload and uploads sensor information to Ubidots.

Documented upload interval:

```text
90 seconds
```

The documented payload includes:

```text
rawVoltage
bmePressure
sensorStatus
```

The firmware documentation notes that water level and pressure fields were commented out in the referenced upload implementation.

The Qt dashboard consumes cloud-side device/variable data through its application data layer.

---

# Qt/QML Application Architecture

The desktop application is divided between a C++ backend and QML presentation layer.

## C++ Backend

Core backend components include:

```text
ApiClient
TankRepository
TankModel
ConnectionManager
```

### ApiClient

Responsible for:

* HTTP communication
* API requests
* Authentication
* Polling
* JSON parsing
* Network error handling
* Extracting sensor variables

### TankRepository

Responsible for:

* Processing incoming tank readings
* Validating measurements
* Updating tank state
* Maintaining cached values
* Connecting backend readings to the tank model

### TankModel

Represents the tank state exposed to the QML layer.

Typical dashboard properties include:

```text
Water Level
Water Height
Tank Volume
Tank Capacity
Temperature
Sensor Status
Connection Status
Last Updated
```

### ConnectionManager

Responsible for connection-health handling and retry/backoff behavior.

---

# QML UI Architecture

The interface is implemented using Qt Quick / QML.

Major UI components include:

```text
Main.qml
TopBar.qml
Sidebar.qml
DashboardView.qml
DeviceCard.qml
StatCard.qml
StatusCard.qml
TankVisualization.qml
LevelTrendCard.qml
TrendChart.qml
TankInfoCard.qml
RankIndicator.qml
BaseCard.qml
Theme.qml
Metrics.qml
```

Additional application pages include:

```text
Dashboard
Water Tank
Devices
Alerts
History
Settings
```

The UI follows a reusable component-based structure instead of placing all dashboard logic inside a single QML file.

---

# Dashboard Layout

The main dashboard is organized into the following sections:

```text
┌─────────────────────────────────────────────────────────┐
│                    Top Application Bar                  │
├───────────────┬─────────────────────────────────────────┤
│               │ Water Level │ Level Trend │ Tank Info  │
│               ├─────────────────────────────────────────┤
│   Navigation  │       Water Level Indicator             │
│               ├─────────────────────────────────────────┤
│               │ Level │ Volume │ Status │ Temp │ Time  │
│               └─────────────────────────────────────────┘
│               │                  Footer                 │
└───────────────┴─────────────────────────────────────────┘
```

The design uses:

* Rounded cards
* Blue primary accent
* Light background
* Dark typography
* Green health indicators
* Responsive layouts
* Industrial monitoring visual hierarchy

---

# Historical Level Trend

The dashboard provides a historical level-trend visualization.

The graph represents:

```text
Water Level (%)
```

rather than raw centimeters.

For a tank with a 50 cm height:

```text
0 cm  → 0%
25 cm → 50%
50 cm → 100%
```

Therefore:

```text
27.58 cm → 55.16%
```

The chart uses:

* Percentage-based Y-axis
* Time-based X-axis
* Historical readings
* Blue trend line
* Area fill
* Current-value marker
* Configurable history period

The chart must maintain a consistent percentage conversion with the tank visualization and rank indicator.

---

# Water-Level Rank Indicator

The dashboard categorizes water level into four ranges:

|   Range | State    |
| ------: | -------- |
|   0–25% | LOW      |
|  25–50% | MODERATE |
|  50–75% | GOOD     |
| 75–100% | HIGH     |

For example:

```text
55.16% → GOOD
```

The current percentage is represented by a dynamically positioned marker on the horizontal scale.

---

# Current Example State

A representative dashboard state is:

```text
Water Level       : 55.16%
Water Height      : 27.58 cm
Tank Height       : 50.00 cm
Tank Volume       : 11.03 L
Tank Capacity     : 20.00 L
Temperature       : 25.00 °C
Connection        : Connected
Sensor Status     : Healthy
Water Status      : Normal
```

The actual application should use live backend values rather than permanently displaying these example values.

---

# Project Structure

A typical project structure is:

```text
PanoramaWaterTank/
│
├── CMakeLists.txt
│
├── src/
│   ├── main.cpp
│   │
│   ├── ApiClient.h
│   ├── ApiClient.cpp
│   │
│   ├── TankModel.h
│   ├── TankModel.cpp
│   │
│   ├── TankRepository.h
│   ├── TankRepository.cpp
│   │
│   ├── ConnectionManager.h
│   └── ConnectionManager.cpp
│
├── qml/
│   ├── Main.qml
│   ├── TopBar.qml
│   ├── Sidebar.qml
│   ├── DashboardView.qml
│   ├── TankVisualization.qml
│   ├── LevelTrendCard.qml
│   ├── TrendChart.qml
│   ├── TankInfoCard.qml
│   ├── RankIndicator.qml
│   ├── StatCard.qml
│   ├── StatusCard.qml
│   ├── BaseCard.qml
│   ├── Theme.qml
│   └── Metrics.qml
│
├── assets/
│   ├── icons/
│   └── images/
│
└── README.md
```

The exact directory structure may vary according to the current CMake configuration.

---

# Software Requirements

## Development Environment

Recommended environment:

```text
Qt              : Qt 6.x
Qt Creator      : Compatible Qt 6.x version
Build System    : CMake
Language        : C++17/C++20 as configured by project
UI              : Qt Quick / QML
Platform        : Windows / Linux
```

The current project has been developed around Qt 6.11.

---

# Build Instructions

## 1. Clone the Repository

```bash
git clone <YOUR_REPOSITORY_URL>
cd PanoramaWaterTank
```

## 2. Open in Qt Creator

Open:

```text
CMakeLists.txt
```

Configure the project using the installed Qt 6 kit.

Select an appropriate build directory.

## 3. Configure

Allow CMake to configure the project.

Verify that:

* Qt Quick is available.
* Qt QML modules are available.
* Network modules required by the application are available.
* C++ compiler is correctly configured.

## 4. Build

Build the project using Qt Creator or CMake.

Example:

```bash
cmake -S . -B build
cmake --build build
```

Run the resulting executable.

---

# Configuration

The application requires access to the backend/cloud data source used by the project.

Authentication credentials should **not** be hardcoded into source code or committed to GitHub.

Use one of the project's supported configuration mechanisms for:

```text
API URL
Authentication Token
Device ID
Variable IDs
Polling Interval
```

If environment variables are used, an example configuration can be documented as:

```text
UBIDOTS_API_URL=<api-url>
UBIDOTS_TOKEN=<your-token>
UBIDOTS_DEVICE=<device-name>
```

Never commit real API tokens, passwords, private keys, certificates, or other credentials.

The firmware documentation contains a cloud authentication token in its source material; that credential should be treated as compromised if it was ever exposed publicly and should not be copied into this repository.

---

# Data Flow

The complete data path can be represented as:

```text
┌──────────────┐
│ Level Sensor │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│   ADS1115    │
│ Differential │
│     ADC      │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ 10-Sample    │
│ Averaging    │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ Exponential  │
│ Filter       │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ Air Baseline │
│ Offset       │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ Lookup Table │
│ + Interp.    │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ Water Height │
│     cm       │
└──────┬───────┘
       │
       ├──────────────► Percentage
       │
       ├──────────────► Tank Volume
       │
       └──────────────► Alert State
       │
       ▼
┌──────────────┐
│ Shared Data  │
│ Mutex        │
└──────┬───────┘
       │
       ├──────────────► BLE
       │
       ├──────────────► Modbus
       │
       └──────────────► LTE / Ubidots
                                  │
                                  ▼
                         ┌────────────────┐
                         │ Qt/C++ Backend │
                         └───────┬────────┘
                                 │
                                 ▼
                         ┌────────────────┐
                         │    Qt/QML      │
                         │    Dashboard    │
                         └────────────────┘
```

The embedded data-flow stages are documented as ADC → filter → offset calculation → lookup/interpolation → water level, with shared data consumed by BLE, Modbus, and upload tasks.

---

# Communication Interfaces

| Interface  | Purpose                            |
| ---------- | ---------------------------------- |
| I2C        | ADS1115 / BME280                   |
| UART       | EC200U LTE                         |
| RS485      | Modbus RTU                         |
| BLE        | Local configuration and monitoring |
| HTTP       | Cloud upload                       |
| Serial     | Firmware debugging                 |
| Qt Network | Dashboard cloud communication      |

Documented communication timing includes:

```text
BLE updates       : 2 seconds
Cloud upload      : 90 seconds
Modbus polling    : 10 ms
BME retry         : 5 seconds
```

---

# EEPROM Configuration

The firmware stores important operating parameters in EEPROM.

Documented parameters include:

```text
Air baseline (A_air)
Calibration status
Alarm state
Maximum height
BME temperature
BME pressure
BME humidity
Calibration magic
```

The EEPROM size documented by the firmware is:

```text
512 bytes
```

---

# Safety and Fault Handling

The firmware includes several fault-handling mechanisms.

## Watchdog

FreeRTOS task monitoring is used as part of the system safety mechanism.

## Sensor Disconnect Detection

Invalid ADC readings are detected and sensor status is updated accordingly.

The documented safety behavior includes five consecutive bad readings resulting in:

```text
Status = 0
```

## BME280 Fallback

If the BME280 fails:

```text
EEPROM backup data
        ↓
Fallback environmental values
```

## Modem Retry

The LTE connection is periodically checked.

If disconnected:

```text
Connection Failure
        ↓
Retry
        ↓
Reconnect
```

## Thread Safety

Shared sensor data is protected using a mutex to prevent concurrent task access problems.

These mechanisms are documented in the firmware architecture.

---

# Debugging

## Firmware Serial Console

The documented serial interface uses:

```text
Baud Rate: 115200
```

Firmware diagnostics can be used to inspect:

* ADC readings
* Filtered values
* Air baseline
* Calculated water level
* BME280 readings
* Sensor status
* Modem status
* Calibration state

## Qt/QML Debugging

For the desktop application, check:

```text
Qt Creator Application Output
QML warnings
C++ runtime logs
Network requests
JSON parsing
Backend property updates
```

Particular attention should be given to:

```text
WaterHeight
LevelPercentage
TankVolume
TankCapacity
TankHeight
LastUpdated
ConnectionStatus
SensorStatus
```

because these values are shared across multiple dashboard components.

---

# Common Data Consistency Rules

The application should maintain the following relationship:

```text
Water Height
      │
      ├──► Level Percentage
      │
      └──► Tank Volume
```

The same underlying reading must drive:

* Tank fill visualization
* Current level percentage
* Trend chart
* Rank indicator
* Water-level statistic card
* Tank volume
* Tank information

This prevents situations where different dashboard components display different interpretations of the same sensor reading.

---

# UI Design Principles

The dashboard follows an industrial monitoring design philosophy.

### Primary objectives

* Fast visual interpretation
* Clear system status
* Minimal visual noise
* Strong measurement hierarchy
* Consistent units
* Consistent status colors
* Clear fault indication
* Responsive desktop layout

### Status colors

```text
LOW / CRITICAL  → Red
WARNING         → Orange
GOOD            → Yellow/Gold
HIGH / Healthy  → Green
Primary Data    → Blue
```

---

# Water Animation

The tank visualization includes a subtle animated water surface.

The animation should:

* Continuously move horizontally.
* Use smooth wave patterns.
* Preserve the calculated water height.
* Avoid changing the actual level value.
* Remain visually subtle.
* Avoid excessive CPU/GPU usage.

The animation is purely a presentation feature.

It does not modify the sensor reading or calculated water level.

---

# Project Goals

The project aims to provide:

1. A real-time water tank monitoring interface.
2. Accurate conversion between physical tank height and percentage level.
3. Tank-volume visualization based on configured capacity.
4. Historical water-level monitoring.
5. Device and sensor health monitoring.
6. Industrial communication support.
7. Cloud-connected telemetry.
8. Embedded fault handling.
9. Persistent calibration/configuration.
10. A modular Qt/QML desktop interface.

---

# Development Roadmap

Potential future improvements include:

* Additional tank geometry models.
* Multiple tank/device support.
* Configurable alarm thresholds.
* Historical data export.
* CSV report generation.
* More detailed alert history.
* Device configuration interface.
* Offline data caching.
* Improved cloud connection diagnostics.
* Authentication/configuration management.
* Advanced historical analytics.
* Multiple dashboard layouts.
* User access control.
* Production deployment packaging.

---

# Known Design Constraints

The current implementation is designed around a single monitored tank configuration:

```text
Height   : 50.00 cm
Capacity : 20.00 L
```

The firmware architecture itself documents a configurable maximum height with a default of 500 cm. The dashboard tank configuration should therefore be treated as an application/tank-specific configuration rather than assuming that every firmware deployment uses a 50 cm tank.

---

# Security

Do not commit the following to Git:

```text
API tokens
Passwords
Private keys
TLS certificates containing private material
Device credentials
Production credentials
```

Use environment variables or a local configuration file excluded through `.gitignore`.

Example:

```gitignore
.env
*.key
*.pem
credentials.json
secrets.json
```

If a credential has already been committed to a public repository, rotate/revoke it immediately rather than simply deleting it from the latest commit.

---

# Git Workflow

Recommended workflow:

```bash
git clone <repository>
cd PanoramaWaterTank

git checkout -b feature/dashboard-improvement

# Make changes

git status
git add .
git commit -m "Improve water tank dashboard"

git push origin feature/dashboard-improvement
```

Before pushing:

```bash
git status
```

Verify that no credentials, build directories, generated files, or temporary files are included.

---

# Build Artifacts

Do not commit generated build directories.

Typical entries for `.gitignore`:

```gitignore
build/
cmake-build-*/
*.user
*.user.*
*.autosave
```

Generated Qt meta-object files should also not be manually created or committed.

The build system should generate required Qt artifacts automatically.

---

# License

Add the project's intended license here.

For example:

```text
Copyright © 2026 Panorama Electronics.

All rights reserved.
```

Replace this section with the organization's actual licensing terms before publishing the repository.

---

# Project Status

**Status:** Active Development

The project currently contains:

* Qt/QML dashboard interface
* Water tank visualization
* Live tank metrics
* Historical level trend
* Tank information panel
* Water-level ranking
* Device monitoring
* Alert/status presentation
* C++ backend data layer
* Cloud API integration
* Embedded firmware architecture
* BLE / RS485 / LTE communication architecture

The system is being developed toward a production-oriented industrial water-level monitoring solution.

---

# Technology Stack

```text
Embedded
├── ESP32-class MCU
├── FreeRTOS
├── C/C++
├── ADS1115
├── BME280
├── EC200U LTE
├── RS485 / Modbus RTU
├── BLE
└── EEPROM

Desktop Application
├── C++
├── Qt 6
├── Qt Quick
├── QML
├── CMake
└── Qt Network

Cloud
└── Ubidots
```

---

# Summary

Panorama Water Tank Monitor combines embedded sensing, signal processing, industrial communication, cellular connectivity, cloud telemetry, and a Qt/QML desktop HMI into a single water-level monitoring platform.

The embedded device acquires and processes sensor data using ADS1115 and BME280 hardware, maintains calibration/configuration through EEPROM, exposes data through BLE and Modbus RTU, and uploads telemetry through the EC200U LTE modem.

The Qt/QML application consumes the available telemetry and presents it through a structured industrial dashboard containing:

```text
Real-Time Water Level
        ↓
Water Height
        ↓
Percentage Level
        ↓
Tank Volume
        ↓
Status / Alerts
        ↓
Historical Trend
        ↓
Device Health
        ↓
System Monitoring
```

The architecture separates embedded acquisition, communication, cloud transport, C++ data processing, and QML presentation, making the system easier to maintain and extend.

## Repository

```text
Panorama Water Tank Monitor
│
├── Embedded Firmware
│   ├── Sensor acquisition
│   ├── Calibration
│   ├── FreeRTOS tasks
│   ├── BLE
│   ├── Modbus RTU
│   ├── LTE
│   └── Fault handling
│
├── Qt/C++ Backend
│   ├── API communication
│   ├── Data processing
│   ├── Repository
│   ├── Tank model
│   └── Connection management
│
└── Qt/QML Dashboard
    ├── Real-time monitoring
    ├── Tank visualization
    ├── Trend analysis
    ├── Alerts
    ├── Device information
    └── System status
```

**Panorama Water Tank Monitor — Embedded telemetry and industrial Qt/QML visualization platform.**
