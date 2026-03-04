# API Contracts: HelpHintOverlay

**Module**: `HelpHintOverlay` (MatrixRainCore.lib)
**Date**: 2026-03-02

## Public Interface

### Construction

```
HelpHintOverlay ()
```
- Initializes all characters to `Hidden` phase, overlay phase to `Hidden`
- Does NOT start the animation — call `Show()` to begin

### State Control

```
Show () → void
```
- Starts or restarts the reveal animation from the beginning
- Resets all per-character states to `Scrambling` with staggered timers
- Sets overlay phase to `Revealing`
- If already visible (any phase), resets to `Revealing` (FR-010)

```
Dismiss () → void
```
- Triggers the dissolve phase (FR-014: recognized hotkey behavior)
- If in `Revealing` or `Holding`: transitions to `Dissolving`
- If in `Dissolving`: no-op (does not restart or accelerate)
- If in `Hidden`: no-op

```
Hide () → void
```
- Immediately hides the overlay (sets phase to `Hidden`)
- Used for cleanup only, not for normal dismiss flow

### Update

```
Update (deltaTime: float) → void
```
- Advances all per-character animations based on elapsed time
- Called once per render frame from the render thread
- Handles phase transitions: `Revealing` → `Holding` → `Dissolving` → `Hidden`
- During `Revealing`: cycles glyph indices for scrambling characters, resolves characters whose timer expires
- During `Holding`: counts down hold timer
- During `Dissolving`: manages per-character dissolve cycling → fading → hidden
- Pure state mutation — no rendering calls

### Queries

```
IsActive () → bool
```
- Returns `true` if overlay phase is not `Hidden`
- Used by render system to decide whether to draw the overlay

```
GetPhase () → OverlayPhase
```
- Returns current overlay-level phase

```
GetBoundingRect () → D2D1_RECT_F
```
- Returns the screen-space bounding rectangle of the overlay
- Used by rain occlusion logic (FR-012) and feathered border rendering (FR-011)

```
GetCharacters () → span<const HintCharacter>
```
- Returns read-only view of all per-character states
- Used by render system for drawing

```
GetRows () → int
GetCols () → int
```
- Grid dimensions for the render system

### Layout

```
UpdateLayout (viewportWidth: float, viewportHeight: float) → void
```
- Recomputes centering position based on viewport dimensions (FR-013)
- Called on window resize and on `Show()`
- Does not reset animation state

## Thread Safety

- `Update()` and all queries called from render thread under `m_renderMutex`
- `Show()` and `Dismiss()` called from main thread (input handling)
- Shared state protected by the existing `m_renderMutex` in Application
