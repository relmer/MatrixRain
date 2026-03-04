````markdown
# API Contracts: HelpRainDialog

**Module**: `HelpRainDialog` (MatrixRainCore.lib)
**Date**: 2026-03-04

## Public Interface

### Construction

```
HelpRainDialog (usageText: const UsageText &)
```
- Takes a `UsageText` reference for text content (switches only — no hotkeys)
- Pre-computes character positions via `IDWriteTextLayout::HitTestTextPosition`
- Builds randomized reveal queue from all non-space character positions
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
- Enters a message loop that drives both Win32 message handling and the render loop
- Returns `S_OK` when the user dismisses the dialog (Enter, Escape, close button)
- Returns error HRESULT if D3D/D2D initialization fails
- Blocking call — returns only when the dialog is dismissed

### Queries

```
IsRevealComplete () → bool
```
- Returns `true` when all characters in the reveal queue have been locked in

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

The rain animation has two distinct phases, driven by two **independent streak pools**.

### Phase 1: Text Reveal

**Goal**: Usage text materializes through per-character rain streaks.

**Reveal queue mechanic**:
1. Pre-compute the (x, y) pixel position of every non-space character in the usage text using `IDWriteTextLayout::HitTestTextPosition`
2. Shuffle the list into a randomized queue
3. Each tick, dequeue the next character(s) — spawn a short reveal streak at that character's x-position, starting ~4 cells above its y-position
4. The streak falls downward. When the streak head reaches the character's y-position, the character **locks in** — rendered in its final color/intensity from that point forward
5. The streak continues ~3 cells below the lock-in point and fades out

**Reveal pool (Pool 1)**:
- Exactly N streaks total (one per non-space character)
- Queue drains at a fixed rate calibrated so the last character locks in at t ≈ 3 seconds (`kRevealDurationSeconds`)
- Each streak spawns at its target character's exact x-position
- This pool is completely independent — decorative streaks do not affect its timing

**Pacing**: The spawn rate is calculated as `N / kRevealDurationSeconds` characters per second. At 60fps with ~200 non-space characters, that's ~4 spawns per frame. Characters materialize scattered across the text like random raindrops.

### Phase 2: Background Rain

**Goal**: Continuous ambient rain throughout the entire window.

- After reveal completes, decorative streak density/speed **backs off** to lower values via tunable multipliers
- Rain fills the entire window — passes through the text area, not just around it
- Resolved text stays readable because the feathered dark glow creates a local contrast zone around each character
- The effect is text floating in rain, not text in a clear bubble

### Decorative Streaks (Pool 2)

**Present during both phases** — purely aesthetic rain across the entire window.

- Spawn at **random pixel x-positions** across the full window width (no column grid — same as the main app's rain where streaks overlap naturally)
- During Phase 1: use the **same** density/speed as the reveal pool for consistent visual intensity
- During Phase 2: density/speed backs off via `kPhase2DensityMultiplier` and `kPhase2SpeedMultiplier`
- Never lock in characters — purely visual
- Completely independent of the reveal pool

### Text Rendering

- **Resolved text color**: White or near-white for maximum legibility against the green rain
- **Feathered glow**: Each resolved text character uses a feathered dark glow effect (concentric shadow layers at decreasing opacity) — same technique as the main app's FPS counter overlay via `DrawFeatheredGlow`. This creates a dark "halo" that ensures text remains readable even when rain passes through the text area
- **Text font**: Proportional (Segoe UI) via DirectWrite, with explicit x-offsets for columnar alignment

### Rain Parameters

| Parameter         | Phase 1 (Reveal)        | Phase 2 (Background)    |
|-------------------|-------------------------|-------------------------|
| Reveal spawn rate | N / 3.0s (guaranteed)   | N/A (reveal complete)   |
| Decorative density| Same as reveal rate     | Reduced by `kPhase2DensityMultiplier` |
| Decorative speed  | Same as reveal streaks  | Slowed by `kPhase2SpeedMultiplier` |
| Trail length      | Short (2-4 chars)       | Standard (5-10 chars)   |
| Head color        | Bright green/white      | Bright green/white      |
| Trail color       | Dimming green           | Dimming green           |

### Tunable Constants

All timing/density parameters are centralized as named constants for easy adjustment:

```cpp
constexpr float  kRevealDurationSeconds    = 3.0f;    // Guaranteed reveal completion time
constexpr int    kStreakLeadCells           = 4;       // Cells above target before lock-in
constexpr int    kStreakTrailCells          = 3;       // Cells below target that fade out
constexpr float  kPhase2DensityMultiplier  = 0.15f;   // Phase 2 decorative density reduction
constexpr float  kPhase2SpeedMultiplier    = 1.5f;    // Phase 2 speed factor (>1 = slower)
```

## Render Loop (Internal)

The render loop runs inside the message loop using `PeekMessage` for non-blocking dispatch:

1. Process all pending Windows messages
2. If `WM_QUIT` received, break
3. Clear render target to black
4. Render all active decorative streaks (both phases — these render first, behind everything)
5. Render all active reveal streaks (Phase 1 only):
   - Head character: bright green/white random glyph
   - Trail: dimming green random glyphs behind head
6. Render resolved text characters with feathered glow — these draw last (on top of rain)
7. Present swap chain
8. Dequeue next reveal characters and spawn their streaks
9. Advance all streak positions, lock in newly-revealed characters
10. Transition to Phase 2 when reveal queue is exhausted and all reveal streaks have completed
11. Repeat until dismissed

## D3D/D2D Initialization (Internal)

Minimal GPU setup — no 3D geometry, no compute shaders, no bloom:
- `D3D11CreateDevice` with `D3D_DRIVER_TYPE_HARDWARE` + `D3D11_CREATE_DEVICE_BGRA_SUPPORT`
- `IDXGIFactory::CreateSwapChain` targeting the dialog's HWND
- `ID2D1Factory::CreateDxgiSurfaceRenderTarget` for 2D drawing
- `IDWriteFactory::CreateTextFormat` with Segoe UI (proportional, 14-16pt, DPI-scaled)
- `ID2D1SolidColorBrush` instances for text (white), rain head (bright green), trail (dim green), glow (black with variable opacity)

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
