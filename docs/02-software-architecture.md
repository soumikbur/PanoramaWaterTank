# Panorama Water Tank Monitor — Qt/QML Desktop Application
## Phase 2: Software Architecture

**Status:** Draft for review. Phase 1 (Requirements Analysis) is approved; this phase proceeds on the stated Phase 1 defaults (single operator, radius/height user-editable defaults, cross-platform target) pending your confirmation.
**No code in this document** — class descriptions are interface *contracts*, not headers.

---

## 1. High-Level Architecture

```text
 ESP32-S3 + Sensor
        │
        ▼
 EC200U-CN 4G Module  (cellular uplink — outside this app's control)
        │
        ▼
 Ubidots REST API  (system of record)
        │
        ▼
 ┌─────────────────────────────────────────────────────────┐
 │ NETWORK LAYER                                            │
 │   ApiClient — HTTP, auth, JSON decoding, raw errors      │
 │   ConnectionManager — retry/backoff, online/offline state│
 └─────────────────────────────────────────────────────────┘
        │  raw reading / raw error (transport-agnostic shape)
        ▼
 ┌─────────────────────────────────────────────────────────┐
 │ REPOSITORY / DATA LAYER                                  │
 │   TankRepository — validates, caches last-good reading,  │
 │   decides what the rest of the app is allowed to see     │
 └─────────────────────────────────────────────────────────┘
        │  validated reading
        ▼
 ┌─────────────────────────────────────────────────────────┐
 │ BUSINESS LOGIC LAYER                                      │
 │   TankModel (QML singleton) — geometry + current state,   │
 │   the one object QML actually binds to                    │
 └─────────────────────────────────────────────────────────┘
        │  invokes
        ▼
 ┌─────────────────────────────────────────────────────────┐
 │ ENGINEERING CALCULATION LAYER                             │
 │   VolumeCalculator — pure geometry math, no state          │
 └─────────────────────────────────────────────────────────┘
        │  derived values flow back into TankModel
        ▼
 Q_PROPERTY bindings (Observer pattern, automatic)
        │
        ▼
 ┌─────────────────────────────────────────────────────────┐
 │ PRESENTATION LAYER (QML)                                  │
 │   ApplicationWindow → Header/Sidebar/DashboardPage → cards │
 └─────────────────────────────────────────────────────────┘

 Cross-cutting, used by every layer above:
   • SettingsManager   (Configuration Layer)
   • Logger            (Logging Layer)
   • ApplicationStateManager / DashboardController (Application Layer — orchestration)
```

**Layer responsibilities, in one line each:**

| Layer | Owns | Never does |
|---|---|---|
| Network | Talking to Ubidots, raw HTTP/JSON | Tank geometry math, UI state |
| Repository/Data | Validation, caching, "what counts as a valid reading" | HTTP details, QML bindings |
| Business Logic | Current tank state, exposed via `Q_PROPERTY` | Networking, math implementation |
| Engineering | Pure area/volume/height/percentage math | Anything stateful, anything Qt-object-based |
| Application | Page navigation, app-level state machine, composition root | Business rules, math |
| Configuration | Persisted settings, defaults, validation of user input | Networking, calculations |
| Logging | Structured log output | Business decisions |
| Presentation | Layout, animation, formatting | Calculation, validation, persistence |

This is a **layered + repository + MVVM hybrid**: strictly one-directional dependencies from Presentation down to Network, with the Repository layer acting as the seam where a future transport (MQTT) or a future consumer (mobile companion, historical store) can be inserted without touching the layers above it.

---

## 2. Folder Structure

```text
PanoramaWaterTank/
├── CMakeLists.txt
├── docs/                      # This document set (Phases 1–13), kept versioned with the code
├── src/
│   └── main.cpp                # Composition root only — wires objects, no logic
├── backend/
│   ├── models/                 # TankModel — QML-facing state objects
│   ├── repository/             # TankRepository — validation + caching seam
│   ├── network/                # ApiClient, ConnectionManager
│   ├── calculations/           # VolumeCalculator — pure, dependency-free math
│   ├── state/                  # ApplicationStateManager, DashboardController
│   ├── settings/                # SettingsManager
│   ├── logging/                 # Logger
│   └── utilities/                # TimeManager, small shared helpers
├── qml/
│   ├── Main.qml
│   ├── pages/                  # DashboardPage.qml, (future) SettingsPage.qml, etc.
│   └── components/             # Header, Sidebar, TankCard/*, InformationCard, RankIndicator, ...
├── resources/
│   ├── icons/
│   └── fonts/
└── tests/
    ├── calculations/            # Unit tests for VolumeCalculator (no Qt event loop needed)
    ├── repository/              # TankRepository tests against a mock ApiClient
    └── models/                  # TankModel tests against a mock TankRepository
```

**Why each folder exists:**

- **`backend/models` vs `backend/repository`** are deliberately separate folders (not just separate classes) so the *boundary itself* is visible in the tree — a new contributor immediately sees "data validation" and "business state" are different concerns.
- **`backend/calculations`** is isolated specifically because it has zero Qt/network dependencies — it's the one folder that could be lifted into a standalone static library later (e.g. shared with a future mobile companion app) without dragging Qt Network or QML along.
- **`backend/state`** is separate from `backend/models` because application-level state (which page is open, are we in a full-screen error state) is a different lifecycle than tank data state.
- **`tests/`** mirrors `backend/` folder-for-folder, and is structured so the lowest layers (`calculations`) can be tested with plain unit tests, while higher layers use mocked dependencies from the layer below — this is only possible *because* of the repository seam described in Section 7.
- **`docs/`** ships in the repo, not as throwaway planning — it's the reference the roadmap phases keep building on.

---

## 3. Class Architecture

Each class below is described as an interface contract: what it exposes, what it depends on, who owns it. No signatures, no code.

### ApiClient — *Network Layer*
- **Purpose:** The only class in the application that knows an HTTP request exists.
- **Responsibilities:** Build authenticated requests against the configured Ubidots endpoint; own the poll timer and per-request timeout; decode the JSON envelope into a transport-neutral raw reading; classify failures (timeout / HTTP status / malformed body / no network).
- **Public interface (conceptual):** configuration properties (base URL, device label, variable labels, token, timeout, poll interval); `start()`, `stop()`, `pollNow()`; emits a raw-reading-received event and a categorized-error event.
- **Internal responsibilities:** in-flight request guard (never overlap two polls), JSON schema tolerance (missing optional fields shouldn't fail the whole reading).
- **Dependencies:** Qt's network stack; reports outcomes to **ConnectionManager**; reports diagnostics to **Logger**; reads initial config from **SettingsManager**.
- **Lifetime:** created once at startup, lives for the process lifetime.
- **Ownership:** owned by **DashboardController** (composition root). Nothing owns *it* upward — it only emits events, it never reaches back into the objects that consume them.

### TankRepository — *Repository / Data Layer*
- **Purpose:** The seam between "however we talk to the backend" and "what the business layer is allowed to trust."
- **Responsibilities:** Receive raw readings from ApiClient; validate them (range/sanity checks, required-field checks); cache the last-known-good reading with a timestamp; decide what happens when a poll fails (serve the cache, don't blank the UI).
- **Public interface:** `currentReading()`, `lastGoodReading()`; a slot that accepts raw readings from ApiClient; emits a validated-reading-changed event and a reading-rejected event (with reason, for the Logger).
- **Dependencies:** listens to **ApiClient**; writes to **Logger**.
- **Lifetime:** process lifetime.
- **Ownership:** owned by **DashboardController**. **TankModel** depends on it (listens to its events) but does not own it — this direction matters, see Section 7.

### TankModel — *Business Logic Layer, QML singleton*
- **Purpose:** The single source of truth for tank geometry and current derived state — the one object the QML layer actually binds to.
- **Responsibilities:** Hold radius/height (mutable, sourced from SettingsManager); hold the current validated reading (height or volume) forwarded by TankRepository; invoke VolumeCalculator on every change; hold status, connection state, last-updated timestamp; expose everything as `Q_PROPERTY`.
- **Public interface:** read-only `Q_PROPERTY` for every displayed value (area, max volume, current height/volume/percentage, remaining volume, temperature, status, connection state, last updated); a slot that receives repository updates; a slot that receives geometry changes from SettingsManager.
- **Dependencies:** calls **VolumeCalculator** (pure function calls); listens to **TankRepository** and **SettingsManager**.
- **Lifetime:** QML singleton — created once, lives the whole session.
- **Ownership:** owned by the QML engine's singleton mechanism. TankModel never reaches "up" into ApiClient or TankRepository beyond receiving their events.

### VolumeCalculator — *Engineering Calculation Layer*
- **Purpose:** Pure, stateless cylindrical-tank math. No Qt object, no signals, no I/O.
- **Responsibilities:** Area, volume, height, percentage, remaining-volume calculations; precision/rounding rules; clamping invalid or out-of-range inputs.
- **Public interface:** a small set of pure functions, each taking numbers in and returning a number out.
- **Dependencies:** none — this is the one piece of the architecture with zero coupling to anything else, by design (see NFR-3, Phase 1).
- **Lifetime / Ownership:** not instantiated as an object at all; stateless, callable from anywhere.

### SettingsManager — *Configuration Layer*
- **Purpose:** The single place configuration is read from and written to.
- **Responsibilities:** Load/save persisted values; validate ranges before accepting a change (e.g. radius must be positive); provide sane defaults; notify dependents when a value changes at runtime (e.g. a future Settings page edits tank height live).
- **Public interface:** `Q_PROPERTY` per setting (API URL, token, device label, variable labels, tank radius/height, poll interval, timeout, theme, log level); implicit save-on-change.
- **Dependencies:** writes diagnostics to **Logger**.
- **Lifetime:** created once at startup.
- **Ownership:** owned by **DashboardController**; **ApiClient** and **TankModel** hold a reference/listen to it, they don't own it.

### ConnectionManager — *Network Layer, cross-cutting*
- **Purpose:** The single authority on "are we online," independent of any one request's outcome.
- **Responsibilities:** Track consecutive failures; own the retry/backoff policy; expose a `ConnectionState` (Connected / Reconnecting / Offline); decide *when* a transient failure becomes a sustained outage.
- **Public interface:** `Q_PROPERTY connectionState`; `reportSuccess()` / `reportFailure(category)` slots called by ApiClient; a `connectionStateChanged` event.
- **Dependencies:** fed by **ApiClient**; consumed by **ApplicationStateManager** and **TankModel**.
- **Lifetime:** process lifetime.
- **Ownership:** owned by **DashboardController**. This exists as its own class specifically so retry policy isn't duplicated or reinvented inside ApiClient — one policy, one place.

### Logger — *Logging Layer*
- **Purpose:** Centralized, categorized logging for the whole application.
- **Responsibilities:** Consistent formatting (timestamp, category, level); write to rotating log files; mirror to console in debug builds.
- **Public interface:** a single logging entry point taking a category, a level, and a message.
- **Dependencies:** none — a leaf utility every other class may call into.
- **Lifetime:** initialized first, before any other backend class.
- **Ownership:** effectively a process-wide singleton.

### TimeManager — *Utility, QML singleton*
- **Purpose:** One source of truth for "now," so clock formatting and "last updated X ago" logic isn't duplicated across QML files.
- **Responsibilities:** Tick a 1-second timer; expose formatted current time/date; convert a timestamp into a relative string.
- **Public interface:** `Q_PROPERTY currentTime`, `Q_PROPERTY currentDate`; an invokable relative-time formatter.
- **Dependencies:** none.
- **Lifetime:** QML singleton, process lifetime.
- **Ownership:** owned by the QML engine.

### DashboardController — *Application Layer, composition root*
- **Purpose:** The one class allowed to know about *all* the others — everything else only knows its direct dependencies.
- **Responsibilities:** Instantiate and own ApiClient, TankRepository, ConnectionManager, SettingsManager; own "which sidebar page is active"; expose the small set of actions QML actually triggers (retry now, navigate to page, open settings).
- **Public interface:** `Q_PROPERTY currentPage`; invokable `retryConnection()`, `navigateTo(page)`.
- **Dependencies:** everything in the backend, at the composition-root level only.
- **Lifetime:** process lifetime, created in `main.cpp`.
- **Ownership:** owns ApiClient/TankRepository/ConnectionManager/SettingsManager (or shares ownership with `main.cpp`'s object tree).

### ApplicationStateManager — *Application Layer*
- **Purpose:** A formal state machine for the *application's* lifecycle (Section 6) — distinct from ConnectionManager, which only tracks network health.
- **Responsibilities:** Own the enumerated app states and valid transitions between them; expose current state so QML can show full-screen states (loading, connection-lost, config-error) instead of a partial/broken dashboard.
- **Public interface:** `Q_PROPERTY currentState`; a `stateChanged(from, to)` event.
- **Dependencies:** fed by **ConnectionManager** and **TankRepository** events.
- **Lifetime:** process lifetime.
- **Ownership:** owned by **DashboardController**.

**Interaction summary:** ApiClient → ConnectionManager + TankRepository → TankModel → VolumeCalculator (called, not listened to) → QML. SettingsManager and Logger are consumed by nearly everything but depend on nothing themselves. DashboardController and ApplicationStateManager sit beside this pipeline, orchestrating rather than participating in the data flow.

---

## 4. QML Architecture

```text
ApplicationWindow (Main.qml)
│
├── Header.qml                     — title, connection badge, clock/date, profile
├── Sidebar.qml                    — nav list, active-page indicator
│     └── NavItem.qml              — single nav row (reusable)
│
├── DashboardPage.qml
│     │
│     ├── TankCard.qml             — the "Water Level" card as a whole
│     │     ├── TankView.qml       — the cylindrical vessel outline + clipping
│     │     ├── TankScale.qml      — 0/25/50/75/100 tick marks + labels
│     │     ├── WaterFill.qml      — the animated fill rectangle
│     │     ├── WaterSurface.qml   — the wave/shimmer overlay on top of the fill
│     │     ├── TankStatistics.qml — current level / height text block beside the tank
│     │     └── StatusBadge.qml    — small status pill (Normal/Low/Critical)
│     │
│     ├── InformationCard.qml      — the key/value tank-info table
│     │     └── InfoRow.qml        — single label/value row (reusable)
│     │
│     ├── RankIndicator.qml        — segmented capacity bar + marker
│     │     └── RankSegment.qml    — single colored band (reusable)
│     │
│     └── StatisticCardsRow.qml
│           └── StatisticCard.qml  — reusable metric tile, used 4×
│
└── Footer.qml
```

| Component | Purpose | Key properties (in) | Signals (out) | Depends on | Reusable? |
|---|---|---|---|---|---|
| `Header` | Top bar | `pageTitle`, `pageSubtitle`, `connected`, `currentTime`, `currentDate` | — | `TimeManager` (via binding from Main.qml) | No — one per app |
| `Sidebar` | Navigation | `currentPage`, nav model | `pageSelected(page)` | `NavItem` | No |
| `NavItem` | One nav row | `icon`, `label`, `active` | `clicked()` | — | Yes |
| `DashboardPage` | Assembles the dashboard | — | — | `TankModel` singleton | No |
| `TankCard` | Card shell for the tank visualization | `fillPercentage`, `currentHeight`, `status`, `lastUpdated` | — | `TankView`, `TankScale`, `WaterFill`, `WaterSurface`, `TankStatistics`, `StatusBadge` | No |
| `TankView` | Vessel outline, defines the clip region | `radius` (visual), geometry | — | — | Yes (any cylindrical vessel) |
| `TankScale` | Tick marks | `divisions` (default 4) | — | — | Yes |
| `WaterFill` | The blue fill rectangle | `fillPercentage`, `animationDuration` | — | — | Yes |
| `WaterSurface` | Wave shimmer | bound to `WaterFill` height | — | — | Yes |
| `TankStatistics` | Big percentage + labels beside the tank | `fillPercentage`, `currentHeight` | — | — | Yes (any single-metric readout) |
| `StatusBadge` | Small status pill | `status` | — | — | Yes |
| `InformationCard` | Key/value table | `model` (list of rows) | — | `InfoRow` | No |
| `InfoRow` | One label/value line | `icon`, `label`, `value` | — | — | Yes |
| `RankIndicator` | Segmented bar + marker | `fillPercentage`, `segments` | — | `RankSegment` | Yes (generalizable to any 4-band metric) |
| `RankSegment` | One colored band | `label`, `color`, `range` | — | — | Yes |
| `StatisticCard` | Reusable metric tile | `icon`, `title`, `value`, `subtitle` | — | — | Yes |
| `Footer` | Bottom bar | — | — | — | No |

**Rule enforced throughout:** every property listed above is *read-only, one-way bound* from `TankModel`/`TimeManager` down into these components. No QML component computes a percentage, converts units, or applies a threshold — those are all C++-side facts passed in already-resolved. QML components only ever branch on *already-computed* strings/numbers (e.g. `status === "Critical"` to pick a color), never recompute them.

---

## 5. Data Flow

```text
1. QTimer (owned by ApiClient) fires
2. ApiClient builds an authenticated GET request → Ubidots
3. Ubidots responds (success, error, or times out)
4. ApiClient decodes the JSON envelope into a raw reading
      success → emits rawReadingReceived(...)
      failure → emits errorOccurred(category, message) → ConnectionManager.reportFailure()
5. TankRepository receives the raw reading
      - validates required fields exist
      - validates values are within physically sane bounds
      valid   → updates lastGoodReading, emits validatedReadingChanged(...)
      invalid → logs + emits readingRejected(reason), TankModel is NOT updated
6. TankModel receives validatedReadingChanged
      - determines height-based or volume-based reading (auto-detect)
      - calls VolumeCalculator for every derived value
      - updates its Q_PROPERTYs, each emitting its own *Changed signal
7. QML property bindings react automatically (Qt's binding engine — no manual "refresh" call anywhere)
8. Visual components animate toward the new bound values
      (WaterFill height, RankIndicator marker position, StatisticCard text)
```

Two independent side channels run in parallel with the above and never block it:
- **ConnectionManager** updates `connectionState` on every success/failure, independent of whether TankRepository accepted the reading — a "connected but rejected reading" state is representable and distinct from "disconnected."
- **TimeManager** ticks its own 1-second timer for the header clock, entirely decoupled from the poll cycle.

---

## 6. Application State Machine

```text
ApplicationStarting
   │  (SettingsManager loads config)
   ▼
LoadingConfiguration
   │  config valid?  ── no ──► ConfigurationError (full-screen, blocks polling)
   │  yes
   ▼
Connecting            (first pollNow() in flight)
   │
   ├── success ─────────────────────────────► Connected
   ├── 401/403 ─────────────────────────────► AuthenticationError
   ├── network/timeout/5xx ──────────────────► NetworkError → Reconnecting
   │
Connected
   │
   ├── new reading arrives ──► Updating ──► back to Connected
   ├── poll fails once ──────► (ConnectionManager still "Connected" — single
   │                            failures don't demote the app state, only
   │                            sustained failures do, see below)
   ├── N consecutive failures ► Offline
   │
Offline
   │  displays "Disconnected", keeps last-good reading on screen
   │  retries on backoff schedule
   ├── success ──► Connected
   ├── still failing ──► Reconnecting (visually identical to Offline, but
   │                     distinguishes "first time offline" from "still retrying"
   │                     for logging purposes)
   ▼
ShuttingDown            (window close requested — stop timers, flush logs)
```

**Entry/exit actions:**

| State | On entry | On exit |
|---|---|---|
| `LoadingConfiguration` | Read SettingsManager, apply to TankModel/ApiClient | — |
| `ConfigurationError` | Show blocking full-screen message, do not start polling | User corrects settings → re-enter `LoadingConfiguration` |
| `Connecting` | Show "Waiting for Live Data" | — |
| `Connected` | Clear any error banners | — |
| `AuthenticationError` | Show blocking message ("check API token"), stop polling | User updates token → re-enter `Connecting` |
| `Offline` / `Reconnecting` | Show "Disconnected" badge, keep last-good values visible | Successful poll → `Connected` |
| `ShuttingDown` | Stop ApiClient timer, flush Logger buffers | — |

**Recovery strategy:** `AuthenticationError` and `ConfigurationError` are *not* auto-retried — they require a human to fix configuration (this matches Phase 1's FR distinction between configuration problems and transient outages). `NetworkError`/`Offline` *are* auto-retried using ConnectionManager's backoff policy (finalized in Phase 8).

---

## 7. Repository Pattern

**Chosen architecture:**

```text
REST API → ApiClient → TankRepository → TankModel → QML
```

**Rejected simpler alternative:**

```text
REST API → ApiClient → TankModel → QML
```

**Why the extra layer earns its complexity:**

| Concern | Without TankRepository | With TankRepository |
|---|---|---|
| **Decoupling** | TankModel must know REST-specific error codes and JSON quirks | TankModel only ever sees a clean "reading" or "no update," transport-agnostic |
| **Testability** | Testing TankModel requires mocking HTTP responses | Testing TankModel requires only a fake TankRepository event — trivial |
| **Future MQTT support** | Would require a second code path inside TankModel, or duplicating ApiClient | A new `MqttClient` simply targets the *same* TankRepository interface; TankModel is untouched |
| **Caching / offline mode** | Logic to "keep the last good value" gets awkwardly bolted onto ApiClient or TankModel | Lives in exactly one place, with one clear owner |
| **Mock repositories for tests** | Not naturally possible | `TankRepository` is an obvious seam to substitute a mock implementation |

This is the single highest-leverage decision in the architecture: it's what makes Section 12 (Future Expansion in Phase 1 — MQTT, historical logging, multi-tank) additive rather than requiring a rewrite of the business layer.

---

## 8. Configuration Architecture

**Storage format: `QSettings`**, not a hand-rolled `config.json`.

Rationale: platform-native storage (Registry on Windows, INI on Linux) with atomic writes and no manual parsing/serialization code to maintain; Qt handles type coercion; it's the same mechanism already used for standard app settings like window geometry, so there's no second persistence mechanism to reason about.

**Managed values:**

| Setting | Default | Validated as |
|---|---|---|
| API base URL | `https://industrial.api.ubidots.com` | non-empty, well-formed URL |
| Auth token | *(empty until configured)* | non-empty before polling starts |
| Device label | `esp32s3` | non-empty |
| Level variable label | `waterlevel` | non-empty |
| Status variable label | `sensorstatus` | may be empty (optional) |
| Temperature variable label | *(pending your answer — Phase 1, open question 1)* | optional |
| Tank radius (m) | `1.20` | `> 0` |
| Tank height (m) | `5.00` | `> 0` |
| Poll interval (ms) | `3000` | `1000–60000` |
| Timeout (ms) | `5000` | `500–30000` |
| Theme | `Light` | enum |
| Log level | `Info` | enum |

**Runtime updates:** every setting is a `Q_PROPERTY` on `SettingsManager`; changing one triggers its `*Changed` signal, which `TankModel` (for geometry) or `ApiClient` (for connection settings) listen to directly — no restart required, no polling of settings values. Validation happens in the setter, before the value is persisted or propagated: an invalid value is rejected and the previous valid value is retained.

---

## 9. Logging Architecture

| Category | Captures |
|---|---|
| **Application** | Startup/shutdown, state machine transitions, settings changes |
| **API** | Every request attempt, HTTP status, response validity |
| **Network** | Low-level connectivity (timeouts, DNS/host failures) |
| **Calculation** | Rejected/clamped readings from VolumeCalculator (e.g. negative height received) |
| **Error** | Aggregation point for anything logged at Error level or above, regardless of category — makes "show me everything that went wrong today" a single-file grep |

**Log levels:** `Trace < Debug < Info < Warning < Error < Critical` (standard Qt logging category levels).

**File locations:** platform-standard app-data logging directory (`QStandardPaths::AppDataLocation`/`logs/`), one active file per day.

**Rotation policy:** daily rotation, retain the last 14 days by default (configurable), oldest files pruned on startup — appropriate for 24/7 operation without unbounded disk growth.

**Timestamp format:** ISO 8601 with local timezone offset, so logs correlate directly with Ubidots' own timestamps.

**Debug vs. Release:** Debug builds mirror everything to the console at `Trace` and above; Release builds write to file only, default threshold `Info`, with `Debug`/`Trace` available via a runtime environment variable for field diagnostics without a rebuild.

---

## 10. Threading Model

**Main/UI thread:** everything in this architecture, by default. Qt's `QNetworkAccessManager` is asynchronous by design — requests are dispatched and responses arrive via the event loop, not by blocking a thread. This means `ApiClient`, `TankRepository`, `TankModel`, and the QML engine all safely live on one thread with no manual synchronization, and this is intentional: it's simpler, and avoids an entire class of Qt-specific bugs (touching QML/UI objects from a worker thread).

**JSON parsing:** the Ubidots payload for this application is small (a handful of scalar fields). Parsing it is sub-millisecond and stays on the main thread — moving it to a worker thread would add complexity (thread-safe hand-off of the result) for no measurable benefit.

**Where a worker thread *would* be introduced later:** if Phase 12's historical logging/CSV export lands, bulk file I/O or large dataset processing should move to `QtConcurrent::run` or a dedicated `QThread`, specifically because that work is no longer "parse five numbers" but "process thousands of rows." The architecture reserves this decision for whichever future class owns that feature (e.g. a `HistoryManager`), rather than pre-building thread machinery nothing currently needs.

**Thread safety in practice:** because no backend class in v1 runs off the main thread, there is no shared-state race to guard against — `Q_PROPERTY` changes, signal emission, and QML binding evaluation are already sequential relative to each other by virtue of the single-threaded event loop.

---

## 11. Dependency Diagram

```text
                     ┌────────────────┐
                     │     Logger     │◄───────────────────────────┐
                     └────────────────┘                            │
                             ▲                                     │
                             │ (write-only, no dependents read from it)
                             │
   ┌──────────────┐   ┌──────────────┐   ┌───────────────────┐    │
   │ApiClient     │──►│ConnectionMgr │   │SettingsManager     │────┘
   └──────────────┘   └──────────────┘   └───────────────────┘
          │                    │                    │
          ▼                    │                    │
   ┌──────────────┐            │                    │
   │TankRepository│            │                    │
   └──────────────┘            │                    │
          │                    │                    │
          ▼                    ▼                    ▼
   ┌─────────────────────────────────────────────────────┐
   │                     TankModel                        │
   └─────────────────────────────────────────────────────┘
          │                                        ▲
          ▼                                        │ (pure calls, not events)
   ┌──────────────┐                        ┌────────────────┐
   │  QML layer   │                        │VolumeCalculator│
   └──────────────┘                        └────────────────┘

   DashboardController + ApplicationStateManager sit "above" this graph:
   they instantiate ApiClient/TankRepository/ConnectionManager/SettingsManager
   at startup and observe ConnectionManager + TankRepository events, but no
   other class depends on *them* — this keeps the composition root from
   leaking into the rest of the graph.
```

No cycles: `TankModel` never calls back into `TankRepository` or `ApiClient`; `VolumeCalculator` depends on nothing; `Logger`/`SettingsManager` are depended-on, never dependents.

---

## 12. Sequence Diagram — One Refresh Cycle

```text
QTimer(ApiClient)
   │ timeout
   ▼
ApiClient.pollNow()
   │ builds request, sends GET
   ▼
Ubidots REST API
   │ HTTP response (200 + JSON body)
   ▼
ApiClient
   │ decode JSON → raw reading {hasHeight, heightMeters, hasVolume, volumeLiters, temperature, sensorStatus}
   │ reportSuccess() ─────────────────────────► ConnectionManager (state → Connected)
   │ rawReadingReceived(...)
   ▼
TankRepository
   │ validate fields + ranges
   │ cache as lastGoodReading, stamp with current time
   │ validatedReadingChanged(...)
   ▼
TankModel
   │ auto-detect height vs. volume
   │ call VolumeCalculator.calculateArea/calculateVolume/calculateHeight/
   │      calculatePercentage/calculateRemainingVolume as needed
   │ update Q_PROPERTY values → each emits its *Changed signal
   ▼
Qt QML binding engine
   │ every binding referencing a changed property is re-evaluated automatically
   ▼
QML components
   │ WaterFill height, RankIndicator marker x, StatisticCard text all update
   │ Behavior{} animations interpolate to the new values
   ▼
Animated Dashboard (user sees the update)
```

Every step above is push-based (signal → slot / binding), not poll-based — nothing in the presentation layer ever asks "has the model changed yet?"; it is told.

---

## 13. Design Patterns

| Pattern | Where | Why it fits |
|---|---|---|
| **MVVM** | QML (View) ↔ TankModel/DashboardController (ViewModel) ↔ TankRepository/ApiClient (Model) | Matches Qt/QML's native idiom — `Q_PROPERTY` + signals *is* Qt's MVVM binding mechanism; fighting it would mean more code, not less |
| **Repository** | TankRepository | Decouples transport from business logic (Section 7) |
| **Observer** | Qt signals/slots and QML property bindings throughout | Built into the framework; used consistently instead of manual "refresh" calls anywhere in the app |
| **Singleton** | `TankModel`, `SettingsManager`-as-QML-singleton (optional), `TimeManager` | Each represents genuinely single, app-wide state — not used as a substitute for proper dependency passing elsewhere |
| **State** | `ApplicationStateManager` | The app's lifecycle has a small, finite set of states with well-defined transitions (Section 6) — a natural fit rather than scattered booleans (`isLoading`, `isError`, `isOffline`...) |
| **Strategy** | ConnectionManager's retry/backoff policy | Isolating the backoff *algorithm* behind one method means it can change (fixed interval → exponential → jittered) without touching ApiClient |
| **Dependency Injection (constructor/property injection at the composition root)** | `main.cpp` / `DashboardController` | Every class receives its collaborators rather than reaching for globals (except the three designated QML singletons) — this is what makes the `tests/` structure in Section 2 possible |
| **Explicitly avoided: Factory** | — | No family of interchangeable object *types* exists yet (one tank, one transport) — introducing a factory now would be speculative complexity; revisit if/when multi-tank (Phase 12) lands |

---

## 14. Architecture Validation

**Strengths**
- The Repository seam (Section 7) is the architecture's load-bearing decision — nearly every "future expansion" item in Phase 1 attaches to it without disturbing TankModel or the QML layer.
- VolumeCalculator's total isolation (Section 3) means the engineering correctness this whole project is built around (Phase 1, Objective O2) is independently verifiable in a unit test, not just "looks right on screen."
- Single-threaded design (Section 10) sidesteps an entire category of Qt/QML threading bugs, appropriate given the actual workload (small JSON payloads, not bulk data — yet).
- Configuration (Section 8) and Logging (Section 9) are both centralized single-owner subsystems, so there's no risk of, say, two different classes independently deciding what counts as a valid tank radius.

**Potential bottlenecks**
- None at the current single-tank, REST-polling scale. The architecture's headroom is intentionally larger than v1's needs.
- If a future historical/charting feature (Phase 12) queries a large local datastore synchronously from the main thread, that would become the first real bottleneck — Section 10 already earmarks where that work should move (worker thread), not whether it's needed yet.

**Scalability considerations**
- **Multiple tanks:** not free — `TankModel` and `TankRepository` are currently singletons representing *one* tank. Supporting multiple tanks means turning them into per-device instances (e.g. keyed by device label) managed by a new `TankCollection`/`DeviceManager` class, with `DashboardController` selecting which tank's model the currently-displayed page binds to. This is a real refactor, but it's contained to the Business/Repository layers — `VolumeCalculator`, `ApiClient`'s per-request logic, and the QML component tree (Section 4) are already tank-agnostic and would not need to change.
- **MQTT integration:** additive. A new `MqttClient` (Network layer) targets the same `TankRepository` interface `ApiClient` does today.
- **Historical logging:** additive. A new `HistoryManager` (own folder under `backend/`) subscribes to `TankRepository`'s validated-reading events and persists them independently; it does not sit inline in the existing pipeline.
- **Alarm systems:** additive. An `AlarmManager` subscribing to `TankModel`'s property-changed events, evaluating rules, and emitting its own notifications — no existing class needs to know it exists.
- **Remote configuration:** `SettingsManager` is already the single owner of configuration; a sync client would read/write through it rather than around it.
- **Future mobile companion app:** the *pattern* transfers (repository seam, isolated calculation layer) but the *code* doesn't — a mobile client would need its own transport and presentation layers. What this architecture does provide is a already-isolated `VolumeCalculator`-equivalent contract (Section 2's note that `backend/calculations` could become a standalone library) that a mobile backend could reuse rather than reimplementing the engineering math from scratch.

**Risks**
- The Repository/Business split (TankRepository vs. TankModel) is easy to erode over time if a future contributor takes a shortcut and has `TankModel` reach past `TankRepository` directly into `ApiClient` "just this once." This should be treated as a hard rule in code review, not just a diagram.
- `DashboardController` as composition root is intentionally the one class that knows about everything — if responsibilities creep into it beyond wiring and page navigation, it becomes a de facto god-object. Its scope (Section 3) should stay narrow.

**Suitability checklist**

| Requirement | Verdict |
|---|---|
| 24/7 industrial operation | Yes — single-threaded async I/O, bounded log growth, no unbounded caches |
| Multiple tanks | Not out-of-the-box; a scoped, contained refactor (above) |
| MQTT integration | Yes, additive |
| Historical logging | Yes, additive |
| Alarm systems | Yes, additive |
| Remote configuration | Yes — SettingsManager is already the single owner |
| Future mobile companion app | Partially — engineering logic is reusable, transport/presentation are not |

---

Phase 2 is complete. Per the phase-gated process: I'll hold here for your review before starting **Phase 3: Application Workflow**.
