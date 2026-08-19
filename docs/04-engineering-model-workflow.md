# Panorama Water Tank Monitor — Qt/QML Desktop Application
## Phase 4: Engineering Model, Data Pipeline & Application Workflow

**Status:** Draft for review. Phase 3 (UI/UX Architecture & Design System) is approved. This document is the precise contract between "a number arrives from Ubidots" and "the dashboard shows the correct thing" — it is what Phase 2's architecture and Phase 3's visuals are both actually *for*.
**No code in this document.**

One refinement surfaces in Section 3 that revises a detail from earlier phases: reading-type detection is reclassified from an inferred behavior to an explicit configuration value. Reasoning is given there.

---

## 1. End-to-End System Workflow

```text
System Startup
   ↓
Initialize Logging                    — first, so every subsequent stage is observable
   ↓
Load Configuration (SettingsManager)  — read persisted values, validate, fall back to
   │                                    defaults + flag ConfigurationError if corrupt (Section 13)
   ↓
Create Core Services (DashboardController, composition root)
   │   — instantiates ApiClient, ConnectionManager, TankRepository, ApplicationStateManager
   ↓
Resolve TankModel (QML singleton) — apply configured geometry BEFORE any QML renders,
   │                                 so the first frame is never a flash of hardcoded defaults
   ↓
Load QML — dashboard chrome appears; Tank Card shows "Waiting for Live Data" (Phase 3 §14)
   ↓
Start ApiClient — immediate first poll, then timer-driven polling begins (Section 6)
   ↓
Authenticate (implicit in the first request — a 401/403 here is detected, not assumed)
   ↓
Receive First Reading
   ↓
Validate Reading (TankRepository, Section 5)
   │   invalid → log, stay in "Waiting for Live Data", retry on next poll
   │   valid   ↓
   ↓
Engineering Calculations (VolumeCalculator, via TankModel — Section 4)
   ↓
Update TankModel (Q_PROPERTY writes, each emitting its *Changed signal)
   ↓
QML Refresh (binding engine reacts automatically — Phase 2 §5, no manual refresh call anywhere)
   ↓
Continuous Monitoring — steady-state loop: poll → validate → calculate → update → refresh,
                          repeating on the configured interval indefinitely
```

**Per-stage notes:**
- **Logging first:** if `SettingsManager` fails to load, that failure itself must be logged — so `Logger` cannot depend on configuration being valid.
- **Geometry applied before QML loads:** this is the ordering decided in Phase 2 §1 (main.cpp resolves the `TankModel` singleton and applies settings *before* `engine.loadFromModule`), restated here because it's the first concrete workflow decision that depends on it.
- **"Authenticate" has no separate step in this API:** Ubidots validates the token on every request rather than through a separate login call — so authentication failure is detected as a property of the *first poll's response*, not a distinct startup phase.

---

## 2. Engineering Data Model

All internal math uses double-precision throughout; the **Display Precision** column applies only at the point of formatting for QML, never earlier (Section 4 explains why).

| Property | Units | Source | Calculation | Validation | Display precision |
|---|---|---|---|---|---|
| Tank Radius | m | SettingsManager (user-configured) | — (input) | `> 0` and `≤ 50` (sanity ceiling) | 2 decimals |
| Tank Height | m | SettingsManager (user-configured) | — (input) | `> 0` and `≤ 50` | 2 decimals |
| Cross-sectional Area | m² | Derived | `π × radius²` | Derived — valid iff radius is valid | 2 decimals |
| Maximum Capacity | L | Derived | `area × height × 1000` | Derived | 2 decimals |
| Current Water Height | m | Derived (from whichever raw reading type is configured, Section 3) | See Section 4 pipeline | `0 ≤ h ≤ height × 1.10` (Section 5) | 2 decimals |
| Current Water Volume | L | Derived | `area × height × 1000` (or direct from API if Volume is the configured reading type) | `0 ≤ v ≤ maxVolume × 1.10` | 2 decimals |
| Remaining Volume | L | Derived | `max(0, maximumVolume − currentVolume)` | Derived | 2 decimals |
| Fill Percentage (display) | % | Derived | `(currentVolume ÷ maximumVolume) × 100`, **clamped** to `[0, 100]` | Derived, clamped | 2 decimals |
| Fill Percentage (raw, internal) | % | Derived | Same formula, **unclamped** — retained only for Overflow detection (Section 10) | Derived | Not displayed |
| Empty Percentage | % | Derived | `100 − fillPercentage (clamped)` | Derived | 2 decimals |
| Capacity Rank | enum: Low / Moderate / Good / High | Derived | Quartile of clamped fill percentage (Phase 3 §8 rank bands) | Derived | Label only |
| Alarm Level | enum: Normal / Low / Critical / Overflow / Sensor Error / Offline | Derived | Section 10 | Derived | Label + color + icon |
| Temperature | °C | API (optional field — Phase 1 open question 1, still pending confirmation) | — (passthrough) | `−10 ≤ t ≤ 80` (plausibility band, Section 5) | 2 decimals |
| Sensor Status | free-text label from API (optional) | API | — (passthrough, informational) | Non-empty string or absent; unrecognized values logged, not rejected | As received |
| Connection Status | enum: Connecting / Connected / Reconnecting / Offline | ConnectionManager | State machine (Phase 2 §6) | — | Label + badge |
| Last Update Time | timestamp | Local (`receivedAt`, when the HTTP response arrived) | — | Compared against a possible device timestamp for clock-mismatch detection (Section 13) | `hh:mm:ss`, plus relative "X ago" via `TimeManager` |

---

## 3. Sensor Input Model

The Ubidots payload may express the water level in one of three physically different ways. The application must resolve all three to the same internal representation (water height) before anything downstream (Section 4) runs.

| Input type | What it represents | Conversion to water height |
|---|---|---|
| **Water Height** | Direct height of the water column, already in meters (confirmed variable label: `waterlevel`) | None — used as-is |
| **Water Volume** | Direct volume, already in liters | `height = volume ÷ (area × 1000)` |
| **Distance From Sensor** | Distance from an ultrasonic/downward-facing sensor mounted at the tank top, down to the water surface | `height = (tankHeight − sensorMountingOffset) − distance` — `sensorMountingOffset` (default `0`, meaning flush-mounted at the true top) is a new geometry-adjacent setting, addressed below |

**How the application determines which path to use — revision from earlier phases:**

Earlier informal drafts (before Phase 1's planning-first pivot) sketched *inferring* the reading type by pattern-matching the variable's field name (e.g. "contains 'vol' → treat as volume"). This specification **replaces that approach**: the reading type is an **explicit configuration value** (`SettingsManager.readingType`, one of `Height` / `Volume` / `Distance`), set once by whoever configures the device, not re-guessed on every poll.

**Why the change:** name-sniffing is fragile — a variable literally labeled `waterlevel` could, in principle, contain either a height or a distance depending on how the device firmware was written, and a silent misclassification would produce a plausible-looking but wrong number with no error anywhere. For a system meant to run unattended for months, "wrong but confident" is a worse failure mode than "requires one extra configuration field." This also means **Section 8 of Phase 2 (Configuration Architecture) gains one new field**, carried forward as an addendum:

| New setting | Default | Validated as |
|---|---|---|
| `readingType` | `Height` (matches the confirmed `waterlevel` variable label) | enum: `Height` / `Volume` / `Distance` |
| `sensorMountingOffset` | `0` m | `≥ 0`, only meaningful/shown when `readingType = Distance` |

For the currently confirmed deployment (`waterlevel` variable, direct height), this resolves trivially — but the architecture no longer depends on that staying true if the sensor hardware ever changes.

---

## 4. Engineering Calculation Pipeline

**Geometry-derived values** (recomputed only when radius/height change, not on every poll):

```text
Radius, Height  →  Area (π × r²)  →  Maximum Volume (Area × Height × 1000)
```

**Per-poll pipeline** (runs once per validated reading):

```text
Raw reading (Height, Volume, or Distance — per configured readingType)
   ↓  (Distance path only: height = tankHeight − offset − distance)
Water Height
   ↓ (if Volume was configured, this branch is skipped — Volume is already known)
   ↓                                          ↑
Water Volume  ◄────────────────────────────────┘  (Height ↔ Volume are always
   │                                                 cross-derived so BOTH are
   │                                                 available regardless of which
   │                                                 one the API actually sent)
   ↓
Fill Percentage — raw/unclamped  =  (Volume ÷ MaximumVolume) × 100
   ↓
   ├── clamp to [0,100] → Fill Percentage (display), Empty Percentage, Capacity Rank
   └── unclamped value retained → Overflow detection (Section 10)
   ↓
Remaining Volume = max(0, MaximumVolume − Volume)
   ↓
Alarm Level evaluation (Section 10 — consumes raw percentage + validation state + sensorStatus)
```

**Rounding policy:** every step above operates on full double-precision values. Rounding to the 2-decimal display precision (Section 2) happens **exactly once**, at the moment a value is formatted for QML — never between calculation steps. Rounding early and then feeding a rounded value into the next formula would compound error across the pipeline (e.g. a height rounded to 2 decimals before computing volume introduces up to ~0.005 m of error, multiplied by the tank's cross-sectional area) — small in this application's numbers, but the discipline matters more as a stated rule than as a magnitude, per Phase 1's Objective O2.

**Clamping policy:** exactly one clamp exists in the whole pipeline — fill percentage for *display* purposes. Height and volume themselves are never clamped for display (a height of `5.15 m` against a `5.00 m` tank is shown as `5.15 m`, not silently capped) — capping the underlying number would hide the exact information an operator needs to recognize a genuine overflow. Only the *percentage* is capped, because "120% full" has no sensible visual representation in the tank graphic (Phase 3 §6).

---

## 5. Validation Rules

| Value | Valid range | Rejected as invalid | Missing field | NaN | Negative | UI reaction on rejection |
|---|---|---|---|---|---|---|
| Water Height (raw) | `0 ≤ h ≤ tankHeight × 1.10` | outside that band | whole reading rejected | rejected | covered by lower bound | last-good value persists; invalid-reading counter increments (Section 10, Sensor Error) |
| Water Volume (raw) | `0 ≤ v ≤ maxVolume × 1.10` | outside that band | whole reading rejected | rejected | covered by lower bound | same as above |
| Distance From Sensor (raw) | `0 ≤ d ≤ tankHeight × 1.10` (post-offset) | outside that band | whole reading rejected | rejected | covered by lower bound | same as above |
| Temperature | `−10°C ≤ t ≤ 80°C` | outside band | **field-level only** — rest of the reading still applies | field-level rejection, not whole-reading | covered by lower bound | last-good temperature persists, or `—` if never received |
| Sensor Status | any non-empty string | — (never rejected) | falls back to computed Alarm Level | n/a | n/a | n/a — unrecognized strings are logged for visibility, not treated as errors |
| Tank Radius / Height (config) | `> 0` and `≤ 50` | `≤ 0` or `> 50` | SettingsManager keeps prior valid value | rejected | rejected | Settings validation message; TankModel geometry unchanged until corrected |

**10% overshoot tolerance rationale:** ultrasonic sensors commonly report a small amount past the nominal tank height due to mounting position and beam spread; a reading of `102%` should be treated as a real (if concerning) Overflow signal, not silently discarded as noise. Anything beyond `110%`, by contrast, is far more likely to indicate the sensor has lost lock and returned garbage than a physically real reading — this is the dividing line between "Overflow" (Section 10) and "Sensor Error."

**Recommended hysteresis (flagged for implementation, not fully specified here):** thresholds in Section 10 sit at hard percentage boundaries; a reading oscillating by sensor noise right at a boundary (e.g. 24.9%/25.1%) could otherwise flap between Alarm Levels every poll. Implementation should apply either a small dead-band (±1%) or a minimum dwell time before downgrading severity — this is called out as an implementation detail to confirm during Phase 13, not a fully specified rule here, since it depends on real sensor noise characteristics not yet measured on the deployed hardware.

---

## 6. API Polling Lifecycle

| Phase | Behavior |
|---|---|
| **Startup polling** | First `pollNow()` fires immediately on `ApiClient.start()` — no wait for the first timer interval, so the dashboard reaches "Connected" as fast as the network allows rather than waiting a full poll interval after launch |
| **Normal polling** | Fixed interval (default 3000 ms, configurable 1000–60000 ms per Phase 2 §8), one request in flight at a time (a new poll is never issued while a previous one is still outstanding) |
| **Timeout handling** | Per-request timeout (default 5000 ms, independent of poll interval) aborts the in-flight request and is treated identically to any other network failure |
| **Retry behavior** | Exponential backoff owned by `ConnectionManager` (Phase 2 §3) — interval doubles per consecutive failure up to a capped multiple of the base interval; resets to the base interval on the next success |
| **Authentication failures (401/403)** | **Not** auto-retried — per Phase 2 §6, this is treated as a configuration problem, not a transient one; polling pauses and `ApplicationStateManager` enters `AuthenticationError` until the operator updates the token |
| **Network failures (timeout/5xx/no network)** | Auto-retried on the backoff schedule indefinitely — the system never gives up trying, consistent with Phase 1 Objective O4 |
| **Recovery** | First successful poll after any failure resets the backoff interval to base, clears the Offline/Reconnecting UI state, and clears the invalid-reading counter (Section 10) |
| **Poll interval changes** | Take effect on the **next scheduled poll**, not mid-cycle — changing the interval does not retroactively reschedule an already-pending timer, avoiding request bursts if an operator adjusts the setting rapidly |
| **Manual refresh** | `DashboardController.retryConnection()` (Phase 2 §3) issues an immediate `pollNow()` regardless of backoff state — available in both `Connected` (for a deliberate "check right now") and `Offline` (after the operator believes they've fixed the underlying issue) |

---

## 7. Data Synchronization

```text
ApiClient  →  Repository  →  TankModel  →  Q_PROPERTY  →  QML
```

**Ownership of "is this update allowed to happen," at each boundary:**

| Boundary | Who decides | On what basis |
|---|---|---|
| ApiClient → Repository | ApiClient | Did the HTTP transaction itself succeed (status code, body present, no timeout)? This is transport-level only — it does not look at whether the *numbers* make sense |
| Repository → TankModel | TankRepository | Do the numbers make physical sense (Section 5)? A transport-successful response can still be rejected here |
| TankModel → Q_PROPERTY | TankModel | Always applies a Repository-approved reading — by the time data reaches this boundary, "should this update happen" has already been fully decided upstream; TankModel's job is purely *what the update means*, not *whether* it happens |
| Q_PROPERTY → QML | Qt's binding engine | Automatic — no class in this application ever manually pushes a value into a QML element; every visual update is a reaction to a property change, never a command |

**Hard rule (restated from Phase 2 §7):** no layer is permitted to skip the one below it. ApiClient never updates TankModel directly, and QML never reads from Repository or ApiClient directly. This ownership table is the enforcement mechanism for that rule — each boundary has exactly one decision-maker, and reviewing a future pull request against this table is how the rule stays true over time rather than eroding.

---

## 8. Application Lifecycle

| Event | Objects that react | What happens |
|---|---|---|
| **Startup** | Logger, SettingsManager, DashboardController, TankModel, ApiClient | Section 1's full sequence |
| **Running (steady state)** | ApiClient (timer), TankRepository, TankModel | Section 1's "Continuous Monitoring" loop, repeating |
| **Settings changed — geometry (radius/height)** | SettingsManager → TankModel | TankModel recalculates **every** derived value immediately (area, max volume, and re-derives current height/volume/percentage against the new geometry) — even without waiting for the next poll, so the display is never showing numbers computed against stale geometry |
| **Settings changed — connection fields (URL/token/device/variables/readingType)** | SettingsManager → ApiClient, ConnectionManager | ApiClient reconfigures; any in-flight request is allowed to complete (its result is simply evaluated against the old config's expectations) rather than aborted mid-flight; ConnectionManager's failure counter resets, since this is effectively a fresh connection attempt |
| **Lost connection** | ApiClient → ConnectionManager → ApplicationStateManager → TankModel (connection state only) | Section 6/10; last-good tank values are explicitly **not** cleared |
| **Reconnection** | ConnectionManager → ApplicationStateManager, TankRepository (invalid-counter reset) | Full state restoration — Offline banner clears, backoff resets |
| **Shutdown** | ApplicationStateManager → ApiClient, Logger | `ApiClient.stop()` halts the timer and aborts any in-flight request cleanly (not left dangling); Logger flushes any buffered writes; SettingsManager's persisted state is already durable (QSettings writes are not batched in memory across the session) |

---

## 9. Operator Workflow

**Scope boundary, stated explicitly:** this is a **monitoring-only** system. There is no remote control, no valve/pump actuation, and none is planned — "Alarm" in this document means *a status the operator should notice*, never an automated response. This matters enough to state plainly given how loaded the word "alarm" is in industrial contexts.

| Scenario | What the operator sees | Available actions |
|---|---|---|
| **First launch** | Because default geometry (1.20 m / 5.00 m) and the confirmed device label/variable labels can ship pre-configured, the realistic first-launch gap is specifically the **API token** — every real Ubidots deployment needs a deployment-specific credential that cannot ship in source control. Dashboard opens in `ConfigurationError`/`AuthenticationError` until it's entered | Open Settings, enter token |
| **Normal monitoring** | Live dashboard, values update automatically | None required — this is the primary, no-interaction-needed state (Phase 1 Objective O1) |
| **Connection lost** | Offline banner, stale (but present) last-known values, de-emphasized timestamp (Phase 3 §14) | Wait for auto-retry (default), or trigger "Retry Now" if the operator knows the issue is already fixed |
| **Sensor fault** | Distinct "Sensor Error" badge (Phase 3 §8) — visually and semantically separate from "Offline," so the operator understands this is a **device-level** problem, not a network problem | No self-service fix in-app — this is a field-service signal, correctly communicated as such rather than disguised as a connectivity issue |
| **Alarm condition (Critical/Overflow)** | Persistent, non-animated status badge (Phase 3 §7 deliberately excludes attention-seeking motion); tank visualization and numbers reflect the real calculated state | None in-app (see scope boundary above) — this is a read-only notification surface |
| **Configuration update** | Editing Settings shows inline validation (Section 5); saving a geometry change recalculates the dashboard immediately (Section 8) | Edit and save; invalid values are rejected with an explanation rather than silently ignored |

---

## 10. Alarm Evaluation

**Distinction maintained from Phase 3 §8:** *Capacity Rank* (Low/Moderate/Good/High) is a purely descriptive quartile label with no operational weight — it is not part of this alarm model, is never subject to hysteresis, and simply reflects which quarter of the tank's range the clamped fill percentage falls in. *Alarm Level*, below, is the operationally meaningful signal.

| Alarm Level | Threshold | Priority (1 = highest) | Color | Icon | Trigger | Recovery condition |
|---|---|---|---|---|---|---|
| **Offline** | N/A — connection state, not a reading-based condition | 1 | `offline` (#64748B) | wifi-off | ConnectionManager reports sustained failure (Section 6) | First successful poll |
| **Sensor Error** | N/A — data-quality state | 2 | `sensorError` (#7C3AED) | sensor-warning | 3 consecutive rejected readings (Section 5's implausible-range rule) | 3 consecutive **accepted** readings (not just one — avoids flapping on a single lucky valid sample) |
| **Overflow** | Raw (unclamped) fill percentage `> 100%` | 3 | `overflow` (#991B1B) | overflow arrows | A single valid reading exceeding 100% | Raw percentage returns to `≤ 100%` |
| **Critical** | Clamped fill percentage `< 10%` | 4 | `critical` (#DC2626) | alert circle | A single valid reading below threshold | Percentage rises `≥ 10%` (plus the hysteresis note, Section 5) |
| **Low** | `10% ≤` clamped fill percentage `< 25%` | 5 | `warning` (#D97706) | warning triangle | — | Percentage leaves this band |
| **Normal** | `25% ≤` clamped fill percentage `≤ 100%` (i.e., not Overflow) | 6 (default) | `success` (#16A34A) | droplet/check | — | — |

**Precedence rule when multiple conditions are true simultaneously:** evaluated top-to-bottom in the priority column — a Sensor Error in progress is shown even if the last valid reading before it looked Critical, because a value we no longer trust cannot simultaneously be relied on to justify a specific alarm severity. Offline outranks everything, for the same reason.

**API-reported `sensorStatus` interaction:** if the API supplies a recognized, non-empty status string, it may **override** the Normal/Low/Critical classification specifically (the device may know something the geometry-only calculation can't, e.g. a firmware-level fault flag) — but it can never override Offline, Sensor Error, or Overflow, since those three are locally-detected facts about data trustworthiness or physical impossibility that no self-reported string should be able to talk the application out of.

---

## 11. Sequence Diagrams

**Successful API update**
```text
Timer → ApiClient.pollNow() → Ubidots (200 OK + JSON)
  → ApiClient decodes → ConnectionManager.reportSuccess()
  → Repository validates (Section 5) → valid
  → Repository.validatedReadingChanged(...)
  → TankModel recalculates (Section 4) → Q_PROPERTY updates
  → QML re-renders, animations run (Phase 3 §7)
```

**Invalid API response** *(malformed JSON or missing required field)*
```text
Timer → ApiClient.pollNow() → Ubidots (200 OK, but body fails to parse / missing field)
  → ApiClient logs parse/shape error → ConnectionManager.reportFailure(category=DataError)
  → Repository is never reached (ApiClient couldn't produce a raw reading at all)
  → TankModel unchanged — last-good values remain on screen
  → scheduleRetry() (backoff)
```

**Network timeout**
```text
Timer → ApiClient.pollNow() → request sent, no response within timeout
  → ApiClient aborts in-flight request → ConnectionManager.reportFailure(category=Timeout)
  → TankModel unchanged
  → scheduleRetry() (backoff increases)
```

**Configuration change** *(e.g. operator updates poll interval or token)*
```text
Settings UI → SettingsManager.setX(value) → validated → persisted → SettingsManager.xChanged()
  → ApiClient reconfigures (Section 8) → ConnectionManager failure counter resets
  → next scheduled poll uses new configuration
```

**Geometry change** *(radius or height edited)*
```text
Settings UI → SettingsManager.setTankRadius/Height(value) → validated → persisted
  → SettingsManager.tankRadiusChanged/tankHeightChanged
  → TankModel.recalculateGeometry() (Phase 2 §3)
  → Area, MaximumVolume recomputed
  → current height/volume re-derived against new geometry (Section 4)
  → Q_PROPERTY updates → QML reflects new numbers immediately, without waiting for the next poll
```

**Application startup**
```text
main() → Logger init → SettingsManager loads/validates
  → DashboardController constructs ApiClient/ConnectionManager/TankRepository
  → TankModel singleton resolved, geometry applied
  → QML loaded (dashboard shows "Waiting for Live Data")
  → ApiClient.start() → immediate pollNow()
  → (continues into "Successful API update" or a failure sequence above)
```

---

## 12. Data Contracts

Field meanings only — no type syntax, no class definitions.

**ApiClient → Repository ("Raw Reading" event)**
| Field | Meaning |
|---|---|
| `success` | Did the HTTP transaction complete with a parseable body? |
| `readingType` | Echo of the configured type (Height/Volume/Distance) — Repository uses this to know which validation band (Section 5) applies |
| `rawValue` | The single numeric measurement, in whatever unit `readingType` implies |
| `temperature` | Optional; absent if the API didn't include it this cycle |
| `sensorStatus` | Optional; absent if not provided |
| `deviceTimestamp` | Optional; the API's own reported time for this measurement, if present |
| `receivedAt` | Always present; local wall-clock time the response arrived — the fallback and cross-check for `deviceTimestamp` (Section 13) |
| `errorCategory` / `errorDetail` | Present only when `success` is false — used for logging and ConnectionManager, never shown verbatim to the operator |

**Repository → TankModel ("Validated Reading" event)**
| Field | Meaning |
|---|---|
| `waterHeight` | Already resolved to meters, regardless of what `readingType` originally was — this is the one place the three input shapes (Section 3) converge into a single representation |
| `temperature` | Passed through if it was present and within the plausibility band (Section 5); otherwise absent |
| `sensorStatus` | Passed through if present |
| `receivedAt` | Passed through unchanged |
| `rejected` / `rejectionReason` | Present when a reading fails validation — this event still fires (so TankModel/ConnectionManager know a poll happened) even though `waterHeight` will be absent |

**TankModel → QML (`Q_PROPERTY` set)**
Every field from Section 2's table, exposed read-only. The meaning of each is fully specified there; nothing is added at this boundary beyond formatting-readiness (raw doubles, ready for QML's own `toFixed`-equivalent display formatting per Phase 3's precision rule).

---

## 13. Failure Scenarios

| Scenario | Detection | Recovery | User notification | Logging |
|---|---|---|---|---|
| **Invalid geometry at load** (corrupt/zero settings) | SettingsManager validation on startup read | Fall back to safe defaults (1.20 m / 5.00 m) | Persistent banner/dialog: "Using default tank geometry — please verify in Settings" (never silent) | Warning |
| **Missing API fields** | Repository field-presence check | Reading rejected, last-good retained | None per single event (would be noisy); escalates to Sensor Error badge after 3 consecutive (Section 10) | Warning, rate-limited: full detail on first occurrence, summarized thereafter |
| **Sensor returns impossible values** | Range validation (Section 5) | Rejected, counted toward Sensor Error | Sensor Error badge after threshold | Warning → Error if sustained |
| **API authentication failure** | HTTP 401/403 | Polling paused, not retried (Section 6) | Blocking dialog, action → Settings | Error |
| **Corrupt configuration file** | QSettings read failure | Fall back to defaults for the affected values only (not a full reset) | Dialog naming which settings were reset | Error |
| **Clock mismatch** (local vs. `deviceTimestamp` skew) | Compare `receivedAt` vs `deviceTimestamp` when both present; flag if skew `> 5 min` | Data still displayed (this doesn't affect safety) | Subtle diagnostic note only — not blocking, since "last updated" *relative* accuracy is a minor observability concern, not a tank-state concern | Warning, once per session (not repeated every poll) |
| **Repeated network failures** | ConnectionManager consecutive-failure count | Backoff continues indefinitely — never stops retrying (Phase 1 O4) | Offline banner persists | Rate-limited: Warning on first failure, periodic summary thereafter, not one line per retry |

---

## 14. Performance & Reliability

| Metric | Target | Basis |
|---|---|---|
| Maximum acceptable API latency | Bounded by configured timeout (default 5000 ms) | Beyond this, treated as a failure regardless of whether a response eventually arrives (Section 6) |
| Maximum update delay to UI | < 1 frame (~16 ms) after a `Q_PROPERTY` change | Qt's binding engine re-evaluates synchronously within the same event-loop iteration (Phase 2 §5, §10) |
| Engineering calculation time | Sub-microsecond per poll | `VolumeCalculator` is O(1) arithmetic on plain doubles (Phase 2 §3) — not a measurable factor at this scale |
| Memory constraints | Flat over time | No unbounded buffers in v1 (no history retention yet — Phase 12); Logger's rotation policy (Phase 2 §9) bounds log growth |
| Logging overhead | Negligible in Release | Level-filtered before message construction, so Trace/Debug-level calls cost nothing when the threshold excludes them |
| Polling efficiency | One request in flight, ever | The `pollNow()` guard (Section 6) prevents request pile-up if the network is slow, independent of the configured interval |

**24/7 suitability confirmation:** every mechanism above was chosen specifically to avoid the two failure modes that actually break long-running desktop applications — unbounded memory growth and a UI thread that can be blocked by a slow network call. Neither is possible in this design: all networking is asynchronous (Phase 2 §10), and nothing in v1 accumulates data without a rotation/bound.

---

## 15. Engineering Review

- **Robustness:** validation actively rejects implausible data (Section 5) rather than displaying it — a system that shows a wrong-but-plausible number is more dangerous than one that visibly says "Sensor Error."
- **Maintainability:** the calculation pipeline (Section 4) remains fully isolated in `VolumeCalculator`, unaffected by this phase's Distance-reading addition — that conversion happens *before* the pipeline, in the Repository→TankModel boundary (Section 12), exactly where Phase 2's layering says data-shape concerns belong.
- **Scalability:** unchanged caveat from Phase 2 §14 — `TankModel`/`TankRepository` are still v1 singletons; multi-tank support remains a scoped, contained refactor, not a redesign.
- **Safety:** explicitly monitoring-only (Section 9) — no control-system surface exists to secure or fail-safe, which is itself a safety-relevant simplification worth keeping true in every future phase.
- **Accuracy:** double precision throughout, single rounding point at display (Section 4) — directly serves Phase 1's Objective O2 (calculated, not API-percentage-driven, correctness).
- **Future MQTT support:** unaffected — an `MqttClient` would produce the same "Raw Reading" contract (Section 12) that `ApiClient` does today, and everything downstream of that boundary is already transport-agnostic.
- **Multiple tanks:** same scoped caveat as above; this phase doesn't make it easier or harder than Phase 2 already assessed.
- **Historical logging:** the "Validated Reading" event (Section 12) is precisely the subscription point a future `HistoryManager` needs — this phase confirms that seam is real and sufficient, not just asserted.

**Completeness for implementation:** yes, with one addendum to carry forward — `readingType` and `sensorMountingOffset` (Section 3) are new `SettingsManager` fields not present in Phase 2 §8's original list, and should be folded into that table when implementation begins.

---

Phase 4 is complete. Holding here, per the phase-gated process, for your review before proceeding.
