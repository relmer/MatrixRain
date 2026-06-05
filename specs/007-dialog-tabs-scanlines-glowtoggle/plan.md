# Implementation Plan: Settings Dialog Overhaul, Scanlines & Glow Toggle (v1.5)

**Branch**: `007-dialog-tabs-scanlines-glowtoggle` | **Date**: 2026-02-23 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/specs/007-dialog-tabs-scanlines-glowtoggle/spec.md`

## Summary

Restructure the settings UI from a single modeless dialog into a two-page modeless
`PropertySheetW` (Visuals / Performance) with a 1 Hz timer that pushes live FPS and
PDH GPU% into the Performance tab title via `PSM_SETTITLE`. Add a CRT scanlines
post-process pass (ported from Casso, modified to drop source-luminance gating and
to receive a CPU-computed line count), three new Visuals-tab controls (Scanlines
Enabled / Intensity / Style), an explicit `Glow Enabled` checkbox on the Performance
tab that fully bypasses the bloom pipeline and disables glow-related controls on
both tabs, and a `Custom…` color combo entry that opens `ChooseColorW` (palette
persisted unconditionally; same-item re-click forces re-open via `CBN_SELENDOK`).
Excise the orphaned fade-timer debug overlay top-to-bottom before the tab
restructure so the dialog code is clean when it is split. OK commits live values
to the registry; Cancel / X / Alt+F4 all roll back via the existing
`ConfigDialogController` snapshot machinery (palette excepted).

## Technical Context

**Language/Version**: C++23 (`/std:c++latest`)
**Primary Dependencies**: Win32 (`comctl32` v6 property sheets, `commdlg` ChooseColorW),
D3D11 + D2D + DirectWrite, PDH (GPU%), existing `MonitorRenderContext` /
`RenderSystem` / `ConfigDialogController` / `RegistrySettingsProvider` /
`QualityPresets` / `SharedState` from v1.4
**Storage**: Windows registry under the existing screensaver settings key
(`RegistrySettingsProvider`) — six new values: `GlowEnabled`, `ScanlinesEnabled`,
`ScanlinesIntensity`, `ScanlinesStyle`, `CustomColor`, `CustomColorPalette`
**Testing**: Microsoft C++ Native Unit Test Framework (`<CppUnitTest.h>`) via
`MatrixRainTests` linking `MatrixRainCore.lib`
**Target Platform**: Windows 10 22H2 / Windows 11 (x64 + ARM64), per-monitor v2 DPI
**Project Type**: Desktop screensaver (Win32 .scr / .exe split into
`MatrixRainCore.lib` + `MatrixRain.exe`)
**Performance Goals**: 60 fps per monitor preserved; scanline pass cost ≤0.2 ms at
4K (single fullscreen-triangle pass, one sample, no branching); Performance-tab
title refresh ≤1 s after underlying readings change
**Constraints**: No new external dependencies (no NuGet, no DLLs); existing v1.4
registry values continue to read/write unchanged (FR-039); palette REG_BINARY
round-trips bit-identically to `CHOOSECOLORW::lpCustColors`; live-preview /
Cancel-rollback must cover every new setting except `CustomColorPalette`
**Scale/Scope**: ~13 new functional requirements groups, 5 user stories, 6 new
registry values, 1 new shader, 2 new property pages, ~30+ touched files (the
fade-timer removal alone hits ApplicationState / ScreenSaverSettings /
RegistrySettingsProvider / RenderSystem / SharedState / RenderParams /
ConfigDialogController / .rc / 3 test files)

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Status | Notes |
|---|---|---|
| I. TDD (NON-NEGOTIABLE) | PASS | Tasks plan pairs every implementation task with a preceding/parallel test task in `MatrixRainTests`. Dialog message handlers that are pure Win32 plumbing (controller-free) are exempt under the standard "thin UI shell" carve-out already used in v1.3/v1.4; everything routed through `ConfigDialogController`, `RegistrySettingsProvider`, `QualityPresets`, `SharedState` push, and the new scanline-style mapping (`lines = 1000 * pow(0.15, style/100)`) is unit-tested. |
| II. Performance-First | PASS | Scanline pass is one fullscreen triangle, one texture sample, no branches; FPS publisher is `std::atomic<float>` with relaxed read on the dialog thread (no lock); GPU% reuses the already-amortised PDH counter from the debug overlay. Title update is rate-limited to 1 Hz to avoid `PSM_SETTITLE` thrash. |
| III. C++23 / Windows Native | PASS | Continues to compile `/std:c++latest`; no APIs older than Win10 1809 (`PSM_SETTITLE` + comctl6 + ChooseColorW + PDH all predate that floor). ARM64 verified in polish phase. |
| IV. Modular Architecture | PASS | New work decomposes cleanly: scanline shader + cbuffer struct live in `RenderSystem`; FPS publisher is a single atomic on `MonitorRenderContext`; ConfigDialogController gains accessors for the new settings; the page split is two new templates + two `DLGPROC`s, no controller surgery beyond accessor additions. |
| V. Type Safety | PASS | `ColorScheme::Custom` is an `enum class` variant; scanline style→lines mapping returns `float`; constant buffer is a POD struct with explicit 16-byte padding (`static_assert(sizeof == 16)`). |
| VI. Library-First | PASS | All logic (settings I/O, snapshot/rollback extension, scanline math, style mapping, color/palette serialisation helpers) lands in `MatrixRainCore.lib`. The two new property-page `DLGPROC`s live in `MatrixRain.exe` (Win32 UI shell), matching where `ConfigDialog.cpp` lives today. |
| VII. Precompiled Headers | PASS | `<commdlg.h>`, `<prsht.h>`, additional `<atomic>` usage all surface through `MatrixRain/pch.h` (UI shell) and `MatrixRainCore/pch.h` (atomic is already present). No `#include <…>` outside pch. |
| VIII. Formatting | PASS | All new code obeys the project's column-aligned declaration rules; 5 blank lines between top-level constructs; pointer/reference column rule; HLSL struct-member column alignment for the scanline shader. |
| IX. Commit Discipline | PASS | Tasks plan is structured so each task → one commit, build+test green before commit, conventional-commits message. Pre-plan hook may auto-commit any stray staged state. |

**Initial gate**: PASS. No deviations to justify.

**Post-Phase-1 re-check**: PASS (see end of Phase 1 below). No new violations
introduced by the data model, contracts, or scanline shader port.

## Project Structure

### Documentation (this feature)

```text
specs/007-dialog-tabs-scanlines-glowtoggle/
├── plan.md              # This file
├── spec.md              # Feature spec (clarifications resolved)
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output
├── quickstart.md        # Phase 1 output
├── contracts/           # Phase 1 output
│   ├── propertysheet.md     # PSH_* / PSP_* flag matrix, page templates, PSM_SETTITLE contract
│   ├── registry-schema.md   # v1.5 registry additions + legacy ShowFadeTimers handling
│   ├── scanline-shader.md   # cbuffer layout, intensity/lines inputs, draw-order placement
│   └── fps-publisher.md     # MonitorRenderContext atomic FPS publisher read/write contract
└── tasks.md             # Phase 2 output (/speckit.tasks — NOT created here)
```

### Source Code (repository root)

```text
MatrixRainCore/                          # static library — all testable logic
├── ApplicationState.{h,cpp}             # remove fade-timer accessors (FR-036)
├── ColorScheme.{h,cpp}                  # add ColorScheme::Custom variant (FR-033)
├── ConfigDialogController.{h,cpp}       # extend snapshot/rollback for 5 new fields (FR-044)
├── ConfigDialogSnapshot.h               # add GlowEnabled, ScanlinesEnabled/Intensity/Style, CustomColor
├── FPSCounter.{h,cpp}                   # unchanged, reused by atomic publisher
├── MonitorRenderContext.{h,cpp}         # add std::atomic<float> m_publishedFps; write per frame, lock-free read (FR-010)
├── QualityPresets.{h,cpp}               # unchanged (scanlines deliberately excluded; FR-026, FR-040)
├── RegistrySettingsProvider.{h,cpp}     # remove VALUE_SHOW_FADE_TIMERS; add 6 new values (FR-027, FR-031, FR-035, FR-036, FR-037, FR-038)
├── RenderParams.h                       # remove showDebugFadeTimes; add glowEnabled, scanlinesEnabled, scanlinesIntensity, scanlinesLineCount, colorScheme.customRgb
├── RenderSystem.{h,cpp}                 # remove RenderDebugFadeTimes; bypass bloom pipeline when !glowEnabled; add scanline post-pass + cbuffer (FR-015, FR-021..FR-025, FR-028b)
├── ScreenSaverSettings.h                # remove m_showFadeTimers; add the 6 new settings + ColorScheme::Custom support
├── SharedState.h                        # remove showDebugFadeTimes; add 5 new live fields + snapshot mirrors
└── (new) ScanlineStyleMapping.{h,cpp}   # pure function: lines = 1000 * pow(0.15, style/100.0); unit-tested in isolation

MatrixRainCore/Shaders/                  # new shaders directory (mirrors Casso layout)
└── scanlines.hlsl                       # ported from ../Casso/Casso/Shaders/CRT/scanlines.hlsl with modifications per FR-023, FR-024, FR-024a

MatrixRain/                              # thin Win32 UI shell .exe
├── ConfigDialog.{h,cpp}                 # REPLACED: now creates PropertySheetW, hosts two page DLGPROCs, owns 1 Hz title timer, wires CBN_SELENDOK custom-color force-open, ChooseColorW + palette REG_BINARY round-trip
├── MatrixRain.rc                        # REPLACED: 2 DIALOG templates (Visuals page, Performance page), removed Show Fade Timers checkbox, new string-table entry "Performance (%u fps, %u%% GPU)", new control IDs in resource.h
├── MatrixRain.manifest                  # already declares comctl32 v6 + per-monitor v2 DPI — verify no change needed
├── main.cpp                             # unchanged
└── resource.h                           # new control IDs for both pages; remove IDC_SHOWFADETIMERS_CHECK

MatrixRainCore/Application.{h,cpp}       # message loop: IsDialogMessage already handles modeless property sheets correctly; verify PropertySheet HWND substitution still works (it is the outer frame HWND returned by PropertySheetW with PSH_MODELESS)

MatrixRainTests/                         # all new logic gets paired tests
├── ConfigDialogControllerTests.cpp      # delete fade-timer cases; add cases for 5 new snapshot/rollback fields, palette persistence-on-cancel exception, custom-color rollback
├── RegistrySettingsProviderTests.cpp    # delete fade-timer cases; add round-trip tests for 6 new values + legacy ShowFadeTimers silently-ignored test (FR-037)
├── ScreenSaverSettingsTests.cpp         # delete fade-timer cases; defaults test for 6 new fields (FR-038)
├── ScanlineStyleMappingTests.cpp        # NEW: verify lines @ {1, 25, 50, 75, 100} match spec values within tolerance (FR-023)
├── ColorSchemeTests.cpp                 # NEW or extended: ColorScheme::Custom round-trip; rendering selects customRgb when scheme == Custom
└── QualityPresetsTests.cpp              # verify scanlines are NOT touched by presets (FR-026, FR-040)
```

**Structure Decision**: Single-project-pair layout (already established for v1.4):
testable logic in `MatrixRainCore`, thin Win32 shell in `MatrixRain`, tests in
`MatrixRainTests`. The new shader sits under `MatrixRainCore/Shaders/` so future
ports (more Casso effects) have an obvious home. The property-sheet page DLGPROCs
stay in `MatrixRain/ConfigDialog.cpp` because they are pure UI plumbing; every
piece of state they touch is delegated to `ConfigDialogController`.

## Complexity Tracking

No constitutional violations. Section intentionally empty.
