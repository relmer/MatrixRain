# API Contracts: HelpHintOverlay

**Module**: `HelpHintOverlay` (MatrixRainCore.lib)
**Date**: 2026-03-02 (updated 2026-03-06)

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
- Initializes TextSweepEffect with per-row staggered timing
- Sets all characters to `Hidden`, assigns `targetGlyphIndex` from text content
- Assigns `randomGlyphIndex` once per character (deterministic from position)
- Sets overlay phase to `Revealing`
- If already visible (any phase), restarts from `Revealing` (FR-010)

```
Dismiss () → void
```
- Triggers the dismiss sweep (FR-014: recognized hotkey behavior)
- If in `Revealing` or `Holding`: calls `TextSweepEffect::StartDismiss()`
- If in `Dismissing`: no-op (does not restart or accelerate)
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
- Advances TextSweepEffect and updates per-character state based on sweep progress
- Called once per render frame from the render thread
- Two-pass per-character update:
  - **Pass 1 — glyph selection**: For each character, compute sweep head position from `GetRevealProgress(row)`. Characters ahead of the streak show `Hidden`. Characters in the streak zone show their `randomGlyphIndex`. Characters behind the streak show their `targetGlyphIndex` (resolved).
  - **Pass 2 — opacity and glow**: For each character, compute `opacity` from `GetOpacity(row, normCol)` and `glowIntensity` from `GetGlowIntensity(row, normCol)`. Glow drives green→white color transition.
- During dismiss: `GetDismissProgress(row)` drives a right-to-left sweep that reverses characters back to `randomGlyphIndex` then `Hidden`
- Phase transitions derived from `TextSweepEffect::GetSweepPhase()`
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
- Returns current overlay-level phase (mapped from `SweepPhase`)

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
- Characters include margin columns (invisible, `currentGlyphIndex == SIZE_MAX`)

```
GetAllGlyphs () → span<const wchar_t>
```
- Returns the full glyph palette used for random glyph selection
- Used by render system for character rendering

```
GetRows () → int
GetCols () → int
```
- Grid dimensions for the render system (includes margin columns on each side)

### Layout

```
UpdateLayout (viewportWidth: float, viewportHeight: float) → void
```
- Recomputes centering position based on viewport dimensions (FR-013)
- Called on window resize and on `Show()`
- Does not reset animation state

## Key Constants

| Constant | Value | Purpose |
|----------|-------|---------|
| `MARGIN_COLS` | 1 | Invisible columns on each side for full-width sweep |
| `CHAR_WIDTH` | 16.0f | Character cell width in pixels |
| `CHAR_HEIGHT` | 28.0f | Character cell height in pixels |
| `PADDING` | 20.0f | Padding around the text area |
| `GLOW_LAYERS` | 4 | Number of glow layers for the feathered border |

## TextSweepEffect Initialization

```
m_sweep.Initialize (3, 1.2f, 1.0f, 2.0f, 2.5f, 3.0f, 1.5f)
```
- 3 rows, 1.2s stagger range, 1.0–2.0s per-row duration
- 2.5 fade-in speed, 3.0s hold duration, 1.5 glow decay speed

## Thread Safety

- `Update()` and all queries called from render thread under `m_renderMutex`
- `Show()` and `Dismiss()` called from main thread (input handling)
- Shared state protected by the existing `m_renderMutex` in Application
