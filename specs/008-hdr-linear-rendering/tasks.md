---

description: "Task list for 008 HDR output and linear-light rendering"
---

# Tasks: HDR Output and Linear-Light Rendering

**Input**: Design documents from `specs/008-hdr-linear-rendering/`

**Prerequisites**: [plan.md](plan.md), [spec.md](spec.md), [research.md](research.md), [data-model.md](data-model.md), [contracts/](contracts/), [quickstart.md](quickstart.md)

**Tests**: Required. Constitution I makes TDD non-negotiable for `MatrixRainCore`. Tasks marked **Test-first** run Red→Green inside the task: write the tests, build, **confirm they fail**, implement, confirm they pass, then commit once. GPU-visible behavior is verified with the calibration and benchmark harness and [quickstart.md](quickstart.md).

**Organization**: One phase per user story. US1, US2 and US3 are also the three shippable releases (plan, "Phase Delivery").

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependency on an incomplete task)
- **[Story]**: US1 = linear light (P1), US2 = native HDR output (P2), US3 = highlight headroom (P3)

## Conventions for every task

- Follow `.github/copilot-instructions.md` in full: 5 blank lines between top-level constructs, 3 after a function's leading declarations, column-aligned declarations and assignments, 80-column function-header separators, space before `(` in calls.
- **Doc-comment every public declaration** in new or changed headers (`///` summary, parameters, return), per Constitution IV and Development Standards.
- New `.h`/`.cpp` files in `MatrixRainCore/` MUST be added to **both** `MatrixRainCore/MatrixRainCore.vcxproj` (`ClInclude`/`ClCompile`) and `MatrixRainCore/MatrixRainCore.vcxproj.filters`. New test files MUST be added to `MatrixRainTests/MatrixRainTests.vcxproj`. Pattern: `ScanlineStyleMapping.*`.
- **One task = one commit** (Constitution IX), and every commit builds and passes all tests. Never commit a failing or non-compiling test on its own. Build Debug and Release with `scripts/Invoke-MatrixRainBuild.ps1` and run `scripts/Invoke-MatrixRainTests.ps1` before committing. Conventional Commits, no AI attribution trailers.
- HLSL lives in raw string literals in `MatrixRainCore/RenderSystem.cpp`. The scanline shader exists twice (the string near line 1645 and `MatrixRainCore/Shaders/scanlines.hlsl`); keep both identical.
- Hardware measurements and validation results go in `specs/008-hdr-linear-rendering/baseline.md`.

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Deterministic, measurable baselines before any pipeline change.

- [ ] T001 Record the v1.6 manual performance baseline: on each available monitor, run `x64\Release\MatrixRain.exe` from the `v1.6.0` tag with statistics on (`S`) at each quality preset, and write FPS and GPU% per preset per monitor (with resolution, DPI and GPU model) to `specs/008-hdr-linear-rendering/baseline.md` (SC-006, FR-008)
- [x] T002 **Test-first** — seedable randomness (research R3). Tests in new `MatrixRainTests/unit/RandomSourceTests.cpp`: after `RandomSource::Reseed (42)`, draw sequences on the same thread are identical across reseeds, and different seeds give different sequences; two `AnimationSystem` instances reseeded identically and stepped identically produce identical streak positions and glyph indices. Implementation: new `MatrixRainCore/RandomSource.h` / `.cpp` exposing `std::mt19937 & RandomSource::Engine()` (thread-local, seeded from `std::random_device` unless `Reseed` was called on that thread) and `void RandomSource::Reseed (uint32_t seed)`. Replace `AnimationSystem::m_generator` / `m_randomDevice` (`MatrixRainCore/AnimationSystem.h`), `CharacterStreak::s_generator` / `s_randomDevice` (`MatrixRainCore/CharacterStreak.h`) and `CharacterSet`'s `s_rd` / `s_gen` (`MatrixRainCore/CharacterSet.cpp`) with `RandomSource::Engine()`. Runtime behavior stays random when no seed is set
- [x] T003 [P] **Test-first** — frame metrics (research R3). Tests in new `MatrixRainTests/unit/FrameMetricsTests.cpp` on synthetic images: `MeanLuminance` of a uniform image equals its Rec.709 luma; `HaloFalloffRadius` of a synthetic radial Gaussian returns its analytic half-maximum radius within 0.5 px; both accept 8-bit sRGB (decoded through `SrgbToLinear`-equivalent math) and linear float input. Implementation: new `MatrixRainCore/FrameMetrics.h` / `.cpp` with `float MeanLuminance (std::span<const uint8_t> bgra, UINT width, UINT height)` and `float HaloFalloffRadius (std::span<const uint8_t> bgra, UINT width, UINT height, POINT center)`. Include a local sRGB decode until T006 lands, then switch to `ColorMath`
- [ ] T004 Create the calibration and benchmark harness `tools/HdrCalibration/HdrCalibration.vcxproj` + `main.cpp`, linking `MatrixRainCore.lib`, added to `MatrixRain.sln` but built in no shipped configuration. Keep `main` thin: parse `--adapter warp|hardware`, `--mode reference|benchmark`. **Reference mode**: hidden 1920×1080 window, `RandomSource::Reseed (1)`, fixed elapsed time, render one frame per named settings case (defaults; glow intensity min and max; glow size min and max; scanlines on at Style 1 and 100; custom color `RGB(0, 128, 255)`), read back the back buffer and print `MeanLuminance` and `HaloFalloffRadius` of the isolated head for each case (FR-006). **Benchmark mode**: render 600 frames per quality preset and print mean and p95 GPU ms/frame using D3D11 timestamp queries (Constitution II)
- [ ] T005 Capture the reference: on this branch after T002–T004 and **before any pipeline change** (rendering is still identical to v1.6, since only the RNG source changed), run the harness in reference mode on WARP and in benchmark mode on WARP and the hardware adapter. Record all numbers in `baseline.md` with the commit hash

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: The color math and output-transform plumbing every story uses.

**⚠️ CRITICAL**: No user-story work can begin until this phase is complete.

- [ ] T006 [P] **Test-first** — sRGB transfer. Tests in new `MatrixRainTests/unit/ColorMathTests.cpp` per [contracts/color-math.md](contracts/color-math.md): endpoints 0→0 and 1→1; `SrgbToLinear(0.5) ≈ 0.2140`; round trip within 1e-5 for x ∈ [0,1] in 0.001 steps; continuity at the 0.04045 / 0.0031308 knee; `LinearToSrgb` clamps inputs outside [0,1]. Implementation: `SrgbToLinear`, `LinearToSrgb` (IEC 61966-2-1 piecewise) and a `ColorMathConstants` block of `constexpr` values in new `MatrixRainCore/ColorMath.h` / `.cpp`. Then switch `FrameMetrics` (T003) to use them
- [ ] T007 **Test-first** — instance color. Tests in `MatrixRainTests/unit/ColorMathTests.cpp` for `InstanceLinearColor (Color4 srgb, float brightness, float highlightGain)`: returns `SrgbToLinear(srgb * brightness * (1 + 0.3 * brightness)) * highlightGain` per RGB channel; alpha passed through; **regression**: with `highlightGain == 1`, `LinearToSrgb(result)` equals `min(1, srgb * brightness * (1 + 0.3 * brightness))`, the v1.6 glyph shader's output for full coverage, within 1e-4 across brightness ∈ [0,1] and every `ColorScheme` color (FR-005). Implementation in `MatrixRainCore/ColorMath.h` / `.cpp`
- [ ] T008 [P] **Test-first** — output constant buffer layout. Test in `MatrixRainTests/unit/RenderSystemScanlineBypassTests.cpp`: `sizeof (OutputTransformCb) == 16` and field order `outputMode, sdrWhiteScale, headroom, isFinalPass` ([contracts/output-transform.md](contracts/output-transform.md)). Implementation: `struct alignas (16) OutputTransformCb { uint32_t outputMode; float sdrWhiteScale; float headroom; uint32_t isFinalPass; }` with `static_assert (sizeof (OutputTransformCb) == 16)` in `MatrixRainCore/RenderSystem.h` next to `ScanlineCb`
- [ ] T009 Add the output-transform plumbing to `MatrixRainCore/RenderSystem.cpp`: a raw string `s_kszOutputTransformHlsl` spliced into every candidate final-pass shader (composite, scanline) containing `cbuffer OutputCb : register(b1)`, `LinearToSrgb` (transliterated from T006 with identical constants), and `OutputTransform (float3)`, which returns the input unchanged when `g_isFinalPass == 0`, `LinearToSrgb (saturate (x))` for `g_outputMode == 0`, and the input unchanged for the HDR branch until US2. Create `m_outputConstantBuffer` (16 bytes, dynamic) alongside the existing constant buffers and add `HRESULT RenderSystem::UploadOutputTransformConstants (const OutputTransformCb &)` (Map/Unmap like `UploadScanlineConstants`). Nothing binds it yet, so behavior is unchanged

**Checkpoint**: Color math tested; final-pass shaders can include the transform. Rendering is unchanged.

---

## Phase 3: User Story 1 — Physically Correct Light Blending on Every Display (Priority: P1) 🎯 MVP

**Goal**: All light combination in linear light with float intermediates; SDR output encoded once; v1.6 look preserved at defaults and for existing saved settings.

**Independent Test**: On an SDR monitor, v1.6 and this build look the same at defaults; fades on black don't band; overlapping halos are brighter than either alone ([quickstart.md](quickstart.md) Phase 1).

**Note**: T011–T017 change the color contract together: inputs become linear and the final passes start encoding. Each commit builds and passes tests, but between T011 and T017 the image renders too dark. Do them consecutively, and never release from the middle of that range.

- [ ] T010 [US1] Change the render-target formats in `RenderSystem::CreateBloomResources` (`MatrixRainCore/RenderSystem.cpp`): scene texture and post-bloom target → `DXGI_FORMAT_R16G16B16A16_FLOAT`; bloom texture and blur temp → `DXGI_FORMAT_R11G11B10_FLOAT` (research R2, data-model §6). Glyph and overlay atlases stay `B8G8R8A8_UNORM`. Output is unchanged by this task alone
- [ ] T011 [US1] Linearize glyph colors on the CPU. v1.6's glyph shader computes `rgb = color.rgb * tex.rgb * b * (1 + 0.3 * b)` and `a = color.a * tex.a * b` (b = brightness). In `RenderSystem::BuildCharacterInstanceData`, choose the sRGB color as today (white head vs. scheme or custom color), then store `InstanceLinearColor (srgb, character.brightness, 1.0f)` in `data.color.rgb`, keeping `data.color.a` and `data.brightness` unchanged. Rewrite the glyph pixel shader (`s_kszPixelShaderSource`) to `rgb = input.color.rgb * tex.rgb` and `a = input.color.a * tex.a * input.brightness`. The brightness and `+30%` terms move out of the RGB path into `InstanceLinearColor`, while alpha fading stays identical to v1.6 (research R1)
- [ ] T012 [US1] Linearize overlay instance colors: in `RenderSystem::BuildOverlayInstances`, convert each overlay instance's sRGB tint with `SrgbToLinear` per channel before upload. The overlay pixel shader and premultiplied blend stay unchanged
- [ ] T013 [US1] Keep the no-scene fallback correct: in `RenderSystem::Render`, the branch that draws glyphs directly into the back buffer when `m_sceneRTV` is null must upload **sRGB** (non-linearized) instance colors, since no output transform runs there. Add a `bool linearizeColors` parameter to `UpdateInstanceBuffer` and pass `false` on that path
- [ ] T014 [US1] Rework the bloom extract shader (`s_kszBloomExtractShaderSource`) for linear input: keep `max (luminance, maxChannel)` and the smoothstep shape, with the threshold changed from `0.1` to `≈ 0.010` (`SrgbToLinear(0.1)`) and the ramp end from `0.6` to `≈ 0.318`, as named `constexpr` starting values that T018 tunes
- [ ] T015 [US1] Rework the bloom composite shader (`s_kszBloomCompositeShaderSource`) to additive linear bloom, `linear = scene.rgb + (1 - exp (-bloom.rgb * bloomIntensity)) * kBloomCeiling`, dropping the `(1 - scene)` screen factor (research R3), and return `float4 (OutputTransform (linear), 1)`. In `ApplyBloom`, bind `m_outputConstantBuffer` at `b1` and, before the composite draw, upload `{ 0, 1.0f, 1.0f, isFinalPass }` with `isFinalPass = scanlines ? 0 : 1` (U2 fix: the flag lives in `OutputTransformCb`, not the bloom buffer)
- [ ] T016 [US1] Apply the same rule to the glow-off scene copy in `RenderSystem::Render` (the branch calling `RenderFullscreenPass (pCompositeTarget, m_compositePS, …)` with a null bloom SRV): bind `b1` and upload `isFinalPass = scanlines ? 0 : 1` before the draw. With glow off, `bloomIntensity` must evaluate to no bloom contribution: bind the bloom buffer with intensity 0, or have the shader skip bloom when slot 1 is null
- [ ] T017 [US1] Move scanline darkening to linear light and make the scanline pass final: in the scanline HLSL (string near `RenderSystem.cpp:1645` **and** `MatrixRainCore/Shaders/scanlines.hlsl`) compute `c.rgb *= darken` on the linear post-bloom sample, then return `float4 (OutputTransform (c.rgb), c.a)`. In `ApplyScanlinePass`, bind `b1` and upload `isFinalPass = 1`. Update the shader header comment with the pass order (FR-001, FR-004, FR-007)
- [ ] T018 [US1] Calibrate with the harness (T004): run reference mode on the new pipeline for **every** settings case and compare with T005. Tune the extract thresholds (T014), `kBloomCeiling` (T015) and, if needed, the glow-intensity mapping until mean luminance and halo radius are within 5% of the reference for the defaults **and** each non-default case (FR-005, FR-006, SC-001). Record the final constants and per-case metrics in `baseline.md`, with a comment in `RenderSystem.cpp` citing the calibration
- [ ] T019 [US1] Hardware check of glyph edge weight (plan Risk 2): compare v1.6 and this build at 100% zoom on an SDR monitor. If strokes read visibly thinner, add a coverage gamma `tex.a = pow (tex.a, kCoverageGamma)` in the glyph pixel shader, with `kCoverageGamma` tuned until stroke weight matches, and re-run T018's metrics. Otherwise record "not needed" in `baseline.md`
- [ ] T020 [US1] Performance gate (Constitution II, SC-006): re-run the harness benchmark mode (WARP and hardware) and the T001 manual FPS readings. No preset may be more than 5% slower than T005/T001. If one is, retune that preset in `MatrixRainCore/QualityPresets.cpp` and record why
- [ ] T021 [US1] Run [quickstart.md](quickstart.md) Phase 1 checks 1–7 and record the results

**Checkpoint**: Phase 1 release candidate. Ship after T045.

---

## Phase 4: User Story 2 — Native HDR Output That Matches the User's SDR Brightness (Priority: P2)

**Goal**: Per-monitor native scRGB output on HDR-enabled monitors, at the user's SDR brightness, with correct overlays, mixed-mode support, live HDR toggling and silent fallback.

**Independent Test**: With Windows HDR on, brightness and color match v1.6; overlays are correct; toggling HDR while running works 10/10 times ([quickstart.md](quickstart.md) Phase 2).

- [ ] T022 [P] [US2] **Test-first** — mode selection. Tests in new `MatrixRainTests/unit/OutputModeSelectionTests.cpp` covering every row of the `SelectOutputMode` table in [contracts/color-math.md](contracts/color-math.md): not HDR-enabled → `Sdr`; HDR-enabled but scRGB unsupported → `Sdr`; HDR + supported + `ScreenSaverMode::ScreenSaverPreview` → `Sdr` (FR-017); HDR + supported + any other mode → `Hdr`. Implementation: `enum class OutputMode { Sdr, Hdr }`, `enum class HdrMode { Auto, Off }` and `SelectOutputMode (bool hdrEnabled, bool scRgbSupported, ScreenSaverMode)` in new `MatrixRainCore/OutputModeSelection.h` / `.cpp`
- [ ] T023 [P] [US2] **Test-first** — display luminance. Tests in new `MatrixRainTests/unit/DisplayLuminanceTests.cpp` for `EffectivePeakNits` ("`reportedPeakNits` if in [80, 10 000], else 400"), `Headroom` ("`max(1, effectivePeakNits / sdrWhiteNits)`, so ≥ 1 always"), `SdrWhiteScale` (`nits / 80`) and `SdrWhiteNitsFromDisplayConfig` (`level / 1000 * 80`, "Clamp to [80, 480]"), including 0, negative and white-above-peak inputs (FR-026). Implementation: `struct DisplayLuminance { bool hdrEnabled; bool scRgbSupported; float sdrWhiteNits; float reportedPeakNits; std::wstring deviceName; }` (data-model §2) and the four functions in `MatrixRainCore/ColorMath.h` / `.cpp`
- [ ] T024 [US2] Add `class IDisplayLuminanceProvider` (`DisplayLuminance Query (IDXGISwapChain1 *) noexcept`, `bool IsStale() noexcept`) in new `MatrixRainCore/IDisplayLuminanceProvider.h`, and a scripted `InMemoryDisplayLuminanceProvider` (queue of `DisplayLuminance` results, settable stale flag) in new `MatrixRainCore/InMemoryDisplayLuminanceProvider.h`, following `IAdapterProvider` / `InMemoryAdapterProvider` ([contracts/display-luminance-provider.md](contracts/display-luminance-provider.md)). Interface only, so no test of its own; it is exercised by T025
- [ ] T025 [US2] **Test-first** — mode tracker. Tests in `MatrixRainTests/unit/DisplayLuminanceTests.cpp` for `OutputModeTracker` (data-model §7), driven by `InMemoryDisplayLuminanceProvider`: SDR→HDR→SDR sequences return `Reconfigure (Hdr)` then `Reconfigure (Sdr)`; unchanged input returns `None`; a white-level change returns `None` but updates the `sdrWhiteScale` the tracker exposes; reporting a reconfigure failure leaves the tracker in `Sdr`; the preview display mode never leaves `Sdr`. Implementation: new `MatrixRainCore/OutputModeTracker.h` / `.cpp`
- [ ] T026 [US2] Implement `WindowsDisplayLuminanceProvider` in new `MatrixRainCore/WindowsDisplayLuminanceProvider.h` / `.cpp`:
  - `IDXGISwapChain::GetContainingOutput` → `IDXGIOutput6::GetDesc1`: `hdrEnabled` when `ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020`, plus `MaxLuminance` and `DeviceName`.
  - `IDXGISwapChain3::CheckColorSpaceSupport (DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709)` → `scRgbSupported`.
  - `QueryDisplayConfig` + `DisplayConfigGetDeviceInfo (DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME)` to match `DeviceName`, then `DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL`.
  - `IsStale` via a cached `IDXGIFactory1::IsCurrent`.
  - Every failure returns `hdrEnabled = false`, `scRgbSupported = false`, `sdrWhiteNits = 80` and never throws (FR-015). EHM `-A` macros on OS calls.
- [ ] T027 [US2] Plumb the display mode into each monitor context (U3): add a `ScreenSaverMode` parameter to `MonitorRenderContext::Initialize` (`MatrixRainCore/MonitorRenderContext.h` / `.cpp`), stored as `m_displayMode`, and pass `Application`'s current mode at every call site in `MatrixRainCore/Application.cpp`. Update any test that constructs or initializes a `MonitorRenderContext`
- [ ] T028 [US2] Make the swap chain mode-aware in `RenderSystem::CreateSwapChain`: take an `OutputMode`; `Hdr` → `DXGI_FORMAT_R16G16B16A16_FLOAT` + `IDXGISwapChain3::SetColorSpace1 (DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709)`; `Sdr` → `B8G8R8A8_UNORM` + `SetColorSpace1 (DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709)`. Store `m_outputMode`; on any HDR failure, recreate as `Sdr`
- [ ] T029 [US2] Add `RenderSystem::ReconfigureOutputMode (OutputMode)`: `ReleaseRenderTargetResources`, `ResizeBuffers (0, w, h, newFormat, 0)`, `SetColorSpace1`, `CreateRenderTargetView`, `RecreateDirect2DBitmap`; fall back to `Sdr` on failure and report the result (data-model §7)
- [ ] T030 [US2] Make the D2D target follow the back buffer: in `RenderSystem::RecreateDirect2DBitmap`, use `DXGI_FORMAT_R16G16B16A16_FLOAT` (premultiplied) when `m_outputMode == Hdr`. In `RenderFPSCounter`, convert brush colors with `SrgbToLinear` and scale by `sdrWhiteScale` in HDR so text sits at SDR white (research R9, FR-014)
- [ ] T031 [US2] Complete the HDR branch of `OutputTransform` (T009): for `g_outputMode == 1`, `return min (max (x, 0), g_headroom) * g_sdrWhiteScale` (Phase 2: headroom is always 1, capping at SDR white; FR-013). Upload `outputMode = 1` and the current `sdrWhiteScale` when `m_outputMode == Hdr`, keeping the per-pass `isFinalPass` rule from T015–T017
- [ ] T032 [US2] Wire detection into `MonitorRenderContext`: own a `std::unique_ptr<IDisplayLuminanceProvider>` (default `WindowsDisplayLuminanceProvider`, injectable) and an `OutputModeTracker`. At initialize, select the mode with `m_displayMode` before creating the swap chain. On the render thread, at 1 Hz and whenever `IsStale()`, re-query, apply the tracker's action via `ReconfigureOutputMode`, and push `sdrWhiteScale` to the render system. Measure one query and record it in `baseline.md` (target < 1 ms)
- [ ] T033 [US2] Restore the output mode after device loss: confirm that the `WM_APP_REBUILD_CONTEXTS` path in `MatrixRainCore/Application.cpp` re-initializes contexts through T032's initialize-time selection, and add a comment there saying so (FR-016)
- [ ] T034 [US2] Publish HDR presence for the dialog: add `std::atomic<int> hdrMonitorCount` to `MatrixRainCore/SharedState.h`, maintained by each `MonitorRenderContext` (increment on entering `Hdr`, decrement on leaving `Hdr` or shutting down)
- [ ] T035 [US2] Validate on hardware: run [quickstart.md](quickstart.md) Phase 2 checks 1–10. Re-run the harness benchmark on the hardware adapter and the manual FPS readings **with an HDR monitor in HDR mode** against the 5% threshold (SC-006). Record the results, whether toggling HDR raised `WM_DISPLAYCHANGE` (research R7), and the Auto HDR outcome (research R11)

**Checkpoint**: Phase 2 release candidate: native HDR, no visible change.

---

## Phase 5: User Story 3 — Glowing Highlights Beyond SDR White (Priority: P3)

**Goal**: Streak heads and their glow exceed SDR white up to each display's peak, with a hue-preserving roll-off, controlled by HDR mode (Auto/Off) and highlight brightness (0–100).

**Independent Test**: With HDR mode Auto, heads are brighter than SDR white and trails are unchanged; the slider works live and persists; Off matches US2 ([quickstart.md](quickstart.md) Phase 3).

- [ ] T036 [P] [US3] **Test-first** — highlight gain. Tests in `MatrixRainTests/unit/ColorMathTests.cpp` for `HighlightGain (headroom, highlightBrightness, HdrMode, OutputMode)`: 1 for `OutputMode::Sdr` or `HdrMode::Off`; otherwise `headroom ^ (clamp (b, 0, 100) / 100)`; monotonic in `b`; equals `headroom` at 100 and 1 at 0; a 600-nit peak with 240-nit white at the default 80 gives ≥ 2.0 (SC-005). Implementation in `MatrixRainCore/ColorMath.h` / `.cpp`
- [ ] T037 [P] [US3] **Test-first** — tone mapping. Tests in `MatrixRainTests/unit/ColorMathTests.cpp` for `ToneMapHighlights (float rgb[3], float headroom)`: identity when `max(rgb) <= 1`; output max < `headroom`, approaching it as input → ∞; channel ratios preserved within 1e-5 (FR-019); C¹ continuity at `max = 1` (finite-difference slope within 1e-3 each side); `headroom == 1` caps at 1. Implementation: an extended-Reinhard shoulder on the max channel with white point `headroom`, scaling RGB by `mapped / max` (research R8), in `MatrixRainCore/ColorMath.h` / `.cpp`
- [ ] T038 [US3] Transliterate `ToneMapHighlights` into `s_kszOutputTransformHlsl` (same constants) and use it in the HDR branch: `ToneMapHighlights (max (x, 0), g_headroom) * g_sdrWhiteScale`. Upload the monitor's real `Headroom` instead of 1
- [ ] T039 [US3] **Test-first** — settings and persistence. Tests in `MatrixRainTests/unit/RegistrySettingsProviderTests.cpp` for `HdrMode` ("DWORD 0 = Auto, 1 = Off"; "Unknown → 0") and `HighlightBrightness` ("0–100", default 80, "Clamped"): round-trip, missing-value defaults and clamping ([contracts/settings-ui.md](contracts/settings-ui.md)). Implementation: `HdrMode m_hdrMode { HdrMode::Auto }`, `int m_highlightBrightness { DEFAULT_HIGHLIGHT_BRIGHTNESS }` with `MIN_HIGHLIGHT_BRIGHTNESS = 0`, `MAX_HIGHLIGHT_BRIGHTNESS = 100`, `DEFAULT_HIGHLIGHT_BRIGHTNESS = 80` in `MatrixRainCore/ScreenSaverSettings.h` (clamped in the existing clamp routine), and `VALUE_HDR_MODE = L"HdrMode"` / `VALUE_HIGHLIGHT_BRIGHTNESS = L"HighlightBrightness"` read and write in `MatrixRainCore/RegistrySettingsProvider.cpp`, next to the scanline values
- [ ] T040 [US3] **Test-first** — live plumbing. Tests in `MatrixRainTests/unit/ConfigDialogControllerTests.cpp` for `UpdateHdrMode` / `UpdateHighlightBrightness`: live propagation to `ApplicationState` and `SharedState`, rollback on Cancel, restoration by `ResetToDefaults` (FR-023). Implementation: `liveHdrMode`, `liveHighlightBrightness`, `snapshotHdrMode`, `snapshotHighlightBrightness` and `Snapshot::hdrMode` / `highlightBrightness` in `MatrixRainCore/SharedState.h`; setters in `MatrixRainCore/ApplicationState.cpp`; `UpdateHdrMode` / `UpdateHighlightBrightness` plus snapshot, Cancel and Reset handling in `MatrixRainCore/ConfigDialogController.h` / `.cpp`, mirroring `scanlinesEnabled`
- [ ] T041 [US3] Apply the gain to heads only: add `hdrMode` and `highlightBrightness` to `MatrixRainCore/RenderParams.h` (column-aligned), fill them in `MonitorRenderContext::Render`, and in `RenderSystem::BuildCharacterInstanceData` pass `HighlightGain (headroom, highlightBrightness, hdrMode, m_outputMode)` to `InstanceLinearColor` **only** for white (head) instances. Trails and overlays keep gain 1 (FR-018, FR-020)
- [ ] T042 [US3] Dialog controls in `MatrixRain/MatrixRain.rc` and `MatrixRain/resource.h`: an "HDR" group on the Visuals page below the scanline controls, without growing the dialog, containing `IDC_HDR_MODE_COMBO` ("Auto", "Off"), `IDC_HIGHLIGHT_PROMPT`, `IDC_HIGHLIGHT_SLIDER` (0–100), `IDC_HIGHLIGHT_VALUE` (`NN%`) and `IDC_HDR_INFO` (ⓘ) ([contracts/settings-ui.md](contracts/settings-ui.md))
- [ ] T043 [US3] Dialog behavior in `MatrixRain/ConfigDialog.cpp`:
  - Initialize both controls from settings; route changes to `UpdateHdrMode` / `UpdateHighlightBrightness`; include them in the reset resync.
  - Info tooltip: "Affects monitors with Windows HDR turned on. Other monitors are unchanged."
  - Grey the slider while mode is Off.
  - Grey the whole group, with tooltip "No monitor has HDR turned on in Windows.", while `SharedState::hdrMonitorCount == 0`, re-evaluated on the existing 1 s dialog timer.
  - Resolve the Visuals page through the sheet, like `ApplyScanlinesEnabledUI` (FR-021–024).
- [ ] T044 [US3] Tune and validate on hardware: confirm the default highlight brightness of 80 is comfortable in a dim room (if not, adjust `DEFAULT_HIGHLIGHT_BRIGHTNESS` and T036's expectation together). Run [quickstart.md](quickstart.md) Phase 3 checks 1–9. Re-run the benchmark and manual FPS readings with HDR mode Auto on an HDR monitor against the 5% threshold (SC-006). Record the results

**Checkpoint**: Phase 3 release candidate: full feature.

---

## Phase 6: Polish & Cross-Cutting Concerns

- [ ] T045 [P] (Phase 1 release) Add a `CHANGELOG.md` entry: smoother glow and fades, linear-light blending, no settings changes
- [ ] T046 [P] (Phase 2 release) Add a `CHANGELOG.md` entry: native HDR output on HDR monitors at the Windows SDR brightness, with no visible change
- [ ] T047 [P] (Phase 3 release) Add a `CHANGELOG.md` entry, with an upgrade note that HDR owners will see brighter heads by default and how to set HDR mode to Off
- [ ] T048 [P] Add a short "HDR" section to `README.md`: what Auto and Off do, highlight brightness, and that the Windows SDR brightness slider sets trail brightness
- [ ] T049 Mark `specs/008-hdr-linear-rendering/spec.md` **Status** as Implemented, and point the Spec Kit context block in `.github/copilot-instructions.md` at the next feature

---

## Dependencies & Execution Order

### Phase dependencies

- **Setup (T001–T005)**: T001 has no dependencies. T002 and T003 before T004; T004 before T005.
- **Foundational (T006–T009)**: after Setup; blocks every story. T006 also finishes T003's switch to `ColorMath`.
- **US1 (T010–T021)**: after Foundational.
- **US2 (T022–T035)**: after US1; it needs the linear pipeline and the output transform in the final passes.
- **US3 (T036–T044)**: after US2; it needs HDR output, headroom and `m_outputMode`.
- **Polish**: T045 after US1, T046 after US2, T047–T049 after US3.

The stories are **sequential by design**: each release builds on the previous
one (plan, "Phase Delivery").

### Key task dependencies

- T002, T003 → T004 → T005 (reference captured before T010)
- T007 → T011 (instance colors need `InstanceLinearColor`)
- T008, T009 → T015, T016, T017 (final passes use `OutputTransformCb` and the HLSL helpers)
- T010–T017 → T018 → T019, T020, T021
- T022, T023, T024 → T025; T026, T027, T028, T029 → T032; T031 → T035
- T036, T037 → T038, T041; T039 → T040 → T041, T043; T034 → T043

## Parallel Opportunities

- **Setup**: T003 alongside T002.
- **Foundational**: T006 and T008 (different files).
- **US2**: T022 and T023 (different new files); T026 alongside T025 once T024 is done.
- **US3**: T036 and T037 (same test file, independent functions: draft together, commit separately).
- **Polish**: T045–T048 once their phase is done.

### Parallel example: User Story 2

```text
Task: "T022 OutputModeSelection.* — SelectOutputMode (test-first)"
Task: "T023 DisplayLuminance derived functions in ColorMath.* (test-first)"
```

## Implementation Strategy

### MVP first (User Story 1)

1. Setup T001–T005: seed seam, metrics, harness and baselines before touching the pipeline.
2. Foundational T006–T009.
3. US1 T010–T021, calibrating against the harness.
4. **Stop and validate** (quickstart Phase 1), then T045 and release as a minor version.

### Incremental delivery

1. US1 → release (smoother glow, same look, all users).
2. US2 → release (native HDR, no visible change: isolates output-path regressions).
3. US3 → release (highlight headroom and settings).

Each release keeps the previous behavior intact: Phase 2 never produces a gain
above 1, and HDR mode Off reproduces Phase 2.
