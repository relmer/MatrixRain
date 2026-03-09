````markdown
# API Contracts: UsageDialog

**Module**: `UsageDialog` (MatrixRainCore.lib)
**Date**: 2026-03-04 (updated 2026-03-06)

## Public Interface

### Construction

```
UsageDialog (usageText: const UsageText &)
```
- Takes a `UsageText` reference for text content (switches only — no hotkeys)
- Pre-computes character positions via `IDWriteTextLayout::HitTestTextPosition`
- Creates `ScrambleRevealEffect` for per-cell animation
- Does NOT create the window or start animation — call `Show()` to begin

### Display

```
Show () → HRESULT
```
- Creates a custom top-level window (non-resizable, with close button)
- Creates D3D11 device, swap chain, and D2D render target for this window
- Sizes the window using 2x text bounding box, capped at 80% of primary monitor work area per dimension
- Centers window on primary monitor
- Registers window class, creates window, shows it
- Initializes `ScrambleRevealEffect` with cell count from non-space characters, then calls `StartReveal()`
- Enters a message loop that drives both Win32 message handling and the render loop
- Returns `S_OK` when the user dismisses the dialog (Enter, Escape, close button)
- Returns error HRESULT if D3D/D2D initialization fails
- Blocking call — returns only when the dialog is dismissed

### Queries

```
IsRevealComplete () → bool
```
- Delegates to `ScrambleRevealEffect::IsRevealComplete()`
- Returns `true` when all cells have reached `Settled` phase

## Window Behavior

- **Style**: `WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU` — title bar, close button, non-resizable
- **Title**: Uses `UnicodeSymbols::EmDash` — "MatrixRain — Help"
- **Background**: Solid black
- **Close triggers**: Escape key, Enter key, close button (X), Alt+F4

## Content

The dialog displays **command-line switches only** — no hotkeys. Hotkeys are shown by the in-app `?` key overlay (US5) rendered directly on the main window.

```
MatrixRain v1.1.1001

Usage: MatrixRain.exe [/option]

Options:
  /c              Show settings dialog
  /?              Display this help message

Screensaver Options:
  /s              Run as full-screen screensaver
  /p <HWND>       Preview in the specified window
  /a              Password change (unsupported, exits quietly)
```

Switches are split into two sections: general **Options** and **Screensaver Options**. Each entry has a name and description rendered with explicit x-offsets for columnar alignment.

## Font and Text Layout

- **Font**: Proportional (Segoe UI), 14-16pt, DPI-scaled via DirectWrite
- **Column alignment**: Each switch entry stores name and description as separate fields. DirectWrite renders them at explicit x-offsets: measure the widest name in each section, add padding, and that becomes the description column's x-origin. This provides perfect vertical alignment regardless of proportional character widths.
- **Section headers**: "Options:" and "Screensaver Options:" rendered as single-column left-aligned text
- **Text block**: Horizontally and vertically centered in the window

## Animation Phases

The dialog uses `ScrambleRevealEffect` for a per-cell scramble-reveal animation with background matrix rain.

### Phase 1: Text Reveal (Scramble-Reveal)

**Goal**: Usage text materializes through per-cell scramble-reveal animation.

**Scramble-reveal mechanic**:
1. Each non-space character cell starts in `Hidden` phase
2. Cells transition to `Cycling` — displaying rapidly changing random glyphs in dark green
3. Each cell receives a random lock time within the reveal duration (1.8s)
4. When a cell's lock time arrives, it enters `LockFlash` — the final character appears with a yellow flash that decays to mid green
5. After the flash, the cell enters `Settled` — displaying the resolved character, pulsing from mid green through white to grey

**ScrambleRevealEffect initialization**:
```cpp
m_scramble.Initialize (cellCount, kRevealDuration, 1.0f, kCycleInterval, kFlashDuration, -1.0f)
```
- `cellCount` = total non-space characters
- `kRevealDuration = 1.8f` — all cells lock within this window
- `kCycleInterval = 0.065f` — random glyph change interval during cycling
- `kFlashDuration = 1.0f` — yellow→green flash duration at lock-in
- `holdDuration = -1.0f` — hold indefinitely (dialog stays until dismissed)

### Phase 2: Background Rain

**Goal**: Continuous ambient matrix rain throughout the entire window.

- Background rain runs throughout both phases via the main `AnimationSystem`
- After reveal completes, rain density is reduced via `kBgDensityMultiplier = 0.15f` and speed is increased via `kPhase2SpeedMultiplier = 1.5f`
- Rain fills the entire window — passes through the text area, not just around it
- Resolved text stays readable because the feathered dark glow creates a local contrast zone around each character
- The effect is text floating in rain, not text in a clear bubble

### Text Rendering

- **Resolved text color**: Driven by `ComputeScrambleColor` — transitions through dark green (cycling) → yellow flash → mid green → white pulse → grey (settled)
- **Feathered glow**: Each resolved text character uses a feathered dark glow effect (concentric shadow layers at decreasing opacity) — same technique as the main app's FPS counter overlay via `DrawFeatheredGlow`. This creates a dark "halo" that ensures text remains readable even when rain passes through the text area
- **Text font**: Proportional (Segoe UI) via DirectWrite, with explicit x-offsets for columnar alignment

### Tunable Constants

All timing/density parameters are centralized as named constants:

```cpp
constexpr float  kRevealDuration           = 1.8f;    // Time window for all cells to lock in
constexpr float  kCycleInterval            = 0.065f;  // Random glyph change interval
constexpr float  kFlashDuration            = 1.0f;    // Yellow→green flash at lock-in
constexpr float  kBgDensityMultiplier      = 0.15f;   // Phase 2 background rain density
constexpr float  kPhase2SpeedMultiplier    = 1.5f;    // Phase 2 rain speed factor
```

## Render Loop (Internal)

The render loop runs inside the message loop using `PeekMessage` for non-blocking dispatch:

1. Process all pending Windows messages
2. If `WM_QUIT` received, break
3. Call `ScrambleRevealEffect::Update(deltaTime)` to advance cell phases
4. Clear render target to black
5. Render background matrix rain (present during both phases)
6. For each non-space character, query `ScrambleRevealEffect::GetCell(index)`:
   - `Hidden` cells: not rendered
   - `Cycling` cells: render random glyph in dark green
   - `LockFlash` cells: render resolved glyph with yellow→green flash color
   - `Settled` cells: render resolved glyph with feathered glow, color from `ComputeScrambleColor`
7. Present swap chain
8. Transition to Phase 2 rain density when `IsRevealComplete()` returns true
9. Repeat until dismissed

## D3D/D2D Initialization (Internal)

Minimal GPU setup — no 3D geometry, no compute shaders, no bloom:
- `D3D11CreateDevice` with `D3D_DRIVER_TYPE_HARDWARE` + `D3D11_CREATE_DEVICE_BGRA_SUPPORT`
- `IDXGIFactory::CreateSwapChain` targeting the dialog's HWND
- `ID2D1Factory::CreateDxgiSurfaceRenderTarget` for 2D drawing
- `IDWriteFactory::CreateTextFormat` with Segoe UI (proportional, 14-16pt, DPI-scaled)
- `ID2D1SolidColorBrush` instances for text colors (per `ComputeScrambleColor`), rain (green), glow (black with variable opacity)

## Sizing (Internal)

```
static ComputeWindowSize (usageText: const UsageText &, dpi: float) → SIZE
```
- Measures text content bounding box using `IDWriteTextLayout::GetMetrics()`
- Multiplies by 2x to create padding around the centered text
- Caps each dimension independently at 80% of primary monitor work area
- Returns the window client area size

## Thread Safety

- For `/?` path: runs on main thread before main window creation. Single-threaded.
- No shared state with the main app's render pipeline — has its own D3D/D2D context.

## Unicode Symbols

All Unicode characters used in this module are defined in `UnicodeSymbols.h`:
- `UnicodeSymbols::EmDash` (`\u2014`) — used in the window title "MatrixRain — Help"

````
