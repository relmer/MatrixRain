---
description: "Task list for feature implementation"
---

# Tasks: Multi-Monitor User Control and GPU Efficiency

**Input**: Design documents from `/specs/006-multimon-gpu-efficiency/`
**Prerequisites**: spec.md, plan.md, research.md, data-model.md, contracts/, quickstart.md (all present)
**Tests**: REQUIRED for all core-library code per constitution principle I (TDD is non-negotiable). UI/dialog code in `MatrixRain.exe` is exempt per the same principle's scope exemption.

**Organization**: Tasks are grouped by user story to enable independent implementation, testing, and shipping of each story.

## Format: `[ID] [P?] [Story?] Description`

- **[P]**: Can run in parallel (different files, no incomplete dependencies)
- **[Story]**: User story this task belongs to (`US1`–`US5`)
- File paths are absolute within the repo root `C:\Users\relmer\source\repos\relmer\MatrixRain\`.

## Path Conventions

- Core static library: `MatrixRainCore\`
- Executable (Win32 entry + dialog UI): `MatrixRain\`
- Unit tests: `MatrixRainTests\unit\`
- Single Visual Studio solution: `MatrixRain.sln`

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: One-time bootstrap shared across all user stories.

- [ ] T001 [P] Add `#include <dxgi1_6.h>` to `MatrixRainCore\pch.h` in the DirectX/DXGI block, alphabetically after the existing `<dxgi1_2.h>`; rebuild the entire solution once to refresh PCH.
- [ ] T002 [P] Reserve new dialog control IDs in `MatrixRain\resource.h` (no .rc changes yet): `IDC_MULTIMONITOR_CHECK`, `IDC_MULTIMONITOR_INFO`, `IDC_GPU_COMBO`, `IDC_GPU_INFO`, `IDC_QUALITY_PRESET_COMBO`, `IDC_QUALITY_PRESET_INFO`, `IDC_GRAPHICS_ADVANCED_CHECK`, `IDC_GRAPHICS_ADVANCED_INFO`, `IDC_GLOWPASSES_SLIDER`, `IDC_GLOWPASSES_LABEL`, `IDC_GLOWPASSES_INFO`, `IDC_GLOWRES_SLIDER`, `IDC_GLOWRES_LABEL`, `IDC_GLOWRES_INFO`, `IDC_GLOWSMOOTH_SLIDER`, `IDC_GLOWSMOOTH_LABEL`, `IDC_GLOWSMOOTH_INFO`, `IDC_GLOWINTENSITY_INFO`, `IDC_GLOWSIZE_INFO`. Bump `_APS_NEXT_CONTROL_VALUE` accordingly.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: None. Each user story is independently implementable with its own settings field; no shared blocking infrastructure exists beyond Phase 1 setup. Existing `RegistrySettingsProvider` already supports DWORD and REG_SZ patterns (the latter via `ColorScheme`), so no foundational provider work is required.

> **Checkpoint**: Phase 1 complete → user story implementation can now begin in parallel across staffed developers.

---

## Phase 3: User Story 1 — Adapt immediately when monitors are added or removed (Priority: P1) 🎯 MVP

**Goal**: Fix the ghost-monitor render-thread defect by responding to monitor add/remove and GPU device-loss events at runtime, rebuilding the per-monitor render contexts within 1 second.

**Independent Test**: With MatrixRain running spanning two monitors, disconnect one monitor; verify within 1 second that GPU utilization drops to a level within 10% of the "started in undocked state" baseline. Reconnect and verify MatrixRain resumes rendering on it without restart. Manually reset the active GPU's driver via Device Manager and verify automatic recovery.

### Tests for User Story 1

- [ ] T003 [P] [US1] Create `MatrixRainTests\unit\DeviceLostTests.cpp` with truth-table tests for `IsDeviceLost(HRESULT)`: returns `true` for `DXGI_ERROR_DEVICE_REMOVED`, `DXGI_ERROR_DEVICE_RESET`, `DXGI_ERROR_DEVICE_HUNG`, `DXGI_ERROR_DRIVER_INTERNAL_ERROR`, `D3DDDIERR_DEVICEREMOVED`; returns `false` for `S_OK`, `E_FAIL`, `DXGI_STATUS_OCCLUDED`, `E_INVALIDARG`. Verify tests fail (helper not yet implemented). Add file to `MatrixRainTests.vcxproj`.
- [ ] T004 [P] [US1] Create `MatrixRainTests\unit\RebuildCoalescerTests.cpp` with tests for `RebuildCoalescer`: `RequestRebuild()` returns true on first call; subsequent calls return false until `Consume()`; after `Consume()`, next `RequestRebuild()` returns true again; concurrent calls from multiple threads coalesce to exactly one true return (use `std::thread` ×N and an atomic counter). Verify tests fail. Add to vcxproj.

### Implementation for User Story 1

- [ ] T005 [P] [US1] Create `MatrixRainCore\DeviceLost.h` and `MatrixRainCore\DeviceLost.cpp` exposing `bool IsDeviceLost(HRESULT hr)` per the contract in research R-003. Pure free function. Add both files to `MatrixRainCore.vcxproj`. T003 must now pass.
- [ ] T006 [P] [US1] Create `MatrixRainCore\RebuildCoalescer.h` and `MatrixRainCore\RebuildCoalescer.cpp` with a class wrapping `std::atomic_flag`: `bool RequestRebuild()` returns true iff this is the first request since the last `Consume()`; `void Consume()` clears the flag. Add files to vcxproj. T004 must now pass.
- [ ] T007 [US1] Modify `MatrixRainCore\RenderSystem.h` and `MatrixRainCore\RenderSystem.cpp`: change `void Present()` signature to `HRESULT Present()` (currently at `RenderSystem.cpp:1753-1758`); return the HRESULT from `m_swapChain->Present(1, 0)`. Update the single caller in `MonitorRenderContext.cpp:401` to capture the result into a local variable (still unused at this step).
- [ ] T008 [US1] Modify `MatrixRainCore\MonitorRenderContext.cpp::RenderThreadProc` (`:327-402`): after the Present call at `:401`, if `IsDeviceLost(hr)` is true, log via existing console/EHM, set an `m_deviceLost` flag, `PostMessage(m_hwnd, WM_APP_REBUILD_CONTEXTS, 0, 0)`, then `break` out of the render loop. Add `m_deviceLost` (`std::atomic<bool>`) to `MonitorRenderContext.h` (initialized false; observed by `Application` during rebuild). Include `DeviceLost.h`.
- [ ] T009 [US1] Modify `MatrixRainCore\Application.h` to add a private `RebuildCoalescer m_rebuildCoalescer;` member. Modify `MatrixRainCore\Application.cpp::HandleMessage` (`:1081…`) to add `case WM_DISPLAYCHANGE:` that calls `m_rebuildCoalescer.RequestRebuild()`; if it returns true, `PostMessage(m_mainHwnd, WM_APP_REBUILD_CONTEXTS, 0, 0)`; always return 0.
- [ ] T010 [US1] Modify `MatrixRainCore\Application.cpp::HandleMessage` `case WM_APP_REBUILD_CONTEXTS` (`:1105`): call `m_rebuildCoalescer.Consume()` at the top of the handler so the next topology change can request again. No other behavior change required here — `RebuildContextsForCurrentMode` (`:883-940`) already does the teardown/re-enumerate/restart cycle.

> **Checkpoint**: Build (`MSBuild MatrixRain.sln /p:Configuration=Debug /p:Platform=x64`, no `/m`). Run full test suite (`vstest.console MatrixRainTests.dll`). Manual QA: launch MatrixRain on a multi-monitor system, disconnect one monitor; observe within 1 second the secondary window closes and GPU utilization drops; reconnect and observe the secondary window reappears. With Device Manager, disable then re-enable the active GPU's driver; MatrixRain recovers without restart and without an error dialog. **Commit when green.**

---

## Phase 4: User Story 2 — Choose whether MatrixRain spans all monitors (Priority: P1)

**Goal**: Add a user-visible setting (default on) to enable/disable multi-monitor spanning; persist across restarts; take effect within 1 second when toggled.

**Independent Test**: Open the configuration dialog on a multi-monitor system, toggle off the "render on all monitors" checkbox, dismiss the dialog; within 1 second only the primary monitor continues rendering. Restart; setting persisted. Toggle back on; all monitors resume within 1 second.

### Tests for User Story 2

- [ ] T011 [P] [US2] Create `MatrixRainTests\unit\MultiMonitorGateTests.cpp` with the full truth table for `ShouldSpanAllMonitors(bool enabled, DisplayMode displayMode, ScreenSaverMode saverMode)` per the contract in research R-001 (Preview/help mode forces single regardless of setting; otherwise honors `enabled`). Verify failing.
- [ ] T012 [P] [US2] Add tests to `MatrixRainTests\unit\ScreenSaverSettingsTests.cpp` (or create if absent) for the new field `m_multiMonitorEnabled`: default `true` on fresh-construct; survives clamp/validation. Verify failing.
- [ ] T013 [P] [US2] Extend `MatrixRainTests\unit\RegistrySettingsProviderTests.cpp` with round-trip tests for the `MultiMonitor` DWORD value: absent → default 1 (true); 0 → false; 1 → true; any other → clamp to true. Verify failing.

### Implementation for User Story 2

- [ ] T014 [P] [US2] Create `MatrixRainCore\MultiMonitorGate.h` and `MatrixRainCore\MultiMonitorGate.cpp` with pure free function `bool ShouldSpanAllMonitors(bool enabled, DisplayMode displayMode, ScreenSaverMode saverMode)`. Add to vcxproj. T011 must now pass.
- [ ] T015 [US2] Modify `MatrixRainCore\ScreenSaverSettings.h` to add `bool m_multiMonitorEnabled = true;`. Update the load/validation block as needed (no clamping required for bool, but ensure registry-default branch sets it to true). T012 must now pass.
- [ ] T016 [US2] Modify `MatrixRainCore\RegistrySettingsProvider.h` to add `static constexpr LPCWSTR VALUE_MULTIMONITOR = L"MultiMonitor";` and the corresponding read/write call in the Load/Save implementations in `.cpp` (use existing `ReadBool`/`WriteBool` helpers). T013 must now pass.
- [ ] T017 [US2] Modify `MatrixRainCore\InMemorySettingsProvider.{h,cpp}` to add the same `m_multiMonitorEnabled` field with default `true` (test-seam parity).
- [ ] T018 [US2] Modify `MatrixRainCore\Application.cpp::ShouldSpanAllMonitors()` (`:374-388`) to delegate to the pure `ShouldSpanAllMonitors(m_appState->GetSettings().m_multiMonitorEnabled, m_displayMode, m_screenSaverMode)` helper. Add `#include "MultiMonitorGate.h"` to `Application.cpp` (not the header — implementation detail).
- [ ] T019 [US2] Modify `MatrixRainCore\ConfigDialogController.h` to declare `void UpdateMultiMonitorEnabled(bool enabled);`. Implement in `ConfigDialogController.cpp` mirroring `UpdateStartFullscreen` (`:135-138`) IN FULL: persist via `m_settingsProvider->Save()`, update in-memory `m_settings.m_multiMonitorEnabled`, AND register the new field in the controller's pre-dialog snapshot consumed by `CancelChanges`/`CancelLiveMode` (so dialog Cancel reverts the live preview per FR-031b). Return so the caller can POST the rebuild message.
- [ ] T020 [US2] Modify `MatrixRain\MatrixRain.rc` to add the checkbox `CONTROL "Render on all monitors", IDC_MULTIMONITOR_CHECK, "Button", BS_AUTOCHECKBOX | WS_TABSTOP, …` and the adjacent `IDC_MULTIMONITOR_INFO` owner-draw button (per R-009 placeholder; owner-draw paint comes in US5). Grow the dialog template height as needed to make room. Verify the dialog still loads and lays out correctly.
- [ ] T021 [US2] Modify `MatrixRain\ConfigDialog.cpp::OnInitDialog` (~`:259` block) to `CheckDlgButton(hDlg, IDC_MULTIMONITOR_CHECK, settings.m_multiMonitorEnabled ? BST_CHECKED : BST_UNCHECKED)`. Add a new `OnMultiMonitorCheck` handler that calls `pController->UpdateMultiMonitorEnabled(isChecked)` and `PostMessage(g_mainHwnd, WM_APP_REBUILD_CONTEXTS, 0, 0)` for the live rebuild. Wire it into `OnCommand` (`~:644`).

> **Checkpoint**: Build + full tests + manual QA: open dialog on multimon system, uncheck the new checkbox; secondary monitors stop rendering within 1 second. Restart; setting persisted. Toggle back on; monitors resume. **Commit when green.**

---

## Phase 5: User Story 3 — Pick which GPU MatrixRain uses on hybrid laptops (Priority: P2)

**Goal**: Add a dropdown listing real adapter names (with "(default)" appended to the system default), persist by description string, exclude software adapters, fall back to default if the saved adapter is missing.

**Independent Test**: On a hybrid laptop, open the dialog; verify the GPU list shows real adapter names with `" (default)"` on the default; software adapters absent. Select a non-default GPU; within 1 second Task Manager shows MatrixRain on the new GPU. Restart; persisted. Edit `HKCU\…\MatrixRain\GpuAdapter` to a fake name; restart; MatrixRain starts on default without error dialog.

### Tests for User Story 3

- [ ] T022 [P] [US3] Create `MatrixRainTests\unit\AdapterSelectionTests.cpp` with tests for `ResolveAdapter(adapters, savedDescription)` per the contract: empty saved → `nullopt`; non-matching saved → `nullopt`; matching by description → the matching `LUID`; multiple adapters with the same description (degenerate) → first match wins. Use `InMemoryAdapterProvider` seeds. Verify failing.
- [ ] T023 [P] [US3] Add tests in `AdapterSelectionTests.cpp` for `FormatAdapterLabel(adapter)`: `m_isDefault == true` → `m_description + L" (default)"`; `m_isDefault == false` → `m_description` unchanged; empty description handled gracefully (returns `L" (default)"` if default, else empty — exact behavior documented and tested).
- [ ] T024 [P] [US3] Extend `MatrixRainTests\unit\RegistrySettingsProviderTests.cpp` with round-trip tests for the `GpuAdapter` REG_SZ value: absent → default `L""`; `L"NVIDIA Whatever"` → preserved exactly; long descriptions (≥128 chars) preserved. Verify failing.

### Implementation for User Story 3

- [ ] T025 [P] [US3] Create `MatrixRainCore\IAdapterProvider.h` with the `struct AdapterInfo` and abstract `class IAdapterProvider` per `contracts/adapter-provider.md`. Add to vcxproj.
- [ ] T026 [P] [US3] Create `MatrixRainCore\InMemoryAdapterProvider.h` and `.cpp`: constructor takes `std::vector<AdapterInfo>` (stored by value); `EnumerateAdapters()` returns a copy. Add to vcxproj.
- [ ] T027 [P] [US3] Create `MatrixRainCore\AdapterSelection.h` and `.cpp` with pure `std::optional<LUID> ResolveAdapter(const std::vector<AdapterInfo>&, const std::wstring&)` and `std::wstring FormatAdapterLabel(const AdapterInfo&)`. Add to vcxproj. T022, T023 must now pass.
- [ ] T028 [US3] Create `MatrixRainCore\WindowsAdapterProvider.h` and `.cpp`. Use `CreateDXGIFactory1` for enumeration via `IDXGIFactory1::EnumAdapters1` + `GetDesc1`; obtain `IDXGIFactory6` via `QueryInterface` and call `EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_UNSPECIFIED, IID_PPV_ARGS(&defaultAdapter))` to identify the system default LUID. Skip adapters with `(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0`. Use ComPtr for COM lifetimes; use EHM (`CHRA` for external APIs). Add to vcxproj.
- [ ] T029 [US3] Modify `MatrixRainCore\ScreenSaverSettings.h` to add `std::wstring m_gpuAdapter;` (default `L""`).
- [ ] T030 [US3] Modify `MatrixRainCore\RegistrySettingsProvider.{h,cpp}` to add `VALUE_GPU_ADAPTER = L"GpuAdapter"` and load/save via existing `ReadString`/`WriteString` helpers (already used by `ColorScheme`). T024 must now pass.
- [ ] T031 [US3] Modify `MatrixRainCore\InMemorySettingsProvider.{h,cpp}` to add `m_gpuAdapter` parity.
- [ ] T032 [US3] Modify `MatrixRainCore\RenderSystem.h` to change `HRESULT Initialize(HWND hwnd, int width, int height)` signature to `HRESULT Initialize(HWND hwnd, int width, int height, std::optional<LUID> adapterLuid)`. Modify `RenderSystem.cpp::Initialize` (`:146-180`): if `adapterLuid.has_value()`, obtain `IDXGIFactory4`+ via `CreateDXGIFactory1`/`QueryInterface`, call `EnumAdapterByLuid(*adapterLuid, IID_PPV_ARGS(&adapter))`; on success call `D3D11CreateDevice(adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, …)`; on lookup failure log + fall back to `D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, …)`. Default-adapter path (no LUID) preserves existing behavior.
- [ ] T033 [US3] Modify `MatrixRainCore\MonitorRenderContext.h` and `.cpp` to add `std::optional<LUID>` parameter to `Initialize`; forward to `RenderSystem::Initialize`.
- [ ] T034 [US3] Modify `MatrixRainCore\Application.cpp`: in `Initialize` (or a new `ResolveAdapterOnce()` helper called from there), construct a `WindowsAdapterProvider`, call `EnumerateAdapters()`, call `ResolveAdapter(adapters, settings.m_gpuAdapter)`, cache the optional `LUID` as `m_resolvedAdapter`. Pass `m_resolvedAdapter` to every `MonitorRenderContext::Initialize` call. In `RebuildContextsForCurrentMode` (`:883-940`), re-resolve the adapter before re-initializing contexts (so a device-loss recovery after a GPU removal picks up the fallback path automatically).
- [ ] T035 [US3] Modify `MatrixRainCore\ConfigDialogController.{h,cpp}` to add `void UpdateGpuAdapter(const std::wstring& description)` mirroring `UpdateColorScheme` (`:62-78`) IN FULL: persist, update in-memory state, AND register the new field in the controller's pre-dialog snapshot consumed by `CancelChanges`/`CancelLiveMode` so dialog Cancel reverts the live preview per FR-031b.
- [ ] T036 [US3] Modify `MatrixRain\MatrixRain.rc` to add `LTEXT "GPU:"`, `COMBOBOX IDC_GPU_COMBO …, CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP`, and `IDC_GPU_INFO` owner-draw button. Grow dialog height to fit.
- [ ] T037 [US3] Modify `MatrixRain\ConfigDialog.cpp`: in `OnInitDialog`, construct a `WindowsAdapterProvider`, enumerate, populate `IDC_GPU_COMBO` via `CB_ADDSTRING` using `FormatAdapterLabel`. Track the underlying descriptions in a `std::vector<std::wstring>` indexed by combo position so `OnGpuChange` (a new handler mirroring `OnColorSchemeChange` at `:345-363`) can call `pController->UpdateGpuAdapter(descriptions[CB_GETCURSEL])` and `PostMessage(g_mainHwnd, WM_APP_REBUILD_CONTEXTS, 0, 0)`.

> **Checkpoint**: Build + full tests + manual QA on a hybrid laptop: dropdown shows real names with "(default)"; pick non-default; restart; persisted; runs on the picked GPU per Task Manager. Edit registry to fake name; restart; falls back silently. **Commit when green.**

---

## Phase 6: User Story 4 — Cap frame rate on high-refresh monitors (Priority: P2)

**Goal**: On any monitor with native refresh > 60 Hz, limit MatrixRain rendering to ~60 FPS; on ≤60 Hz monitors, leave existing vsync behavior untouched with zero added overhead.

**Independent Test**: Enable debug statistics. On a >60 Hz monitor, observe FPS ~60. On a 60 Hz monitor, FPS ~60 (unchanged). Mixed-refresh multimon: each monitor independently shows ~60. Compare GPU utilization on the high-refresh path before/after — expect ≥50% reduction.

### Tests for User Story 4

- [ ] T038 [P] [US4] Create `MatrixRainTests\unit\FrameLimiterTests.cpp`: `ShouldEngageFrameLimiter(refreshHz)` truth table for `0, 30, 59, 60, 61, 75, 120, 144, 240` (only `> 60` returns true); `FrameLimiter::TargetFps(60)` produces a `WaitForNextFrame` that returns immediately on the first call; the second call returns approximately 16.6ms (±2ms) after the first. Use `std::chrono::steady_clock` measurements with reasonable tolerance for CI flakiness. Verify failing.

### Implementation for User Story 4

- [ ] T039 [P] [US4] Create `MatrixRainCore\FrameLimiter.h` and `.cpp` with: pure free function `bool ShouldEngageFrameLimiter(unsigned monitorRefreshHz)` and class `FrameLimiter { void TargetFps(unsigned); void WaitForNextFrame(); }` using `std::chrono::steady_clock` for the last-frame timestamp. Add to vcxproj. T038 must now pass.
- [ ] T040 [US4] Modify `MatrixRainCore\MonitorRenderContext.h` to add `std::optional<FrameLimiter> m_frameLimiter;` member. Modify `MonitorRenderContext::Initialize` (`.cpp`) to accept the monitor's `refreshHz` (from `DEVMODE::dmDisplayFrequency` or existing `MonitorInfo`); if `ShouldEngageFrameLimiter(refreshHz)`, construct `m_frameLimiter` with `TargetFps(60)`; else leave `nullopt`. Plumb the refresh rate through `Application.cpp` when constructing each `MonitorRenderContext`.
- [ ] T041 [US4] Modify `MatrixRainCore\MonitorRenderContext.cpp::RenderThreadProc` (`:327-402`): at the top of each loop iteration (after the `m_inTransition` skip at `:349-353` and before the `lock_guard` at `:356`), call `if (m_frameLimiter) m_frameLimiter->WaitForNextFrame();`. This sleeps only when the limiter is engaged; on ≤60 Hz monitors the call is skipped entirely, preserving zero overhead.

> **Checkpoint**: Build + full tests + manual QA: on >60 Hz monitor, debug-stats FPS reads ~60; GPU usage on the high-refresh path measurably lower than baseline. On a 60 Hz monitor, FPS reads ~60 (existing). **Commit when green.**

---

## Phase 7: User Story 5 — Graphics quality presets and advanced controls (Priority: P3)

**Goal**: Add a Quality preset combobox (Low/Medium/High/Custom) with an advanced disclosure that reveals three discrete sliders (Passes / Resolution / Smoothness) and information tips on each quality control. Glow Intensity owns true on/off via the existing slider. Custom-snap behavior, dynamic dialog resize, first-run heuristic, and tooltip accessibility all per the locked design.

**Independent Test**: Verify all 10 acceptance scenarios in spec User Story 5: preset combo entries; advanced controls hidden by default; toggle reveals advanced + grows dialog; moving advanced flips to Custom; selecting named preset snaps advanced; switching to Custom restores LastCustom or stays put; Glow Intensity 0 → label `"0% (glow disabled)"` + true bypass; hover on any ⓘ shows tooltip; keyboard Tab + Space on any ⓘ shows same tooltip; first-run heuristic picks correct default by GPU class.

### Tests for User Story 5

- [ ] T042 [P] [US5] Create `MatrixRainTests\unit\QualityPresetsTests.cpp` exhaustively covering:
  - `LookupPresetValues(QualityPreset::Low/Medium/High)` returns exactly the rows from `contracts/quality-preset-mapping.md`.
  - `DetectActivePreset(values)` returns the named preset whose row exactly matches `values`, else `QualityPreset::Custom`. Test each named row plus several off-table combinations.
  - `ApplyPresetSnap(preset, current, lastCustom)`: named preset → returns lookup; `Custom` + `lastCustom.has_value()` → returns `*lastCustom`; `Custom` + `!lastCustom.has_value()` → returns `current` unchanged.
  - `PickDefaultQualityPreset(adapters, totalPixels)`: discrete adapter (vram ≥ 256 MB, not software) → `High`; integrated-only + totalPixels ≤ 16M → `Medium`; integrated-only + totalPixels > 16M → `Low`. Use `InMemoryAdapterProvider` seeds.
  - Verify the constants `kDiscreteVramThresholdMb` and `kHeavyTotalPixelsThreshold` are at their documented values via public-test accessors (or extern constexpr declarations in the header for test visibility).
  - Verify failing.
- [ ] T043 [P] [US5] Extend `MatrixRainTests\unit\RegistrySettingsProviderTests.cpp` with round-trip tests for: `QualityPreset` REG_SZ accepting `"Low"`/`"Medium"`/`"High"`/`"Custom"`/`""`; all four `LastCustom_*` DWORDs read as a unit (any missing → `LastCustomGraphicsValues = nullopt`); `ShowAdvancedGraphics` DWORD default 0. Verify failing.
- [ ] T044 [P] [US5] Extend `MatrixRainTests\unit\ConfigDialogControllerTests.cpp` (or create) with tests for the new controller methods: `UpdateQualityPreset(Low)` snaps advanced values; `UpdateAdvancedGraphicsValues(custom)` drifts preset to Custom and persists `LastCustom_*`; round-trip persists across `Load`. Verify failing.

### Implementation for User Story 5

- [ ] T045 [P] [US5] Create `MatrixRainCore\QualityPresets.h` and `MatrixRainCore\QualityPresets.cpp` with: `enum class QualityPreset`; `enum class ResolutionDivisor`; `enum class BlurTaps`; `struct AdvancedGraphicsValues`; `static constexpr` heuristic constants; pure helpers `LookupPresetValues`, `DetectActivePreset`, `ApplyPresetSnap`, `PickDefaultQualityPreset`. Add to vcxproj. T042 must now pass.
- [ ] T046 [US5] Modify `MatrixRainCore\ScreenSaverSettings.h` to add `QualityPreset m_qualityPreset = QualityPreset::High;`, `AdvancedGraphicsValues m_advancedValues` (initialized to `LookupPresetValues(QualityPreset::High)`), `std::optional<AdvancedGraphicsValues> m_lastCustom;`, `bool m_showAdvancedGraphics = false;`. Validate/clamp on load (out-of-range integers clamp; invalid enum strings → default). Include `QualityPresets.h`.
- [ ] T047 [US5] Modify `MatrixRainCore\RegistrySettingsProvider.{h,cpp}` to add all new value-name constants and load/save:
  - `VALUE_QUALITY_PRESET` REG_SZ
  - `VALUE_LASTCUSTOM_GLOW_INTENSITY`, `VALUE_LASTCUSTOM_PASSES`, `VALUE_LASTCUSTOM_RESOLUTION`, `VALUE_LASTCUSTOM_SMOOTHNESS` (DWORD; all-or-nothing read — if any absent, do not populate `m_lastCustom`)
  - `VALUE_SHOW_ADVANCED_GRAPHICS` DWORD
  T043 must now pass.
- [ ] T048 [US5] Modify `MatrixRainCore\InMemorySettingsProvider.{h,cpp}` for field parity.
- [ ] T049 [US5] Modify `MatrixRainCore\RenderSystem.h` and `RenderSystem.cpp` to parametrize bloom:
  - Add member fields `int m_blurPasses = 3`, `ResolutionDivisor m_bloomResolutionDivisor = Half`, `BlurTaps m_blurTaps = High` with setters used by the existing snapshot path.
  - Replace literal `3` in the loop at `:1372` with `m_blurPasses`.
  - Replace `width / 2` / `height / 2` in `CreateBloomResources` (`:1175-1176`) and the bloom viewport at `:1337` with division by `static_cast<int>(m_bloomResolutionDivisor)` (the enum's integer values are the divisors).
  - Compile and store three blur-shader variants (5-tap, 9-tap, 13-tap) at startup alongside the existing extract/composite shaders (`:1089-1100` area); select the active variant by `m_blurTaps` in `ApplyBloom`.
  - When bloom-buffer dimensions change (because the resolution divisor changed), recreate `m_bloomTexture`, `m_bloomRTV`, `m_bloomSRV`, `m_blurTempTexture`, `m_blurTempRTV`, `m_blurTempSRV` — use the existing resize teardown path.
- [ ] T050 [US5] Modify `MatrixRainCore\RenderSystem.cpp`: at the top of the existing post-process branch (around `:1726`), if `m_glowIntensityPercent == 0` take the direct-to-backbuffer fallback (`:1731`-style path) and skip `ApplyBloom` entirely. Verify that the existing fallback path correctly renders the scene without the post-process pipeline.
- [ ] T051 [US5] Modify `MatrixRainCore\Application.cpp::Initialize` to call `PickDefaultQualityPreset(adapters, totalMonitorPixels)` when `settings.m_qualityPreset` is the "not yet set" sentinel (loaded from an empty `QualityPreset` REG_SZ) AND `settings.m_lastCustom == nullopt`. Persist the chosen preset immediately via the controller/settings-save path so subsequent runs skip the heuristic.
- [ ] T052 [US5] Modify `MatrixRainCore\ConfigDialogController.{h,cpp}` to add:
  - `void UpdateQualityPreset(QualityPreset)` — calls `ApplyPresetSnap`, writes back `m_settings.m_advancedValues`, persists.
  - `void UpdateAdvancedGraphicsValues(const AdvancedGraphicsValues&)` — writes new values; updates `m_lastCustom` (always); recomputes `m_qualityPreset = DetectActivePreset(values)`; persists.
  - `void UpdateShowAdvancedGraphics(bool)` — persists.
  ALL three new setters MUST also register their corresponding fields in the controller's pre-dialog snapshot consumed by `CancelChanges`/`CancelLiveMode` so dialog Cancel reverts every live preview per FR-031b (including the cascade where moving an advanced slider auto-flips the preset to Custom — that flip must also revert).
  T044 must now pass.
- [ ] T053 [US5] Modify `MatrixRain\MatrixRain.rc` to:
  - Add a `GROUPBOX "Graphics quality"` containing `IDC_QUALITY_PRESET_COMBO`, `IDC_QUALITY_PRESET_INFO`, `IDC_GRAPHICS_ADVANCED_CHECK`, `IDC_GRAPHICS_ADVANCED_INFO`, and the three advanced sliders with their value labels and info buttons.
  - Add `IDC_GLOWPASSES_SLIDER` (range 1..4) with `IDC_GLOWPASSES_LABEL` to the right, `IDC_GLOWPASSES_INFO` further right, and four `LTEXT "1" "2" "3" "4"` aligned beneath the tick positions.
  - Add `IDC_GLOWRES_SLIDER` (range 0..3) with `IDC_GLOWRES_LABEL` and `IDC_GLOWRES_INFO`, plus `LTEXT "Eighth" "Quarter" "Half" "Full"` beneath ticks.
  - Add `IDC_GLOWSMOOTH_SLIDER` (range 0..2) with `IDC_GLOWSMOOTH_LABEL` and `IDC_GLOWSMOOTH_INFO`, plus `LTEXT "Low" "Medium" "High"` beneath ticks.
  - Add `IDC_GLOWINTENSITY_INFO` next to the existing Glow Intensity slider; `IDC_GLOWSIZE_INFO` next to Glow Size.
  - Lay out the dialog at its EXPANDED size (all advanced controls visible). The `OnInitDialog` code will collapse them if the saved `ShowAdvancedGraphics` is false.
- [ ] T054 [US5] Modify `MatrixRain\ConfigDialog.cpp::InitializeSlider` (`:76-85`):
  - Send `TBM_SETTICFREQ` per the locked per-slider table (`Density`: 5; `AnimSpeed`: 5; `GlowIntensity`: 10; `GlowSize`: 5; new discrete sliders: 1).
  - For `IDC_ANIMSPEED_SLIDER`, additionally send `TBM_SETTIC, 0, 100` to add the 21st tick at 100.
  - Special-case label text: `IDC_GLOWINTENSITY_SLIDER` at value 0 → `"0% (glow disabled)"`; the three discrete sliders use mapped strings (`"1".."4"`, `"Eighth"`/`"Quarter"`/`"Half"`/`"Full"`, `"Low"`/`"Medium"`/`"High"`).
  - Extend the WM_HSCROLL handler (`:294-330`) to use the same mapping when updating value labels live.
- [ ] T055 [US5] Modify `MatrixRain\ConfigDialog.cpp::OnInitDialog`:
  - Populate `IDC_QUALITY_PRESET_COMBO` with `"Low"`, `"Medium"`, `"High"`, `"Custom"` via `CB_ADDSTRING`; set selection from `settings.m_qualityPreset`.
  - Set initial state of `IDC_GRAPHICS_ADVANCED_CHECK` from `settings.m_showAdvancedGraphics`.
  - Record the rects of all advanced controls (`GetWindowRect` + `MapWindowPoints` → client coords). Compute `m_advancedBlockHeight`.
  - If `!settings.m_showAdvancedGraphics`: `ShowWindow(SW_HIDE)` each advanced control; `SetWindowPos` the dialog to shrink by `m_advancedBlockHeight`; `MoveWindow` the OK/Cancel/Reset buttons up by the same delta. Implement an inverse transform handler on `IDC_GRAPHICS_ADVANCED_CHECK` `BN_CLICKED` that toggles between collapsed and expanded.
- [ ] T056 [US5] Modify `MatrixRain\ConfigDialog.cpp`: implement WM_HSCROLL handlers for `IDC_GLOWPASSES_SLIDER`, `IDC_GLOWRES_SLIDER`, `IDC_GLOWSMOOTH_SLIDER`. Each reads the new value, constructs an `AdvancedGraphicsValues` from all three sliders + glow intensity, calls `pController->UpdateAdvancedGraphicsValues(values)`, then updates `IDC_QUALITY_PRESET_COMBO` selection from `DetectActivePreset(values)` (typically flipping to Custom).
- [ ] T057 [US5] Modify `MatrixRain\ConfigDialog.cpp`: implement `CBN_SELCHANGE` handler for `IDC_QUALITY_PRESET_COMBO`. Translate selection → `QualityPreset`. Call `pController->UpdateQualityPreset(preset)`; the controller updates `m_advancedValues` (via `ApplyPresetSnap`). The dialog then reflects the new values back into the three advanced sliders via `TBM_SETPOS` and re-runs `InitializeSlider`-style label updates.
- [ ] T058 [US5] Modify `MatrixRain\ConfigDialog.cpp::OnInitDialog`: create a shared `WC_TOOLTIP` window (`CreateWindowEx(..., TOOLTIPS_CLASS, ...)`). For each `IDC_*_INFO` control, register a tool with `TTM_ADDTOOL` using flags `TTF_IDISHWND | TTF_SUBCLASS`, `lpszText = LPSTR_TEXTCALLBACK`. Implement `TTN_GETDISPINFO` handler that switches on `((LPNMHDR)lParam)->idFrom` (the hwnd) to return the matching infotip text per the locked strings in `plan.md`/research R-009 (each ending with `"Significant GPU performance impact."`, `"Moderate GPU performance impact."`, or `"Small GPU performance impact."`).
- [ ] T059 [US5] Modify `MatrixRain\ConfigDialog.cpp`: implement `WM_DRAWITEM` for each `IDC_*_INFO` button — draw a 1-pixel circle outline within the button rect, then `DrawText`/`TextOut` a centered lowercase "i" using the dialog's font. Implement `BN_CLICKED` keyboard activation: on click (Space/Enter on focused button), look up the matching `TTF_TRACK` tool registration, position the tip near the button (`TTM_TRACKPOSITION`), and `TTM_TRACKACTIVATE TRUE`. Set a `SetTimer` for 5 seconds (or hook `WM_KILLFOCUS` / `WM_KEYDOWN VK_ESCAPE`) to `TTM_TRACKACTIVATE FALSE`.

> **Checkpoint**: Build + full tests + manual QA per quickstart US5: all 12 walkthrough steps pass. **Commit when green.**

---

## Phase 8: Polish & Cross-Cutting Concerns

- [ ] T060 [P] Run end-to-end quickstart validation (build → test → manual QA per `specs\006-multimon-gpu-efficiency\quickstart.md` Section 4 for all 5 user stories on a single development workstation).
- [ ] T061 Manual QA on a real hybrid laptop (Surface Book 3 / Surface Laptop Studio 2 / Optimus laptop) covering: undock/redock GPU%, GPU dropdown switch + Task Manager verification, multimon-off GPU%, high-refresh display 60 FPS confirmation, pre/post v1.3-baseline GPU% comparison per SC-001/SC-003/SC-004/SC-005, AND a suspend/resume cycle (close lid or `Start → Sleep`, wait ≥10s, resume) verifying MatrixRain continues running and that the device-loss recovery path tolerates the resulting `DXGI_ERROR_DEVICE_REMOVED`. Capture before/after GPU% screenshots and append to `specs\006-multimon-gpu-efficiency\quickstart.md` or a sibling QA notes file.
- [ ] T062 [P] Update `CHANGELOG.md` with the v1.4 release section summarizing the five user-visible improvements and citing this feature spec.
- [ ] T063 Update `specs\006-multimon-gpu-efficiency\spec.md` "Status" field from `Draft` to `Implemented`; mark the requirements checklist as fully complete.

---

## Dependencies & Execution Order

### Phase dependencies

- **Phase 1 (Setup)**: no dependencies; can start immediately. T001 and T002 are independent and parallelizable.
- **Phase 2 (Foundational)**: empty by design (see rationale at top of Phase 2).
- **Phase 3 (US1, P1, MVP)**: after Phase 1; independent of all other user stories.
- **Phase 4 (US2, P1)**: after Phase 1; independent of US1 and others. (Touches different files than US1 except both edit `Application.cpp`; sequence T021 after T009/T010 if a developer works both stories serially. Otherwise merge resolution is trivial — different functions in the same file.)
- **Phase 5 (US3, P2)**: after Phase 1 (needs T001 for `<dxgi1_6.h>`); independent of US1/US2/US4/US5. Touches `RenderSystem` and `MonitorRenderContext` signatures.
- **Phase 6 (US4, P2)**: after Phase 1; touches `MonitorRenderContext` — sequence after US3 (T032/T033) when a single developer is doing both (same files), or accept a small merge if parallel.
- **Phase 7 (US5, P3)**: after Phase 1; the largest phase. Touches `RenderSystem` parametric bloom (so sequence after any US3 changes to `RenderSystem.cpp` signature for a clean merge).
- **Phase 8 (Polish)**: after all desired user stories are complete.

### Within each user story

- Tests (constitution-mandated for core code) MUST be written and fail before their corresponding implementation task.
- Pure helpers ([P] tasks within a story) can land in parallel.
- Helpers before consumers (e.g., T005/T006 before T008/T009 in US1; T014 before T018 in US2; T027 before T034 in US3; T039 before T041 in US4; T045 before T051/T052 in US5).
- Within US5, the rendering parametrization (T049, T050) can land before any UI changes (T053+) and gives an early visual check by manually editing settings via the registry.

### Parallel opportunities

- **Across stories**: Once Phase 1 lands, all five user-story phases can be worked in parallel by separate developers (US1 has the lightest file footprint; US5 is the heaviest). Recommended order if solo: **US1 → US2 → US4 → US3 → US5** (US4 is small and unlocks a meaningful GPU win quickly; US3 sets up the adapter plumbing that US5's first-run heuristic consumes).
- **Within Phase 1**: T001 ∥ T002.
- **Within each user story's test layer**: every test task is `[P]` (different test files).
- **Within each user story's helper layer**: every helper-file task is `[P]` (different source files); consumers that modify shared files (`RenderSystem`, `MonitorRenderContext`, `Application`, `ConfigDialog`, `ScreenSaverSettings`, `RegistrySettingsProvider`) are NOT parallelizable within a story but ARE parallelizable across stories if developers coordinate or accept light merges.

---

## Parallel Example: User Story 1

```bash
# Phase 3 — tests, both run in parallel:
Task: "T003 [P] [US1] Create MatrixRainTests\unit\DeviceLostTests.cpp …"
Task: "T004 [P] [US1] Create MatrixRainTests\unit\RebuildCoalescerTests.cpp …"

# Verify both test files fail to build / fail to pass (helpers don't exist yet).

# Phase 3 — helpers, both run in parallel:
Task: "T005 [P] [US1] Create MatrixRainCore\DeviceLost.{h,cpp} …"
Task: "T006 [P] [US1] Create MatrixRainCore\RebuildCoalescer.{h,cpp} …"

# Verify both test files now pass.

# Phase 3 — sequential consumers (each touches a different existing file):
Task: "T007 [US1] Modify MatrixRainCore\RenderSystem.{h,cpp} …"
Task: "T008 [US1] Modify MatrixRainCore\MonitorRenderContext.cpp …"
Task: "T009 [US1] Modify MatrixRainCore\Application.{h,cpp} …"
Task: "T010 [US1] Modify MatrixRainCore\Application.cpp …"
```

---

## Implementation Strategy

### MVP first (User Story 1)

1. Complete Phase 1: Setup (T001-T002).
2. Skip Phase 2 (intentionally empty).
3. Complete Phase 3: US1 (T003-T010).
4. **STOP and VALIDATE**: undock/redock manual QA + Device Manager GPU-disable test.
5. Deploy / demo. **This single story is the v1.4 defect-fix release if needed.**

### Incremental delivery

1. Phase 1 → Foundation ready.
2. + Phase 3 (US1) → MVP defect fix; demo.
3. + Phase 4 (US2) → Multimon opt-out; demo.
4. + Phase 6 (US4) → Frame cap; demo (quietly bigger win than US2).
5. + Phase 5 (US3) → GPU dropdown; demo.
6. + Phase 7 (US5) → Quality presets and advanced UI; demo.
7. + Phase 8 (Polish) → Release.

### Parallel team strategy

With three developers after Phase 1 lands:
- Developer A: US1 (Phase 3) → US3 (Phase 5)
- Developer B: US2 (Phase 4) → US4 (Phase 6)
- Developer C: US5 (Phase 7)

Merge conflicts will be concentrated on `RenderSystem.{h,cpp}` (US3 + US5), `MonitorRenderContext.{h,cpp}` (US3 + US4), and `Application.cpp` (all five). Sequence the merges so the signature changes (US3) land first.

---

## Notes

- Constitution principle I (TDD non-negotiable for core library): every core-library task in this file is preceded by a `[test]`-equivalent test task. UI/dialog work in `MatrixRain.exe` is exempt per the scope-exemption clause.
- Constitution principle IX (commit discipline): **one task = one commit**. Each commit must build clean and pass the full test suite (`354 baseline + new tests`). Do not bundle tasks.
- Constitution principle VIII (formatting): all new code follows the project's 5-blank-lines-between-top-level-constructs, column-aligned-with-separate-pointer-column conventions. `git diff` each task before committing.
- Constitution principle VII (PCH): the only new system header is `<dxgi1_6.h>` in T001; no other `#include <…>` directives are added to non-PCH files.
- Conventional Commits + `Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>` trailer on every commit.
- `Version.h` is auto-bumped pre-build; exclude from `git add`.
- `MSBuild` invocation MUST NOT use `/m` (transient PCH `C3859`/`C1076` failures).
- Each "Checkpoint" line ends a deployable increment.
