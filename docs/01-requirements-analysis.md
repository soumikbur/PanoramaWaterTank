# Panorama Water Tank Monitor — Qt/QML Desktop Application
## Phase 1: Requirements Analysis

**Status:** Draft for review — no implementation until this phase is approved.
**Reference design:** Panorama dashboard screenshot (React web app, reused here as the visual baseline).
**Companion system:** A React/TypeScript web dashboard already exists for the same tank and the same Ubidots backend. This desktop application is a parallel, independent client — not a fork of it — sharing only the data source and visual language.

---

## 1. Application Objectives

| # | Objective |
|---|-----------|
| O1 | Provide a always-on, desktop SCADA-style monitor for a single cylindrical water tank instrumented with an ESP32-S3. |
| O2 | Replace "raw percentage from the API" with a **calculated** fill state derived from tank geometry, so the displayed numbers are engineering-correct regardless of what the sensor firmware sends. |
| O3 | Present the data with the visual clarity of industrial HMI software (WinCC / Ignition / FactoryTalk-class), not a consumer dashboard. |
| O4 | Stay connected to a live Ubidots endpoint continuously, tolerating network drops without losing the last-known-good reading. |
| O5 | Be architected so it can grow (multi-tank, history, alerts) without a rewrite. |

**Explicitly out of scope for v1** (candidates for Phase 12 — Future Expansion): historical charting, CSV/PDF export, multi-tank support, authentication/login, notifications, MQTT.

---

## 2. User Roles

The reference React app has Admin/Employee roles. For the **desktop** application, the initial requirement is a single-operator control-room context (one machine, one screen, physically access-controlled), so:

| Role | v1 Scope |
|---|---|
| **Operator** (default, only role in v1) | Full read access to all dashboard data. Can edit local settings (API endpoint, tank geometry, refresh interval) via the Settings page. |
| Admin / Employee split | **Deferred.** Flagged as a Phase 12 candidate if this desktop app later needs to run on shared/kiosk machines. Raised as an open question below. |

---

## 3. Functional Requirements

| ID | Requirement | Priority |
|---|---|---|
| FR-1 | Display current fill percentage, computed from geometry, not read verbatim from the API. | Must |
| FR-2 | Display current water height (m), current volume (L), remaining volume (L), and maximum capacity (L). | Must |
| FR-3 | Animate the tank visualization's water level to match the calculated height/percentage. | Must |
| FR-4 | Show a segmented capacity-rank bar (four bands) with a marker at the current percentage. | Must |
| FR-5 | Show tank metadata: name, device label, radius, height, cross-section area, connection state, last-updated time. | Must |
| FR-6 | Show temperature and sensor/system status, sourced from the API when available. | Must |
| FR-7 | Poll the Ubidots backend on a configurable interval (default within 2–5 s) without blocking the UI. | Must |
| FR-8 | Auto-detect whether the API is reporting **height** or **volume** and run the correct conversion. | Must |
| FR-9 | Persist configuration (API base URL, device label, token, tank radius/height, polling interval) across restarts. | Must |
| FR-10 | Sidebar navigation matching the reference design (Dashboard, Water Tank, Devices, Alerts, History, Settings). | Must |
| FR-11 | Dashboard is fully functional in v1. Other sidebar destinations may be placeholders, clearly marked, in v1. | Should |
| FR-12 | Show a distinct "Waiting for Live Data" state before the first successful API response. | Must |
| FR-13 | Show a distinct "Disconnected" state on failure, while continuing to display the last valid reading. | Must |
| FR-14 | Live clock and date in the header, independent of API connectivity. | Should |
| FR-15 | All numeric values rendered to 2 decimal places. | Must |

---

## 4. Non-Functional Requirements

| ID | Requirement |
|---|---|
| NFR-1 | **Separation of concerns:** QML contains no calculations beyond formatting (e.g. `toFixed(2)`); all math lives in C++. |
| NFR-2 | **Single source of truth:** tank geometry (radius, height) is held in exactly one place (`TankModel`) and every derived value is recomputed from it. |
| NFR-3 | **Testability:** the engineering-calculation layer must be a plain C++ class with no Qt event-loop or network dependency, so it can be unit-tested headlessly. |
| NFR-4 | **Resilience:** a network failure must never crash the app, freeze the UI, or reset displayed values to zero. |
| NFR-5 | **Portability:** builds on Windows and Linux with Qt 6.8+ and CMake, no platform-specific code paths in v1. |
| NFR-6 | **No hardcoded operational values** (URLs, tokens, tank dimensions) in QML or compiled into the binary — all externalized and user-editable. |
| NFR-7 | **Consistent design language** with the existing React dashboard (color palette, spacing, typography feel) without literally sharing code. |

---

## 5. Performance Goals

| Metric | Target |
|---|---|
| Cold start to first frame | < 2 s |
| Continuous uptime | 24/7, no restart required |
| Memory growth over 24 h | Effectively flat (no leaks from repeated polling/JSON parsing) |
| UI thread blocking per poll cycle | 0 ms (networking is fully async via `QNetworkAccessManager`) |
| Animation frame budget | 60 fps target for tank fill / marker transitions |
| CPU usage at idle (between polls) | Negligible — no busy-waiting, no per-frame polling of static data |

---

## 6. Engineering Requirements

The tank is cylindrical. The **only** trusted inputs are physical geometry (radius, height) plus one live measurement (height *or* volume) from the sensor. Everything else is derived:

- Cross-sectional area ← radius
- Maximum volume ← area × height
- Current height ← current volume ÷ area *(if API gives volume)*
- Current volume ← current height × area *(if API gives height)*
- Fill percentage ← current volume ÷ maximum volume
- Remaining volume ← maximum volume − current volume
- Status/rank ← thresholds applied to fill percentage, overridden by an explicit sensor-reported status if the API provides one

Precision, rounding, clamping, and invalid-value handling for these equations are detailed in **Phase 7 — Engineering Planning** (not yet written).

---

## 7. API Requirements

Based on the confirmed live setup:

- **Backend:** Ubidots (industrial API)
- **Hardware pipeline:** sensor → ESP32-S3 → EC200U-CN 4G module → Ubidots
- **Confirmed variable labels:** `waterlevel`, `sensorstatus` (temperature availability to be confirmed — see open questions)
- Requests authenticated via API token
- Configurable: base URL, device label, variable label(s), token, timeout, poll interval
- Must tolerate: timeout, malformed JSON, HTTP 401/403/404/500, no network, and a response that's missing an expected field

Full request/response contract, retry/backoff strategy, and offline-mode behavior are detailed in **Phase 8 — API Planning** (not yet written).

---

## 8. Error Handling Requirements

At minimum, the application must distinguish and visibly communicate:

| Condition | Required UI behavior |
|---|---|
| No successful response yet | "Waiting for Live Data" |
| Previously connected, now failing | "Disconnected," last good reading stays on screen |
| HTTP 401/403 | Treated as a configuration problem (bad token), not a transient outage |
| HTTP 404 | Device/variable not found — configuration problem |
| HTTP 500 / timeout / no network | Transient outage — auto-retry |
| Malformed JSON | Logged, treated as a failed poll, does not crash parsing |
| Missing expected field | Logged, treated as a failed poll |

Retry strategy (fixed interval vs. exponential backoff) is finalized in Phase 8.

---

## 9. Future Scalability Requirements

The architecture chosen in Phase 2 must not preclude, even if it doesn't build, the following later additions:

- Multiple tanks/devices in one dashboard
- Historical data view + CSV/PDF export
- Role-based access (Admin/Employee, mirroring the React app)
- Alert rules with notifications (desktop, email, SMS)
- MQTT as an alternative/parallel transport to REST polling
- Remote/cloud configuration sync

---

## 10. Open Questions (need your input before Phase 2)

1. **Temperature variable:** confirmed variable labels are `waterlevel` and `sensorstatus`. Is there a live `temperature` variable on the Ubidots device, or should temperature be treated as optional/omitted from v1 (similar to how the React app treats battery/signal/temperature as optional)?
2. **Roles:** confirm single-operator (no login) is correct for the desktop app, even though the React app has Admin/Employee.
3. **Tank dimensions:** the reference mockup uses radius 1.20 m / height 5.00 m — are these the *real* dimensions of the physical tank, or placeholder numbers I should treat as defaults only (user-editable in Settings)?
4. **Deployment target OS:** Windows, Linux, or both, for the first build?

If you'd like, answer inline and I'll fold the answers into Phase 1 before moving on — otherwise I'll proceed to **Phase 2: Software Architecture** using the defaults stated above.
