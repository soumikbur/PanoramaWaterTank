# Panorama Water Tank Monitor — Qt/QML Desktop Application
## Phase 3: UI/UX Architecture & Design System

**Status:** Draft for review. Phase 2 (Software Architecture) is approved. This document defines the visual/interaction contract every QML component (Phase 2, Section 4) must follow — it precedes, and constrains, the workflow and implementation phases still to come.
**No code in this document.**

---

## 1. Design Philosophy

This is a **control-room instrument**, not a consumer app. Every choice below is justified against that use case specifically — an operator glancing at this screen for two seconds several times an hour, for months, possibly from a few feet away on a secondary monitor.

| Principle | What it means here | Why |
|---|---|---|
| **Minimalism** | Flat surfaces, no gradients, no glassmorphism, no decorative imagery | Decoration competes for attention with the numbers that actually matter; industrial HMI convention (WinCC/Ignition/FactoryTalk) deliberately avoids it |
| **Information density over whitespace-as-aesthetic** | Generous spacing where it aids scanning (Section 4), but no wasted "hero" space | The dashboard's job is to answer "what's the tank doing" in one glance, not to look impressive |
| **Visual hierarchy is functional, not decorative** | Size/weight/color are allocated strictly by *operational importance* (Section 5), never by "what looks nice" | An operator's eye should land on fill % and status before anything else, every time, without having to think about it |
| **Engineering readability** | Every number is always on screen, never behind a hover/tooltip/click | This is monitored continuously, not explored interactively — hiding data behind interaction is a workflow anti-pattern for SCADA |
| **Operator workflow** | Status is glanceable without reading — color + icon + position all agree with each other | Reduces cognitive load during fatigue, shift changes, or when the operator isn't looking directly at the screen |
| **Industrial HMI principles** | Color carries *fixed, learned meaning* (green/amber/red never means anything else anywhere in the app); no two different states ever share a color | Operators build muscle memory across weeks of use — inconsistent color use actively erodes trust in the display |
| **Accessibility** | Status is never color-only (Section 13); contrast is WCAG AA minimum | A monitoring tool that fails for a color-blind operator, or in bad ambient lighting, is a safety gap |
| **Long-duration usability** | Muted, desaturated palette (Section 2); no looping strobe/pulse/bounce animations; no autoplaying attention-grabbers | This runs 24/7 on a screen someone looks at repeatedly all day — anything visually "loud" becomes fatiguing or gets tuned out (which defeats its purpose during an actual alarm) |

---

## 2. Design System

### 2.1 Color Palette

| Token | Hex | Usage |
|---|---|---|
| `primary` | `#2563EB` | Primary numbers (fill %), active nav item, primary accents, focus rings |
| `primaryPressed` | `#1D4ED8` | Pressed/active state of interactive primary elements |
| `primarySurface` | `#EAF2FD` | Light-tint backgrounds behind primary-colored content (status chip background, icon tile background) |
| `background` | `#F4F6F9` | App/page background, behind all cards |
| `surface` | `#FFFFFF` | Card and panel background |
| `surfaceAlt` | `#F8FAFC` | Recessed surfaces inside a card (e.g. tank body interior before fill) |
| `divider` | `#E5E9F0` | Card borders, table row dividers, header/footer borders |
| `dividerStrong` | `#CBD5E1` | Tick marks, scale lines — needs slightly more contrast than a divider |
| `success` | `#16A34A` | "Normal"/"Optimal" status text and icons |
| `successSurface` | `#E8F8EE` | Background behind success-colored badges |
| `warning` | `#D97706` | "Low" status |
| `warningSurface` | `#FEF3E2` | Background behind warning badges |
| `critical` | `#DC2626` | "Critical" status, error dialogs, destructive emphasis |
| `criticalSurface` | `#FDECEC` | Background behind critical badges |
| `overflow` | `#991B1B` | Distinct, darker red reserved *only* for the Overflow alarm — must never be confused with Critical at a glance (Section 8) |
| `sensorError` | `#7C3AED` | Device/sensor-level faults — deliberately outside the green/amber/red family so it reads as "different kind of problem" (Section 8) |
| `offline` | `#64748B` | Disconnected/offline states, muted and deliberately unalarming (a dropped connection with a cached last-good value is not the same severity as a Critical tank level) |
| `offlineSurface` | `#F1F5F9` | Background behind offline/disabled badges |
| `disabled` | `#CBD5E1` | Disabled controls, placeholder scale ticks |
| `textPrimary` | `#0F172A` | Titles, primary values, table values |
| `textSecondary` | `#64748B` | Labels, captions, subtitles |
| `textOnPrimary` | `#FFFFFF` | Text/icons placed on top of `primary`-colored fills |

**Placement rule:** exactly one accent color (`primary`) is used for anything *not* status-related. Status colors (`success`/`warning`/`critical`/`overflow`/`sensorError`/`offline`) are reserved exclusively for status communication and never reused decoratively (e.g. green is never used just because "it looks fresh" on an unrelated element).

### 2.2 Typography

**Font family:** `Segoe UI` (Windows) / `Inter` (Linux fallback) / system-ui as final fallback — a neutral, highly legible UI sans-serif. No display/decorative fonts anywhere.

| Style token | Size | Weight | Line height | Letter spacing | Used for |
|---|---|---|---|---|---|
| `display` | 44px | Bold (700) | 1.1 | -0.5px | The single most important number on screen: current fill % in the Tank Card |
| `h1` | 20px | DemiBold (600) | 1.3 | 0 | Page/dashboard title (Header) |
| `h2` | 16px | DemiBold (600) | 1.3 | 0 | Card titles ("Water Level", "Tank Information", "Water Level Indicator") |
| `valueLarge` | 22px | Bold (700) | 1.2 | 0 | Statistic card primary values |
| `valueMedium` | 16px | DemiBold (600) | 1.3 | 0 | Status badge text, table row values |
| `body` | 14px | Regular/Medium (400/500) | 1.4 | 0 | Nav labels, default body text |
| `label` | 13px | Medium (500) | 1.3 | 0.1px | Field labels, stat card titles, subtitles |
| `caption` | 12px | Regular (400) | 1.3 | 0.15px | Tick mark labels, timestamps, footer text, table row labels |

No size below 12px anywhere (Section 13 — accessibility floor).

### 2.3 Spacing System

Base unit: **4px**. Scale: `4 · 8 · 12 · 16 · 20 · 24 · 32 · 40 · 48 · 64`.

| Value | Used for |
|---|---|
| 4 | Icon-to-text gap inside a tight row (table rows, nav items) |
| 8 | Gap between a label and its value directly beneath it |
| 12 | Internal padding of small elements (status chip, nav item vertical padding) |
| 16 | Standard internal card padding on compact cards (statistic cards) |
| 20 | Standard internal card padding on primary cards (Tank Card, Information Card, Rank Indicator) |
| 24 | Page margin (Dashboard content inset from window edge); gap between major cards in a row |
| 32 | Vertical gap between dashboard rows (Tank/Info row → Rank row → Stat row) |
| 40 | Reserved for large full-screen overlay content padding (Loading/Offline/Error states) |
| 48 | Reserved for future dense-data pages (e.g. History) needing extra section separation |
| 64 | Reserved for empty-state illustration/message vertical centering |

### 2.4 Border Radius

| Token | Value | Used for |
|---|---|---|
| `radiusSmall` | 8px | Nav items, chips, small buttons |
| `radiusMedium` | 12px | Cards (Tank Card, Information Card, Rank Indicator, Statistic Cards) |
| `radiusLarge` | 16px | Dialogs/overlays (ErrorDialog) |
| `radiusPill` | height/2 (fully rounded) | Status badges, tank cap ends |
| `radiusTank` | 14px on body, full round on cap | The tank vessel specifically (Section 6) |

### 2.5 Elevation / Shadow

Kept intentionally subtle — this is a flat, minimal system, not a Material-style stacked-card look.

| Level | Blur | Vertical offset | Opacity | Used for |
|---|---|---|---|---|
| `none` | — | — | — | Page background, dividers |
| `sm` | 6px | 1px | 4% black | Default resting state of all cards |
| `md` | 12px | 4px | 6% black | Hovered/focused interactive cards (if any become clickable later) |
| `lg` | 24px | 8px | 10% black | Overlays/dialogs (ErrorDialog) — the only elements allowed to visually "float above" the page |

### 2.6 Icon System

- **Style:** monoline (single consistent stroke weight), outline-only (no filled icons except tiny status dots), geometric/neutral — not playful.
- **Sizes:** 16px (inline with text/labels), 20px (nav items, table rows), 24px (card icon tiles), 40–44px (large icon-tile accents).
- **Stroke width:** 1.75px at 20–24px sizes, scaling proportionally at other sizes.
- **Color rule:** icons always inherit the color of their adjacent text/status (never a fixed independent color) — an icon next to `critical`-colored text is `critical`-colored, full stop. This reinforces Section 1's "status is never color-only" principle by keeping icon and text visually locked together.

---

## 3. Design Tokens

A centralized token catalog is the single source every component reads from — no component ever hardcodes a hex value, pixel size, or duration. This is the design-system equivalent of Phase 2's "single source of truth" principle applied to *appearance* instead of *data*.

| Token group | Contains |
|---|---|
| **AppColors** | Every entry in Section 2.1, exposed as named values (not literals) — e.g. a component asks for "the critical color," never `#DC2626` directly |
| **AppTypography** | Every style row in Section 2.2 as a named, complete style (family + size + weight + line-height + letter-spacing bundled together, not assembled piecemeal per component) |
| **AppMetrics** | The spacing scale (2.3), radius tokens (2.4), and fixed layout constants (sidebar width, header height, footer height — Section 4) |
| **AppAnimations** | Named duration/easing pairs per Section 7 (e.g. `waterFillTransition`, `markerTransition`, `statusColorTransition`, `hoverTransition`) — components reference the *name* of the animation, not raw milliseconds, so tuning happens in one place |
| **AppIcons** | The icon name → glyph mapping (Section 2.6, Section 12) — components ask for an icon by semantic name (`"tank"`, `"alert"`, `"temperature"`) and never know or care how it's actually rendered underneath |

Practical effect: a future theme change, a rebrand, or an animation-speed tuning pass touches exactly one token layer and nothing in the component tree (Phase 2, Section 4) needs to change.

---

## 4. Dashboard Layout

```text
Window default: 1536 × 1024   Minimum: 1280 × 800

┌───────────────────────────────────────────────────────────────────────┐
│ HEADER — height 76                                                     │
├───────────┬───────────────────────────────────────────────────────────┤
│           │  content margin: 24 on all sides                           │
│ SIDEBAR   │  ┌───────────────────────┐  ┌───────────────────────────┐  │
│ width 260 │  │ TANK CARD             │  │ INFORMATION CARD           │  │
│           │  │ ~42% of content width │  │ fills remaining width       │  │
│           │  │ height 320            │  │ height 320                  │  │
│           │  └───────────────────────┘  └───────────────────────────┘  │
│           │           gap: 20                                          │
│           │  ┌───────────────────────────────────────────────────────┐│
│           │  │ RANK INDICATOR — full width, height 160                ││
│           │  └───────────────────────────────────────────────────────┘│
│           │           gap: 32 (row separation, per Section 2.3)        │
│           │  ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐              │
│           │  │ STAT 1 │ │ STAT 2 │ │ STAT 3 │ │ STAT 4 │  height 100    │
│           │  └────────┘ └────────┘ └────────┘ └────────┘              │
│           │           gap between cards: 20                            │
├───────────┴───────────────────────────────────────────────────────────┤
│ FOOTER — height 40                                                      │
└───────────────────────────────────────────────────────────────────────┘
```

| Region | Spec |
|---|---|
| Window | Default 1536×1024, minimum enforced 1280×800 (below this, information density degrades past the readability floor — see Section 9) |
| Header | Height 76, horizontal padding 24, border-bottom `divider` |
| Sidebar | Fixed width 260 (does not scale with window width — Section 9), internal padding 12, `NavItem` vertical gap 6 |
| Footer | Height 40, centered caption text |
| Content margin | 24 on all sides, inset from sidebar/header/footer |
| Tank Card / Information Card row | Two columns, gap 20, equal height (320), Tank Card ≈ 42% width, Information Card fills the rest |
| Rank Indicator row | Full content width, height 160 |
| Statistic Cards row | 4 equal-width columns, gap 20, height 100 |
| Row-to-row vertical gap | 32 |

---

## 5. Visual Hierarchy

Ranked by visual weight (size + color + position), justified by what an operator needs first:

| Rank | Element | Why it's here |
|---|---|---|
| 1 | **Fill percentage** (`display` style, `primary` color, 44px) | The single number that answers "is there water" — must be readable before anything else is even parsed |
| 2 | **Tank animation** | Large, spatial, pattern-recognizable even in peripheral vision — an operator can sense "tank looks low" without reading a number at all |
| 3 | **Connection / alarm status** | If the data can't be trusted (offline) or the tank is in a Critical/Overflow state, this must be impossible to miss — placed in both the header (always visible) and the Tank Card (in context) |
| 4 | **Rank indicator bar** | A secondary confirmation of #1 — reinforces the percentage with a spatial/color read, useful for a fast glance from across the room |
| 5 | **Statistic cards** (volume, capacity, temperature) | Important, but secondary — supporting detail once the operator already knows "how full" and "is it okay" |
| 6 | **Tank metadata table** (radius, height, device, area) | Reference information — rarely changes, rarely needs to be read, but must be available for troubleshooting/verification |
| 7 | **Last update / footer** | Lowest emphasis — smallest text, most muted color; only scanned deliberately, e.g. to confirm the feed isn't stale |

This ordering is enforced through Section 2's typography/color tokens (rank 1 gets `display`+`primary`; rank 7 gets `caption`+`textSecondary`), never through ad-hoc per-component styling decisions.

---

## 6. Tank Visualization Design

| Aspect | Specification |
|---|---|
| **Shape** | Cylindrical vessel: flat-sided rounded-rectangle body with a shallow rounded cap on top (matches the reference image's real-tank silhouette, not an abstract bar) |
| **Dimensions** | Visual body ~150–200 device-independent px wide, height = ~85% of the Tank Card's available vertical space, independent of the numeric aspect ratio of the real tank (this is a *symbol*, not a scale drawing) |
| **Outline** | 3px stroke, `dividerStrong` color — deliberately heavier than a normal card border so the vessel silhouette reads clearly even at a glance |
| **Body fill (empty)** | `surfaceAlt` — a very slightly recessed tone so the vessel reads as a container even before any water is drawn |
| **Water fill** | Solid `primary` blue, clipped to the vessel's rounded shape, height driven by `TankModel.fillPercentage` — never by a raw API value (Phase 1, Objective O2) |
| **Water surface** | A thin, lighter-blue band at the top edge of the fill, animated with a slow horizontal drift (Section 7) — signals "this is live," not a static fill graphic |
| **Tick marks** | Fixed horizontal ticks at 0/25/50/75/100%, `dividerStrong`, positioned along the vessel's right edge — these are geometry constants, not data-driven |
| **Scale labels** | "0%…100%" text beside each tick, `caption` style, `textSecondary` |
| **Percentage display** | The `display`-style number beside the tank (Section 5, rank 1) |
| **Height display** | Shown in the Information Card and a Statistic Card, `valueMedium`/`valueLarge`, always in meters, 2 decimal places (Phase 1, FR-15) |
| **Volume display** | Shown in a Statistic Card, always in liters, 2 decimal places |

**Engineering mapping (the core design constraint of this whole app):** the vessel drawing never receives a percentage from the network layer. It receives `TankModel.fillPercentage`, which only exists after `VolumeCalculator` has converted a raw height-or-volume reading through the tank's actual geometry (Phase 2, Section 5). Visually, this means the tank drawing is *only ever wrong if the configured radius/height is wrong* — never because of a mismatched unit or an API quirk. This constraint should be visible to a reviewer just by tracing "what property does this component bind to" back through Phase 2's data flow diagram.

---

## 7. Animation System

Every animation exists to make a *state change* legible, never to decorate. None loop indefinitely except the water surface shimmer, and that one specifically exists to signal liveness.

| Animation | Duration | Easing | Trigger | Purpose |
|---|---|---|---|---|
| Water fill height | 600ms | InOutQuad | `fillPercentage` changes | Smooths a poll-to-poll jump into a readable transition instead of a jarring snap |
| Water surface shimmer | 2200ms (loop, ping-pong) | InOutSine | Continuous while fill > 0 | Passive "this is live data" signal — the one intentionally decorative motion, kept slow and subtle so it never distracts |
| Rank indicator marker | 500ms | InOutQuad | `fillPercentage` changes | Mirrors the fill animation so both indicators visibly agree with each other during a transition |
| Status badge color | 250ms | Linear (ColorAnimation) | `status` changes | Fast enough to feel immediate for an alarm state change, without an abrupt hard-cut |
| Statistic card value | 250ms opacity fade-through | InOutQuad | Bound value changes | A subtle "this just updated" cue without a distracting slide/scale effect |
| Sidebar item hover/active | 150ms | Linear (ColorAnimation) | Mouse hover / selection | Immediate enough to feel responsive on a desktop pointer device |
| Page transition (Dashboard ↔ other pages) | 200ms | Linear (opacity crossfade only) | Sidebar navigation | Deliberately *not* a slide/push transition — those read as "consumer app," a flat crossfade is the SCADA-appropriate choice and avoids motion that could feel disorienting on a display someone is watching passively |

**Explicit exclusions:** no bounce/elastic easing anywhere, no scale-pulse on updated numbers, no shake/attention-seeking animation even for Critical/Overflow states (severity is communicated by color+icon+badge persistence, Section 8 — not by motion, which becomes fatiguing over a 24/7 duty cycle and risks being misread as a glitch).

---

## 8. Status & Alarm Design

| State | Color | Icon | Label | Behavior |
|---|---|---|---|---|
| **Normal** | `success` | Droplet/checkmark | "Normal" | Steady badge, no special treatment |
| **Low** | `warning` | Warning triangle | "Low" | Steady badge; percentage/tank visuals unchanged in style, only the status badge and rank-bar segment reflect it |
| **Critical** | `critical` | Alert circle | "Critical" | Steady badge (no flashing, Section 7); Tank Card border may switch from `divider` to `critical` at reduced opacity as a secondary, non-animated cue |
| **Overflow** | `overflow` (distinct dark red) | Overflow/double-arrow-up icon | "Overflow" | Same steady treatment as Critical, but deliberately different color+icon so it is never confused with Critical during a fast glance |
| **Sensor Error** | `sensorError` (purple) | Sensor/chip-warning icon | "Sensor Error" | Communicates a *device-level* fault distinct from a *level* alarm or a *connectivity* problem — operator should not conflate "the tank is fine but the sensor is lying" with "the tank is actually critical" |
| **API Error** | `critical` | Cloud-off/plug icon | "API Error" | Same red family as Critical (it *is* a serious problem) but a distinct icon makes clear the *tank* isn't necessarily in trouble — the *feed* is |
| **Offline** | `offline` (muted gray) | Wifi-off icon | "Disconnected" | Deliberately unalarming color — a dropped connection with a cached last-good reading is lower severity than an active tank alarm, and should not visually compete with a real Critical state |
| **Connecting** | `primary` (blue, informational) | Pulse/spinner glyph (static icon, not spinning — Section 7 excludes decorative motion here too) | "Connecting…" | Transient, expected during startup |
| **Waiting for Data** | `offline`-family, lighter | Hourglass/empty-tank icon | "Waiting for Live Data" | Shown before the first successful reading only (Phase 2, Section 6) |

**Design rule:** the four "something is wrong" families (`warning`, `critical`, `overflow`, `sensorError`) are each visually distinct from each other by *both* hue and icon, specifically so an operator scanning quickly can distinguish "the water is low" from "the sensor is broken" from "the tank is overflowing" — three very different required responses — without reading the text label.

---

## 9. Responsive Behavior

The application targets desktop monitors from 1280×720 up to 2560×1440. It is **not** a fluidly responsive layout in the web sense — it is a fixed-grid layout (Section 4) that adapts at defined breakpoints, appropriate for a control-room display that's typically viewed at one of a small number of known resolutions.

| Resolution | Behavior |
|---|---|
| **1280×720** | Below the enforced minimum height (800) — window opens at minimum size (1280×800) rather than the native resolution; this is the readability floor referenced in Section 4 |
| **1366×768** | Minimum comfortably supported size; content margin reduces 24→16, card gaps reduce 20→16 to preserve the two-column Tank/Info row without cramping |
| **1600×900** | Baseline design target — all spacing/sizing values in this document apply as specified |
| **1920×1080** | Baseline still applies; extra width is absorbed by the Information Card (which fills remaining space per Section 4) rather than stretching the Tank Card, since the tank's visual size should stay consistent regardless of monitor width |
| **2560×1440** | A global UI scale factor (driven by `AppMetrics`, Section 3) increases spacing and typography proportionally rather than simply letting whitespace grow — on a large control-room monitor, the goal is readability *from further away*, not more empty space |

**Statistic card row reflow:** at widths below ~1400px effective content width, the 4-card row may reflow to a 2×2 grid rather than compressing each card below a legible minimum width — this is the one place true reflow (not just spacing adjustment) occurs.

**Sidebar:** fixed width (260) at all resolutions — it does not shrink, since its content (icon + label nav items) has a fixed minimum legible width and collapsing it to icon-only is out of scope for v1 (flagged as a Phase 12-style future option if a narrower target resolution is ever required).

---

## 10. Component Inventory

Extends Phase 2 Section 4 with the visual/interaction contract each component owns.

| Component | Purpose | Inputs (properties) | Outputs (signals) | Dependencies | Reusable? |
|---|---|---|---|---|---|
| `Header` | Top identification + connection/time bar | `pageTitle`, `pageSubtitle`, `connected`, `currentTime`, `currentDate` | — | `AppColors`, `AppTypography` | No |
| `Sidebar` | Primary navigation | `currentPage`, nav model | `pageSelected(page)` | `NavigationItem` | No |
| `NavigationItem` | One nav row | `icon`, `label`, `active` | `clicked()` | `AppColors`, `AppIcons` | Yes |
| `TankCard` | Card shell hosting the tank visualization | `fillPercentage`, `currentHeight`, `status`, `lastUpdated` | — | `TankView`, `TankScale`, `WaterFill`, `WaterSurface`, `TankStatistics`, `StatusBadge` | No |
| `TankView` | Vessel outline + clip region | geometry constants | — | `AppColors` | Yes (any cylindrical vessel) |
| `TankScale` | Fixed tick marks + labels | `divisions` (default 4) | — | `AppTypography` | Yes |
| `WaterFill` | Animated fill rectangle | `fillPercentage` | — | `AppAnimations` | Yes |
| `WaterSurface` | Shimmer overlay | bound to `WaterFill` | — | `AppAnimations` | Yes |
| `TankStatistics` | Large percentage + supporting labels | `fillPercentage`, `currentHeight` | — | `AppTypography` | Yes |
| `StatusBadge` | Status pill (color+icon+label) | `status` | — | `AppColors`, `AppIcons` (Section 8 mapping) | Yes |
| `InformationCard` | Key/value metadata table | `model` | — | `InfoRow` | No |
| `InfoRow` | One label/value line | `icon`, `label`, `value` | — | `AppTypography` | Yes |
| `RankIndicator` | Segmented capacity bar + marker | `fillPercentage`, `segments` | — | `RankSegment` | Yes |
| `RankSegment` | One colored band | `label`, `color`, `range` | — | `AppColors` | Yes |
| `StatisticCard` | Reusable metric tile | `icon`, `title`, `value`, `subtitle` | — | `AppTypography`, `AppIcons` | Yes |
| `Footer` | Bottom caption bar | — | — | `AppTypography` | No |
| `LoadingOverlay` | Full-screen initial-load state | — | — | `AppColors` | Yes (any full-screen state) |
| `OfflineOverlay` | Non-blocking staleness indicator (banner, not full-screen — Section 14) | `lastUpdated` | — | `AppColors` | Yes |
| `ErrorDialog` | Blocking modal for Authentication/Configuration errors | `title`, `message`, `actionLabel` | `actionTriggered()` | `AppColors`, `AppTypography` | Yes (any blocking error) |

---

## 11. Theme Architecture

- **Light theme (v1, default):** the palette in Section 2.1, matching the reference design. Industrial HMI convention favors light backgrounds for control-room ambient lighting and print/screenshot legibility for incident reports — this is a deliberate choice, not a default left unexamined.
- **Dark theme (future, not built in v1):** reserved as a Phase 12-style addition. Because every component reads colors through `AppColors` tokens (Section 3) rather than literal values, adding a dark palette means defining a second token set — no component code changes.
- **Theme switching:** owned by a future `ThemeManager` (a small, focused class — likely a QML singleton alongside `TimeManager`, Phase 2 Section 3) exposing the *currently active* resolved `AppColors` set. Components never ask "is dark mode on" and branch — they simply always read `AppColors.background`, and `ThemeManager` decides what that resolves to.
- **Shared color definitions:** semantic token *names* (`primary`, `surface`, `critical`, etc.) are theme-independent and permanent; only their resolved hex values change per theme. This "semantic over literal" naming discipline (Section 3) is what makes theme switching additive rather than a refactor.

---

## 12. Asset Architecture

```text
resources/
├── icons/            kebab-case, semantic names (icon-tank.svg, not icon-blue-drum.svg)
│     ├── nav-*.svg          (sidebar navigation set)
│     ├── status-*.svg       (Section 8 status glyphs)
│     ├── stat-*.svg         (statistic card icon tiles)
│     └── ui-*.svg           (clock, calendar, user, connection)
├── fonts/             bundled fallback only if system font unavailable/unlicensed for redistribution
├── branding/
│     └── panorama-logo.svg
└── images/            reserved, empty in v1 (no illustrations needed for a data-dense HMI)
```

**Production target:** a proper SVG icon set per Section 2.6 (24×24 canvas, monoline, 1.75px stroke), matching the naming convention above.

**Implementation contingency:** if finalized SVG assets aren't ready when a given phase is implemented, a temporary vector-drawn placeholder set following the *same* naming/sizing contract (i.e., every component still asks for an icon by semantic name, e.g. `"tank"`) may be substituted and swapped later with zero component-level changes — this is exactly what the `AppIcons` token layer (Section 3) is for.

**Naming convention:** kebab-case, always semantic (what it *represents*), never descriptive of its current visual appearance (what it *looks like*) — a naming discipline that survives a future re-skin.

---

## 13. Accessibility

| Requirement | Specification |
|---|---|
| **Minimum contrast ratio** | 4.5:1 for body/label text against its background (WCAG AA); 3:1 for large text (`display`, `h1`, `valueLarge`) and icons — verified per color pairing in Section 2.1, not assumed |
| **Minimum font size** | 12px (`caption`) floor, enforced everywhere (Section 2.2) — nothing smaller ships |
| **Keyboard navigation** | Sidebar nav items are Tab-focusable in visual order; Enter/Space activates the focused item; all interactive elements (nav, future settings inputs, dialog actions) reachable without a pointing device |
| **Focus indicators** | 2px `primary`-colored outline, 2px offset, shown specifically on keyboard focus (not suppressed after a mouse click, not shown for pure mouse interaction) — never relies on the browser/OS default, which is inconsistent across platforms |
| **Color-blind friendly status colors** | Every status (Section 8) pairs its color with a distinct icon *and* text label — no status is ever communicated by hue alone; the four "problem" states are additionally distinguished by icon shape, not just color, specifically to remain distinguishable under red/green or blue/purple color-vision deficiencies |
| **Screen scaling support** | Built on Qt's high-DPI scaling; because sizing derives from the `AppMetrics` token scale (Section 3) rather than hardcoded pixel values scattered per component, the entire UI scales coherently under OS-level display scaling or the Section 9 large-monitor scale factor |

---

## 14. UI State Architecture

Maps directly onto Phase 2 Section 6's application state machine — this section defines what the *operator sees* in each state.

| State | What's on screen |
|---|---|
| **Initial Loading** | `LoadingOverlay` only — logo, no dashboard chrome yet (sidebar/header not shown until configuration is confirmed loadable) |
| **Waiting for Live Data** | Full dashboard chrome (header, sidebar) is visible; Tank Card shows the vessel outline with an empty/dashed fill and the label "Waiting for Live Data" instead of a percentage; Statistic Cards show `—` placeholders instead of `0.00` (a real zero and "no data yet" must never look identical) |
| **Connected** | Full dashboard, green "Connected" badge in the header, all values populated |
| **Updating** | Visually identical to Connected — the only cue is the brief per-value fade animation (Section 7); no spinner or loading indicator, since updates happen every few seconds and a persistent loading cue would itself become noise |
| **Offline** | Dashboard remains fully visible with the last-known values still displayed (never blanked); `OfflineOverlay` renders as a slim, non-blocking banner (not a full-screen takeover — the operator still needs to see the last-known state); header badge switches to gray "Disconnected"; the "Last Updated" timestamp visually de-emphasizes further (or is explicitly flagged) once its age exceeds a small multiple of the poll interval, so staleness is legible even without reading the exact timestamp |
| **Authentication Error** | Blocking `ErrorDialog`: "Authentication failed — check your API token," with an action pointing to Settings; dashboard behind it is dimmed but its last-known values remain visible through the dim, not hidden |
| **Configuration Error** | Blocking `ErrorDialog`: names the specific missing/invalid setting (e.g. "Tank height must be greater than zero"), action points to Settings |
| **Empty State** | Not expected in v1's single-tank scope, but defined for completeness: if geometry is entirely unconfigured, the Tank Card shows a neutral placeholder ("Tank not configured") with a Settings call-to-action, rather than a misleading 0% |

---

## 15. Final Design Review

**Strengths**
- Every visual decision in this document traces back to a named principle in Section 1 — nothing here is "because it looked good," which makes the system defensible in review and consistent as new components get added in later phases.
- The token architecture (Section 3) means the two most likely future asks — a dark theme and a large-monitor control-room deployment — are both already additive (Sections 9, 11) rather than requiring rework.
- Status design (Section 8) is unusually rigorous for a v1: six distinct states, each with a color *and* icon *and* label, specifically built to avoid the two most common HMI mistakes — color-only status communication, and conflating "the tank has a problem" with "the feed has a problem."
- Animation is entirely restrained to legibility purposes (Section 7) — appropriate for 24/7 viewing, and avoids the single most common industrial-HMI design mistake (decorative motion that gets more fatiguing than helpful within days of continuous use).

**Weaknesses**
- The 4-column statistic row (Section 4) has only one defined reflow behavior (2×2 at narrow widths) — if a 5th statistic card is ever needed (a real possibility once temperature/battery/signal all become live), this grid needs a second look rather than just adding a column.
- Sidebar's fixed 260px width (Section 9) is a simplicity choice, not a scalability one — it's the one component in this document without a defined "future" story if a narrower deployment target ever appears.

**Risks**
- Color-blind accommodation (Section 13) is specified as a requirement but the actual icon shapes per status (Section 8) haven't been drawn yet — this needs explicit validation once real icon assets exist, not just assumed compliant because the intent is documented.
- The Overflow vs. Critical color distinction (`#991B1B` vs `#DC2626`) is a fairly subtle hue difference on its own — Section 8's requirement that icon shape carry equal weight to color here is important and should not be relaxed during implementation for the sake of expedience.

**Future improvements**
- A defined "extra-wide" statistic grid variant (5–6 cards) ahead of temperature/battery/signal all going live simultaneously.
- Dark theme token set (Section 11), once there's an actual deployment need (e.g. a control room running dimmed displays overnight).
- A documented icon-shape audit for color-vision-deficiency accessibility once final SVG assets replace any placeholder icon set (Section 12).

**Production-readiness confirmation:** this design system is appropriate for a continuously-operated industrial monitoring application — it is restrained rather than decorative, every status is multiply-encoded (not color-only), animation exists solely to aid legibility, and the token architecture means both of the two most likely near-term requests (dark theme, large-monitor scaling) are additive rather than structural changes.

---

Phase 3 is complete. Holding here, per the phase-gated process, for your review before proceeding.
