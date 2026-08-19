# Panorama Water Tank Monitor — Qt/QML Desktop Application
## Phase 6: Development Workflow & Incremental Implementation Roadmap

**Status:** Draft for review. Phase 5 (Implementation Blueprint) is approved. This document introduces no new architecture — every module, class, property, and threshold referenced below is defined in Phases 1–5 and is only being organized into an execution plan here.
**No code in this document.**

---

## 1. Overall Development Strategy

**Incremental development:** the build order follows Phase 5 §18's dependency graph exactly — each module is implemented, compiled, and unit-tested before the next module that depends on it begins. No module is ever started against an unfinished dependency.

**Feature-driven implementation:** one Git feature branch per module or component (Section 3), each ending in a working, tested, mergeable increment — never a multi-module branch that can't be independently verified.

**Continuous compilation:** CI builds on every push to a feature branch, not just on merge to `develop`. For a Qt/CMake project with a dozen interdependent classes, a break is far cheaper to fix the same day it's introduced than after several more modules have been layered on top of it.

**Continuous testing:** Phase 5 §17's per-class test suite runs in CI on every push, not only before release. A module is not "done" until its own tests are green in CI, not just green locally.

**Modular integration:** two unfinished modules are never integrated together. A module is only ever integrated once it independently passes its own test suite — this is what makes Section 8's mock-first sequencing possible at all.

**Risk reduction:** the zero-dependency modules (`Logger`, `VolumeCalculator`, `TimeManager` — Phase 5 §18) are built first specifically because they carry the least risk and the most reuse value, so the team's process is proven on low-stakes work before the highest-risk module (`ApiClient`, the only module touching real external infrastructure) is attempted.

**Parallel development opportunities:** the single largest parallelization opportunity in this roadmap is that QML component visual work (Section 6) requires only Design Tokens and `TankModel`'s default/static values — it does not require `ApiClient` or live data. This means Milestone 5 (Networking) and Milestone 6 (QML Components) can run **concurrently**, either as two developers working in parallel or one developer alternating focus without blocking either track. This is expanded in Section 16.

**Code review strategy:** every pull request is checked against Phase 5 §15's directory dependency rules specifically — "does this PR introduce a forbidden dependency" is a standing review checklist item, not a judgment call left to reviewer memory.

**Why this methodology fits a Qt/QML industrial desktop application specifically:** the layered architecture from Phase 2 maps directly onto independently testable Git branches, which is exactly what continuous integration wants structurally. More importantly, the failure mode this application must avoid — a subtly wrong number displayed with total confidence — is worse than a slower rollout, so continuous testing at every increment is weighted more heavily here than raw development speed would be for, say, a consumer prototype.

---

## 2. Master Development Roadmap

```text
Development Environment
        ↓
Project Skeleton
        ↓
Backend Infrastructure        (Logger, SettingsManager, TimeManager)
        ↓
Engineering Layer             (VolumeCalculator)
        ↓
Repository Layer              (TankModel, TankRepository)
        ↓
Network Layer                 (ApiClient, ConnectionManager — against a mock server)
        ↓
State Management              (ApplicationStateManager, DashboardController)
        ↓
QML Foundation                (Design Tokens, Theme, ApplicationWindow shell)
        ↓
Reusable Components           (Header, Sidebar, cards, badges — Section 6)
        ↓
Tank Visualization            (TankView, TankScale, WaterFill, WaterSurface)
        ↓
Dashboard Assembly            (full DashboardPage composed)
        ↓
Backend–Frontend Integration  (wiring, still against the mock server)
        ↓
Real API Integration          (swap mock for live Ubidots endpoint)
        ↓
Testing                       (full Section 9 workflow)
        ↓
Optimization                  (performance pass, Section 12)
        ↓
Packaging                     (installers, Section 14)
        ↓
Release
```

**Mapping to Phase 5 §19's milestones:**

| Roadmap stage | Phase 5 milestone | Key modules |
|---|---|---|
| Development Environment, Project Skeleton | M1 | CMake, blank window |
| Backend Infrastructure | M2 | Logger, SettingsManager, TimeManager |
| Engineering Layer | M3 | VolumeCalculator |
| Repository Layer | M4 | TankModel, TankRepository (fake source) |
| Network Layer | M5 | ApiClient, ConnectionManager (mock server) |
| State Management | M5–M6 bridge | ApplicationStateManager, DashboardController |
| QML Foundation, Reusable Components, Tank Visualization | M6 | All 20 QML components (Phase 5 §8) |
| Dashboard Assembly | M7 | DashboardPage |
| Backend–Frontend Integration | M7–M8 bridge | Full wiring, still against mock |
| Real API Integration | M8 | Live Ubidots endpoint |
| Testing | M9 | Full Phase 5 §17 suite |
| Optimization, Packaging, Release | M10 | Soak test, installers |

**Reasoning behind this order:** it is Phase 5 §18's dependency graph, unrolled into a single linear sequence with one deliberate insertion — **Real API Integration is placed after, not during, Backend–Frontend Integration**, which is the mock-first principle explained fully in Section 7. Every other ordering decision (engineering before repository, repository before network, backend before QML, static QML before animated QML) is a direct restatement of dependencies already fixed in Phase 5 — nothing here is a new sequencing decision.

---

## 3. Git Workflow

**Branch structure:**

| Branch | Purpose |
|---|---|
| `main` | Always production-ready; only receives merges from `release/*` or `hotfix/*` |
| `develop` | Integration branch; always compiles and passes its test suite; receives merges from `feature/*` |
| `feature/*` | One module or component per branch; branched from and merged back into `develop` |
| `release/*` | Cut from `develop` at Milestone 9 completion; only bugfixes land here, no new features |
| `hotfix/*` | Cut from `main` for a post-release production fix; merged back into both `main` and `develop` |

**Feature branch naming** (mirrors Phase 5's class/component list exactly, so a branch name and a Phase 5 section heading are always the same term):

```text
feature/logger
feature/settings-manager
feature/time-manager
feature/volume-calculator
feature/tank-model
feature/tank-repository
feature/connection-manager
feature/api-client
feature/application-state-manager
feature/dashboard-controller
feature/qml-design-tokens
feature/header
feature/sidebar
feature/navigation-item
feature/tank-card
feature/tank-view
feature/tank-scale
feature/water-fill
feature/water-surface
feature/tank-statistics
feature/status-badge
feature/information-card
feature/info-row
feature/rank-indicator
feature/rank-segment
feature/statistic-card
feature/footer
feature/loading-overlay
feature/offline-overlay
feature/error-dialog
feature/dashboard-assembly
feature/mock-api-server
feature/integration
feature/testing-suite
```

**Commit strategy:** small, single-purpose commits using a conventional-commits-style prefix — `feat:`, `fix:`, `test:`, `docs:`, `refactor:` — each referencing the module it touches (e.g. `feat(tank-model): implement fillPercentage clamping per Phase 4 §4`).

**Pull request workflow:** every `feature/*` branch opens a PR into `develop` requiring: (1) green CI (compile + that module's Phase 5 §17 tests), (2) one reviewer approval checked against Phase 5 §15's directory rules, (3) the relevant row(s) of Phase 5 §1's class contract or §8's component contract satisfied in full — a PR implementing only part of a class's specified interface is not mergeable.

**Code review process:** reviewer checklist —
- Does this PR add a dependency forbidden by Phase 5 §15?
- Does the implemented public interface match Phase 5 §1/§2/§8 exactly (no undocumented properties, no missing ones)?
- Are the specified unit tests (Phase 5 §17) present and passing?
- For QML components: does it match Phase 3's design tokens rather than a locally hardcoded value?

**Merge strategy:** squash-merge `feature/*` → `develop` (one clean commit per completed module); merge-commit `develop` → `release/*` (preserves milestone-level granularity for later audit); merge-commit `release/*` → `main` at the actual release point.

**Version tagging:** semantic versioning `vMAJOR.MINOR.PATCH`. Release candidates are tagged on the `release/*` branch as `vX.Y.0-rc.N`; the final tag (`vX.Y.Z`) is applied on `main` only, at the exact commit merged from `release/*` (Section 14).

---

## 4. Development Milestones

Each milestone below expands Phase 5 §19's completion criteria with the full field set requested.

### Milestone 1 — Project Skeleton
- **Objective:** a compiling, launchable, empty shell.
- **Modules implemented:** CMake configuration only; no backend classes yet.
- **Dependencies:** none.
- **Expected deliverables:** `CMakeLists.txt`, `main.cpp`, an empty `Main.qml`.
- **Expected screenshot:** a blank `ApplicationWindow` at the correct default size (1536×1024) with the correct background color (`#F4F6F9`).
- **Expected backend functionality:** none.
- **Unit tests required:** none.
- **Integration tests required:** none.
- **Definition of Done:** `cmake --build` succeeds with zero warnings; the app launches and exits cleanly.
- **Exit criteria:** Section 10's checklist passes at skeleton scope.

### Milestone 2 — Core Backend
- **Objective:** the three zero/near-zero-dependency utility classes, fully working and tested.
- **Modules implemented:** Logger, SettingsManager, TimeManager.
- **Dependencies:** M1.
- **Expected deliverables:** all three classes per Phase 5 §1's contracts.
- **Expected screenshot:** none (no UI change yet).
- **Expected backend functionality:** a log file is created in the correct format; settings persist across a restart; the clock/date values are correctly formatted.
- **Unit tests required:** Phase 5 §17's Logger and SettingsManager test rows in full.
- **Integration tests required:** none yet (these three don't interact with each other).
- **Definition of Done:** all three pass their unit tests in CI; a manual restart-and-verify persistence check passes.
- **Exit criteria:** Section 10's checklist.

### Milestone 3 — Engineering Layer
- **Objective:** `VolumeCalculator` fully correct.
- **Modules implemented:** VolumeCalculator.
- **Dependencies:** none (parallelizable with M2, per Phase 5 §18).
- **Expected deliverables:** all six functions from Phase 5 §5.
- **Expected screenshot:** none.
- **Expected backend functionality:** correct results for the reference geometry (`r=1.20, h=5.00 → area≈4.5239 m², maxVolume≈22619.47 L`).
- **Unit tests required:** 100% of Phase 5 §17's VolumeCalculator cases, including boundary/NaN/negative/overflow.
- **Integration tests required:** none (pure function).
- **Definition of Done:** all tests green; the sub-microsecond performance benchmark (Phase 5 §17) passes as a regression guard.
- **Exit criteria:** Section 10's checklist.

### Milestone 4 — Data Layer
- **Objective:** `TankModel` and `TankRepository` correct against a **fake** reading source.
- **Modules implemented:** TankModel, TankRepository.
- **Dependencies:** M2 (SettingsManager, Logger), M3 (VolumeCalculator).
- **Expected deliverables:** both classes per Phase 5 §1/§2's full contracts.
- **Expected screenshot:** none.
- **Expected backend functionality:** feeding the 5 synthetic scenarios (Phase 5 §19 M4) produces hand-verified correct `Q_PROPERTY` values; geometry changes trigger full recalculation (Phase 4 §11).
- **Unit tests required:** Phase 5 §17's TankModel and TankRepository rows.
- **Integration tests required:** fake-source → TankRepository → TankModel, full pipeline.
- **Definition of Done:** all 5 synthetic scenarios pass; the Overflow raw/clamped split (Phase 4 §4) is verified explicitly.
- **Exit criteria:** Section 10's checklist.

### Milestone 5 — Networking
- **Objective:** `ApiClient` and `ConnectionManager` correct — first against a mock server, then verified against the real device.
- **Modules implemented:** ApiClient, ConnectionManager, plus the mock API server itself (`feature/mock-api-server`).
- **Dependencies:** M2 (SettingsManager, Logger).
- **Expected deliverables:** both classes per Phase 5 §1/§3's contracts; a standalone local mock HTTP server reproducing Phase 4 §12's contract, including deliberately bad responses.
- **Expected screenshot:** none (headless).
- **Expected backend functionality:** correct categorization of `200`/`401`/`403`/`404`/`500`/timeout/malformed-JSON against the mock; a subsequent, deliberately cautious pass against the real Ubidots endpoint (Section 7).
- **Unit tests required:** ConnectionManager's backoff math in isolation.
- **Integration tests required:** ApiClient ↔ mock server, full matrix of response types.
- **Definition of Done:** every failure category produces the exact `errorCategory` specified in Phase 5 §3; a memory-profiler pass specifically checks `QNetworkReply` lifecycle (Section 15's Memory Leaks risk).
- **Exit criteria:** Section 10's checklist, plus a clean profiler pass.

### Milestone 6 — QML Components
- **Objective:** every component from Phase 5 §8 built and visually correct in isolation. **Runs concurrently with Milestone 5** (Section 16).
- **Modules implemented:** all 20 QML components, per Section 6's internal ordering.
- **Dependencies:** M4 for data-shaped default values; does **not** depend on M5.
- **Expected deliverables:** every `.qml` file listed in Phase 5 §8.
- **Expected screenshots:** each component individually, reviewed against Phase 3's reference screenshot.
- **Expected backend functionality:** none new — components bind to `TankModel`'s existing (M4) default/static values.
- **Unit tests required:** none (QML components are visually reviewed, not unit-tested in v1 — see Section 9).
- **Integration tests required:** none yet (Dashboard Assembly, M7, is the integration point).
- **Definition of Done:** every component matches its Phase 5 §8 property/signal contract and Phase 3's visual tokens exactly.
- **Exit criteria:** visual review sign-off per component.

### Milestone 7 — Dashboard Assembly
- **Objective:** the full `DashboardPage`, composed and laid out correctly, still against static/default data.
- **Modules implemented:** DashboardPage.qml, Main.qml shell, Sidebar navigation wiring.
- **Dependencies:** M6.
- **Expected deliverables:** the complete dashboard layout.
- **Expected screenshot:** full-dashboard comparison against the Phase 3 reference at 1600×900 (baseline) plus one narrow (1366×768) and one wide (2560×1440) check per Phase 3 §9.
- **Expected backend functionality:** none new.
- **Unit tests required:** none.
- **Integration tests required:** layout-only integration (components compose without overlap/clipping at all three tested resolutions).
- **Definition of Done:** matches Phase 3 §4's layout spec at all tested resolutions.
- **Exit criteria:** Section 10's checklist, UI-scope items.

### Milestone 8 — Integration
- **Objective:** full live wiring — mock first, then real Ubidots — per Section 8's integration sequence.
- **Modules implemented:** ApplicationStateManager, DashboardController wiring; all Behavior/animation blocks (added last, per Section 6).
- **Dependencies:** M5, M7.
- **Expected deliverables:** a fully live dashboard.
- **Expected screenshot:** the live dashboard, values cross-checked against the existing React dashboard for the same device.
- **Expected backend functionality:** end-to-end live polling, correct calculated values, correct state transitions for every Phase 2 §6 app state.
- **Unit tests required:** none new (all prior suites still green — Section 9's regression policy).
- **Integration tests required:** full Phase 4 §11 sequence diagrams reproduced against the live device.
- **Definition of Done:** live values match the independent React-dashboard cross-check within expected rounding tolerance.
- **Exit criteria:** Section 10's checklist, full scope.

### Milestone 9 — Testing & Hardening
- **Objective:** every Phase 4 §13 failure scenario deliberately triggered and confirmed correct.
- **Modules implemented:** none new — this milestone is verification only.
- **Dependencies:** M8.
- **Expected deliverables:** a test execution report (Section 13).
- **Expected screenshot:** each error/overlay state (Loading, Offline, AuthenticationError, ConfigurationError) captured on demand.
- **Expected backend functionality:** unchanged, verified.
- **Unit tests required:** full Phase 5 §17 suite, one final complete run.
- **Integration tests required:** every Phase 4 §13 scenario, manually triggered (pull the network cable, use a wrong token, enter invalid geometry, etc.).
- **Definition of Done:** every scenario's specified UI behavior (Phase 4 §13's table, column by column) is confirmed.
- **Exit criteria:** Section 10's checklist plus a signed-off failure-scenario matrix.

### Milestone 10 — Production Release
- **Objective:** a 24-hour soak-verified, packaged, released build.
- **Modules implemented:** none new — packaging and release process only.
- **Dependencies:** M9.
- **Expected deliverables:** platform installer(s), release notes, tagged `main`.
- **Expected screenshot:** the installed, first-launched application on a clean machine.
- **Expected backend functionality:** unchanged, soak-verified.
- **Unit tests required:** full suite, run once more against the exact release-candidate build.
- **Integration tests required:** deployment verification (Section 14).
- **Definition of Done:** Section 14's full release checklist.
- **Exit criteria:** stakeholder sign-off (Section 9's User Acceptance stage).

---

## 5. Backend Module Implementation Order

| Module | Purpose | Dependencies | Prerequisites | Complexity | Testing strategy | Expected output |
|---|---|---|---|---|---|---|
| **Logger** | Centralized logging | None | M1 | Low | Unit: level filtering, rotation (Phase 5 §17) | A working, categorized log file |
| **SettingsManager** | Persisted configuration | Logger | Logger built | Low-Medium | Unit: validation + persistence round-trip | Settings survive a restart |
| **TimeManager** | Clock source | None | M1 | Low | Unit: formatting correctness | Correctly formatted time/date strings |
| **VolumeCalculator** | Pure engineering math | None | None (parallel with above) | Medium | Unit: full Phase 5 §5 case set | Verified-correct geometry math |
| **TankGeometry** *(not a separate class — see note)* | — | — | — | — | — | — |
| **TankModel** | QML-facing state | VolumeCalculator, SettingsManager | Both built | Medium-High | Unit: 5 synthetic scenarios (M4) | Correct `Q_PROPERTY` set under all input paths |
| **TankRepository** | Validation/caching seam | SettingsManager, VolumeCalculator, Logger | SettingsManager, VolumeCalculator built | Medium | Unit: boundary validation; integration with fake source | Correct accept/reject decisions |
| **ConnectionManager** | Connection health + backoff | Logger | Logger built | Low-Medium | Unit: backoff math | Correct state transitions under simulated failure |
| **ApiClient** | HTTP transport | ConnectionManager, SettingsManager, Logger | ConnectionManager built | High (highest-risk module) | Integration: mock server matrix (M5) | Correct categorization of every response type |
| **DashboardController** | Composition root | All of the above | All of the above built | Low (mostly wiring) | Integration: smoke test (startup completes) | Full backend object graph constructed correctly |
| **ApplicationStateManager** | App-level state machine | ConnectionManager, TankRepository | Both built | Medium | Unit: full transition table | Correct state for every input combination |

**Note on "TankGeometry":** this is not a distinct class in the approved architecture (Phase 2 §3, Phase 5 §1). The geometry properties and their validation live in `TankModel`'s Geometry property group (Phase 5 §2), backed by `VolumeCalculator`'s pure functions. It is listed here only to confirm explicitly that no new class is being introduced to cover it — the row is a pointer, not a module.

**Dependency graph** (restated from Phase 5 §18, unchanged):

```text
Logger ──┬─────────────────────────────► (everything logs)
         ▼
SettingsManager
         ├──────────────► VolumeCalculator  (parallel branch)
         │                      │
         │                      ▼
         ├──────────────► TankModel
         ▼                      ▼
TankRepository ◄────────────────┘
         ▼
ApiClient / ConnectionManager
         ▼
ApplicationStateManager
         ▼
DashboardController
```

---

## 6. QML Development Workflow

```text
Design Tokens
        ↓
Theme
        ↓
ApplicationWindow
        ↓
Header
        ↓
Sidebar
        ↓
Cards          (TankCard shell, InformationCard shell, StatisticCard shell — structure only)
        ↓
Tank Components (TankView, TankScale, WaterFill, WaterSurface, TankStatistics, StatusBadge)
        ↓
Rank Indicator
        ↓
Statistic Cards (populated)
        ↓
Dashboard Assembly
        ↓
Animations
        ↓
Overlays        (LoadingOverlay, OfflineOverlay, ErrorDialog)
```

**Why this sequence minimizes rework:**
- **Tokens and Theme first:** every component styled after this point reads from stable, named values (Phase 3 §3) — nothing gets re-skinned later because nothing was ever hardcoded to begin with.
- **Structural shell before content:** `ApplicationWindow`/`Header`/`Sidebar` establish the layout constraints (Phase 3 §4) that cards must fit inside — building cards first would risk sizing them against assumptions the shell later invalidates.
- **Static content before animation:** every card, the tank visualization, and the rank indicator are built and visually verified as **static** components first. Animating a component that isn't yet correctly laid out means debugging layout and motion simultaneously — this order guarantees layout is already correct before a single `Behavior{}` block is written.
- **Overlays last:** `LoadingOverlay`/`OfflineOverlay`/`ErrorDialog` only make sense relative to a dashboard that already exists to overlay — building them earlier would mean testing them against nothing.

---

## 7. API Integration Workflow

**Sequence:**
1. **Mock API development** — a local HTTP stub reproducing Phase 4 §12's exact contract, including deliberately malformed/erroring responses.
2. **JSON parser validation** — `ApiClient`'s decode logic unit-tested against the mock's known payloads, before any real network call is attempted.
3. **Repository integration** — `TankRepository` validated against the same mock, now exercising Phase 4 §5's exact boundary conditions on demand (e.g. instructing the mock to return exactly `110.01%` to test the reject boundary — something a real sensor cannot be asked to do on cue).
4. **Backend model updates** — `TankModel` exercised end-to-end, still entirely against the mock.
5. **Live Ubidots integration** — only once the full pipeline is proven correct against the mock does `ApiClient` get pointed at the real `apiBaseUrl`, initially with a conservative poll interval to avoid unnecessary load on the real device while any last real-world surprise (an unexpected extra field, a slightly different value type) is ironed out.
6. **Error simulation** — continues indefinitely against the mock, even after live integration begins, as a permanent regression asset (real API failures are rare and hard to trigger on demand).
7. **Failure recovery testing** — same, ongoing.
8. **Performance validation** — timing measured against both the mock (a consistent baseline) and the live endpoint (real-world numbers) for comparison.

**Why a mock API must come first — do not begin development with the live production API:**
- **Failure paths are difficult or impossible to trigger on demand against a real, working device.** A `401`, a malformed body, or a timeout can be produced instantly and repeatably by a mock; waiting for a real one to happen naturally is not a viable test strategy.
- **Unnecessary load on the real device during early, iterative debugging is avoidable and should be avoided** — a tight debug loop restarting the app dozens of times an hour has no reason to hit real infrastructure each time.
- **Decouples backend/UI development from live hardware availability** — a developer can complete Milestones 4–8's mock-dependent work with zero dependency on the actual ESP32-S3/Ubidots deployment being reachable at that exact moment.

---

## 8. Backend–Frontend Integration Strategy

**Registration of backend classes:** `TankModel` and `TimeManager` register as QML singletons (`QML_ELEMENT` + `QML_SINGLETON`, auto-scanned by `qt_add_qml_module` per Phase 5 §16's build pattern). `ApiClient`, `TankRepository`, `ConnectionManager`, `SettingsManager`, `ApplicationStateManager`, and `DashboardController` are **never** registered for QML at all except `DashboardController`'s narrow, intentional QML-facing surface (`currentPage`, `navigateTo`, `retryConnection` — Phase 5 §1) — this is a restated hard rule from Phase 4 §7, not a new decision.

**Q_PROPERTY bindings:** implementation work here is literally wiring each row of Phase 5 §10's binding matrix — the matrix is the task list, not a reference to consult after the fact.

**Signals and slots:** same relationship to Phase 5 §11's matrix.

**Model updates:** `TankModel.applyReading` gets its final *live* wiring only at Milestone 8, following Section 7's mock-first sequence — it is wired against the mock well before that, at Milestone 4.

**Animation triggers:** `Behavior{}` blocks are attached only to already-built, already-visually-verified static components (Section 6's ordering) — never built simultaneously with a component's first draft.

**State updates:** `ApplicationStateManager.currentState` is wired to `LoadingOverlay`/`ErrorDialog` visibility only once `ApplicationStateManager` independently passes its Phase 5 §17 transition-table tests.

**Error propagation:** verified specifically via the mock API's error-simulation modes (Section 7) before ever relying on a real failure to prove the UI reacts correctly — a real failure is a confirmation, not the first test.

**Integration sequence** (the literal order engineers wire things together, each step verified before the next begins):
```text
1. Register TankModel/TimeManager singletons
   → verify QML reads their default, unconfigured values (compiles, blank dashboard shows defaults)
2. Wire SettingsManager → TankModel geometry
   → verify a Settings change visibly recalculates the still-static dashboard
3. Wire mock ApiClient → TankRepository → TankModel
   → verify the dashboard updates end-to-end from mock data
4. Wire ApplicationStateManager → Loading/Error overlays
   → verify each app state visually, one at a time
5. Swap mock ApiClient configuration for the live Ubidots endpoint (Milestone 8)
   → verify against the real device
6. Add animations last (Section 6)
```

---

## 9. Testing Workflow

| Testing type | Occurs at | Entry criteria | Exit criteria |
|---|---|---|---|
| **Unit Testing** | Continuous, from M2 onward | Module code exists | 100% of that module's Phase 5 §17 cases pass |
| **Component Testing** (QML, visual) | M6 | Component + tokens exist | Matches Phase 3 reference visually |
| **Integration Testing** | M4 (fake source), M5 (mock server), M8 (live device) | Constituent units individually pass unit tests | Phase 4 §11's sequence diagrams reproduced successfully |
| **Mock API Testing** | M5 onward, permanently | Mock server built | Never fully exits — remains a permanent regression asset |
| **UI Testing** | M6–M7 | Components exist | Layout matches Phase 3 §4 at every Phase 3 §9 breakpoint |
| **Performance Testing** | M9 | Feature-complete build | Phase 4 §14 targets met (Section 12) |
| **Memory Leak Testing** | M5 (early pass on ApiClient specifically), M9–M10 (full) | Feature-complete build (full pass) / ApiClient complete (early pass) | Flat memory profile over the tested run |
| **Regression Testing** | Every milestone from M4 onward | New code merged | Zero previously-passing test now failing |
| **Soak Testing** | M10 | All other stages passed | 24-hour clean run (72-hour recommended, not mandatory — Section 12) |
| **User Acceptance Testing** | M10, after soak | Soak test passed | Operator confirms live values match reality and the Phase 4 §9 workflow feels right |

---

## 10. Continuous Verification Checklist

Run after **every** milestone, not only at project end:

- [ ] Project compiles successfully — zero errors
- [ ] Application launches without crash
- [ ] Zero compiler warnings (CI treats warnings as errors)
- [ ] No runtime crashes during a basic smoke pass
- [ ] All unit tests for modules completed so far pass
- [ ] Memory usage within expected bounds for this milestone's scope
- [ ] Logging operational — file created, correctly formatted, correct category/level
- [ ] Settings persistence verified — change → restart → value retained
- [ ] API behavior validated against the mock server's known scenarios
- [ ] UI responsive — no visible jank interacting with sidebar/scroll/hover at this milestone's UI scope

---

## 11. Debugging Workflow

**Recommended order — always work backward from the visible symptom toward the earliest point of failure in Phase 5 §13's initialization sequence,** since most "the dashboard shows a wrong number" reports actually originate several steps earlier than where the symptom is visible:

1. **Startup failures** — check Application-category log output first (Logger initializes first, so it has caught the earliest possible failure); cross-reference against Phase 5 §13's exact expected order.
2. **Configuration issues** — SettingsManager's validation log entries, Application category.
3. **API failures** — API + Network category logs; cross-reference against the mock server's expected response for the same request to isolate a real-vs-mock behavioral difference.
4. **JSON parsing errors** — verify the payload field-by-field against Phase 4 §12's contract, using a raw HTTP inspection tool alongside the console.
5. **Engineering calculation errors** — Calculation-category logs, cross-checked against Phase 5 §17's hand-verified expected outputs for the same inputs.
6. **QML binding errors** — Qt/QML's own console warnings for broken bindings; Debug builds run with binding-diagnostic tooling (e.g. `qmllint`-style checks) enabled.
7. **Animation issues** — verify the underlying bound property is actually changing correctly **before** suspecting the animation itself; Phase 5 §10's binding matrix is the reference for what should be driving a given animation.
8. **Memory leaks** — a memory profiler (e.g. Qt Creator's built-in profiler, or Valgrind on Linux), run specifically during the M9–M10 soak window rather than continuously, since profiling overhead itself skews normal operation.
9. **Connection problems** — ConnectionManager's Network-category logs, confirmed against Section 7's mock-based regression suite before assuming it's a genuine network issue.
10. **Performance bottlenecks** — the QML Profiler specifically for animation/binding-evaluation cost; a general CPU profiler for anything outside QML.

---

## 12. Performance Validation Workflow

| Metric | Threshold | Validation method |
|---|---|---|
| Startup time | < 2s | Timestamp log line at Logger-init vs. first-QML-frame |
| CPU usage (idle between polls) | Negligible | OS-level process monitor, M9 |
| Memory consumption | Flat over 24h | Periodic samples logged to CSV during the M10 soak, graphed post-hoc |
| API latency | Bounded by configured timeout (5000ms default) | ApiClient's own request/response timestamps, logged |
| UI refresh latency | < 1 frame (~16ms) post property-change | QML Profiler binding-evaluation timeline |
| Animation smoothness | 60 fps target | QML Profiler frame timeline, during M6–M7 component review |
| Polling accuracy | Actual interval tracks configured interval within ±100ms, even under backoff | Logged actual-vs-expected poll timestamps over an extended run |
| Long-term stability | 24h soak mandatory (M10 exit gate); 72h soak recommended before production sign-off | Continuous run with periodic health checks |

---

## 13. Documentation Workflow

| Document | Owner | Update frequency |
|---|---|---|
| Architecture documentation (this Phase 1–6 set) | Whoever makes the decision | Immediately, whenever an implementation-time ambiguity is resolved — appended as an addendum, never left undocumented in code comments alone |
| API documentation (Phase 4 §12's contracts) | Whoever last touches ApiClient's parsing code | Same PR as any field addition |
| Developer guide (new, lightweight — "how do I add a component/setting") | Whoever completes M7 | As new patterns emerge |
| User manual (operator-facing, Phase 4 §9 translated to plain instructions with real screenshots) | Written after M8 | At each subsequent UI-affecting change |
| Deployment guide | Written during M10 | Per release |
| Configuration guide (Phase 5 §6, translated to operator language) | Same as user manual | At each new/changed setting |
| Testing reports (Phase 5 §17 execution results) | Whoever runs each testing stage | Archived per milestone, not just at project end |
| Changelog | Whoever merges to `develop` | Every merge — one line per feature branch, so release notes (Section 14) are compiled by filtering, not reconstructed from memory |

---

## 14. Release Workflow

1. **Release Candidate creation:** cut from `develop` once M9's exit criteria are met; branch `release/X.Y`, tag `vX.Y.0-rc.1`.
2. **Final QA:** the full Section 9 testing workflow re-run against the RC build specifically — not `develop`'s latest, the RC's exact binary.
3. **Regression testing:** one final full pass of Section 9's regression policy.
4. **Packaging:** platform installers (Windows: an NSIS/MSIX-style installer; Linux: an AppImage or `.deb`, per Phase 2 §5's portability requirement), built from `release/X.Y` only.
5. **Installer generation:** automated via CI on `release/X.Y`, producing a versioned, checksummed artifact.
6. **Version numbering:** semantic versioning — MAJOR for a breaking config/data-format change, MINOR for a new feature, PATCH for a bugfix.
7. **Git tagging:** `vX.Y.Z` applied on `main`, at the exact commit merged from `release/X.Y` — never applied on `develop`.
8. **Release notes:** compiled from the changelog (Section 13), organized by category (Engineering/Networking/UI/Fixes).
9. **Deployment verification:** install the packaged artifact on a **clean machine**, not the dev machine, and confirm first-launch behavior matches Phase 4 §9's "First launch" scenario exactly.
10. **Rollback strategy:** the previous release's installer is retained as the rollback target. `SettingsManager`'s persisted configuration format must remain backward-compatible across a MINOR/PATCH release specifically, so a rollback never corrupts or discards the operator's existing configuration — a concrete, testable requirement, not just a policy statement.

**Criteria for declaring production-ready:** M10's soak test passed, **and** Final QA passed, **and** deployment verification passed on a clean machine, **and** the three Phase 5 §20 open items (temperature variable, real tank dimensions, alarm hysteresis tuning) are either confirmed or explicitly accepted by the stakeholder as shipped-with-defaults.

---

## 15. Risk Assessment

| Risk area | Description | Probability | Impact | Detection | Mitigation | Recovery |
|---|---|---|---|---|---|---|
| **Networking** | Cellular/4G uplink (EC200U-CN) drops intermittently — a more common failure mode than typical office networking | Medium-High | Medium (Offline handling is already designed for this — Phase 4 §6) | ConnectionManager failure rate, Network-category logs | Backoff policy avoids hammering a flaky link | Automatic — no operator action required |
| **API availability** | Ubidots service outage, indistinguishable from a device-side outage from this app's perspective | Low | Medium (same Offline handling, but possibly longer duration) | Sustained failure across all requests | Capped backoff prevents runaway retry frequency | Automatic |
| **Data corruption** | `SettingsManager`'s backing file corrupted (e.g. unclean shutdown mid-write) | Low | Medium (Phase 4 §13 already specifies fallback-to-defaults + notification) | Load-time validation | Validation-on-load already designed in | Operator re-enters affected settings, guided by the notification |
| **Geometry configuration** | Operator enters the wrong radius/height — a human-error risk, not a software risk | Medium (one-time manual entry, easy to mistype) | **High** — every displayed number becomes systematically, confidently wrong, with **no automatic error state** triggered, since the numbers stay internally consistent | **None automatic — this is a real, acknowledged gap** | `maximumVolume` updates live as the operator types (Phase 5 §6), so an implausible capacity is visually obvious by inspection — a practical, not technical, safeguard | Operator corrects the value; TankModel recalculates immediately, no data loss |
| **UI synchronization** | A binding accidentally implemented with a manual/imperative update instead of a property binding | Low-Medium | Medium (a stale-looking value, not a crash) | Code review against Phase 5 §10's binding matrix | The "no manual refresh anywhere" rule (Phase 2 §5) enforced at review time | Fix and re-test the specific binding |
| **Performance** | An inefficient binding chain, or an animation left running off-screen | Low | Low-Medium (CPU creep, relevant given 24/7 operation) | M9 performance testing | QML Profiler review at M6–M7, not deferred to M9 | Targeted optimization pass |
| **Memory leaks** | A QObject without a parent, or a `QNetworkReply` not deleted | Low-Medium (a known Qt pitfall specifically around `QNetworkReply`) | **High if present** — exactly what a 24/7 app cannot tolerate | Early profiler pass at M5 (ApiClient specifically), full soak at M10 | `deleteLater()` discipline on every reply; profiler review is part of M5's Definition of Done, not deferred | Targeted fix, then re-run the soak test from zero |
| **Future scalability** | An implementation shortcut couples code to "there is exactly one tank" beyond where Phase 2/5 intended | Medium | Low now, Medium later | Code review against Phase 5 §15's directory rules | The directory-dependency rules themselves | Refactor when multi-tank is actually prioritized (Phase 2 §14's already-scoped plan) |

---

## 16. Critical Path Analysis

**Modules that block subsequent work:** `Logger` blocks everything (nothing else can even log its own construction without it). `VolumeCalculator` blocks `TankModel`. `TankModel` blocks every QML data-bound component. `ApiClient`/`TankRepository` block **real, live** integration testing — but critically, they do **not** block QML component visual work.

**Tasks that can run in parallel:**
- `VolumeCalculator` and `TimeManager` (no shared dependency — Phase 5 §18).
- **Milestone 5 (Networking) and Milestone 6 (QML Components)** — the single largest parallelization opportunity in the whole roadmap. M6 needs only Design Tokens and `TankModel`'s default/static values from M4; it has no dependency on M5's live-networking work at all. A two-person team can run these concurrently; a solo developer can alternate focus between them without either blocking the other.

**Highest-priority milestones:** M3 (Engineering Layer) and M5 (Networking) — M3 because Phase 1's entire premise (Objective O2, calculated-not-raw correctness) depends on it being right, and M5 because it's the only milestone touching real external infrastructure outside the team's full control.

**Highest integration-risk components:** `ApiClient` (external dependency, the most failure modes of any module in the system — Section 15's Networking/API-availability rows) and the `TankRepository`↔`TankModel` boundary specifically (the one place a subtle validation-band bug would produce a wrong-but-plausible number silently — the worst failure mode identified in Section 15).

**Recommendations for minimizing schedule delay:**
- Front-load M3 and M5's mock-based work as early as possible, even if QML progress "looks" further along at a glance — a late-discovered problem in the engineering math or the API contract is far more expensive to fix once the entire UI has been built around an assumption that turns out wrong.
- Actively use the M5/M6 parallelization opportunity rather than defaulting to a strictly linear reading of Section 2's roadmap — it's schedule-neutral once identified, but easy to miss if the roadmap diagram is followed top-to-bottom without noticing the branch.

---

## 17. Final Development Readiness Review

| Dimension | Assessment |
|---|---|
| **Completeness** | Every module and component from Phase 5 has a place in this roadmap — nothing is orphaned |
| **Maintainability** | The Git workflow (Section 3) and directory rules (Phase 5 §15, enforced via Section 3's review checklist) give ongoing structure, not just a one-time build sequence |
| **Scalability** | Unchanged from Phase 5 §20 — carried forward honestly, not re-litigated or hidden here |
| **Testability** | Section 9's staged workflow ties every testing type to concrete entry/exit criteria — nothing is left as a vague "test as you go" |
| **Risk profile** | Section 15's highest risk (geometry misconfiguration) has a real, if acknowledged-imperfect, mitigation; every other identified risk has a designed detection and automatic recovery path |
| **Build strategy** | Phase 5 §16, unchanged and appropriate at v1's single-executable scale |
| **Integration strategy** | Section 8's mock-first sequence is the single most important process decision in this document — it de-risks the one module (ApiClient) with the most external failure modes before any real dependency is introduced |
| **Development efficiency** | Section 16's M5/M6 parallelization is real, schedule-relevant, and explicitly called out rather than left implicit |
| **Suitability for CI/CD** | Every milestone has automatable exit criteria (compiles, tests pass, checklist items) — nothing in this roadmap requires a human judgment call to gate progress except the final stakeholder sign-off in Section 14 |
| **Suitability for long-term maintenance** | Section 13 keeps documentation alive as an ongoing process tied to specific owners and triggers, not a static artifact abandoned once planning ends |

**Conclusion:** the project is fully prepared to move from planning into implementation. This is confirmed the final planning phase — no Phase 7 planning document is proposed, and continuing to add planning documents beyond this point would not provide proportional value against the depth already established across Phases 1–6. The concrete next action is **Milestone 1: Project Skeleton**.
