# Implementation Plan: HDR Output and Linear-Light Rendering

**Branch**: `008-hdr-linear-rendering` | **Date**: 2026-09-21 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/008-hdr-linear-rendering/spec.md`

## Summary

Move MatrixRain's render pipeline from gamma-encoded 8-bit math to linear
light with float intermediates (Phase 1), present natively in HDR (scRGB) on
monitors where Windows HDR is on while matching the user's SDR brightness
(Phase 2), then let streak heads and their glow rise above SDR white with a
hue-preserving roll-off, controlled by two new settings (Phase 3).

The approach keeps v1.6's look by converting each glyph instance's
*final* display color to linear on the CPU (research R1). It recalibrates the
bloom for linear light against a deterministic reference frame (R3). It folds
all output encoding and tone mapping into the existing last full-screen pass
via one shared transform (R4). It detects HDR per monitor through DXGI output
descriptions behind a mockable provider (R5, R6). Each phase ships
independently (R13).

## Technical Context

**Language/Version**: C++23 (`/std:c++latest`), HLSL Shader Model 5.0 (runtime-compiled from source strings, as today)

**Primary Dependencies**: Direct3D 11.1, DXGI 1.6 (`IDXGIOutput6`, `IDXGISwapChain3`), Direct2D 1.1 / DirectWrite (statistics overlay), Win32 DisplayConfig API (SDR white level). All are Windows SDK; no new third-party code.

**Storage**: Registry, the existing screensaver settings key (two new DWORD values, Phase 3)

**Testing**: Microsoft C++ Native Unit Test Framework (`MatrixRainTests`) for pure logic; a WARP-device calibration and benchmark harness (developer tool) for Phase 1 look-matching and per-phase performance; manual validation on HDR hardware per [quickstart.md](quickstart.md)

**Target Platform**: Windows 11, feature level 11.0+ GPUs (x64 and ARM64 builds, as today)

**Project Type**: Desktop application / screensaver (core static library + thin exe)

**Performance Goals**: Hold the display refresh rate at each quality preset; no monitor more than 5% below v1.6's frame rate at the same preset on the same hardware (SC-006). HDR detection and SDR-white polling < 1 ms per monitor per second.

**Constraints**: One `RenderSystem` and render thread per monitor; mixed HDR/SDR monitors; the screensaver preview window stays SDR; device-lost recovery must restore the output mode; no new full-screen passes (the transform folds into the existing last pass).

**Scale/Scope**: ~7 new core modules (random source, frame metrics, color math, output mode selection and tracker, display luminance provider and fake), changes to `RenderSystem` (formats, shaders, swap-chain reconfiguration, D2D target), a small `MonitorRenderContext` polling hook, and, in Phase 3, the settings chain plus two dialog controls.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-checked after Phase 1 design.*

| Principle | Status | How this plan complies |
|---|---|---|
| I. TDD (non-negotiable) | ✅ | Every pure function in [contracts/color-math.md](contracts/color-math.md) and the mode-selection table is written test-first. The display provider is a seam (`IDisplayLuminanceProvider` + `InMemory…` fake), so mode transitions are testable without hardware. GPU output is verified via the calibration harness and quickstart, as for all rendering today. |
| II. Performance-first | ✅ | Format choices are justified by bandwidth (R2: `R11G11B10` for the bloom chain). No new full-screen pass (R4). Detection is rate-limited (R5, R6). **Benchmark tests**: the calibration harness's benchmark mode times frames per quality preset (WARP for repeatability, hardware for real numbers). It is recorded before any change and re-run after each phase against a 5% threshold (SC-006). |
| III. C++23 / Windows native | ✅ | Windows SDK APIs only; `<dxgi1_6.h>` is already in `pch.h`. x64 and ARM64 per constitution 1.2.0. |
| IV. Modular architecture | ✅ | Color math, mode selection and luminance querying are separate modules with one-way dependencies into `RenderSystem`. Every public declaration in the new headers gets a doc comment (tasks.md conventions; also Development Standards, Documentation). |
| V. Type safety | ✅ | `enum class OutputMode`, `enum class HdrMode`; `static_assert` on the new constant buffer layout; `noexcept` pure functions. |
| VI. Library-first | ✅ | All code in `MatrixRainCore`; the dialog changes live in `MatrixRain/ConfigDialog.cpp` alongside the existing controls, as today. The calibration tool's logic (`FrameMetrics`, `RandomSource`) is in the core library and unit-tested; its `main` only wires them up. |
| VII. PCH | ✅ | No new system headers needed. |
| VIII. Formatting | ✅ | Follows `.github/copilot-instructions.md` (5/3 blank lines, column alignment, 80-column headers). |
| IX. Commit discipline | ✅ | One task, one commit, each building and passing tests. Test-first tasks write the failing test *and* the implementation, and commit once green (Red→Green inside the task), so no commit leaves a failing or non-compiling test. |

**Post-design re-check**: ✅ No violations. The design adds a seam and pure
modules and changes no architecture boundaries. Complexity Tracking is empty.

## Project Structure

### Documentation (this feature)

```text
specs/008-hdr-linear-rendering/
├── plan.md              # This file
├── research.md          # Phase 0: decisions R1–R13
├── data-model.md        # Phase 1: entities, formats, state transitions
├── quickstart.md        # Phase 1: validation guide per phase
├── contracts/
│   ├── color-math.md                  # pure functions + mode-selection table
│   ├── output-transform.md            # final-pass cbuffer and HLSL function
│   ├── display-luminance-provider.md  # OS-query seam
│   └── settings-ui.md                 # registry values + dialog controls (Phase 3)
├── checklists/requirements.md
└── tasks.md             # /speckit-tasks output (not created here)
```

### Source Code (repository root)

```text
MatrixRainCore/
├── RandomSource.h / .cpp                    # NEW  one seedable engine per thread (calibration determinism)
├── FrameMetrics.h / .cpp                    # NEW  mean luminance, frame comparison (calibration)
├── ColorMath.h / .cpp                       # NEW  sRGB transfer, instance color, luminance, tone map, highlight gain
├── OutputModeSelection.h / .cpp             # NEW  SelectOutputMode, OutputMode, HdrMode
├── OutputModeTracker.h / .cpp               # NEW  per-monitor mode state machine
├── IDisplayLuminanceProvider.h              # NEW  seam
├── WindowsDisplayLuminanceProvider.h / .cpp # NEW  DXGI + DisplayConfig
├── InMemoryDisplayLuminanceProvider.h       # NEW  test fake
├── RenderSystem.h / .cpp                    # CHANGED  formats, shaders, output cbuffer (b1), swap-chain reconfigure, D2D target format
├── MonitorRenderContext.h / .cpp            # CHANGED  ScreenSaverMode input, 1 Hz luminance poll, mode re-selection hook
├── AnimationSystem.h, CharacterStreak.h, CharacterSet.cpp  # CHANGED  use RandomSource
├── RenderParams.h                           # CHANGED  (P3) hdrMode, highlightBrightness
├── ScreenSaverSettings.h                    # CHANGED  (P3) HdrMode, HighlightBrightness
├── RegistrySettingsProvider.cpp             # CHANGED  (P3) two DWORD values
├── SharedState.h, ApplicationState.cpp      # CHANGED  (P3) live plumbing
├── ConfigDialogController.h / .cpp          # CHANGED  (P3) Update* + snapshot/reset
└── Application.cpp                          # CHANGED  display-change notification to contexts

MatrixRain/
├── ConfigDialog.cpp, MatrixRain.rc, resource.h  # CHANGED  (P3) HDR group on Visuals tab

MatrixRainTests/unit/
├── RandomSourceTests.cpp                    # NEW
├── FrameMetricsTests.cpp                    # NEW
├── ColorMathTests.cpp                       # NEW
├── OutputModeSelectionTests.cpp             # NEW
├── DisplayLuminanceTests.cpp                # NEW  (derived values, provider fake transitions)
├── RenderSystemScanlineBypassTests.cpp      # CHANGED  cbuffer size asserts incl. OutputTransformCb
└── RegistrySettingsProviderTests.cpp,
    ConfigDialogControllerTests.cpp          # CHANGED  (P3) new settings round-trip, Cancel, Reset

tools/HdrCalibration/                        # NEW (developer tool, not shipped)  thin main: reference-frame metrics + benchmark mode (R3)
```

**Structure Decision**: Existing three-project layout (core `.lib`, thin
`.exe`, native test project), unchanged. New logic enters as small core
modules following the established `I…Provider` / `Windows…Provider` /
`InMemory…Provider` pattern. The calibration harness lives outside the
shipped solution configurations so it adds nothing to release builds.

## Phase Delivery

| Release | Scope | Visible change | Exit criteria |
|---|---|---|---|
| Phase 1 | R1–R4 (SDR only), R10 | Smoother glow and fades; look otherwise matched | Quickstart Phase 1 table; SC-001, SC-002, SC-006 |
| Phase 2 | R4 (HDR mode), R5–R7, R9, R11 | None by design | Quickstart Phase 2 table; SC-003, SC-004, SC-007, SC-008 |
| Phase 3 | R8, R12 | Highlights above SDR white; two new settings | Quickstart Phase 3 table; SC-005 |

## Risks

| Risk | Mitigation |
|---|---|
| Phase 1 recalibration can't match v1.6 closely enough (FR-005) | Calibration harness gives numbers, not opinions; the instance-level conversion (R1) already makes the glyph cores identical, so only the bloom constants need tuning. |
| Antialiased glyph edges look thinner when blended in linear light | Compare on hardware in Phase 1; if needed, apply a coverage gamma to atlas alpha in the glyph shader (one constant). |
| Toggling HDR raises `WM_DISPLAYCHANGE` and rebuilds every monitor | Accepted by the relaxed FR-011; per-monitor in-place reconfiguration still covers changes the OS reports only through DXGI. |
| DisplayConfig SDR white level unavailable on some drivers | Falls back to 80 nits and logs once; HDR output remains correct, only dimmer than the user's setting. |
| FP16 bandwidth costs frames on integrated GPUs | `R11G11B10` bloom chain; re-tune the Low preset if SC-006 fails. |

## Complexity Tracking

No constitution violations; nothing to justify.
