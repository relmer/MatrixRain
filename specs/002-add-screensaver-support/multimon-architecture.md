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
