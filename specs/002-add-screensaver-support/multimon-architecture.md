# MatrixRain Multi-Monitor Architecture

Branch: `feat/multimon-rendering` · Tasks: T045–T048

## Decisions (locked)
- **Per-output swap chains** (one per monitor), windowed flip-model.
- **Independent rain field per monitor** — own AnimationSystem/Viewport at native DPI.
- **One render thread per monitor**, each `Present(1)` → native per-monitor VSync.
- **Detection via Win32** (`EnumDisplayMonitors`/`GetMonitorInfo`/`GetDpiForMonitor`),
  source of truth. DXGI output→adapter mapping is optional/best-effort.
- **Density: per physical area** at native DPI (consistent glyph size/spacing on every display).
- Multimon applies whenever in the **Fullscreen display mode** — covers BOTH `/s`
  screensaver (always fullscreen) AND normal Alt+Enter fullscreen. Windowed =
  single primary window; `/p` preview = single child window. Single-monitor
  collapses to today's behavior (no regression).
- Fullscreen exit restores saved windowed placement; fallback = 3/4 on primary.
- Test seam: virtual **`IRenderSystem`** interface for spy-based orchestration tests.

## Component diagram

```
                          UI / MESSAGE THREAD
   ┌──────────────────────────────────────────────────────────────┐
   │  Application (coordinator)                                     │
   │   - MonitorEnumerator  (EnumDisplayMonitors → descriptors)     │
   │   - InputSystem / exit-state (centralized)                     │
   │   - Overlays + live ConfigDialog  (PRIMARY only)               │
   │   - owns N MonitorRenderContext, builds/tears down on          │
   │     mode transitions (barrier: quiesce→rebuild→resume)         │
   │                                                                │
   │   WindowProc(hwnd) ── GWLP_USERDATA ──► owning context         │
   │     WM_SIZE / WM_DPICHANGED / WM_DISPLAYCHANGE / input         │
   │            │ enqueue (no render-resource mutation here)        │
   └────────────┼───────────────────────────────────────────────────┘
                │  per-context command queues + shared UI state (mutex)
                ▼
   ┌───────────────────────┐   ┌───────────────────────┐   ┌─────────────┐
   │ MonitorRenderContext 0│   │ MonitorRenderContext 1│   │   ...  N-1   │
   │  (PRIMARY)            │   │  (secondary)          │   │             │
   │ ─ HWND @ rcMonitor    │   │ ─ HWND @ rcMonitor    │   │             │
   │ ─ RenderSystem        │   │ ─ RenderSystem        │   │             │
   │    (own device +      │   │    (own device +      │   │             │
   │     swapchain + RT +  │   │     swapchain + RT +  │   │             │
   │     bloom + D2D)      │   │     bloom + D2D)      │   │             │
   │ ─ AnimationSystem     │   │ ─ AnimationSystem     │   │             │
   │ ─ Viewport/Density    │   │ ─ Viewport/Density    │   │             │
   │ ─ render thread:      │   │ ─ render thread:      │   │             │
   │   drain cmds;         │   │   drain cmds;         │   │             │
   │   snapshot shared;    │   │   snapshot shared;    │   │             │
   │   Update; Render;     │   │   Update; Render;     │   │             │
   │   [overlays];         │   │   Present(1)          │   │             │
   │   Present(1)          │   │                       │   │             │
   └──────────┬────────────┘   └──────────┬────────────┘   └──────┬──────┘
              ▼                           ▼                        ▼
          Monitor 0                   Monitor 1                Monitor N-1
        (native refresh)            (native refresh)         (native refresh)
```

## Per-frame flow (each render thread, independently)
1. Drain this context's command queue (resize / DPI / displaychange / close).
2. Snapshot shared UI state (density/speed/glow/color) under mutex.
3. `Update(dt)` — dt from this thread's own clock.
4. `Render()` — rain; PRIMARY also renders overlays/stats/dialog (overlay mutex).
5. `Present(1)` — blocks on THIS monitor's VBlank (paces this thread only).

## Mode transition (barrier)
Alt+Enter / enter-exit `/s`, handled once on the focused window:
1. Signal all render threads to pause; join/quiesce.
2. Release old contexts (each frees its device/swapchain/RTs).
3. Re-enumerate monitors; create new windows + contexts.
4. Publish context list; start render threads.
Avoids ad-hoc mutation mid-frame.

### Windowed placement save/restore
- Entering fullscreen: SAVE current windowed placement (monitor + size + pos)
  before rebuilding into N contexts.
- Exiting fullscreen: RESTORE that saved placement (same monitor/size/pos).
- No saved placement (launched into fullscreen or `/s`): fall back to centered
  on primary at **3/4** of monitor size (matches existing windowed default;
  `Application.cpp` uses `* 3 / 4`).

## Threading ownership rules
- A context's RenderSystem is touched ONLY by its render thread.
- UI thread: window create/destroy + message receipt + **enqueue** events.
- Shared UI state: read-mostly, mutex-guarded; broadcast reaches all contexts.
- Single exit flag (set by centralized input handler) observed by all threads;
  UI loop posts quit when exit requested. `/s`: any window's input → exit.

## Robustness
- `Present` checks `DXGI_ERROR_DEVICE_REMOVED`/`_RESET` → coordinated rebuild-all.
- `WM_DISPLAYCHANGE`: `/s` exits cleanly; normal mode re-enumerates (full dynamic
  hotplug otherwise out of scope for v1).

## Testability (T045/T047) — orchestration, not GPU pixels
With MonitorEnumerator (fake descriptors) + `IRenderSystem` spy:
- N descriptors → N contexts/windows, correct bounds/size/DPI/primary flag.
- Each context's render system: Render()+Present() once per frame.
- Only PRIMARY context receives overlay params.
- Hotkey/shared state broadcasts to all contexts.
- Single-monitor input → all (one) contexts stop.
Actual on-screen rain per monitor = manual QA (dual-display checklist).

## Build order
1. MonitorEnumerator + unit tests.
2. IRenderSystem interface (extract from RenderSystem) + spy.
3. MonitorRenderContext (window + render loop body + command queue).
4. Application coordinator: build/teardown N contexts; per-thread lifecycle.
5. Mode-transition barrier (windowed ↔ N-monitor fullscreen; `/s`).
6. T045/T047 orchestration tests.
7. Manual multi-monitor QA + quickstart checklist.

---

# Continuation guide (resume on a multi-monitor machine)

Branch: `feat/multimon-rendering` (pushed to origin). Base: `f61c0fc`.

## Status: code-complete + unit-tested, NOT yet run on real multi-monitor hardware

All development and testing so far happened on a SINGLE-monitor box, so every
run only exercised the single-monitor collapse path. The N>1 fan-out is verified
only at the logic level (orchestration unit tests), never on screen. The whole
point of resuming on your multimon machine is to run the manual QA below.

## What's implemented (commit map)

- `fa6dc54` Win32 monitor enumeration (`IMonitorProvider`/`WindowsMonitorProvider`/
  `InMemoryMonitorProvider` + `MonitorInfo`). Win32 is the source of truth (NOT DXGI).
- `97d60c2` `IRenderSystem` seam + `SpyRenderSystem`; `RenderParams` promoted to top level.
- `8657db6` Per-device glyph atlas (`GlyphAtlas` owns the 4 GPU resources; each
  device builds its own via `RenderSystem::BuildGlyphAtlas()`).
- `750753d` `MonitorRenderContext` — per-monitor pipeline (own D3D device/swapchain
  + AnimationSystem/Viewport/DensityController/FPSCounter + render thread). HWND is
  UI-thread-owned; the context only observes it (never `DestroyWindow`).
- `aa5c6c1` Application holds `std::vector<MonitorRenderContext>` + raw `m_primary`
  + `m_monitorProvider` (plumbing; still 1 context).
- `94beb78` Fan-out: `CreateFullscreenContexts` (one window+context per monitor),
  `ShouldSpanAllMonitors` (Fullscreen only; excludes `/p` preview and `/?`),
  `RebuildContextsForCurrentMode` barrier, Alt+Enter + config-dialog toggle route
  through it (dialog toggle deferred via `WM_APP_REBUILD_CONTEXTS`).
- `251951d` FIX: seed the viewport size in `MonitorRenderContext::Initialize`
  (the window-creation `WM_SIZE` is dropped because the context isn't registered
  yet; without this the viewport was 0x0 → near-zero invisible streaks).
- `c49db85` Orchestration tests: pure `MonitorLayout::PlanFullscreenPlacements`
  and `RenderThreadInputs::MakeRenderThreadInputs` extracted from Application;
  17 GPU-free tests.

## Build / test / run (exact commands + gotchas)

NOTE: tool paths below are from the original dev box (VS 18 Enterprise). Adjust
to wherever Visual Studio / the test platform live on your multimon machine.

- Build the SOLUTION (not the test vcxproj alone — the core pre-build
  `IncrementVersion.ps1` uses a relative path that fails from another CWD):
  `MSBuild.exe MatrixRain.sln /p:Configuration=Debug /p:Platform=x64`
  - Do NOT pass `/m` (parallel) — it triggered transient PCH "Unable to commit
    memory" (C3859/C1076) failures. Single-process build is reliable.
  - `/W4` + warnings-as-errors + SDLCheck: unused locals (C4189) / unreferenced
    params (C4100) break the build.
- Tests (full suite = 354):
  `vstest.console.exe x64\Debug\MatrixRainTests.dll`
  Pipe through `Select-String "Total tests|Passed:|Failed:"`. The
  `/Logger:console;verbosity=...` form breaks on PowerShell semicolon parsing.
- Run modes: `MatrixRain.exe` (windowed), `MatrixRain.exe /s` (screensaver
  fullscreen, multimon), Alt+Enter toggles windowed<->fullscreen, `/p <hwnd>`
  preview (single child), `/c` config, `/?` usage.
- `Version.h` is auto-bumped by the pre-build step — leave it OUT of commits.

## Manual dual-monitor QA checklist (the remaining todo)

Run each on a 2+ monitor setup; ideally include monitors at DIFFERENT DPI/scaling.

1. `MatrixRain.exe /s` → rain fills EVERY monitor edge-to-edge, no gaps/black bars.
2. Each monitor's rain is independent (streaks don't cross bezels) and paced to
   that monitor's refresh (no stutter on a mixed-refresh setup).
3. Color cycling is in SYNC across all monitors (shared clock via SharedState).
4. Density/speed/glow changes (hotkeys + live config dialog) apply to ALL monitors.
5. Overlays (help hint, hotkey `?`, usage) + the config dialog appear on the
   PRIMARY monitor only.
6. `/s` exit-on-input: moving the mouse / key press on ANY monitor exits cleanly.
7. Normal mode → Alt+Enter → fullscreen spans all monitors; Alt+Enter again →
   back to a single windowed window on the primary, no crash, no orphaned windows.
8. Open the config dialog, toggle "start fullscreen" → rebuild happens without the
   dialog orphaning/crashing (deferred via `WM_APP_REBUILD_CONTEXTS`).
9. **HIGHEST-RISK UNKNOWN — secondary-window z-order:** confirm secondary monitors
   aren't hidden behind the taskbar / other windows. Windows are created exStyle 0
   (NOT `WS_EX_TOPMOST`) to match the working single-window behavior. If secondary
   windows sit behind other windows, add `WS_EX_TOPMOST` in
   `Application::CreateWindowAtBounds` (currently passes `0` for dwExStyle).
10. Per-monitor DPI: drag the window across a DPI boundary (windowed) and confirm
    `WM_DPICHANGED` rescales; in fullscreen each window stays pinned to its monitor
    (we only apply the DPI-suggested rect in Windowed mode).
11. Unplug/replug a monitor while running: v1 only guarantees `/s` exits cleanly on
    `WM_DISPLAYCHANGE`; full dynamic hotplug in normal mode is out of scope.

## Deliberate v1 deferrals (not bugs)

- Automated "Render()/Present() once per context per frame" coverage — needs a
  render-system factory/injection seam into `MonitorRenderContext` (it currently
  `make_unique<RenderSystem>()` directly). Deferred to the manual QA above.
- UI->render command queue with coalesced resize/DPI (architecture §"Cross-thread
  safety"). Current code mutates via the per-context render mutex + transition
  barrier instead; fine for v1.
- Full dynamic hotplug (`WM_DISPLAYCHANGE` rebuild in normal mode).
- Optional `HMONITOR -> IDXGIOutput` adapter mapping (perf optimization only).

## Key code locations

- `MatrixRainCore/Application.cpp`: `CreateRenderContexts` / `CreateSingleContext`
  / `CreateFullscreenContexts` / `ShouldSpanAllMonitors` / `AddContext` /
  `CreateWindowAtBounds` / `RebuildContextsForCurrentMode` / `StartRenderThreads` /
  `StopRenderThreads` / `ContextForHwnd` / `HandleMessage` (WM routing by HWND) /
  `OnSysKeyDown` (Alt+Enter) / `OnDpiChanged`.
- `MatrixRainCore/MonitorRenderContext.*`: per-monitor pipeline + render thread.
- `MatrixRainCore/MonitorLayout.*` + `RenderThreadInputs.*`: pure, tested coordinator logic.
- Tests: `MatrixRainTests/unit/MonitorLayoutTests.cpp`,
  `RenderThreadInputsTests.cpp`, `MonitorEnumeratorTests.cpp`, `IRenderSystemTests.cpp`.
