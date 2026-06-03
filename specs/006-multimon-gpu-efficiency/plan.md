# Implementation Plan: Multi-Monitor User Control and GPU Efficiency

**Branch**: `006-multimon-gpu-efficiency` | **Date**: 2026-06-03 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/specs/006-multimon-gpu-efficiency/spec.md`

## Summary

Five coordinated improvements to MatrixRain v1.3.1984 (already-merged multi-monitor base):

1. **Multi-monitor optional** — A persisted on/off setting (default on) gates the existing per-monitor fan-out path.
2. **Runtime topology + device-loss response** — `WM_DISPLAYCHANGE` and `Present` HRESULT inspection drive a coalesced rebuild of monitor render contexts, fixing the ghost-monitor render thread that today holds GPU at ~90% after an undock.
3. **GPU adapter selection** — A new `IAdapterProvider`/`AdapterInfo` trio (mirroring the existing monitor-provider pattern) plus a config-dialog combobox lets the user pick the rendering adapter on hybrid laptops. `D3D11CreateDevice` is changed to accept a specific adapter, and persistence is by adapter description string.
4. **Frame-rate cap on high-refresh monitors** — A pure `FrameLimiter` helper engages per monitor only when the monitor's native refresh exceeds 60 Hz, capping to 60 FPS; at ≤60 Hz the existing `Present(1, 0)` vsync path is preserved with no limiter overhead.
5. **Graphics quality preset spectrum** — A new "Quality" combobox (Low / Medium / High / Custom) bundles per-knob quality settings; an advanced disclosure toggle reveals three discrete sliders (Passes / Resolution / Smoothness) plus accessible information tips on every quality control. The existing Glow Intensity slider gains a true "0% = disabled" mode that bypasses the bloom pipeline. First-run defaults are chosen by a pure static GPU-capability heuristic.

The plan preserves the existing architecture (per-monitor `MonitorRenderContext`, central `Application` coordinator, `RegistrySettingsProvider`, `ConfigDialog`/`ConfigDialogController` split, in-memory provider pattern for tests) and extends it without breaking changes. Net code shape is +1 provider trio, +5 pure helpers, +1 dialog group with disclosure, +5 settings fields. The "High" quality preset is intentionally calibrated to today's exact rendering so existing users see no visible change after upgrade.

## Technical Context

**Language/Version**: C++23 (`/std:c++latest`), MSVC v18 (Visual Studio 2026 Enterprise).  
**Primary Dependencies**: D3D11 (existing), DXGI — **add `<dxgi1_6.h>` to `pch.h`** for `IDXGIFactory6::EnumAdapterByGpuPreference`; Win32 (existing trackbars, dialogs; new tooltip control `WC_TOOLTIP` and owner-draw button); existing EHM (Error Handling Macros) and ComPtr.  
**Storage**: Windows registry (existing key `HKCU\Software\relmer\MatrixRain`); new values `MultiMonitor` (DWORD), `GpuAdapter` (REG_SZ), `QualityPreset` (REG_SZ), `LastCustom_Passes` / `LastCustom_Resolution` / `LastCustom_Smoothness` / `LastCustom_GlowIntensity` (DWORDs), `ShowAdvancedGraphics` (DWORD). Persisted via the existing `RegistrySettingsProvider` helpers.  
**Testing**: Microsoft C++ Native Unit Test Framework (`<CppUnitTest.h>`) via the existing `MatrixRainTests.vcxproj`. Run with `vstest.console.exe` at `…\Common7\IDE\Extensions\TestPlatform\vstest.console.exe`. Existing baseline: **354 tests passing on master**.  
**Target Platform**: Windows 11 x64 only. Modern DXGI GPU-preference APIs (`IDXGIFactory6`) are available; legacy `NvOptimusEnablement`/`AmdPowerXpressRequestHighPerformance` exports are intentionally **not** added in v1.4 (see research.md).  
**Project Type**: Desktop application (`MatrixRainCore.lib` + `MatrixRain.exe` + `MatrixRainTests.dll`).  
**Performance Goals**: (1) On a >60 Hz monitor, render at ≤65 FPS, reducing GPU work by ≥50% relative to v1.3 baseline. (2) On a Surface-class integrated GPU with one Full-HD monitor, GPU utilization ≤70% of v1.3 baseline. (3) Topology change and GPU selection change take effect within 1 second. (4) On undock from a multi-monitor system, GPU utilization within 1 second drops to within 10% of "started in undocked state" baseline.  
**Constraints**: Warnings-as-errors clean (`/W4 /WX /sdl`); no `/m` flag for MSBuild (PCH `C3859/C1076` transient memory failures); auto-bumped `Version.h` excluded from commits; existing settings keys/value layout must remain compatible (no migration of pre-existing values); single-monitor users see no visible change; "High" preset must be visually identical to today's default; flicker on dialog resize must be minimized (`SetWindowPos` with `SWP_NOZORDER | SWP_DRAWFRAME` + suppressing `WM_NCCALCSIZE` redraw bursts).  
**Scale/Scope**: 5 user stories, 39 functional requirements, 10 success criteria. Code estimate ~2,000 new LOC (~800 core + helpers, ~500 dialog/UI, ~700 tests). One feature branch, multiple atomic commits per constitution.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-evaluated after Phase 1 design (re-evaluation results identical — no design choices conflict with any principle).*

| Principle | Compliance | Notes |
|-----------|------------|-------|
| **I. TDD (non-negotiable)** | PASS | All pure helpers (`ShouldSpanAllMonitors` core, `ResolveAdapter`, `FormatAdapterLabel`, `IsDeviceLost`, `FrameLimiter`, `ShouldEngageFrameLimiter`, `PickDefaultQualityPreset`, `ApplyPresetSnap`, `DetectCustomDrift`, `LookupQualityPresetValues`, `LookupResolutionDivisor`, `LookupBlurTapCount`, `IsGlowDisabled`, `CoalesceRebuildRequest`) live in `MatrixRainCore` and are TDD'd against `MatrixRainTests`. Win32 dialog wiring, owner-draw of the info indicator, and DXGI device creation are out of TDD scope per the constitution's exemption for entry-point/UI code; they receive manual QA per the success criteria and the existing in-memory provider pattern for any logic they invoke. |
| **II. Performance-First** | PASS | Performance is the primary motivator. Frame cap, idle bloom-pipeline skip (via Glow Intensity 0), preset-driven resolution divisor, kernel-tap variants, and the elimination of the ghost-monitor render thread are all measured against the existing v1.3 baseline as success criteria SC-001/SC-004/SC-005. |
| **III. C++23 + Windows native** | PASS | No new tech stack. New code uses `std::optional`, structured bindings, `std::format`, `std::chrono::steady_clock`, `std::ranges` where idiomatic. Wide-string everywhere (`L"…"`, `wstring`/`wstring_view`). |
| **IV. Modular architecture** | PASS | New code introduces one new interface (`IAdapterProvider`) with two implementations (`WindowsAdapterProvider`, `InMemoryAdapterProvider`) and ≤14 pure free-function helpers grouped by concern (`AdapterSelection.{h,cpp}`, `FrameLimiter.{h,cpp}`, `QualityPresets.{h,cpp}`, `MultiMonitorGate.{h,cpp}`, `DeviceLost.{h,cpp}`, `RebuildCoalescer.{h,cpp}`). Dependencies flow one direction: providers/helpers ← `Application` ← `MonitorRenderContext` ← `RenderSystem`. No circular dependencies. |
| **V. Type safety + modern idioms** | PASS | `enum class QualityPreset`, `enum class ResolutionDivisor`, `enum class BlurTaps`. `std::optional<wstring>` for "no GPU saved". ComPtr for COM lifetimes. `const`-correct throughout. |
| **VI. Library-first** | PASS | All logic in `MatrixRainCore.lib`. `MatrixRain.exe` gains only `ConfigDialog.cpp` UI glue (already its role today), `.rc` resource changes, and `resource.h` IDs. No business logic added to `.exe`. |
| **VII. PCH** | PASS | One new include (`<dxgi1_6.h>`) added to `pch.h` in the appropriate alphabetical group. No system headers added to other files. Test PCH continues to `#include` core PCH. |
| **VIII. Code formatting and style** | PASS | All new code follows the existing project conventions: 5 blank lines between top-level constructs, column-aligned declarations with `*`/`&` as their own column, function header comment banners (80 char), Win32 trackbar/combo pattern matching `ConfigDialog.cpp` precedent. Hungarian-prefix discipline preserved where the surrounding code uses it; modern names elsewhere. |
| **IX. Commit discipline** | PASS | Tasks.md (next phase) breaks work into atomic units, each producing a single commit after build + full test pass. Sequencing: spec/plan/research/data-model/quickstart/contracts (one commit total — this branch initialization) → then per-task commits. |

**Verdict**: No violations. Complexity Tracking section omitted (nothing to justify).

## Project Structure

### Documentation (this feature)

```text
specs/006-multimon-gpu-efficiency/
├── spec.md                                  # Feature spec (already created)
├── plan.md                                  # This file
├── research.md                              # Phase 0 — decisions/rationale/alternatives
├── data-model.md                            # Phase 1 — entities, fields, validation
├── quickstart.md                            # Phase 1 — how to validate the feature
├── contracts/
│   ├── registry-schema.md                   # Persistent settings: keys, types, defaults
│   ├── adapter-provider.md                  # IAdapterProvider/AdapterInfo contract
│   ├── quality-preset-mapping.md            # Preset → knob values + custom-snap rules
│   └── tick-mark-conventions.md             # Slider tick frequencies and labeling rules
├── checklists/
│   └── requirements.md                      # Spec quality checklist (already created)
└── tasks.md                                 # (Phase 2 — created by /speckit.tasks, not by /speckit.plan)
```

### Source code (repository root)

This is a single-product Windows desktop application using the existing three-project Visual Studio solution (per Constitution VI). No new top-level project is added; all new code lands in the existing tree.

```text
MatrixRain.sln
│
├── MatrixRainCore/                          # Static library (all logic, tested)
│   ├── pch.h                                # + <dxgi1_6.h> in DirectX/DXGI block
│   │
│   ├── ScreenSaverSettings.h                # + multimon/gpu/preset/last-custom fields
│   ├── ISettingsProvider.h                  # (existing — no change needed)
│   ├── RegistrySettingsProvider.{h,cpp}     # + read/write new values
│   ├── InMemorySettingsProvider.{h,cpp}     # + new fields (test seam)
│   │
│   ├── IAdapterProvider.h                   # NEW — pure interface, AdapterInfo struct
│   ├── WindowsAdapterProvider.{h,cpp}       # NEW — DXGI enumeration impl
│   ├── InMemoryAdapterProvider.{h,cpp}      # NEW — test seam
│   ├── AdapterSelection.{h,cpp}             # NEW — pure helpers: ResolveAdapter,
│   │                                        #                    FormatAdapterLabel
│   │
│   ├── FrameLimiter.{h,cpp}                 # NEW — refresh-gated frame pacing
│   ├── DeviceLost.{h,cpp}                   # NEW — IsDeviceLost(HRESULT) classifier
│   ├── RebuildCoalescer.{h,cpp}             # NEW — atomic-flag coalescing helper
│   ├── MultiMonitorGate.{h,cpp}             # NEW — pure ShouldSpanAllMonitors core
│   ├── QualityPresets.{h,cpp}               # NEW — preset↔knob map, snap, custom drift
│   │
│   ├── RenderSystem.{h,cpp}                 # MOD — accept adapter; capture Present
│   │                                        #       HRESULT; expose IsDeviceLost flag;
│   │                                        #       parametric bloom (passes/res/taps);
│   │                                        #       bypass bloom when intensity == 0
│   ├── MonitorRenderContext.{h,cpp}         # MOD — accept adapter + frame limiter;
│   │                                        #       gate Present on device-lost path
│   ├── Application.{h,cpp}                  # MOD — multimon gate; WM_DISPLAYCHANGE
│   │                                        #       handler; coalesced rebuild;
│   │                                        #       resolve adapter once at startup
│   ├── ConfigDialogController.{h,cpp}       # MOD — new UpdateMultiMonitorEnabled,
│   │                                        #       UpdateGpuAdapter, UpdateQuality*,
│   │                                        #       UpdateShowAdvancedGraphics
│   └── (existing files unchanged)
│
├── MatrixRain/                              # .exe — Win32 entry + dialog UI only
│   ├── MatrixRain.rc                        # MOD — new controls in dialog template
│   ├── resource.h                           # MOD — new IDC_* IDs
│   └── ConfigDialog.cpp                     # MOD — populate combos, wire sliders,
│                                            #       advanced disclosure resize,
│                                            #       infotip control + tooltip wiring
│
├── MatrixRainTests/                         # CppUnitTest DLL
│   └── unit/
│       ├── AdapterSelectionTests.cpp        # NEW — ResolveAdapter, FormatAdapterLabel
│       ├── FrameLimiterTests.cpp            # NEW — sleep math, >60Hz gate
│       ├── DeviceLostTests.cpp              # NEW — HRESULT classification table
│       ├── MultiMonitorGateTests.cpp        # NEW — ShouldSpanAllMonitors truth table
│       ├── QualityPresetsTests.cpp          # NEW — preset map, snap, drift, defaults
│       ├── RebuildCoalescerTests.cpp        # NEW — flag set/clear/coalesce
│       ├── RegistrySettingsProviderTests.cpp # MOD — round-trip new values
│       ├── ConfigDialogControllerTests.cpp  # MOD — new Update*/persistence flow
│       └── (existing tests unchanged)
│
└── specs/006-multimon-gpu-efficiency/       # This feature's artifacts
```

**Structure Decision**: Existing three-project layout. No new projects. New core/test files only; modifications to existing files limited to those listed above. This honors Constitution principles IV (modular), VI (library-first), and VII (PCH discipline).

## Complexity Tracking

> Constitution Check passes with no violations. No complexity justification required.
