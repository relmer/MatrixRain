# Phase 0 Research — Multi-Monitor User Control and GPU Efficiency

**Feature**: `006-multimon-gpu-efficiency`
**Status**: Complete. All technical unknowns were resolved during the pre-spec planning discussion with the user; this document consolidates the chosen approaches, the rationale, and the alternatives considered, so subsequent phases can proceed without further investigation.

---

## R-001: Multi-monitor enabled/disabled — runtime apply

**Decision**: Toggling the setting POSTs `WM_APP_REBUILD_CONTEXTS` to the application's main window; the existing `RebuildContextsForCurrentMode` flow (`Application.cpp:883-940`) tears down all `MonitorRenderContext` threads, re-enumerates monitors, recomputes the layout, and starts the new set. Settings-driven gate is via a pure `ShouldSpanAllMonitors(bool multimonEnabled, DisplayMode displayMode, ScreenSaverMode saverMode)` helper used by `Application::ShouldSpanAllMonitors()` (`Application.cpp:374-388`).

**Rationale**: This is the same teardown/rebuild path already used today for Alt+Enter (mode toggle), so we get atomicity, lock ordering, and overlay/dialog handling for free. The pure gate is unit-testable in isolation, eliminating logic from the Win32 layer.

**Alternatives considered**: Apply at next launch only (rejected per user direction — feels broken). Selectively start/stop per-monitor contexts without a full rebuild (rejected — added complexity for a setting that changes infrequently and is already cheap to rebuild).

---

## R-002: Runtime monitor add/remove detection

**Decision**: Add a `case WM_DISPLAYCHANGE:` handler to `Application::HandleMessage` (`Application.cpp:1081…`). On receipt, set an `std::atomic_flag m_rebuildPending` and POST a single `WM_APP_REBUILD_CONTEXTS`. The `WM_APP_REBUILD_CONTEXTS` handler clears the flag and runs `RebuildContextsForCurrentMode`. This coalesces the burst of `WM_DISPLAYCHANGE` messages the system broadcasts to every top-level window in a multi-window app.

**Rationale**: `WM_DISPLAYCHANGE` is the standard, kernel-broadcast notification. Windows delivers it to every top-level window (which on multimon = once per `MonitorRenderContext` hwnd). Without coalescing we would trigger N rebuilds for one topology change. The atomic flag is the simplest viable coalescing strategy and is pure-testable as a `CoalesceRebuildRequest(flag)` helper.

**Alternatives considered**: `WM_DEVICECHANGE` filter for monitor devices (rejected — fires for many unrelated device classes, more complex filter, no advantage over `WM_DISPLAYCHANGE`). Polling `EnumDisplayMonitors` on a timer (rejected — wasteful and laggy). `IDXGIFactory::RegisterStereoStatusWindow` and friends (irrelevant API).

---

## R-003: Device-loss detection and recovery

**Decision**: Capture the HRESULT returned from `m_swapChain->Present(1, 0)` in `RenderSystem::Present` (`RenderSystem.cpp:1753-1758`). Pure helper `bool IsDeviceLost(HRESULT)` returns true for `DXGI_ERROR_DEVICE_REMOVED`, `DXGI_ERROR_DEVICE_RESET`, `DXGI_ERROR_DEVICE_HUNG`, `DXGI_ERROR_DRIVER_INTERNAL_ERROR`, and `D3DDDIERR_DEVICEREMOVED`. On true, the `MonitorRenderContext` render thread exits its loop and POSTs `WM_APP_REBUILD_CONTEXTS` to its HWND. The main-thread rebuild re-resolves the chosen adapter via `ResolveAdapter` (which falls back to default if the prior selection vanished) and reinitializes.

**Rationale**: `Present` is the canonical detection point for D3D11. Classifying via a pure helper lets us unit-test the HRESULT mapping without a real device. The render-thread-posts-to-UI-thread pattern reuses the existing rebuild barrier, ensuring lock-correctness.

**Alternatives considered**: Calling `GetDeviceRemovedReason` only on `DXGI_ERROR_DEVICE_REMOVED` (kept as best-effort logging within the same path). Polling `IDXGIDevice::GetCreationFlags` or similar (no useful liveness signal). Catching the error at `D3D11CreateDevice` rebuild time only (insufficient — first symptom is at Present).

---

## R-004: GPU adapter enumeration and persistence

**Decision**:
- **Enumeration**: New `IAdapterProvider` interface with `WindowsAdapterProvider` (uses `CreateDXGIFactory1` → `IDXGIFactory1::EnumAdapters1` for the list; uses `CreateDXGIFactory2` → `IDXGIFactory6::EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_UNSPECIFIED, …)` to identify the system default; filters out adapters with `DXGI_ADAPTER_FLAG_SOFTWARE`).
- **Persistence**: Adapter is stored by `DXGI_ADAPTER_DESC1::Description` (REG_SZ at value name `GpuAdapter` under `HKCU\Software\relmer\MatrixRain`). Empty string = "use system default".
- **Resolution**: Pure helper `ResolveAdapter(const vector<AdapterInfo>& adapters, const wstring& savedDescription) -> std::optional<LUID>`. Returns the matching adapter's LUID, or `nullopt` if the saved description is empty or not found (caller then calls `D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, …)`).
- **Device creation**: `RenderSystem::Initialize` (`RenderSystem.cpp:146-180`) gains an optional adapter LUID parameter. If supplied, `IDXGIFactory4::EnumAdapterByLuid` looks up the adapter and `D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, …)` creates the device on it. If absent, the existing `nullptr` + `D3D_DRIVER_TYPE_HARDWARE` path is preserved unchanged.

**Rationale**: Description-string persistence trades a tiny risk of driver-update renames for the considerable benefit of human-readable, reboot-stable identification. LUID is not guaranteed stable across reboots; enumeration index is not stable across hardware changes. The mandatory `D3D_DRIVER_TYPE_UNKNOWN` paired with a non-null adapter is a documented D3D11 requirement. Excluding software adapters matches the user's intent ("hybrid laptops, pick integrated vs discrete").

**Alternatives considered**: LUID-based persistence (rejected — not stable). Enumeration index (rejected — fragile). Synthetic "Default" + "High performance" preference entries instead of real adapter names (rejected per user — show real names). Legacy `extern "C" __declspec(dllexport) DWORD NvOptimusEnablement = 1;` and `int AmdPowerXpressRequestHighPerformance = 1;` exports (rejected for v1.4 — they are static, evaluated at process load, and would force the discrete GPU always, defeating the integrated-for-power case; on Windows 10 1803+ the DXGI-based path is sufficient).

---

## R-005: Hybrid-laptop discrete-GPU routing

**Decision**: Rely entirely on `IDXGIFactory6::EnumAdapterByGpuPreference` for advertising the high-performance adapter and on `D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, …)` for actually creating the device on the user's chosen physical GPU. Document the cross-adapter present-time copy (DWM runs on the integrated GPU) as expected behavior.

**Rationale**: On Windows 10 1803+ with current drivers the DXGI path correctly routes the process to the chosen physical GPU on NVIDIA Optimus, AMD PowerXpress, and Surface-class hybrid systems. Adding the legacy magic-export symbols would *force* discrete always (they are evaluated at process-load time before any user setting is read), which is exactly the wrong behavior for a "let the user pick" feature.

**Alternatives considered**: Adding the legacy magic exports as a belt-and-suspenders fallback (deferred — revisit only if real-world testing on a target hybrid configuration shows the OS-level APIs are insufficient). Modifying the per-app OS GPU preference at `HKCU\…\DirectX\UserGpuPreferences` (rejected — too invasive; that's a user-controlled OS setting).

---

## R-006: Frame-rate cap on high-refresh monitors

**Decision**: Pure `FrameLimiter` class wrapping a `std::chrono::steady_clock` last-frame timestamp. API: `void TargetFps(unsigned)` to configure, `void WaitForNextFrame()` to block (sleep) until next frame slot. A pure free function `bool ShouldEngageFrameLimiter(unsigned monitorRefreshHz)` returns true iff `monitorRefreshHz > 60`. `MonitorRenderContext::Initialize` accepts the monitor's refresh rate and, when the engage predicate is true, calls `WaitForNextFrame` at the top of each render-loop iteration before the existing `Present(1, 0)`; otherwise the limiter is bypassed entirely and the loop is unchanged.

**Rationale**: `steady_clock` is monotonic and the natural fit. Per-monitor refresh is already available via `DEVMODE::dmDisplayFrequency` or the existing monitor-enumeration data. Gating on `> 60Hz` keeps the 60Hz case zero-overhead. The pure helpers are trivially unit-tested (elapsed → sleep math; engage truth table).

**Alternatives considered**: `QueryPerformanceCounter` (no benefit over `steady_clock` for ~60Hz timing). DXGI `WaitableObject` swap chains (more complex; would force a separate code path for the high-refresh case; benefit not justified). Global frame cap across all monitors regardless of refresh (rejected per user direction).

---

## R-007: Bloom-pipeline parametrization

**Decision**: Replace the hardcoded values in `RenderSystem::ApplyBloom` (`RenderSystem.cpp:1328-1395`) with run-time parameters:
- **Blur iterations**: was hardcoded `for (int pass = 0; pass < 3; ++pass)` at `:1372`; becomes `m_blurPasses` (1..4).
- **Bloom buffer resolution**: was hardcoded `width / 2` / `height / 2` (`:1175-1176`, viewport `:1337`); becomes `width / m_bloomResolutionDivisor` (divisor ∈ {1, 2, 4, 8} mapping Full/Half/Quarter/Eighth).
- **Blur kernel taps**: select one of three precompiled blur shader variants (5-tap, 9-tap, 13-tap) at bind time. Variants are compiled at startup alongside the existing extract/composite shaders.
- **Glow on/off**: when Glow Intensity == 0, the entire bloom pipeline is bypassed (the existing direct-to-backbuffer fallback at `:1731` is taken).

**Rationale**: Discrete shader variants for kernel taps avoid the dynamic-loop cost of a uniform-driven blur kernel and let HLSL fully unroll each variant. The integer divisor for resolution preserves the existing texture-recreation path with no special-case math. Bypassing the pipeline at intensity == 0 gives the true power savings the user expects from "off" (no extract pass, no blur passes, no composite pass).

**Alternatives considered**: One blur shader with a `loopCount` uniform (rejected — dynamic loops are slow; lost optimization headroom). Skipping individual passes by conditional `RenderFullscreenPass` calls in the loop (less clean than parametrizing the loop bound).

---

## R-008: Quality preset spectrum and custom-snap behavior

**Decision**:
- **Presets**: `enum class QualityPreset { Low, Medium, High, Custom }`.
- **Preset → knob map** (in `QualityPresets.cpp`, pure lookup table):

  | Preset | GlowIntensity | Passes | Resolution | Smoothness |
  |--------|---------------|--------|------------|------------|
  | Low    | 75            | 1      | Quarter    | Low (5)    |
  | Medium | 100           | 2      | Half       | Med (9)    |
  | High   | 100           | 3      | Half       | High (13)  |

- **High == today's exact values** — by design, so existing users see no change after upgrade.
- **First-run heuristic**: `PickDefaultQualityPreset(AdapterClass, dedicatedVramMB, totalPixelCount)`:
  - Any discrete adapter → `High`.
  - Integrated only, totalPixelCount ≤ ~16M → `Medium`.
  - Integrated only, totalPixelCount > ~16M → `Low`.
- **Custom-snap behavior** (pure helpers `ApplyPresetSnap`, `DetectCustomDrift`):
  - Preset → named preset: all advanced controls snap to that preset's row.
  - Any advanced control change: preset auto-flips to `Custom`; current values become the in-memory `LastCustom`.
  - Preset → `Custom`: if a saved `LastCustom` exists in the registry, restore it; else leave the advanced controls at their current values (whatever the previous preset's values were until the user touches one).
- **Persistence**: `QualityPreset` (REG_SZ "Low"/"Medium"/"High"/"Custom"). `LastCustom_Passes` / `LastCustom_Resolution` / `LastCustom_Smoothness` / `LastCustom_GlowIntensity` (DWORDs) — written whenever `LastCustom` is updated, even if the active preset is named (so the user gets their last custom back when they later switch to Custom).

**Rationale**: Three named presets cover the meaningful design points and avoid the redundancy of the originally proposed Battery (already reachable via Glow Intensity 0) and Ultra (gratuitous for a screensaver). The first-run heuristic is intentionally static and run only when no preset is saved (no runtime adaptive downshift), per the user's explicit preference. The custom-snap behavior makes preset switching predictable and never silently loses user work.

**Alternatives considered**: Five-preset spectrum (rejected as redundant — see above). Three presets with auto-saved per-preset "last custom" overrides (over-engineered). Continuous quality slider 0..100 (rejected — there's no natural continuum across the three independent knobs).

---

## R-009: Information tip ("ⓘ") control

**Decision**: Each information tip is an owner-drawn `BS_OWNERDRAW | WS_TABSTOP` push button (~12×12 dialog units) drawn via `WM_DRAWITEM` as a 1-pixel circle outline with a lowercase "i" centered inside. A single shared `WC_TOOLTIP` window (created in `OnInitDialog`) hosts the actual tooltip text. Each info button is registered as a tool with `TTF_IDISHWND | TTF_SUBCLASS` for hover behavior. For keyboard activation, each info button's `BN_CLICKED` handler positions the tooltip below/right of the button and calls `TTM_TRACKACTIVATE` on a parallel `TTF_TRACK` registration; the tooltip dismisses on kill-focus, ESC, or a 5-second timer. Per-button text is supplied via `TTN_GETDISPINFO`, keyed by control ID.

**Rationale**: Owner-draw guarantees a crisp circle + "i" at any DPI without depending on a specific font glyph being installed. Real `BUTTON` controls are naturally focusable (Tab) and fire `BN_CLICKED` on Space/Enter, so keyboard accessibility comes for free. The single shared tooltip + per-tool text is the standard Win32 idiom and minimizes window count.

**Alternatives considered**: Static control with the Unicode "info" code point (U+2139 ℹ or U+24D8 ⓘ) (rejected — font-availability fragile, not focusable). Segoe MDL2 Assets "Info" glyph U+E946 in a label (rejected — font/DPI fragility, not focusable). Custom small dialog modal popup instead of a tooltip (over-engineered, breaks Win32 conventions).

---

## R-010: Configuration dialog dynamic resize

**Decision**: Lay out the `.rc` template at the EXPANDED size (with the advanced controls visible). In `OnInitDialog`, after loading settings:
1. Record each advanced control's rect (via `GetWindowRect` + screen-to-client).
2. Compute `advancedBlockHeight` = (lowest advanced control's bottom) − (highest advanced control's top) + spacing.
3. If the user's last `ShowAdvancedGraphics` setting is off (the default), hide the advanced controls (`ShowWindow(SW_HIDE)`), shrink the dialog by `advancedBlockHeight` (`SetWindowPos`), and move the OK/Cancel/Reset row (and anything below the advanced block) up by the same amount.

On `BN_CLICKED` of `IDC_GRAPHICS_ADVANCED_CHECK`, run the inverse transform and persist the new setting. Use DPI-aware computation by sourcing the height from the actual control rects (which already scale with the dialog's font), not from a hardcoded pixel value.

**Rationale**: Working from the expanded layout in the `.rc` keeps the layout authority in the resource editor (and avoids fragile DPI math at runtime). Capturing the block height from actual control rects in `OnInitDialog` automatically picks up the correct DPI-scaled value. Moving the buttons (and any controls below the advanced block) by the same delta keeps the visual layout consistent in both states.

**Alternatives considered**: Two separate dialog templates (rejected — control IDs would need to differ, doubling event-handling code). Fixed taller dialog with reserved empty space (rejected per user — leaves an empty gap that looks broken). `SetWindowRgn` to clip out the advanced area (visual artifacts, doesn't reflow buttons).

---

## R-011: Tick-mark conventions for trackbars

**Decision**: Apply the following rules to all percentage trackbars in the dialog (recorded as a contract in `contracts/tick-mark-conventions.md`):
- **Discrete sliders** (Passes, Resolution, Smoothness): `TBS_AUTOTICKS` plus labeled ticks under each tick mark (achieved with `LTEXT` statics aligned at the tick screen-x positions in the dialog template).
- **Percentage sliders, common rule**: tick frequency chosen so a tick falls at the midpoint of the range and ~21 ticks total are produced. Concrete per-slider settings:

  | Slider          | Range   | Mid | `TBM_SETTICFREQ` | Ticks | Notes                              |
  |-----------------|---------|-----|------------------|-------|------------------------------------|
  | Density         | 0..100  | 50  | 5                | 21    | Exact midpoint tick                |
  | Speed           | 1..100  | —   | 5                | 21    | freq=5 → 20 ticks 1..96; one extra explicit tick at 100 via `TBM_SETTIC` |
  | Glow Intensity  | 0..200  | 100 | 10               | 21    | Exact midpoint tick                |
  | Glow Size       | 50..200 | 125 | 5                | 31    | freq=10 would miss 125; freq=5 keeps midpoint, accept denser tick count |

- All percentage sliders are unlabeled and non-snapping.

**Rationale**: A midpoint tick is a strong visual anchor and the user explicitly requested it. The 21-tick target keeps visual density consistent; Glow Size is the outlier because its range (150) doesn't divide cleanly. Speed needs one explicit tick because its range (1..100) doesn't permit an integer midpoint and `TBM_SETTICFREQ` would otherwise stop at 96.

**Alternatives considered**: Widening Glow Size range to 50..250 to fit the 21-tick rule (rejected — invents range purely to satisfy tick math; alters user-visible setting space for no functional reason). freq=10 on Glow Size yielding 16 ticks but no midpoint tick (rejected per user requirement). Per-slider TBM_SETTIC arrays everywhere (over-engineered for sliders that already work with `TBS_AUTOTICKS`).

---

## R-012: Test strategy

**Decision**: Extend the existing pure-helper + InMemoryProvider test pattern (already used by `MonitorEnumeratorTests`, `MonitorLayoutTests`, `IRenderSystemTests`, `RenderThreadInputsTests`). New unit tests target the pure helpers introduced by this feature; Win32 UI and DXGI device-creation paths are covered by manual QA per the success criteria, since they are out of TDD scope per the constitution's exemption.

**Rationale**: Reuses the proven test architecture (354 tests passing on master). Every meaningful behavior introduced by this feature is decomposable into a pure function that is exhaustively testable without a real device, monitor, or window. The Win32 wiring layer is thin glue; manual QA on real hybrid hardware is the highest-value remaining verification.

**Alternatives considered**: WTL/CppWinUI integration tests for the dialog (excessive scope). Hooking `D3D11CreateDevice` for adapter-routing tests (fragile, low ROI).

---

## R-013: Build/test environment quirks (carried over from prior work)

**Decision**: No new tooling. Documented constraints:
- Build: `MSBuild.exe /p:Configuration=Debug /p:Platform=x64 MatrixRain.sln` (no `/m` — transient PCH `C3859`/`C1076` memory failures).
- Test: `vstest.console.exe MatrixRainTests.dll /Platform:x64 /InIsolation`.
- `Version.h` is auto-bumped pre-build; exclude from commits.
- Commit messages: PowerShell has no heredoc — write to a temp file, `git commit -F`, remove. Conventional Commits + `Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>` trailer.
- `/W4 /WX /sdl`: C4189 (unused variable) and C4100 (unreferenced parameter) break builds; use `UNREFERENCED_PARAMETER` or remove the binding.

**Rationale**: Carried forward from the prior multimon work that ships on this base; no change.

---

## Open items

None. All NEEDS CLARIFICATION items were resolved in the pre-spec planning conversation.
