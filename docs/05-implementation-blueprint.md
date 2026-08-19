# Panorama Water Tank Monitor — Qt/QML Desktop Application
## Phase 5: Detailed Component & Class Specifications (Implementation Blueprint)

**Status:** Draft for review. Phase 4 (Engineering Model, Data Pipeline & Application Workflow) is approved. This is the final planning document — per the brief, planning stops here; what follows is the exact contract a developer implements against, with no architectural decisions left open during coding. Several genuine ambiguities left implicit in Phases 2–4 are resolved explicitly below (flagged inline as **Resolution:**).
**No code in this document.**

---

## 1. Backend Class Specifications

Each class below follows the same 13-point contract. Thread affinity is **main/UI thread for all ten classes** (Phase 2 §10) unless noted.

### DashboardController
| | |
|---|---|
| Purpose | Composition root — the one class allowed to know about every other backend class |
| Responsibilities | Construct and own ApiClient, ConnectionManager, TankRepository, ApplicationStateManager; own current sidebar page; expose the minimal action surface QML calls into |
| Lifetime / Ownership | Process lifetime; constructed in `main.cpp`, parented to the application object |
| Constructor Dependencies | None injected — it *is* the root that constructs its own children, in the order fixed in Section 13 |
| Public Interface | `Q_PROPERTY currentPage`; `Q_INVOKABLE navigateTo(page)`; `Q_INVOKABLE retryConnection()` |
| Internal Data | `m_currentPage`; owning references to its four children |
| Signals | `currentPageChanged` |
| Slots | none beyond property setters |
| Q_PROPERTY exposure | `currentPage` only — **Resolution:** it does *not* re-expose `connectionState` or tank data as convenience passthroughs; QML binds to `ConnectionManager`/`TankModel` singletons directly, keeping this class's surface deliberately narrow (Phase 2 §14 risk note) |
| Error Handling | None of its own — delegates to children; construction failure of a child is the only failure mode, and is fatal at startup (Section 13) |
| Logging | Application category — page navigation, `retryConnection()` invocations |
| Dependencies | Owns ApiClient, ConnectionManager, TankRepository, ApplicationStateManager |
| Future Extension Points | A `currentDeviceId` property, if/when multi-tank support (Phase 2 §14) is added |

### ApiClient
| | |
|---|---|
| Purpose | Sole owner of HTTP transport to Ubidots |
| Responsibilities | Phase 4 §6 polling lifecycle in full |
| Lifetime / Ownership | Process lifetime; owned by DashboardController |
| Constructor Dependencies | `ConnectionManager&`, `Logger&` (both required at construction — **Resolution:** unlike Phase 2's looser description, ApiClient cannot be default-constructed; it always needs somewhere to report outcomes) |
| Public Interface | Setters/getters for `baseUrl`, `deviceLabel`, `token`, `levelVariable`, `volumeVariable`, `distanceVariable`, `temperatureVariable`, `statusVariable`, `readingType`, `sensorMountingOffset`, `timeoutMs`, `refreshIntervalMs`; `start()` / `stop()` / `pollNow()` |
| Internal Data | `QNetworkAccessManager`; poll `QTimer`; per-request timeout `QTimer`; single in-flight `QNetworkReply*` guard |
| Signals | `rawReadingReceived(RawReading)` (Phase 4 §12); `errorOccurred(category, message)` |
| Slots | none required — configuration setters trigger reconfiguration directly |
| Q_PROPERTY exposure | **None.** ApiClient is never QML-facing (Phase 4 §7's hard rule) |
| Error Handling | Never throws; every failure path ends in `errorOccurred` + a `ConnectionManager::reportFailure(category)` call |
| Logging | API category (every request + status); Network category (timeouts, host failures) |
| Dependencies | `ConnectionManager` (reports to, and queries `nextPollDelay()` from — **Resolution:** the backoff *policy* lives entirely in ConnectionManager; ApiClient's timer just asks "how long until the next try" after every result, rather than computing backoff itself, closing the policy/mechanism overlap left implicit in Phase 2 §3); `Logger` |
| Future Extension Points | A future `MqttClient` implements the same `rawReadingReceived` contract as a sibling, not a subclass — no inheritance hierarchy is anticipated |

**Resolution — request queue:** there is no queue. If the poll timer fires while a request is still in flight, that tick is skipped entirely (logged at Trace level) rather than queued for later. `retryConnection()` (manual refresh) respects the same single-in-flight guard — it triggers an immediate `pollNow()` only if idle.

### TankRepository
| | |
|---|---|
| Purpose | Validation + caching seam between transport and business logic |
| Responsibilities | Phase 4 §5 validation bands; last-known-good caching; rejected/accepted counters feeding Sensor Error (Phase 4 §10) |
| Lifetime / Ownership | Process lifetime; owned by DashboardController |
| Constructor Dependencies | `SettingsManager&`, `Logger&` |
| Public Interface | `currentReading()`, `lastGoodReading()`; slot `onRawReadingReceived(RawReading)` |
| Internal Data | `m_lastGoodReading` (+ timestamp); `m_consecutiveInvalidCount`; `m_consecutiveValidCount`; a locally cached `area`/`maximumVolume` (see resolution below) |
| Signals | `validatedReadingChanged(ValidatedReading)`; `readingRejected(reason)` |
| Slots | `onRawReadingReceived(RawReading)`; `onGeometryChanged()` |
| Q_PROPERTY exposure | None — not QML-facing |
| Error Handling | Never throws; rejections are always logged and signaled, never silently dropped |
| Logging | Calculation category (Phase 2 §9) for rejections; escalates to Error category only if Sensor Error triggers |
| Dependencies | `SettingsManager` (read-only, for current radius/height); `VolumeCalculator` (direct pure-function calls — see resolution); `Logger` |
| Future Extension Points | The exact seam a future `HistoryManager` subscribes to, and where `MqttClient` output would be redirected instead of `ApiClient`'s (Phase 4 §15) |

**Resolution — how Repository validates against tank geometry without depending on TankModel:** Phase 2 §11 forbids `TankRepository` from depending on `TankModel` (would create a cycle, since `TankModel` already depends on `TankRepository`). But Phase 4 §5's validation bands (`0 ≤ h ≤ tankHeight × 1.10`) require knowing tank height. Resolution: `TankRepository` listens to `SettingsManager` directly for `tankRadius`/`tankHeight` and calls `VolumeCalculator`'s pure functions itself to derive its own `area`/`maximumVolume` for bound-checking purposes only. This is a small, harmless duplication of a cheap calculation — `VolumeCalculator` has zero dependencies, so *anyone* may call it — and it preserves the one-directional dependency graph from Phase 2 §11 without compromise.

### TankModel
Class-level contract only — the full property catalog is Section 2.

| | |
|---|---|
| Purpose | QML-facing single source of truth for tank geometry and current derived state |
| Lifetime / Ownership | QML singleton (`QML_ELEMENT` + `QML_SINGLETON`) |
| Constructor Dependencies | None (QML singletons are constructed with no arguments by the engine) — receives initial geometry via setter calls from `main.cpp` (Phase 4 §1) |
| Public Interface | The full `Q_PROPERTY` set, Section 2 |
| Internal Data | Raw/unclamped fill percentage retained separately from the clamped display value (Phase 4 §4) |
| Signals | One `*Changed` signal per property |
| Slots | `applyReading(ValidatedReading)`; `setConnectionState(state)`; geometry setters double as slots connected to `SettingsManager`'s change signals |
| Error Handling | Does not itself validate (already done upstream) but defensively guards divide-by-zero (`maximumVolume == 0`) before computing percentage |
| Logging | Calculation category, only for defensive clamps it has to apply |
| Dependencies | Calls `VolumeCalculator`; listens to `TankRepository` and `SettingsManager` |
| Future Extension Points | Multi-tank support replaces this singleton with a keyed collection (Phase 2 §14) |

### VolumeCalculator
Class-level contract only — the full function catalog is Section 5.

| | |
|---|---|
| Purpose | Pure, stateless cylindrical-tank math |
| Lifetime / Ownership | Not instantiated — free functions |
| Constructor Dependencies | None |
| Public Interface | Section 5 |
| Thread Affinity | **None required** — safe to call from any thread (no shared state); v1 only ever calls it from the main thread |
| Error Handling | Never throws; guards divide-by-zero, NaN, and Infinity as a defensive last line (primary validation is Phase 4 §5, upstream) |
| Logging | None — a zero-dependency function has no logging dependency, by design |
| Dependencies | None |
| Future Extension Points | Non-cylindrical geometries add sibling functions (`calculateRectangularVolume`, etc.) — existing functions are never modified for this |

### SettingsManager
| | |
|---|---|
| Purpose | Single owner of persisted configuration |
| Responsibilities | Load/save via `QSettings`; validate every setter; provide defaults; notify on runtime change |
| Lifetime / Ownership | Process lifetime; owned by DashboardController |
| Constructor Dependencies | `Logger&` |
| Public Interface | Full property set, Section 6 |
| Q_PROPERTY exposure | Every setting in Section 6, all live-updating (no restart required, by design — Section 6) |
| Error Handling | Setter rejects an invalid value and retains the prior valid one, returning/signaling failure rather than silently no-op'ing |
| Logging | Application category, for every accepted change; Error category for a rejected write |
| Dependencies | `Logger` only |
| Future Extension Points | A remote-config sync client (Phase 2 §14) would read/write through this class, never around it |

### ConnectionManager
| | |
|---|---|
| Purpose | Sole authority on connection health and retry policy |
| Responsibilities | Track consecutive failures; own backoff math; expose `ConnectionState` |
| Lifetime / Ownership | Process lifetime; owned by DashboardController |
| Constructor Dependencies | `Logger&` |
| Public Interface | `Q_PROPERTY connectionState`; slots `reportSuccess()` / `reportFailure(category)`; `nextPollDelay()` query; `resetBackoff()` |
| Internal Data | Consecutive failure count; current backoff multiplier |
| Signals | `connectionStateChanged` |
| Q_PROPERTY exposure | `connectionState` — QML may bind to this singleton-adjacent instance directly (exposed via DashboardController or as its own QML-registered type, implementation's choice) |
| Error Handling | N/A — this class *is* the error-tracking mechanism for everything else |
| Logging | Network category |
| Dependencies | `Logger` only |
| Future Extension Points | Backoff *algorithm* (Strategy pattern, Phase 2 §13) can change independently — fixed → exponential → jittered — without touching `ApiClient` |

### Logger
| | |
|---|---|
| Purpose | Centralized, categorized logging |
| Responsibilities | Section 7 in full |
| Lifetime / Ownership | Initialized first, before any other backend class; effectively process-wide |
| Constructor Dependencies | None |
| Public Interface | `log(category, level, message)`; convenience wrappers `logApi`, `logNetwork`, `logCalculation`, `logApplication`, `logError`; `Q_PROPERTY currentLogLevel` (read/write, so a future Settings page can adjust verbosity live) |
| Thread Safety | Mutex-guarded writer — safe to call from any thread even though v1 only calls from the main thread (defensive future-proofing per Phase 4 §14) |
| Error Handling | A logging failure (e.g. disk full) must never crash the app — falls back to console-only silently |
| Dependencies | None — a leaf utility |
| Future Extension Points | A remote log sink subscribing to the same `log()` entry point, additive |

### TimeManager
| | |
|---|---|
| Purpose | Single source of "now" |
| Responsibilities | Tick a 1-second timer; format current time/date; convert a timestamp to a relative string |
| Lifetime / Ownership | QML singleton, process lifetime |
| Constructor Dependencies | None |
| Public Interface | `Q_PROPERTY currentTime`, `Q_PROPERTY currentDate`; `Q_INVOKABLE relativeTime(timestamp)` |
| Internal Data | 1-second `QTimer` |
| Dependencies | None |
| Future Extension Points | None anticipated — this class is intentionally minimal and complete |

### ApplicationStateManager
| | |
|---|---|
| Purpose | Formal application-lifecycle state machine (Phase 2 §6) |
| Responsibilities | Own the enumerated states and the valid-transition table; reject and log any attempted illegal transition rather than silently allowing it |
| Lifetime / Ownership | Process lifetime; owned by DashboardController |
| Constructor Dependencies | `ConnectionManager&`, `TankRepository&`, `Logger&` — **not** `TankModel` or `ApiClient` directly (Phase 2 §11's dependency graph) |
| Public Interface | `Q_PROPERTY currentState`; signal `stateChanged(from, to)` |
| Dependencies | `ConnectionManager`, `TankRepository` (subscribes to both) |
| Future Extension Points | None anticipated at v1 scope |

---

## 2. TankModel Specification — Complete Property Catalog

**Resolution — write access:** every property below is technically settable at the C++ level (needed for `SettingsManager`/`TankRepository` to push values in), but by convention is **read-only from QML**. A future Settings page writes through `SettingsManager`, never directly through `TankModel` — this keeps exactly one write path into tank state, consistent with Phase 4 §7's ownership table.

### Geometry

| Name | Type | Units | Source | RO/RW (QML) | Default | Validation | Notification | QML Usage |
|---|---|---|---|---|---|---|---|---|
| `tankName` | string | — | SettingsManager | RO | `"Panorama Water Tank"` | non-empty | `tankNameChanged` | InformationCard |
| `deviceLabel` | string | — | SettingsManager | RO | `"esp32s3"` | non-empty | `deviceLabelChanged` | InformationCard |
| `tankRadius` | real | m | SettingsManager | RO | `1.20` | `>0, ≤50` | `tankRadiusChanged` | InformationCard; drives Engineering group |
| `tankHeight` | real | m | SettingsManager | RO | `5.00` | `>0, ≤50` | `tankHeightChanged` | InformationCard; drives Engineering group |

### Engineering

| Name | Type | Units | Source | RO/RW | Default | Validation | Notification | QML Usage |
|---|---|---|---|---|---|---|---|---|
| `tankArea` | real | m² | Derived | RO | `0` | derived | `tankAreaChanged` | InformationCard, StatisticCard 3 |
| `maximumVolume` | real | L | Derived | RO | `0` | derived | `maximumVolumeChanged` | InformationCard, StatisticCard 3 |
| `currentHeight` | real | m | Derived | RO | `0` | `0 ≤ h ≤ tankHeight×1.10` (pre-validated upstream) | `currentHeightChanged` | TankStatistics, InformationCard |
| `currentVolume` | real | L | Derived | RO | `0` | `0 ≤ v ≤ maxVolume×1.10` | `currentVolumeChanged` | TankStatistics, StatisticCard 2 |
| `fillPercentage` | real | % | Derived, **clamped** [0,100] | RO | `0` | clamped | `fillPercentageChanged` | WaterFill height, TankStatistics, RankIndicator marker, StatisticCard 1 |
| `fillPercentageRaw` | real | % | Derived, **unclamped** | RO | `0` | unclamped, internal only | `fillPercentageRawChanged` | **Not bound in any QML component** — internal input to `alarmLevel` only (documented here specifically so no future contributor accidentally binds a >100 value to a percentage-shaped widget) |
| `emptyPercentage` | real | % | `100 − fillPercentage` | RO | `100` | derived | `emptyPercentageChanged` | Reserved — not used in v1 layout |
| `remainingVolume` | real | L | Derived | RO | `0` | `≥0` | `remainingVolumeChanged` | StatisticCard 2 subtitle |
| `capacityRank` | string enum: `Low`/`Moderate`/`Good`/`High` | — | Derived from clamped `fillPercentage` | RO | `"Low"` | derived | `capacityRankChanged` | RankIndicator active segment |

### Status & Alarm

| Name | Type | Units | Source | RO/RW | Default | Validation | Notification | QML Usage |
|---|---|---|---|---|---|---|---|---|
| `alarmLevel` | string enum: `Normal`/`Low`/`Critical`/`Overflow`/`SensorError` | — | Derived (Phase 4 §10) | RO | `""` (unassessed — see resolution below) | derived | `alarmLevelChanged` | StatusBadge (subject to the priority rule below) |
| `sensorStatus` | string | — | API passthrough (optional) | RO | `""` | informational only | `sensorStatusChanged` | InformationCard optional row |

**Resolution — StatusBadge's actual displayed value:** `StatusBadge` does **not** bind solely to `TankModel.alarmLevel`. It resolves via a priority rule between `ApplicationStateManager.currentState` and `TankModel.alarmLevel`: if the app state is anything other than `Connected` (`Connecting`/`WaitingForData`/`Offline`/`Reconnecting`/`AuthenticationError`/`ConfigurationError`), that state's label/color/icon wins. Once `Connected`, `alarmLevel` takes over. This formalizes Phase 4 §10's "Offline outranks everything" rule down to exactly which property wins, and explains why `alarmLevel` defaults to an empty/unassessed value rather than `"Normal"` — that default is never actually shown, because `ApplicationStateManager` is still in `WaitingForData` at that point.

### API-Sourced

| Name | Type | Units | Source | RO/RW | Default | Validation | Notification | QML Usage |
|---|---|---|---|---|---|---|---|---|
| `temperature` | real | °C | API (optional) | RO | `0.0` | `−10 ≤ t ≤ 80` | `temperatureChanged` | StatisticCard 4, InformationCard |
| `hasTemperature` | bool | — | Derived (has a value ever been received) | RO | `false` | — | `hasTemperatureChanged` | Guards whether the temperature row renders a value or an em-dash (Phase 1's "optional device variables handled gracefully" requirement, resolved precisely) |

### Connection

| Name | Type | Units | Source | RO/RW | Default | Validation | Notification | QML Usage |
|---|---|---|---|---|---|---|---|---|
| `connectionState` | string enum: `Connecting`/`Connected`/`Reconnecting`/`Offline` | — | Set by DashboardController wiring from ConnectionManager | RO | `"Connecting"` | — | `connectionStateChanged` | Header badge; StatusBadge priority input |

### Display / Time

| Name | Type | Units | Source | RO/RW | Default | Validation | Notification | QML Usage |
|---|---|---|---|---|---|---|---|---|
| `lastUpdated` | string (formatted `hh:mm:ss`) | — | Derived via TimeManager from `receivedAt` | RO | `"--"` | — | `lastUpdatedChanged` | InformationCard, TankCard footer |
| `lastUpdatedTimestamp` | datetime (raw) | — | Derived | RO | invalid/null | — | `lastUpdatedTimestampChanged` | Internal — feeds TimeManager's independently-ticking relative-time recalculation; not bound directly by most components |

---

## 3. ApiClient Specification

| Aspect | Specification |
|---|---|
| **Configuration** | All fields in Phase 4 §3's addendum plus Phase 2 §8's original list — 14 total settings (Section 6 below), all mutable at runtime |
| **HTTP lifecycle** | Build request → set headers → send → await response or timeout → decode → emit outcome. No manual redirect handling (Qt's default redirect policy applies; Ubidots does not redirect this endpoint) |
| **Authentication** | `X-Auth-Token` header, set on every request when `token` is non-empty; omitted entirely (not sent empty) when unset, so a missing-token 401 is unambiguous in logs |
| **Polling** | Single recurring `QTimer`; interval sourced from `ConnectionManager::nextPollDelay()` after every result (success → base interval; failure → backoff-adjusted) |
| **Retry** | Delegated entirely to `ConnectionManager` (Section 1) — ApiClient has no retry logic of its own beyond re-arming its timer with the delay it's given |
| **Timeout** | Separate per-request `QTimer`, default 5000 ms, aborts the in-flight `QNetworkReply` on expiry |
| **Parsing** | Extracts exactly one numeric field per the configured `readingType` (`levelVariable`, `volumeVariable`, or `distanceVariable`), plus optional `temperatureVariable`/`statusVariable` — unrecognized extra fields in the JSON body are ignored, not treated as errors |
| **Request queue** | None (Section 1's resolution) — overlapping ticks are skipped, not queued |
| **Response handling** | Success → `rawReadingReceived` + `ConnectionManager::reportSuccess()`; any failure → `errorOccurred` + `ConnectionManager::reportFailure(category)`, never both |
| **State transitions** | ApiClient does not hold connection state itself — it only ever *reports into* ConnectionManager, which owns the actual state (Phase 2 §3) |
| **Connection lifecycle** | `start()` → immediate `pollNow()` then timer-driven; `stop()` → timer stopped, in-flight request aborted cleanly, no further signals emitted after `stop()` returns |
| **Logging** | API category for every attempt; Network category for transport-level failures specifically |
| **Configuration updates** | Any setter call takes effect on the **next** scheduled poll (Phase 4 §6) — never interrupts an in-flight request |
| **Manual refresh** | `pollNow()` called directly bypasses the timer/backoff schedule but still respects the single-in-flight guard |

---

## 4. Repository Specification

| Aspect | Specification |
|---|---|
| **Validation** | Phase 4 §5's bands, evaluated using Repository's own geometry cache (Section 1's resolution) |
| **Caching** | `lastGoodReading` retained indefinitely (never expires on its own — staleness is ConnectionManager's concern, not Repository's) |
| **Last-known-good reading** | Overwritten **only** by a reading that passes validation — a rejected reading never touches this cache |
| **Rejected readings** | Logged (Calculation category), increment `m_consecutiveInvalidCount`; at count `3`, `TankModel.alarmLevel` becomes `SensorError` (Phase 4 §10) |
| **Accepted readings** | Reset `m_consecutiveInvalidCount` to `0`; increment `m_consecutiveValidCount` — at count `3` *while currently in* `SensorError`, the alarm clears (asymmetric 3-in/3-out, avoiding single-sample flapping) |
| **Data ownership** | Repository owns *validity*; TankModel owns *interpretation*. Repository never computes a percentage or an alarm level — only "is this number physically plausible" |
| **Repository events** | `validatedReadingChanged` (accepted) and `readingRejected(reason)` (rejected) — both always fire exactly once per poll outcome, so downstream consumers (including a future `HistoryManager`) can distinguish "no update happened" from "an update was rejected" |
| **Mock repository strategy** | Tests substitute a fake raw-reading source feeding the *real* `TankRepository` (validation-focused tests), or substitute `TankRepository` itself when testing `TankModel` in isolation (Section 17) |
| **Offline behavior** | Repository takes no explicit action when the connection drops — it simply stops receiving new raw readings, and `lastGoodReading` ages in place. Offline detection and its UI consequences belong entirely to `ConnectionManager`/`ApplicationStateManager` |

---

## 5. Engineering Layer Specification

Every function is pure (no state, no side effects), operates on IEEE-754 doubles, and guards `NaN`/`Infinity` via `std::isfinite` at entry — any non-finite input is treated identically to an invalid input and yields `0`.

| Function | Equation | Guards |
|---|---|---|
| `calculateArea(radius)` | `π × radius²` | `radius ≤ 0` → `0` |
| `calculateVolume(area, height)` | `area × height × 1000` | either input `≤ 0` → `0` |
| `calculateHeight(volume, area)` | `(volume ÷ 1000) ÷ area` | `area ≤ 0` → `0`; `volume < 0` → treated as `0` |
| `calculateFillPercentageRaw(currentVolume, maximumVolume)` | `(currentVolume ÷ maximumVolume) × 100` | `maximumVolume ≤ 0` → `0`; **unclamped** — may legitimately return `>100` |
| `clampPercentageForDisplay(rawPercentage)` | `std::clamp(rawPercentage, 0, 100)` | — |
| `calculateRemainingVolume(maximumVolume, currentVolume)` | `max(0, maximumVolume − currentVolume)` | — |

**Overflow handling:** realistic tank volumes (single/double-digit thousands of liters) are nowhere near a double's ~1.8×10³⁰⁸ ceiling — numeric overflow is not a practical concern at this application's scale, noted explicitly so it's clear this was considered rather than overlooked.

**Negative values / NaN / Infinity:** guarded identically at every function boundary as a defensive last line — Phase 4 §5 is the primary gate; these guards exist so a bug elsewhere in the pipeline degrades to "shows zero" rather than "shows garbage" or crashes.

**Unit conversions:** the single m³↔L conversion (`× 1000`) exists in exactly **one** place (`calculateVolume`) — deliberately not duplicated anywhere else in the codebase, since a scattered conversion constant is a well-known bug magnet.

**Future tank geometries:** a non-cylindrical shape adds sibling functions (`calculateRectangularVolume(length, width, height)`, etc.) following the same zero-dependency, zero-state pattern — existing cylinder functions are never modified to accommodate this (open/closed principle, restated from Phase 2 §13's Factory-pattern deferral).

---

## 6. Settings Specification

All settings are `QSettings`-backed (Phase 2 §8), live-updating, and require **no application restart** — this is a deliberate architectural property, not an accident, restated here because it's the single most implementation-relevant fact about this class.

| Group | Name | Type | Default | Allowed Range | Validation | Restart Required | Live Update | Dependencies |
|---|---|---|---|---|---|---|---|---|
| API | `apiBaseUrl` | string | `https://industrial.api.ubidots.com` | — | well-formed URL, non-empty | No | Yes | — |
| API | `apiToken` | string | *(empty)* | — | non-empty before polling starts | No | Yes | — |
| API | `deviceLabel` | string | `esp32s3` | — | non-empty | No | Yes | — |
| API | `levelVariable` | string | `waterlevel` | — | non-empty | No | Yes | Used only if `readingType = Height` |
| API | `volumeVariable` | string | `volume` | — | may be empty | No | Yes | Used only if `readingType = Volume` |
| API | `distanceVariable` | string | `distance` | — | may be empty | No | Yes | Used only if `readingType = Distance` |
| API | `temperatureVariable` | string | `temperature` | — | may be empty (optional field) | No | Yes | — |
| API | `statusVariable` | string | `sensorstatus` | — | may be empty | No | Yes | — |
| API | `readingType` | enum | `Height` | `Height`/`Volume`/`Distance` | must be one of the three | No | Yes | Determines which of the three variable-label fields above is active |
| API | `sensorMountingOffset` | real (m) | `0` | `≥0` | numeric | No | Yes | Only meaningful when `readingType = Distance` |
| API | `refreshIntervalMs` | int | `3000` | `1000–60000` | numeric range | No | Yes | — |
| API | `timeoutMs` | int | `5000` | `500–30000` | numeric range | No | Yes | — |
| Tank | `tankName` | string | `Panorama Water Tank` | — | non-empty | No | Yes | — |
| Tank | `tankRadius` | real (m) | `1.20` | `>0, ≤50` | numeric range | No | Yes | Triggers full TankModel recalculation (Phase 4 §11) |
| Tank | `tankHeight` | real (m) | `5.00` | `>0, ≤50` | numeric range | No | Yes | Triggers full TankModel recalculation |
| Logging | `logLevel` | enum | `Info` | `Trace`/`Debug`/`Info`/`Warning`/`Error`/`Critical` | must be a valid level | No | Yes | — |
| Logging | `logRetentionDays` | int | `14` | `1–90` | numeric range | No | Yes | — |
| Theme | `theme` | enum | `Light` | `Light`/`Dark` | must be valid | No | Yes | `Dark` is accepted and persisted but has no visual effect until a future theme is implemented (Phase 3 §11) — **not** rejected, simply inert, so a setting made today isn't lost when the feature ships |

**No settings require a restart** — every consumer (`ApiClient`, `TankModel`, `Logger`) listens for its relevant `*Changed` signal and reconfigures live. This was an explicit design goal carried through since Phase 2 §8 and is confirmed achievable at this level of detail.

---

## 7. Logger Specification

| Aspect | Specification |
|---|---|
| **Categories** | `Application`, `API`, `Network`, `Calculation`, `Error` (Phase 2 §9) |
| **Levels** | `Trace < Debug < Info < Warning < Error < Critical` |
| **File structure** | One file per day: `panorama-YYYY-MM-DD.log`, under the platform's standard app-data logging directory |
| **Rotation** | Daily; retains `logRetentionDays` (default 14, Section 6) most recent files; oldest pruned on startup |
| **Formatting** | `[ISO8601 timestamp+offset] [LEVEL] [CATEGORY] message` — one line per entry, no multi-line entries (a message containing newlines is escaped, not split) |
| **Performance** | Level check occurs **before** message construction — a suppressed Trace/Debug call costs a single integer comparison, never builds the string |
| **Thread safety** | Mutex-guarded writer; safe from any thread (defensive — v1 only calls from main thread, Section 1) |
| **Debug builds** | Console mirror at `Trace` and above, in addition to the file |
| **Release builds** | File only, default threshold `Info`; `logLevel` (Section 6) is adjustable at runtime without rebuilding, for field diagnostics |
| **Future remote logging** | A network log sink subscribing to the same `log()` entry point — additive, not a redesign |

---

## 8. QML Component Specifications

Extends Phase 3 §10. Animation and accessibility details are **not repeated per component** here — they follow the `AppAnimations`/accessibility tokens defined in Phase 3 §7/§13 uniformly, except where a component introduces a state Phase 3 didn't already cover (noted below).

| Component | Purpose | Key bound properties (source) | Signals | Visual states | Sizing (Phase 3 §4) |
|---|---|---|---|---|---|
| `Header` | Top bar | `TankModel.connectionState`, `TimeManager.currentTime`/`currentDate` | — | connected / disconnected badge | height 76 |
| `Sidebar` | Navigation | `DashboardController.currentPage` | `pageSelected(page)` | active / inactive per item | width 260 |
| `NavigationItem` | One nav row | `active` (from Sidebar) | `clicked()` | default / hover / active | — |
| `DashboardPage` | Assembles the dashboard | — | — | — | fills content area |
| `TankCard` | Card shell | `TankModel.fillPercentage`, `.currentHeight`, `.alarmLevel`/`.connectionState` (via StatusBadge's priority rule, Section 2) | — | Normal / Low / Critical / Overflow border tint (Phase 3 §8) | height 320 |
| `TankView` | Vessel outline | geometry constants only | — | static | — |
| `TankScale` | Tick marks | static (`divisions=4`) | — | static | — |
| `WaterFill` | Fill rectangle | `TankModel.fillPercentage` | — | animating / settled | Behavior: 600ms InOutQuad |
| `WaterSurface` | Shimmer | bound to `WaterFill.height` | — | continuous loop while `fillPercentage > 0` | 2200ms InOutSine |
| `TankStatistics` | Percentage + labels | `TankModel.fillPercentage`, `.currentHeight` | — | — | — |
| `StatusBadge` | Status pill | Priority-resolved value (Section 2) | — | 9 states total (6 alarm + 3 connection-only: Connecting/WaitingForData/Offline) | — |
| `InformationCard` | Metadata table | `TankModel.*` (geometry, engineering, connection, display groups) | — | — | height 320 |
| `InfoRow` | One row | passed-in `icon`/`label`/`value` | — | — | — |
| `RankIndicator` | Capacity bar | `TankModel.fillPercentage`, `.capacityRank` | — | 4 static segments + moving marker | height 160; 500ms InOutQuad |
| `RankSegment` | One band | static per segment | — | — | — |
| `StatisticCard` | Metric tile | one `TankModel.*` value each, ×4 instances | — | value-updated fade (250ms) | height 100 |
| `Footer` | Caption bar | static text | — | — | height 40 |
| `LoadingOverlay` | Initial load | `ApplicationStateManager.currentState == Starting/LoadingConfiguration` | — | full-screen, blocks dashboard | — |
| `OfflineOverlay` | Staleness banner | `TankModel.connectionState == Offline`, `TankModel.lastUpdated` | — | slim banner, non-blocking (Phase 3 §14) | — |
| `ErrorDialog` | Blocking modal | `ApplicationStateManager.currentState == AuthenticationError/ConfigurationError` | `actionTriggered()` | blocking, dims dashboard behind it without hiding last-known values | — |

---

## 9. Component Communication Matrix

| Component | Reads | Writes | Emits | Depends On |
|---|---|---|---|---|
| `Header` | `TankModel.connectionState`, `TimeManager.*` | — | — | `TankModel`, `TimeManager` |
| `Sidebar` | `DashboardController.currentPage` | — | `pageSelected(page)` | `DashboardController`, `NavigationItem` |
| `NavigationItem` | `active` (parent-passed) | — | `clicked()` | — |
| `TankCard` | `TankModel.*` (tank subset) | — | — | `TankView`, `TankScale`, `WaterFill`, `WaterSurface`, `TankStatistics`, `StatusBadge` |
| `WaterFill` | `TankModel.fillPercentage` | — | — | — |
| `RankIndicator` | `TankModel.fillPercentage`, `.capacityRank` | — | — | `RankSegment` |
| `InformationCard` | `TankModel.*` (metadata subset) | — | — | `InfoRow` |
| `StatisticCard` ×4 | one `TankModel.*` value each | — | — | — |
| `StatusBadge` | `TankModel.alarmLevel`, `ApplicationStateManager.currentState` | — | — | Priority rule, Section 2 |
| `OfflineOverlay` | `TankModel.connectionState`, `.lastUpdated` | — | — | — |
| `ErrorDialog` | `ApplicationStateManager.currentState` | — | `actionTriggered()` | `DashboardController.navigateTo("Settings")` (on trigger) |

No component in this table ever **writes** to `TankModel` or any backend class — confirming Phase 4 §7's boundary holds at the component level, not just in principle.

---

## 10. Property Binding Matrix

Covers the meaningfully data-bound properties (not every literal QML property — static layout constants are omitted as not meaningful here).

| Bound property | Source | Update trigger | Animation | Formatting | Fallback |
|---|---|---|---|---|---|
| `WaterFill.height` | `TankModel.fillPercentage` | property change | 600ms InOutQuad | — | `0` height pre-first-reading |
| `RankIndicator` marker `x` | `TankModel.fillPercentage` | property change | 500ms InOutQuad | — | leftmost position |
| `TankStatistics` percentage text | `TankModel.fillPercentage` | property change | none (instant) | `toFixed(2) + "%"` | `"--"` before first reading (Phase 3 §14) |
| `StatusBadge` color/icon/label | Priority-resolved (Section 2) | either input changes | 250ms ColorAnimation (color only) | — | `WaitingForData` styling |
| `StatisticCard 1` value | `TankModel.fillPercentage` | property change | 250ms opacity fade | `toFixed(2) + "%"` | `"--"` |
| `StatisticCard 2` value/subtitle | `TankModel.currentVolume` / `.remainingVolume` | property change | 250ms opacity fade | `toFixed(2) + " L"` | `"--"` |
| `StatisticCard 3` value/subtitle | `TankModel.maximumVolume` / `.tankArea` | property change | 250ms opacity fade | `toFixed(2)` + unit | `"--"` |
| `StatisticCard 4` value | `TankModel.temperature` (guarded by `.hasTemperature`) | property change | 250ms opacity fade | `toFixed(2) + " °C"` | `"—"` if `hasTemperature == false` |
| `InfoRow` values (×11) | corresponding `TankModel.*` | property change | none | per-field (Section 2's units) | `"--"` or `"—"` per field semantics |
| `Header` connection badge | `TankModel.connectionState` | property change | 150ms ColorAnimation | — | `Connecting` styling |
| `Header` clock/date | `TimeManager.currentTime`/`currentDate` | 1s timer tick | none | `hh:mm:ss` / `dd MMMM yyyy` | — |

---

## 11. Signal & Slot Matrix

| Signal | Emitter | Receiver(s) | Effect |
|---|---|---|---|
| `rawReadingReceived(RawReading)` | ApiClient | TankRepository | Validation begins (Section 4) |
| `errorOccurred(category, message)` | ApiClient | ConnectionManager (`reportFailure`), Logger | Backoff scheduled; failure logged |
| `validatedReadingChanged(ValidatedReading)` | TankRepository | TankModel | Full recalculation (Phase 4 §4) |
| `readingRejected(reason)` | TankRepository | Logger; ApplicationStateManager (invalid-count tracking, indirectly via TankModel's alarm state) | Logged; may contribute to `SensorError` |
| `connectionStateChanged` | ConnectionManager | ApplicationStateManager, TankModel (`setConnectionState` slot) | UI badge updates (Header, StatusBadge) |
| `tankRadiusChanged` / `tankHeightChanged` | SettingsManager | TankModel | Full geometry recalculation (Phase 4 §11 sequence) |
| `apiBaseUrlChanged` / `apiTokenChanged` / etc. | SettingsManager | ApiClient | Reconfiguration, effective next poll |
| `stateChanged(from, to)` | ApplicationStateManager | QML root (LoadingOverlay/ErrorDialog visibility) | Full-screen state components show/hide |
| `pageSelected(page)` | Sidebar | DashboardController | `currentPage` updates, `DashboardPage` swaps visible content |
| `actionTriggered()` | ErrorDialog | DashboardController | Navigates to Settings |
| every `TankModel.*Changed` | TankModel | QML binding engine (automatic, not a manual slot) | Bound components re-render (Section 10) |

---

## 12. Memory Ownership

| Concern | Rule |
|---|---|
| **QObject ownership** | Backend classes (ApiClient, ConnectionManager, TankRepository, ApplicationStateManager) are constructed with `DashboardController` as their `QObject` parent — Qt's parent-child mechanism cascades destruction automatically, no manual `delete` anywhere |
| **Singleton ownership** | `TankModel` and `TimeManager` are owned by the `QQmlEngine` (QML_SINGLETON semantics) — destroyed when the engine is destroyed, at shutdown |
| **QML ownership** | The visual QML object tree is owned entirely by the QML engine, standard Qt/QML behavior — no C++ class ever manually parents a QML-declared item |
| **Controller ownership** | `DashboardController` itself is owned by `main.cpp`'s top-level object (parented to `QGuiApplication` or held on the stack for the duration of `app.exec()`) |
| **Application lifetime** | No backend object is dynamically created or destroyed during normal operation in v1 — everything constructed at startup lives until shutdown. This is a deliberate v1 simplicity choice, revisited only if a dynamic multi-tank device list is added (Phase 2 §14) |
| **Object destruction order** | `QQmlApplicationEngine` is declared **after** `QGuiApplication` in `main.cpp` so its destructor runs first, per standard Qt idiom — this ensures QML-owned objects (including the `TankModel`/`TimeManager` singletons) are torn down before the application object itself |
| **Shutdown sequence** | `ApplicationStateManager` transitions to `ShuttingDown` on window-close request, which **explicitly** (not just via destructor ordering) calls `ApiClient::stop()` and flushes `Logger`'s buffered writes *before* the `QObject` tree teardown begins — explicit sequencing here, not left to implicit destructor order, since log flush timing during arbitrary teardown isn't guaranteed |

---

## 13. Initialization Order

```text
1. Logger                          — first; nothing else can log otherwise
2. SettingsManager                 — needs Logger for its own diagnostics
3. DashboardController constructed, which in turn constructs (in order):
     a. ConnectionManager          — no dependencies beyond Logger
     b. TankRepository             — needs SettingsManager (geometry bound-checks)
     c. ApiClient                  — needs ConnectionManager + SettingsManager (initial config)
     d. ApplicationStateManager    — needs ConnectionManager + TankRepository (subscribes to both)
4. main.cpp resolves TankModel and TimeManager QML singletons EAGERLY
     (not left to lazy QML-triggered construction, for deterministic startup)
   → applies SettingsManager's geometry/name/deviceLabel to TankModel
5. main.cpp wires the full signal/slot graph (Phase 4 §1's diagram) —
     ApiClient ↔ TankRepository ↔ TankModel ↔ ConnectionManager ↔ ApplicationStateManager
6. engine.loadFromModule(...) — QML tree builds; dashboard renders in "Waiting for Live
     Data" (Phase 3 §14), since ApiClient has not started yet
7. apiClient.start() — first pollNow() fires
8. app.exec() — event loop begins; polling continues on its configured interval
```

**Why this exact order is required:**
- **Logger first:** every other class's constructor may want to log; nothing may log before it exists.
- **Settings before DashboardController's children:** every child needs configuration at construction or immediately after.
- **TankRepository before ApiClient:** ensures a receiver exists before ApiClient could theoretically ever emit — correct dependency order for clarity and for avoiding a startup race class of bug, even though Qt's signal/slot mechanism itself wouldn't literally drop a signal here (ApiClient hasn't started yet regardless).
- **TankModel geometry applied before QML loads:** avoids a flash of hardcoded defaults on the first rendered frame (Phase 4 §1).
- **Polling starts last, after every consumer of its output already exists and is wired:** this is what prevents "first reading arrives, nothing is listening yet" as a class of startup bug — by construction, not by luck.

---

## 14. Runtime Lifecycle

This section is intentionally condensed — the full detail is Phase 4 §1 (startup) and §8 (event-driven lifecycle); only what Phase 4 left unresolved is added here.

| Event | Phase 4 reference | Implementation-level addition |
|---|---|---|
| Startup | §1 | Section 13's exact object order |
| First poll | §1, §6 | **Resolution:** if the very first poll fails, the app stays in `WaitingForData` (retries silently) rather than transitioning to `Offline` — `Offline` specifically means "was `Connected`, now isn't"; a connection that has never succeeded has nothing to be "offline" *from* |
| Normal poll | §6 | No addition |
| Settings changed | §8 | No addition |
| Geometry changed | §8, §11 sequence | No addition |
| Network failure | §6, §10 | No addition |
| Recovery | §6 | No addition |
| Shutdown | §8 | Section 12's explicit shutdown sequencing |

---

## 15. Directory-Level Specification

| Directory | Purpose / Files | Allowed dependencies | Forbidden dependencies |
|---|---|---|---|
| `backend/calculations` | `VolumeCalculator.h/.cpp` | None (not even Qt beyond basic numeric types) | Everything else in the project |
| `backend/logging` | `Logger.h/.cpp` | None | Everything else |
| `backend/settings` | `SettingsManager.h/.cpp` | `backend/logging` | Everything else — must stay a leaf |
| `backend/utilities` | `TimeManager.h/.cpp` | None | Everything else |
| `backend/repository` | `TankRepository.h/.cpp` | `backend/calculations`, `backend/settings`, `backend/logging` | `backend/models` (no upward dependency), `backend/network` internals beyond the generic `RawReading` contract |
| `backend/network` | `ApiClient.h/.cpp` | `backend/state` (ConnectionManager), `backend/settings`, `backend/logging`, Qt Network | `backend/models`, `backend/repository` internals |
| `backend/models` | `TankModel.h/.cpp` | `backend/calculations`, Qt Core/Qml | `backend/network` directly |
| `backend/state` | `ConnectionManager`, `ApplicationStateManager`, `DashboardController` | `DashboardController` alone may depend on everything (composition-root privilege); `ConnectionManager`/`ApplicationStateManager` keep the narrower dependencies listed in Section 1 | `ConnectionManager` must not depend on `backend/models` or `backend/repository`; `ApplicationStateManager` must not depend on `TankModel` or `ApiClient` directly |
| `qml/components` | All reusable QML | `TankModel`/`TimeManager`/`SettingsManager` singletons (read-only), design tokens | Any C++ business logic, network calls, or calculation logic (Phase 2 NFR-1) |
| `qml/pages` | `DashboardPage.qml` (+future pages) | `qml/components`, `DashboardController` (navigation only) | Same restrictions as `qml/components` |
| `resources/icons`, `resources/fonts` | Assets only | — | Any code dependency |
| `tests` | Mirrors `backend/` structure | Whatever it's testing, plus mocks/fakes | — (the one directory allowed broad access, by necessity) |
| `docs` | This document set | — | Any code dependency |

---

## 16. Build Architecture

| Aspect | Specification |
|---|---|
| **CMake targets** | A single executable target, `PanoramaWaterTank`, for v1 — **Resolution:** `backend/calculations` is *structured* as if it could become a standalone static library later (Phase 2 §2), but is **not** extracted into one now; a library boundary only pays for itself once there's a second consumer (e.g. a future mobile companion), which doesn't exist yet — extracting it later is a pure `CMakeLists.txt` change, not a code change |
| **Static/shared libraries** | None in v1 |
| **Resources** | `qt_add_qml_module`'s built-in `RESOURCES` mechanism for icons/fonts |
| **Compiler requirements** | C++17 |
| **Qt modules** | `Core`, `Quick`, `QuickControls2`, `Network` always; `+Svg` only if the production SVG icon set (Phase 3 §12) is in use — the placeholder Canvas-drawn icon contingency requires no additional module |
| **Build profiles** | **Debug:** Trace-level console logging, assertions enabled, no optimization. **Release:** Info-level file logging (Section 7), optimizations on, assertions disabled |
| **Testing target(s)** | A separate CMake target under `tests/`, linked against **Qt Test** (`QtTest` module) — chosen over an external framework like GoogleTest specifically because it's already part of the Qt toolchain this project depends on, avoiding a second test-tooling story |

---

## 17. Testing Blueprint

| Class | Unit tests | Integration tests | Mocks needed | Key edge cases | Perf/regression |
|---|---|---|---|---|---|
| `VolumeCalculator` | Every function against known geometry (e.g. `r=1.20, h=5.00 → area≈4.5239 m², maxVolume≈22619.47 L`) | None needed — unit tests are full coverage for a pure function | None | `r=0`, `h=0`, negative inputs, `NaN`/`Infinity` inputs, `volume > maximumVolume` (raw `>100%`) | A benchmark asserting sub-microsecond call time, as a regression guard against future accidental overhead (e.g. unnecessary string formatting creeping in) |
| `TankRepository` | Accept/reject decisions at exact boundary values (Phase 4 §5 — e.g. exactly `110%` accepted, `110.01%` rejected) | Real `ApiClient` (or a local mock HTTP server) → `TankRepository`, verifying end-to-end JSON → validated event | Fake `SettingsManager` (or constructor-injected test geometry), fake raw-reading source | 3-consecutive-invalid triggers `SensorError`; 3-consecutive-valid recovers | Once real sensor noise data exists, a regression test capturing the resolved hysteresis behavior (Phase 4 §5, still open) |
| `TankModel` | Full recalculation correctness on Height/Volume/Distance paths; geometry-change recalculation (Phase 4 §11) | With a fake `TankRepository` event stream | Fake `TankRepository`, fake `SettingsManager` | `maximumVolume = 0` (defensive guard); alarm precedence ordering (Section 2) | — |
| `ApiClient` / `ConnectionManager` | `ConnectionManager` backoff math in isolation (no real networking) | Against a local mock HTTP server simulating `200`/`401`/`403`/`404`/`500`/timeout/malformed-JSON | Local mock HTTP server (e.g. a lightweight `QTcpServer`-based stub) | Single-in-flight guard under a slow response; manual refresh during an in-flight request | Backoff timing correctness across a simulated multi-failure sequence |
| `SettingsManager` | Validation rejects out-of-range values, retains prior value | Persistence round-trip: write → destroy → reconstruct → read back | None | Corrupt/missing config file on load (Phase 4 §13) | — |
| `Logger` | Level filtering suppresses below-threshold messages without constructing the string (assert via a call-counting stub formatter) | Multi-day simulated run verifying rotation | Simulated clock advancement | Disk-full write failure doesn't crash the app | — |
| `ApplicationStateManager` | Full transition table (Phase 2 §6) — every legal transition succeeds, every illegal one is rejected and logged | — | Fake `ConnectionManager`/`TankRepository` events | Rapid flapping between states doesn't desync the UI | — |
| `DashboardController` | — | Smoke test: full startup wiring (Section 13) completes without crashing, produces the expected initial state | — | — | — |
| **Whole-app acceptance** | — | Scripted scenario against a mock Ubidots server: start → `WaitingForData` → mock responds → `Connected` with correct calculated values → mock returns `401` → `AuthenticationError` → etc. — directly runs Phase 4 §11's sequence diagrams as literal test scripts | Full mock Ubidots server | Every Phase 4 §13 failure scenario, deliberately triggered | 24-hour soak (Milestone 10, Section 19) |

---

## 18. Implementation Dependencies

```text
Logger ──┬──────────────────────────────────────────────► (everything logs)
         │
         ▼
SettingsManager
         │
         ├──────────────► VolumeCalculator   (no dependency — could start in parallel)
         │                      │
         │                      ▼
         ├──────────────► TankModel
         │                      │
         ▼                      ▼
TankRepository ◄────────────────┘
         │
         ▼
ApiClient / ConnectionManager
         │
         ▼
ApplicationStateManager
         │
         ▼
DashboardController
         │
         ▼
QML Components  ◄── TimeManager (no dependency — can be built in parallel with the
         │                       Logger→...→TankModel chain; only needs to exist
         │                       before QML components that bind to it)
         ▼
Dashboard Assembly
         │
         ▼
Integration (Section 13's wiring, end to end)
```

No step depends on an unfinished module above it — `VolumeCalculator` and `TimeManager` are the two genuinely parallelizable branches, since both have zero dependencies on anything else in the graph.

---

## 19. Development Milestones

| # | Milestone | Completion criteria |
|---|---|---|
| M1 | Project Skeleton | `cmake --build` succeeds; app launches to a blank `ApplicationWindow` with the correct title and background color |
| M2 | Core Backend (non-networked) | `Logger`, `SettingsManager`, `TimeManager` implemented and unit-tested; settings persist across a restart; a log file is written in the specified format |
| M3 | Engineering Layer | `VolumeCalculator` fully implemented; 100% of Section 17's specified test cases pass, including boundary/NaN/negative cases |
| M4 | Data Layer | `TankRepository` + `TankModel` implemented against a **fake** raw-reading source; feeding 5 synthetic scenarios (Height path, Volume path, Distance path, an Overflow case, a rejected/invalid case) produces hand-verified correct `Q_PROPERTY` values |
| M5 | Networking | `ApiClient` + `ConnectionManager` implemented against a mock server, then the real Ubidots endpoint; correctly categorizes at least one each of success/timeout/401/malformed-JSON |
| M6 | QML Components | Every component in Section 8 built and visually reviewed in isolation against Phase 3's reference screenshot |
| M7 | Dashboard Assembly | Full `DashboardPage` composed and wired to the real `TankModel`/`TimeManager` singletons; static (non-live) layout matches Phase 3 §4 |
| M8 | Integration | Full live polling against the real device; dashboard values cross-checked against the existing React dashboard for the same device as an independent correctness check |
| M9 | Testing & Hardening | Full Section 17 suite passing; every Phase 4 §13 failure scenario manually triggered and confirmed to produce its specified UI behavior |
| M10 | Production Release | 24-hour continuous soak test: flat memory profile, confirmed log rotation, zero unhandled crashes |

---

## 20. Final Implementation Readiness Review

| Dimension | Assessment |
|---|---|
| **Architecture completeness** | Complete — every class in Phase 2's diagram now has a full contract (Section 1), and the two dependency ambiguities Phase 2 left implicit (Repository's geometry access, ApiClient/ConnectionManager's backoff coupling) are resolved in this document |
| **Component completeness** | Complete — all 20 QML components from Phase 3 §10 have bound-property, signal, and sizing specs (Section 8–10) |
| **Maintainability** | Strong — the directory-level dependency rules (Section 15) are specific enough to enforce in code review, not just aspirational |
| **Scalability** | Unchanged assessment from Phase 2 §14/Phase 4 §15 — multi-tank remains a scoped, contained refactor; MQTT, history, and alarms remain additive |
| **Testability** | Strong — every class has an explicit mock strategy (Section 17); the pure-function engineering layer is fully unit-testable in isolation, directly serving Phase 1's NFR-3 |
| **Industrial readiness** | Appropriate — 24/7 operation is designed for (bounded memory, bounded logs, async I/O, indefinite retry) and Milestone 10 explicitly verifies it empirically rather than assuming the design holds |
| **Qt/QML best practices** | Followed — `Q_PROPERTY`/signal-driven binding throughout, no manual refresh calls anywhere, QML singletons used only for genuinely singleton concerns, composition-root dependency injection rather than ad hoc globals |
| **Long-term support** | Supported by the token architecture (Phase 3 §3), the directory dependency rules (Section 15), and the fact that no settings require a restart (Section 6) — configuration and appearance changes are both low-risk operations post-launch |
| **Risk assessment** | Three genuinely open items remain, none of which block starting implementation (all have working defaults): (1) whether a live `temperature` variable exists on the Ubidots device (Phase 1, still unanswered); (2) whether `1.20 m / 5.00 m` are the tank's real dimensions or placeholders (Phase 1, still unanswered); (3) the exact hysteresis behavior at alarm boundaries (Phase 4 §5, explicitly deferred pending real sensor noise data). All three should be confirmed before Milestone 10, not before Milestone 1 |

**Conclusion:** this specification is complete enough for implementation to begin. Planning concludes at Phase 5, per the brief — no Phase 6 planning document is proposed. The three open items above are tracked, not blocking, and the natural next step is **Milestone 1** whenever you're ready to move from planning into implementation.
