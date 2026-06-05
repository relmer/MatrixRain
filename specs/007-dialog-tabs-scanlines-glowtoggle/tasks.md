---
description: "Task list for v1.5 Settings Dialog Overhaul, Scanlines & Glow Toggle"
---

# Tasks: Settings Dialog Overhaul, Scanlines & Glow Toggle (v1.5)

**Branch**: `007-dialog-tabs-scanlines-glowtoggle`
**Inputs**: `spec.md` (clarifications resolved), `plan.md`, `research.md` (R1–R9),
`data-model.md`, `contracts/{propertysheet,registry-schema,scanline-shader,fps-publisher}.md`,
`quickstart.md`

**Tests**: REQUIRED. Per constitution principle I, every implementation task is
paired with a preceding/parallel test task (Red→Green). Thin Win32 UI plumbing
(page DLGPROCs, `EnableWindow` helpers, `WM_TIMER` formatting) is exempt under
the v1.3/v1.4 carve-out; everything routed through `ConfigDialogController`,
`RegistrySettingsProvider`, `SharedState`, `RenderSystem`, and the new
`ScanlineStyleMapping` is unit-tested.

**Story label key** (matches user-input phase plan, not spec ordinals):

| Label | Story | Spec § |
|---|---|---|
| US1 | Property-sheet structure + live perf readout | spec US1 (P1) |
| US2 | Glow Enabled checkbox + cross-tab disabled propagation | spec US2 (P1) |
| US3 | CRT scanlines | spec US3 (P2) |
| US4 | Fade-timer feature removal | spec US5 (P1) — sequenced before US1 |
| US5 | Custom color picker | spec US4 (P2) |
| US6 | Upgrade migration note (CHANGELOG) | spec US6 (P3) |

## Format: `[ID] [P?] [Story?] Description`

- **[P]**: Different file / non-overlapping symbols; safe to run in parallel within phase
- **[Story]**: Tag for traceability; setup, foundational, and polish phases omit the tag
- File paths are absolute-from-repo-root
- Each task ends with the spec hint (`FR-…` / `SC-…` / contract-doc / research-doc) it satisfies

## Path Conventions

- Core library: `MatrixRainCore\` (testable logic)
- Win32 UI shell: `MatrixRain\` (.exe, .rc, ConfigDialog)
- Tests: `MatrixRainTests\` (Microsoft C++ Native Unit Test Framework)
- Shaders: `MatrixRainCore\Shaders\`

---

## Phase 1: Foundational

**Purpose**: Pre-allocate IDs, headers, and library skeleton so later phases
slot in without churn. Small and self-contained — no behavior change yet.

- [X] T001 Reserve new control / dialog / string IDs in `MatrixRain\resource.h`: `IDD_VISUALS_PAGE`, `IDD_PERFORMANCE_PAGE`, `IDS_VISUALS_TAB_TITLE`, `IDS_PERFORMANCE_TAB_TITLE_INITIAL`, `IDS_PERFTAB_TITLE_FORMAT`, `IDT_PERF_TITLE_TIMER`, `IDC_GLOW_ENABLED_CHECK`, `IDC_SCANLINES_ENABLED_CHECK`, `IDC_SCANLINES_INTENSITY_SLIDER` + `_LABEL` + `_VALUE` + `_INFO`, `IDC_SCANLINES_STYLE_SLIDER` + `_LABEL` + `_VALUE` + `_INFO`, placeholders for the per-tab control IDs the .rc split will need. Note: the existing v1.4 color combo is `IDC_COLORSCHEME_COMBO` — do not introduce a new `IDC_COLOR_COMBO` (FR-001, FR-005, FR-008, FR-009, contracts/propertysheet.md)
- [X] T002 [P] Add `<commdlg.h>` and `<prsht.h>` to `MatrixRain\pch.h` (Windows headers section, alphabetised, preserving the 3-blank-line group separator); add `<atomic>` to `MatrixRainCore\pch.h` if not already present (research.md R1, R3, R5)
- [X] T003 [P] Create directory `MatrixRainCore\Shaders\` and add a `.gitkeep` or initial placeholder so the scanline shader has its eventual home; add the directory to `MatrixRainCore.vcxproj` as a None/Content item if needed (plan.md "Project Structure", FR-022)
- [X] T004 [P] Create skeleton `MatrixRainCore\ScanlineStyleMapping.h` declaring `float ScanlineLineCount(int style) noexcept;` (header-only or paired .cpp — match project convention for tiny pure functions) (FR-023, data-model.md §7)
- [X] T005 [P] Create skeleton `MatrixRainCore\ScanlineStyleMapping.cpp` with `#include "pch.h"` + own header + body returning `0.0f` (intentional Red so T006's tests fail first) (FR-023)
- [X] T006 [P] Write `MatrixRainTests\unit\ScanlineStyleMappingTests.cpp` asserting line count at style ∈ {1, 25, 50, 75, 100} matches {~981, ~622, ~387, ~241, ~150} within ±2 lines, plus clamp-invariance for inputs already at the [1,100] endpoints (Red against T005 stub) (FR-023, data-model.md §7)
- [X] T007 Implement real body of `MatrixRainCore\ScanlineStyleMapping.cpp` — `return 1000.0f * std::pow(0.15f, static_cast<float>(style) / 100.0f);` — making T006 Green; commit pair as one logical change (FR-023)

**Checkpoint**: Resource IDs reserved, pch additions in, shader folder exists, scanline-mapping math unit-tested. Safe to begin destructive removal.

---

## Phase 2: User Story 4 — Fade-Timer Feature Removal (Priority: P3 — sequenced as P1 dependency)

**Goal**: Excise the orphaned fade-timer debug overlay from production AND
tests top-to-bottom, so the dialog rewrite in Phase 3 operates on a clean
slate. Per research.md R8, this is purely deletions with one new test
(legacy registry value silent-ignore).

**Independent Test**: `git --no-pager grep -i "fadetimer\|fade.timer" -- MatrixRain MatrixRainCore MatrixRainTests` returns zero matches; full test suite green; a legacy `ShowFadeTimers` REG_DWORD in the test hive does not affect `Load()`.

### Tests for User Story 4 ⚠️

- [X] T008 [P] [US4] Delete all `FadeTimer` / `UpdateShowFadeTimers` cases from `MatrixRainTests\ConfigDialogControllerTests.cpp` (R8 production+test inventory)
- [X] T009 [P] [US4] Delete all `m_showFadeTimers` assertions from `MatrixRainTests\ScreenSaverSettingsTests.cpp` (R8)
- [X] T010 [US4] In `MatrixRainTests\RegistrySettingsProviderTests.cpp`: delete `ShowFadeTimers` read/write cases AND add new test `LegacyShowFadeTimersIsSilentlyIgnored` — pre-write a `ShowFadeTimers=1` REG_DWORD into the test hive, call `Load()`, assert success and that the loaded `ScreenSaverSettings` has no surviving fade-timer field (Red until Phase-2 impl tasks remove the read path) (FR-037, R8)

### Implementation for User Story 4

- [X] T011 [US4] `MatrixRainCore\ScreenSaverSettings.h`: remove `m_showFadeTimers` field and any accessor (FR-036, R8)
- [X] T012 [US4] `MatrixRainCore\RegistrySettingsProvider.{h,cpp}`: remove `VALUE_SHOW_FADE_TIMERS` constant and its load/save code paths; do NOT add active cleanup — absence of read = silent ignore (turns T010 Green) (FR-036, FR-037)
- [X] T013 [P] [US4] `MatrixRainCore\ApplicationState.{h,cpp}`: remove `ToggleDebugFadeTimes`, `SetShowDebugFadeTimes`, `GetShowDebugFadeTimes`, change-callback registration, backing bool (FR-036, R8)
- [X] T014 [P] [US4] `MatrixRainCore\SharedState.h`: remove `showDebugFadeTimes` live field AND snapshot mirror (FR-036, data-model.md §4)
- [X] T015 [P] [US4] `MatrixRainCore\RenderParams.h`: remove `showDebugFadeTimes` (FR-036, data-model.md §5)
- [X] T016 [US4] `MatrixRainCore\RenderSystem.{h,cpp}`: remove `RenderDebugFadeTimes` method declaration, definition, and its single call site in the render loop (FR-036)
- [X] T017 [US4] `MatrixRainCore\ConfigDialogController.{h,cpp}`: remove `UpdateShowFadeTimers` and any snapshot/restore touching the bool (FR-036)
- [X] T018 [US4] `MatrixRain\MatrixRain.rc`: remove `IDC_SHOWFADETIMERS_CHECK` "Show fade timers" control entry; `MatrixRain\resource.h`: remove the `IDC_SHOWFADETIMERS_CHECK` `#define` (FR-036, R8)
- [X] T019 [US4] `MatrixRain\ConfigDialog.cpp`: remove `OnShowFadeTimersCheck`, every `CheckDlgButton(..., IDC_SHOWFADETIMERS_CHECK, ...)` site (defaults-apply, settings-load, `WM_COMMAND` dispatch) (FR-036)
- [X] T020 [US4] Run `git --no-pager grep -i "fadetimer\|fade.timer" -- MatrixRain MatrixRainCore MatrixRainTests`; expected: 0 matches. Build x64 Debug + run full test suite — must be green before merging to Phase 3 (quickstart §US5, SC-008)

**Checkpoint**: Fade-timer feature gone. Test suite re-baselined. Dialog code is now safe to restructure without dead controls.

---

## Phase 3: User Story 1 — Property-Sheet Structure + Live Perf Readout (Priority: P1) 🎯 MVP

**Goal**: Convert the single `IDD_MATRIXRAIN_SAVER_CONFIG` dialog into a
modeless two-page `PropertySheetW` (Visuals / Performance), wire the 1 Hz
FPS+GPU title timer, and extend snapshot/rollback for the 5 new
rollback-eligible v1.5 settings (placeholders — actual controls land in
Phases 4–6).

**Independent Test**: Per quickstart §US1 — two tabs labelled "Visuals" and
"Performance (NN fps, NN% GPU)" appear; no Apply button; Density still
applies live; title refreshes ≤1 s; OK commits, Cancel/X/Alt+F4 roll back.

### Tests for User Story 1 ⚠️

- [X] T021 [P] [US1] Write `MatrixRainTests\unit\MonitorRenderContextFpsPublisherTests.cpp`: assert default `m_publishedFps == 0.0f` AND `m_hasPublishedFps == false`; assert that publishing a value updates both atomically and that subsequent loads observe both (Red) (FR-010, contracts/fps-publisher.md, research.md R3)
- [X] T022 [P] [US1] Extend `MatrixRainTests\ConfigDialogControllerTests.cpp` with `EnterLiveMode_SnapshotsAllV15Fields` and `CancelLiveMode_RestoresAllV15Fields` covering all 5 new fields: `glowEnabled`, `scanlinesEnabled`, `scanlinesIntensity`, `scanlinesStyle`, `customColor`. Assert `customColorPalette` is NOT in the snapshot set (FR-035 carve-out) (Red) (FR-004, FR-044, data-model.md §3)

### Implementation for User Story 1

- [X] T023 [US1] `MatrixRainCore\MonitorRenderContext.{h,cpp}`: add `std::atomic<float> m_publishedFps {0.0f};` and `std::atomic<bool> m_hasPublishedFps {false};`; in the per-frame post-`FPSCounter::Tick()` path, `store(..., memory_order_relaxed)` both. Provide lock-free accessor pair `GetPublishedFps(bool & outHasValue) const` (Green for T021) (FR-010, contracts/fps-publisher.md, research.md R3)
- [X] T024 [P] [US1] `MatrixRainCore\ConfigDialogSnapshot.h`: add 5 new fields per data-model §3 (`bool glowEnabled; bool scanlinesEnabled; int scanlinesIntensity; int scanlinesStyle; COLORREF customColor;`). Explicit comment noting palette is INTENTIONALLY excluded per FR-035 (FR-004, FR-044)
- [X] T025 [US1] `MatrixRainCore\ConfigDialogController.{h,cpp}`: extend `EnterLiveMode()`/`CancelLiveMode()` to snapshot/restore the 5 new fields and push restored values into `SharedState`; add `UpdateGlowEnabled`, `UpdateScanlinesEnabled`, `UpdateScanlinesIntensity`, `UpdateScanlinesStyle`, `UpdateCustomColor` accessors (Green for T022) (FR-004, FR-044, data-model.md §3)
- [X] T026 [P] [US1] `MatrixRainCore\SharedState.h`: add live atomics (`liveGlowEnabled`, `liveScanlinesEnabled`, `liveScanlinesIntensity`, `liveScanlinesStyle`, `liveCustomColor`) AND non-atomic snapshot mirrors per data-model.md §4 (FR-044)
- [X] T027 [P] [US1] `MatrixRainCore\RenderParams.h`: add `glowEnabled`, `scanlinesEnabled`, `scanlinesIntensity` (float [0..1]), `scanlinesLineCount` (float), `customColor` (COLORREF). Update the render-thread per-frame copy in `RenderSystem` to fill them from `SharedState` + `ScanlineStyleMapping` (data-model.md §5)
- [X] T028 [US1] Rewrite `MatrixRain\MatrixRain.rc`: replace `IDD_MATRIXRAIN_SAVER_CONFIG` template with two new dialog templates `IDD_VISUALS_PAGE` and `IDD_PERFORMANCE_PAGE`. Visuals page lays out (top→bottom): Density, Speed, Glow Intensity, Glow Size, Color combo (with `Custom…` slot reserved for Phase 6), then Scanlines groupbox placeholder (filled in Phase 5), then Start In Fullscreen. Performance page lays out: GPU Adapter combo, Glow Enabled checkbox placeholder (filled in Phase 4), Quality Preset, Glow Passes, Glow Resolution, Glow Smoothness, Use All Monitors, Show Statistics. Add `STRINGTABLE` entry `IDS_PERFTAB_TITLE_FORMAT L"Performance (%u fps, %u%% GPU)"`, plus `IDS_VISUALS_TAB_TITLE L"Visuals"`, `IDS_PERFORMANCE_TAB_TITLE_INITIAL L"Performance"` (FR-001, FR-005, FR-008, FR-009, research.md R9)
- [X] T029 [US1] Replace `MatrixRain\ConfigDialog.{h,cpp}` with property-sheet host: build `PROPSHEETHEADERW` with `PSH_PROPSHEETPAGE | PSH_MODELESS | PSH_NOAPPLYNOW | PSH_PROPTITLE | PSH_USECALLBACK`; two `PROPSHEETPAGEW` entries with `PSP_USETITLE | PSP_PREMATURE`; `pfnDlgProc` = `VisualsPageDlgProc` / `PerformancePageDlgProc`; `PropertySheetW()` returns the frame `HWND` → store in `Application::m_hConfigDialog` (FR-001, FR-002, FR-013, contracts/propertysheet.md, research.md R1, R7)
- [X] T030 [US1] Implement `VisualsPageDlgProc` and `PerformancePageDlgProc` skeletons in `MatrixRain\ConfigDialog.cpp`: `WM_INITDIALOG` reads from `ScreenSaverSettings` and populates existing controls; existing slider/checkbox handlers re-route to `ConfigDialogController::Update*`; `PSN_APPLY` returns `PSNRET_NOERROR` (live commit); `PSN_RESET` triggers `CancelLiveMode()`. No new control handlers yet — Phases 4-6 fill those (FR-003, FR-004, FR-004a, contracts/propertysheet.md)
- [X] T031 [US1] Verify `Application::RunMessageLoop`'s `if (m_hConfigDialog && IsDialogMessage(m_hConfigDialog, &msg))` branch works unchanged with the property-sheet frame HWND (the frame walks active pages internally). If a manual smoke test reveals lost Tab/Enter routing, document the regression — but per R1 this should require no edit (research.md R1, plan.md "Application")
- [X] T032 [US1] Implement the 1 Hz `WM_TIMER` (id `IDT_PERF_TITLE_TIMER`) handler on the property-sheet frame in `MatrixRain\ConfigDialog.cpp`: `LoadStringW(IDS_PERFTAB_TITLE_FORMAT)` once and cache; per tick read `m_publishedFps`/`m_hasPublishedFps` from primary monitor's `MonitorRenderContext` and the existing PDH GPU% counter; when either reading is unavailable, substitute `0` for that integer (per FR-012 — the format string remains uniformly `%u`/`%u`, no placeholder token); `swprintf_s` final title; `TabCtrl_SetItem(PropSheet_GetTabControl(hSheet), 1, &item)` with `TCIF_TEXT`. Start timer in `PSCB_INITIALIZED`, kill in frame `WM_DESTROY` (FR-009, FR-010, FR-011, FR-012, contracts/propertysheet.md, research.md R2, R9)
- [X] T033 [US1] Implement dismissal semantics per FR-004a: OK → `PSN_APPLY` returns OK → controller commits live to registry, no rollback. Cancel → `PSN_RESET` → `controller->CancelLiveMode()`. X/Alt+F4 → default dialog proc routes `SC_CLOSE` through `IDCANCEL`, no explicit `WM_CLOSE` handler. Manual test: each path produces correct outcome (FR-004, FR-004a, contracts/propertysheet.md)
- [X] T033a [US1] Add `MatrixRainCore\ConfigDialogController.{h,cpp}::CommitLiveMode()`: persist every rollback-eligible field (the 5 new v1.5 fields PLUS the existing v1.4 fields that participate in live-mode rollback) to the registry via `m_settingsProvider->Save(m_currentSettings)`; clear the snapshot. Called by `PSN_APPLY` path from T033 (FR-004a, SC-011)
- [X] T033b [P] [US1] Write `MatrixRainTests\unit\ConfigDialogControllerTests.cpp::CommitLiveMode_WritesAllV15Fields`: enter live mode, mutate all 5 new v1.5 fields + 3 existing v1.4 fields, call `CommitLiveMode()`, assert all 8 fields round-tripped through `InMemorySettingsProvider`'s saved snapshot exactly. Verifies the registry-write path on OK (FR-004a, SC-011)

**Checkpoint**: Two-tab modeless property sheet renders, live-updates the Performance title, snapshot/rollback works for all 5 new fields (driven directly by tests; controls in Phases 4-6). MVP-shippable bar this far is the dialog reorg itself.

---

## Phase 4: User Story 2 — Glow Enabled Checkbox (Priority: P1)

**Goal**: Add explicit Glow on/off toggle, revert Glow Intensity min from 0
to 1, replace the intensity-zero-bypass with an enabled-flag-bypass in
`RenderSystem`, and propagate the disabled state to all glow-related
controls across both tabs with tooltips.

**Independent Test**: Per quickstart §US2 — toggling Glow Enabled OFF immediately bypasses bloom; all glow sliders on BOTH tabs disable with the correct per-tab tooltip; setting persists across runs.

### Tests for User Story 2 ⚠️

- [ ] T034 [P] [US2] Extend `MatrixRainTests\ScreenSaverSettingsTests.cpp`: `Defaults_GlowEnabledIsTrue` (Red) (FR-038, data-model.md §1)
- [ ] T035 [P] [US2] Extend `MatrixRainTests\RegistrySettingsProviderTests.cpp`: `GlowEnabledRoundTrip` writes 0 then 1 and reads back; `MissingGlowEnabledDefaultsToOne` (Red) (FR-020, FR-038)
- [ ] T036 [P] [US2] Extend `MatrixRainTests\RenderSystemTests.cpp` (or create `RenderSystemBloomBypassTests.cpp` if no existing fixture): assert that when `RenderParams::glowEnabled == false`, the bloom pipeline branch is not entered (test via observable side effect — bloom RTV not bound, or via injected mock). If `RenderSystem` is not currently unit-testable at that level, fall back to a logic-only helper `ShouldRunBloomPass(const RenderParams &)` and test that (Red) (FR-015)

### Implementation for User Story 2

- [ ] T037 [P] [US2] `MatrixRainCore\ScreenSaverSettings.h`: add `bool m_glowEnabled = true;` (default per FR-038); add accessor (Green for T034) (data-model.md §1)
- [ ] T038 [US2] `MatrixRainCore\RegistrySettingsProvider.{h,cpp}`: add `VALUE_GLOW_ENABLED = L"GlowEnabled"`; read as DWORD with default 1; write on commit (Green for T035) (FR-020, FR-038, contracts/registry-schema.md)
- [ ] T039 [US2] `MatrixRainCore\RenderSystem.{h,cpp}`: replace existing "if (intensity == 0) bypass bloom" code path with "if (!renderParams.glowEnabled) bypass bloom"; bloom resources stay allocated so re-enabling is instant (Green for T036) (FR-015)
- [ ] T040 [US2] `MatrixRain\MatrixRain.rc`: revert Glow Intensity slider min from 0 to 1 in `IDD_VISUALS_PAGE`; remove the legacy "0% (glow disabled)" special-case label control; add `IDC_GLOW_ENABLED_CHECK` `BS_AUTOCHECKBOX` to `IDD_PERFORMANCE_PAGE` between GPU Adapter combo and Quality Preset slider per FR-008 (FR-007, FR-014, FR-028a)
- [ ] T041 [US2] `MatrixRain\ConfigDialog.cpp`: remove the intensity-value handler's "value == 0 ? render 'disabled' label : render value" branch; intensity now plain integer 1..200 (FR-007)
- [ ] T042 [US2] `MatrixRain\ConfigDialog.cpp`: add `ApplyGlowEnabledUI(HWND hSheet, bool enabled)` helper per research.md R7 — `PropSheet_IndexToHwnd(hSheet, 0)` Visuals page: `EnableWindow` Glow Intensity slider + prompt label + value label + info button + Glow Size trio; `PropSheet_IndexToHwnd(hSheet, 1)` Performance page: `EnableWindow` Quality Preset / Glow Passes / Glow Resolution / Glow Smoothness trios + their info buttons (FR-016, FR-017, research.md R7)
- [ ] T043 [US2] `MatrixRain\ConfigDialog.cpp`: register comctl32 `TOOLTIPS_CLASS` tooltip on each disabled glow control with the correct per-tab text: Visuals-tab disabled controls → `L"Glow is disabled on the performance tab."`; Performance-tab disabled controls → `L"Glow is disabled."` (FR-018, FR-019)
- [ ] T044 [US2] `MatrixRain\ConfigDialog.cpp`: wire `BN_CLICKED` for `IDC_GLOW_ENABLED_CHECK` → `controller->UpdateGlowEnabled(IsDlgButtonChecked(...))` → `ApplyGlowEnabledUI(hSheet, enabled)`. In each page's `WM_INITDIALOG` (and frame `PSCB_INITIALIZED` since `PSP_PREMATURE` guarantees both HWNDs exist), call `ApplyGlowEnabledUI` with the loaded value to sync initial control state (research.md R7)

**Checkpoint**: Glow toggle persists, disables the right controls on both tabs with the right tooltips, bypasses bloom fully when off, slider min is back to 1. Independently shippable.

---

## Phase 5: User Story 3 — CRT Scanlines (Priority: P2)

**Goal**: Port `..\Casso\Casso\Shaders\CRT\scanlines.hlsl` per research.md R6
(drop luminance gating; CPU-supplies line count via `ScanlineStyleMapping`),
add three Visuals-tab controls, three registry values (defaults-on per
SC-011), wire the post-bloom-pre-present pass.

**Independent Test**: Per quickstart §US3 — fresh registry → scanlines visible on first launch; toggling Enabled OFF makes output bit-identical to no-pass; Style 1↔100 sweeps line density 981↔150; effect runs per-monitor.

### Tests for User Story 3 ⚠️

- [ ] T045 [P] [US3] Extend `MatrixRainTests\ScreenSaverSettingsTests.cpp`: `Defaults_ScanlinesEnabledIsTrue`, `Defaults_ScanlinesIntensityIs30`, `Defaults_ScanlinesStyleIs50` (Red) (FR-028, FR-038)
- [ ] T046 [P] [US3] Extend `MatrixRainTests\RegistrySettingsProviderTests.cpp`: three round-trip tests for `ScanlinesEnabled` / `ScanlinesIntensity` / `ScanlinesStyle`; clamping tests asserting registry values 0 and 200 are clamped to 1 and 100 on read for the two int values; missing-key default tests (Red) (FR-027, FR-038, data-model.md §1)
- [ ] T047 [P] [US3] Extend `MatrixRainTests\QualityPresetsTests.cpp` with `QualityPresetDoesNotMutateScanlineSettings`: drive every preset (Low/Medium/High/Custom) and assert `m_scanlinesEnabled`/`Intensity`/`Style` are untouched (Red→Green naturally) (FR-026, FR-040)

### Implementation for User Story 3

- [ ] T048 [P] [US3] `MatrixRainCore\ScreenSaverSettings.h`: add `bool m_scanlinesEnabled = true; int m_scanlinesIntensity = 30; int m_scanlinesStyle = 50;` with clamp helpers; (Green for T045) (data-model.md §1, FR-028, FR-038)
- [ ] T049 [US3] `MatrixRainCore\RegistrySettingsProvider.{h,cpp}`: add `VALUE_SCANLINES_ENABLED`, `VALUE_SCANLINES_INTENSITY`, `VALUE_SCANLINES_STYLE`; load with defaults 1/30/50; clamp int values to `[1,100]` on read and on write; first-run with no `ScanlinesEnabled` value defaults to ON (Green for T046; satisfies SC-013 / FR-028) (FR-027, FR-028, FR-038, contracts/registry-schema.md)
- [ ] T050 [US3] Port `..\Casso\Casso\Shaders\CRT\scanlines.hlsl` → `MatrixRainCore\Shaders\scanlines.hlsl`: keep upstream SPDX/attribution header verbatim; add 1-line "MatrixRain modifications" note; delete `kNativeScanlines = 192.0`; delete the luminance gating (`lum`, `weight`, surrounding `lerp`); collapse cbuffer to 16-byte `ScanlineCb { float g_intensity; float g_linesPerHeight; float g_padding0; float g_padding1; }` per contracts/scanline-shader.md; `darken = lerp(1.0 - g_intensity, 1.0, bright);` (FR-022, FR-023, FR-024, FR-024a, research.md R6)
- [ ] T051 [US3] `MatrixRainCore\RenderSystem.{h,cpp}`: add C++ POD `struct alignas(16) ScanlineCb { float intensity; float linesPerHeight; float pad0; float pad1; }; static_assert(sizeof(ScanlineCb) == 16);`. Add `m_postBloomTarget` (Texture2D + RTV + SRV, same format/dims as backbuffer) created alongside existing bloom resources. Compile new shader as `m_scanlinePS`. Reroute bloom composite to render into `m_postBloomTarget` when scanlines enabled; scanline pass samples it as `t0` and renders into backbuffer. When scanlines disabled, bypass `m_postBloomTarget` (bloom composite → backbuffer direct, as today). Both disabled → unchanged direct-to-backbuffer character path (FR-021, FR-025, FR-028b, research.md R6, contracts/scanline-shader.md)
- [ ] T052 [US3] `MatrixRainCore\RenderSystem.cpp` per-frame top: compute `scanlinesIntensity = settings_intensity / 100.0f; scanlinesLineCount = ScanlineLineCount(settings_style);` and stash in `RenderParams`; upload `ScanlineCb` once per frame (or only when dirty) via `Map`/`Unmap`; skip the pass entirely when `!renderParams.scanlinesEnabled` (FR-023, FR-024, FR-028b)
- [ ] T053 [US3] `MatrixRain\MatrixRain.rc`: add Scanlines groupbox to `IDD_VISUALS_PAGE` after Color combo and before Start In Fullscreen, containing top→bottom: `IDC_SCANLINES_ENABLED_CHECK` (`BS_AUTOCHECKBOX`, checked by default in template), `IDC_SCANLINES_INTENSITY_SLIDER` (range 1..100, default 30) + label trio + info button, `IDC_SCANLINES_STYLE_SLIDER` (range 1..100, default 50) + label trio + info button (FR-005, FR-028a, FR-028c)
- [ ] T054 [US3] `MatrixRain\ConfigDialog.cpp`: in `VisualsPageDlgProc` `WM_INITDIALOG` sync controls from settings; `WM_COMMAND` `BN_CLICKED` for the checkbox → `controller->UpdateScanlinesEnabled(...)` + locally `EnableWindow(FALSE/TRUE)` on the two slider trios + info buttons; `WM_HSCROLL` for the two sliders → `controller->UpdateScanlinesIntensity/Style(...)` (FR-028b)
- [ ] T055 [US3] Migration sanity: with no `Scanlines*` registry values present, launching `MatrixRain.exe /c` shows scanlines ON in preview on first frame (matches SC-013 / quickstart §US6 step 2) (FR-028, SC-013)

**Checkpoint**: Scanlines render by default, three controls work live, three registry values round-trip with clamping, quality presets remain hands-off. Independently shippable.

---

## Phase 6: User Story 5 — Custom Color Picker (Priority: P2)

**Goal**: Add `ColorScheme::Custom`, wire the `Custom…` combo entry to
`ChooseColorW`, support same-item re-click force-reopen via a subclass-based
combo watcher per research.md R4 (CB_GETDROPPEDSTATE on WM_LBUTTONUP /
WM_KEYUP), persist the 16-swatch palette unconditionally as REG_BINARY
per R5, route the active custom RGB through `SharedState` to the render
thread.

**Independent Test**: Per quickstart §US4 — Custom… opens chooser pre-populated with RGB(0,255,0) on first use; re-clicking Custom while Custom is already active re-opens chooser; palette edits survive outer Cancel; active CustomColor rolls back on Cancel.

### Tests for User Story 5 ⚠️

- [ ] T056 [P] [US5] Create `MatrixRainTests\unit\ColorSchemeTests.cpp` (or extend existing): assert `ColorScheme::Custom == 5` (NOT 4 — Custom is appended past the existing v1.4 ordinals Green=0, Blue=1, Red=2, Amber=3, ColorCycle=4 per FR-039 / SC-007); assert all v1.4 ordinals unchanged; round-trip through registry; assert `RenderSystem`-side selection logic picks `customColor` when scheme == Custom (via a small pure helper if RenderSystem isn't directly testable) (Red) (FR-033, FR-034, data-model.md §2)
- [ ] T057 [P] [US5] Extend `MatrixRainTests\RegistrySettingsProviderTests.cpp`: `CustomColorRoundTrip`, `CustomColorAbsentMeansChooserDefault`, `CustomColorPaletteRoundTrip` (write 64 bytes, read back identical), `CustomColorPaletteSizeMismatchYieldsZeroes` (write 32 bytes, read returns all zeroes), and `MissingCustomColorPaletteYieldsZeroes` (Red) (FR-030, FR-031, FR-035, FR-038, data-model.md §1, contracts/registry-schema.md)
- [ ] T058 [P] [US5] Extend `MatrixRainTests\ConfigDialogControllerTests.cpp`: `CustomColorRollsBackOnCancel`, `CustomColorPaletteIsNOTRolledBackOnCancel` (verifies palette stays mutated in `ScreenSaverSettings` after a `CancelLiveMode()` call — palette lives outside snapshot per FR-035) (Red) (FR-004, FR-035)

### Implementation for User Story 5

- [ ] T059 [P] [US5] `MatrixRainCore\ColorScheme.{h,cpp}`: append `Custom = 5` to `enum class ColorScheme` (after the existing `ColorCycle = __StaticColorCount` line). Do NOT renumber existing variants. Update any to-string / from-string helpers to handle the new variant (Green for T056) (FR-033, data-model.md §2)
- [ ] T060 [P] [US5] `MatrixRainCore\ScreenSaverSettings.h`: add `COLORREF m_customColor = RGB(0,255,0);` and `std::array<COLORREF, 16> m_customColorPalette {};` with clear comment that palette is unconditionally-persisted (FR-035 carve-out from rollback) (data-model.md §1, FR-038)
- [ ] T061 [US5] `MatrixRainCore\RegistrySettingsProvider.{h,cpp}`: add `LoadCustomColor`/`SaveCustomColor` (REG_DWORD, default-absent = no write, chooser substitutes RGB(0,255,0)); add `LoadCustomColorPalette`/`SaveCustomColorPalette` (REG_BINARY, exact 64 bytes else zero-fill); both palette helpers `memcpy` directly to/from `COLORREF[16]` matching `CHOOSECOLORW::lpCustColors` layout (Green for T057) (FR-030, FR-031, FR-035, FR-038, contracts/registry-schema.md, research.md R5)
- [ ] T062 [US5] `MatrixRainCore\RenderSystem.{h,cpp}`: when `renderParams.colorScheme == ColorScheme::Custom`, use `renderParams.customColor` as the streak color in lieu of the four built-in palettes (Green for T056's render-side assertion) (FR-033, FR-034)
- [ ] T063 [US5] `MatrixRain\MatrixRain.rc`: add `L"Custom…"` as the sixth entry in `IDC_COLORSCHEME_COMBO` on `IDD_VISUALS_PAGE` (ordinal 5, after Green/Blue/Red/Amber/Cycle — v1.4 ordinals 0..4 preserved per FR-039) (FR-006)
- [ ] T064 [US5] `MatrixRain\ConfigDialog.cpp`: implement the Color combo handler. Handle `CBN_SELCHANGE`: if new index is Custom → open chooser; else update scheme normally. Subclass the combo via `SetWindowSubclass`; in the subclass proc on `WM_LBUTTONUP` and `WM_KEYUP(VK_RETURN)`, capture `CB_GETDROPPEDSTATE` and `CB_GETCURSEL` before calling `DefSubclassProc`; after, if the dropdown was open AND `CB_GETCURSEL` is the Custom index AND no `CBN_SELCHANGE` fires (same-item commit), post a `WM_APP+1` to the page to force-open the chooser. Always update a page-local `s_lastColorComboIndex` after a handled event (FR-029, research.md R4)
- [ ] T065 [US5] `MatrixRain\ConfigDialog.cpp`: implement `OpenCustomColorChooser(HWND hSheet)` — populate `CHOOSECOLORW` with `hwndOwner = hSheet`, `Flags = CC_FULLOPEN | CC_RGBINIT | CC_ANYCOLOR`, `rgbResult = current m_customColor or RGB(0,255,0)`, `lpCustColors = &settings.m_customColorPalette[0]`. On OK: `SaveCustomColorPalette` (unconditional per FR-035), `SaveCustomColor`, `controller->UpdateCustomColor(rgb)` (live push), set `ColorScheme::Custom` via `controller->UpdateColorScheme(Custom)`. On Cancel: no writes (palette stays in registry from any prior OK) (FR-029, FR-030, FR-031, FR-032, FR-035, research.md R5)
- [ ] T066 [US5] `MatrixRain\ConfigDialog.cpp`: on dialog open, `RegistrySettingsProvider::LoadCustomColorPalette` populates `settings.m_customColorPalette` so the first chooser invocation already has any prior swatches (Green for T058's palette-persistence test path) (FR-035, research.md R5)

**Checkpoint**: Custom color picker works end-to-end including same-item re-click; palette persists unconditionally; active color rolls back on Cancel; render thread renders custom RGB.

---

## Phase 7: User Story 6 — Upgrade Migration Note (Priority: P3)

**Goal**: Document the intentional default-on scanlines visual change for
v1.3/v1.4 users in the CHANGELOG, with the one-click disable path.

**Independent Test**: `CHANGELOG.md` v1.5 section contains a prominent
upgrade note mentioning the default-on scanlines change and the disable
path `Visuals → Scanlines Enabled → OFF`.

- [ ] T067 [US6] `CHANGELOG.md`: add v1.5 entry calling out (a) the intentional visible change on upgrade (existing installs see scanlines render on first launch because `ScanlinesEnabled` defaults to ON), (b) the one-click disable path (Visuals tab → Scanlines Enabled → OFF), (c) summary of other v1.5 features (two-tab property sheet, Glow Enabled toggle, Custom color picker, fade-timer overlay removal). Tone per .github/copilot-instructions.md — neutral and professional (NOT snarky) for artifacts (FR-028, SC-013, quickstart §US6)

**Checkpoint**: Upgrade message in place for the v1.5 release.

---

## Phase 8: Polish & Cross-Cutting

- [ ] T068 ARM64 Debug build: `& "C:\Program Files\Microsoft Visual Studio\18\Enterprise\MSBuild\Current\Bin\MSBuild.exe" MatrixRain.sln /p:Configuration=Debug /p:Platform=ARM64 /m` — must succeed with zero `/W4 /WX` warnings (plan Constitution III, quickstart "Definition of done")
- [ ] T069 ARM64 Release build same as T068 with `/p:Configuration=Release` (quickstart "Definition of done")
- [ ] T070 Run `quickstart.md` manual acceptance walkthrough end-to-end for US1..US6; log any deviation as a follow-up task (quickstart, SC-001..SC-011)
- [ ] T071 `git --no-pager grep -i "fadetimer\|fade.timer" -- MatrixRain MatrixRainCore MatrixRainTests` returns zero matches (quickstart §US5, FR-036)
- [ ] T072 Run full `vstest.console.exe x64\Debug\MatrixRainTests.dll /Settings:MatrixRainTests.runsettings` — 100% pass (SC-008)
- [ ] T072a `MatrixRainTests\unit\V14SettingsRegressionTests.cpp`: end-to-end round-trip — load a populated v1.4-shaped settings hive, save it back via `RegistrySettingsProvider`, reload, assert every v1.4 field is byte-identical to its original. Asserts the v1.5 additions don't corrupt or alter any v1.4 field on read/write paths (SC-007)
- [ ] T073 Lint/format pass across all touched files: verify the 5-blank-line top-level rule, the 3-blank-line variable-block rule, column alignment of declarations / assignments / pointer symbols / HLSL semantics per .github/copilot-instructions.md
- [ ] T074 Flip `specs\007-dialog-tabs-scanlines-glowtoggle\spec.md` frontmatter `Status:` from `Draft` to `Implemented`
- [ ] T075 Final tick-through: every `[ ]` in this `tasks.md` flipped to `[x]`; commit with `chore(speckit): mark v1.5 tasks complete`

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 (Foundational)**: no dependencies; T002/T003/T004/T005/T006 are [P], T007 depends on T005+T006
- **Phase 2 (US4 Fade-Timer Removal)**: depends on Phase 1 only; sequenced BEFORE Phase 3 per research.md R8 to clean the dialog before restructure
- **Phase 3 (US1 Property Sheet)**: depends on Phase 2 (clean dialog)
- **Phase 4 (US2 Glow Enabled)**: depends on Phase 3 (uses new page templates, `PSP_PREMATURE` cross-tab propagation, `ConfigDialogController::UpdateGlowEnabled`)
- **Phase 5 (US3 Scanlines)**: depends on Phase 3 (page templates, controller accessors); independent of Phase 4
- **Phase 6 (US5 Custom Color)**: depends on Phase 3 (controller accessors); independent of Phases 4 & 5
- **Phase 7 (US6 CHANGELOG)**: depends on Phase 5 (mentions default-on scanlines behaviour)
- **Phase 8 (Polish)**: depends on Phases 2–7

### Within Phases

- Tests pair Red→Green with their implementation tasks; do not commit impl without seeing the paired test fail first (constitution principle I)
- One task = one commit, conventional-commits format (constitution principle IX)

### Parallel Opportunities

- Phase 1: T002, T003, T004, T005 can run in parallel; T006 parallel after T004/T005
- Phase 2: T008, T009 in parallel (different test files); T013, T014, T015 in parallel (different production files, no shared symbols)
- Phase 3: T021, T022, T024, T026, T027 are [P] (different files)
- Phase 4: T034, T035, T036, T037 are [P] (different files); T044 is the wiring task that depends on T040/T042/T043
- Phase 5: T045, T046, T047 are [P]; T048 [P]; T050 (shader port) parallel with T048; T051..T054 sequential within RenderSystem/ConfigDialog
- Phase 6: T056, T057, T058 are [P]; T059, T060 are [P]; T061..T066 sequential
- After Phase 3 completes, Phases 4, 5, 6 can be developed in parallel by different contributors

---

## Parallel Example: Phase 5 (Scanlines) test kickoff

```text
# Three test files, three different fixtures, no shared state — start together:
Task T045: edit MatrixRainTests\ScreenSaverSettingsTests.cpp
Task T046: edit MatrixRainTests\RegistrySettingsProviderTests.cpp
Task T047: edit MatrixRainTests\QualityPresetsTests.cpp

# In parallel, shader port + settings field can also begin:
Task T048: edit MatrixRainCore\ScreenSaverSettings.h
Task T050: create MatrixRainCore\Shaders\scanlines.hlsl
```

---

## Implementation Strategy

### MVP (ship after Phase 3)

The v1.5 MVP is the property-sheet reorg with live perf readout. After
Phases 1–3 land, the dialog is structurally correct and snapshot/rollback
covers the 5 new fields even though no new control exists yet for the
user to set them. That is shippable as an internal milestone.

### Incremental Delivery (recommended)

1. Phases 1 + 2 + 3 → tag `v1.5-mvp` (dialog reorg + fade-timer gone + live perf readout)
2. Phase 4 → tag `v1.5-glow-toggle`
3. Phase 5 → tag `v1.5-scanlines`
4. Phase 6 → tag `v1.5-custom-color`
5. Phase 7 + Phase 8 → tag `v1.5`

### Parallel Team Strategy

After Phase 3 closes, three contributors can pick up Phase 4, Phase 5, and
Phase 6 independently. The only shared file at risk is `MatrixRain.rc`
(both Phase 4 and Phase 5 add controls to it; Phase 6 adds a combo entry).
Phase 5 owns the Visuals-page edits, Phase 4 owns the Performance-page
edits, Phase 6 owns one combo entry on the Visuals page — coordinate via
short-lived merges or take .rc changes sequentially.

---

## Notes

- [P] = different files, no symbol overlap, safe to start in parallel
- [Story] tag traces every task back to the user-input phase plan (US4 = fade-timer removal, US5 = custom color picker — these intentionally differ from spec.md's US ordinals; the FR/SC hints on each task are the authoritative spec linkage)
- Test deletions in Phase 2 count as "tests" for constitution principle I purposes — they pair 1:1 with the production-symbol removals
- Manual-only verifications are explicitly marked (T020, T031, T033, T055, T068–T072)
- `customColorPalette` is the ONLY new setting NOT in the snapshot — every other v1.5 setting rolls back on Cancel per FR-004 / FR-035
