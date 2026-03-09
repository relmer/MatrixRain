# Data Model: Runtime Help Overlay and Command-Line Help

**Feature**: 003-runtime-help
**Date**: 2026-03-02

## Entities

### 1. HelpHintOverlay

The GPU-rendered overlay showing the three-line help hint with scramble-reveal animation.

**Lifecycle**: Created once at application startup (Normal mode only). Persists for app lifetime (can be re-triggered). Not created in screensaver modes.

| Field | Type | Description |
|-------|------|-------------|
| `m_scramble` | `ScrambleRevealEffect` | Timing oracle for reveal/hold/dismiss animation |
| `m_chars` | `vector<HintCharacter>` | Flat array of all characters in the hint text (including margin columns) |
| `m_rows` | `int` | Number of text rows (3) |
| `m_cols` | `int` | Total columns including margins (`m_textCols + 2 * MARGIN_COLS`) |
| `m_textCols` | `int` | Number of text columns (max width of formatted text, excluding margins) |
| `m_boundingRect` | `D2D1_RECT_F` | Screen-space bounding rectangle for occlusion + feathered border |
| `m_textLines` | `vector<wstring>` | The three lines of hint text |
| `m_allGlyphs` | `vector<uint32_t>` | Glyph set for random glyph assignment (from CharacterConstants + extras) |

**State transitions** (managed by ScrambleRevealEffect):
```
Idle → Revealing → Holding → Dismissing → Done
 ↑                                          ↓
 └──────────────────────────────────────────┘  (re-trigger: Show() restarts)
```

**Layout constants**: `CHAR_WIDTH=16`, `CHAR_HEIGHT=28`, `PADDING=20`, `GLOW_LAYERS=4`, `MARGIN_COLS=1`

### 2. HintCharacter

Per-character animation state for overlay hints (used by both HelpHintOverlay and HotkeyOverlay).

| Field | Type | Description |
|-------|------|-------------|
| `targetGlyphIndex` | `size_t` | Index of the final intended character in the glyph set |
| `currentGlyphIndex` | `size_t` | Index of the currently displayed glyph (random during cycling, target otherwise, `SIZE_MAX` for invisible) |
| `randomGlyphIndex` | `size_t` | Fixed random glyph assigned once on first reveal (deterministic from position) |
| `phase` | `CharPhase` | Per-character phase (Hidden, Cycling, LockFlash, Settled, Dismissing) |
| `opacity` | `float` | Current opacity (0.0–1.0, driven by ScrambleRevealEffect) |
| `glowIntensity` | `float` | Legacy field — color is now computed by `ComputeScrambleColor` based on `CellPhase` |
| `colorR` | `float` | Red component (0.0–1.0, computed by `ComputeScrambleColor`) |
| `colorG` | `float` | Green component (0.0–1.0, computed by `ComputeScrambleColor`) |
| `colorB` | `float` | Blue component (0.0–1.0, computed by `ComputeScrambleColor`) |
| `row` | `int` | Row position in the character grid |
| `col` | `int` | Column position in the character grid (includes margin offset) |
| `isSpace` | `bool` | True if this position is a space (renders as invisible; not animated during reveal) |

**Notes**:
- Characters in the cycling phase display `randomGlyphIndex`
- Characters past the cycling phase display `targetGlyphIndex` (or `SIZE_MAX` if space)
- Margin columns always render as invisible (`SIZE_MAX`) when outside cycling zone

### 3. OverlayPhase (enum class)

| Value | Description |
|-------|-------------|
| `Hidden` | Overlay not visible; rain passes through normally |
| `Revealing` | Characters are cycling and locking into final glyphs |
| `Holding` | All characters resolved; message fully visible |
| `Dissolving` | Characters are cycling random glyphs and fading out |

### 4. CharPhase (enum class)

| Value | Description |
|-------|-------------|
| `Hidden` | Fully transparent, not rendered |
| `Cycling` | Displaying random glyph, cycling at regular intervals |
| `LockFlash` | Just locked into target glyph; yellow flash decaying to mid green |
| `Settled` | Target glyph displayed; pre-pulse mid green, then white pulse, then grey |
| `Dismissing` | Transitioning back to cycling random glyphs before fading |

### 5. UsageDialog

A custom graphical window that displays command-line switches (Options + Screensaver Options) with scramble-reveal animation and background matrix rain. Text is rendered in proportional font (Segoe UI) with per-character positioning via DirectWrite. Uses `ScrambleRevealEffect` — the same per-cell timing oracle as the overlays — so characters cycle through random glyphs and independently lock into their targets with yellow flash and white pulse. Background matrix rain (via `AnimationSystem`) fills the entire window. Used by `/?`/`-?` invocation only — the `?` key uses an in-app overlay instead.

**Lifecycle**: Created on demand when `/?`/`-?` is detected. Runs its own message loop and render loop. Destroyed when the user dismisses the dialog (Enter, Escape, close button). Application exits after dismissal.

| Field | Type | Description |
|-------|------|-------------|
| `m_scramble` | `ScrambleRevealEffect` | Timing oracle for scramble-reveal animation (same as overlays) |
| `m_characterPositions` | `vector<CharPosition>` | Pre-computed (x, y) pixel position for every non-space character (see sub-entity) |
| `m_revealedFlags` | `vector<float>` | Per-character reveal tracking |
| `m_animationPhase` | `AnimationPhase` | `Revealing` or `Background` — which animation phase is active |
| `m_phaseTimer` | `float` | Time elapsed in current phase (seconds) |
| `m_elapsedTime` | `float` | Total elapsed time (seconds) |
| `m_renderSystem` | `unique_ptr<RenderSystem>` | Rendering context for this dialog's window |
| `m_animationSystem` | `unique_ptr<AnimationSystem>` | Background matrix rain animation |
| `m_viewport` | `unique_ptr<Viewport>` | Viewport for this dialog's window |
| `m_densityController` | `unique_ptr<DensityController>` | Rain density control for background rain |

**Tunable constants**:
| Constant | Value | Description |
|----------|-------|-------------|
| `kRevealDuration` | `1.8f` | Duration for the scramble-reveal animation |
| `kCycleInterval` | `0.065f` | How fast characters cycle through random glyphs |
| `kFlashDuration` | `1.0f` | Duration of yellow lock flash |
| `kBgDensityMultiplier` | `0.15f` | Background rain density reduction in post-reveal phase |
| `kPhase2SpeedMultiplier` | `1.5f` | Background rain speed multiplier in post-reveal phase |

### 5a. CharPosition (sub-entity of UsageDialog)

Pre-computed pixel position for a single non-space character in the usage text.

| Field | Type | Description |
|-------|------|-------------|
| `charIndex` | `size_t` | Index into the original text string |
| `x` | `float` | Pixel x-position (from `HitTestTextPosition`) |
| `y` | `float` | Pixel y-position (from `HitTestTextPosition`) |
| `character` | `wchar_t` | The actual character to display when revealed |
| `randomCharacter` | `wchar_t` | Random character displayed during cycling phase |

### 6. UsageText

Shared text content and formatting for the graphical rain dialog (`/?`/`-?`). Builds formatted switch text grouped into Options and Screensaver Options sections. **Not used by the runtime help hint overlay** — that overlay has its own hardcoded 3-line layout (`Settings Enter / Help ? / Exit Esc`) managed by `HelpHintOverlay`. **Not used by the `?` key hotkey overlay** — that uses its own hotkey list.

| Field | Type | Description |
|-------|------|-------------|
| `appName` | `wstring` | Application name for header |
| `switchPrefix` | `wchar_t` | `/` or `-` based on how help was invoked |
| `switches` | `vector<SwitchEntry>` | All supported command-line switches |
| `formattedText` | `wstring` | Pre-formatted text with section headers and aligned columns |

### 7. SwitchEntry

| Field | Type | Description |
|-------|------|-------------|
| `switchChar` | `wchar_t` | The switch character (e.g., `s`, `p`, `c`, `a`, `?`) |
| `argument` | `wstring` | Optional argument description (e.g., `<HWND>` for `/p`) |
| `description` | `wstring` | Brief description of what the switch does |

### 8. HotkeyEntry

| Field | Type | Description |
|-------|------|-------------|
| `keyName` | `wstring` | Display name (e.g., `Space`, `Enter`, `Esc`) |
| `description` | `wstring` | Brief description of the hotkey action |

### 9. ScrambleRevealEffect

Shared per-cell scramble-reveal timing oracle. Manages reveal, hold, and dismiss phases with per-cell staggered timing. `HelpHintOverlay`, `HotkeyOverlay`, and `UsageDialog` delegate all timing to this class.

**Lifecycle**: Owned by each overlay/dialog instance. Initialized with cell count and timing parameters. Drives the overlay's entire animation lifecycle.

| Field | Type | Description |
|-------|------|-------------|
| `m_phase` | `ScramblePhase` | Current phase: Idle, Revealing, Holding, Dismissing, Done |
| `m_revealTimer` | `float` | Time elapsed during reveal phase |
| `m_dismissTimer` | `float` | Time elapsed during dismiss phase |
| `m_postRevealTimer` | `float` | Time elapsed since all cells settled (drives white pulse) |
| `m_holdStartTime` | `float` | Time when hold phase began |
| `m_cellCount` | `int` | Number of cells |
| `m_revealDuration` | `float` | Duration over which cells randomly lock (default 1.5) |
| `m_dismissDuration` | `float` | Duration over which cells randomly unlock and fade (default 1.0) |
| `m_cycleInterval` | `float` | Time between random glyph changes during cycling (default 0.045) |
| `m_flashDuration` | `float` | Duration of lock flash per cell (default 0.15) |
| `m_holdDuration` | `float` | Hold time before auto-dismiss (-1.0 = hold indefinitely) |
| `m_cells` | `vector<CellState>` | Per-cell animation state |
| `m_rng` | `mt19937` | Random number generator for lock/unlock time distribution |

**CellState struct**:

| Field | Type | Description |
|-------|------|-------------|
| `phase` | `CellPhase` | Per-cell phase: Hidden, Cycling, LockFlash, Settled, Dismissing |
| `lockTime` | `float` | Time at which cell locks (reveal) or unlocks (dismiss) |
| `flashTimer` | `float` | Countdown from lock flash to settled |
| `flashDuration` | `float` | Per-cell flash duration (capped so all cells finish by revealDuration) |
| `cycleTimer` | `float` | Accumulator for glyph cycling interval |
| `opacity` | `float` | 0 = invisible, 1 = fully visible |
| `needsCycle` | `bool` | True when consumer should pick a new random glyph |
| `isSpace` | `bool` | Spaces don't cycle or flash |

**ScramblePhase enum** (global animation phase):

| Value | Description |
|-------|-------------|
| `Idle` | Not started |
| `Revealing` | Scramble-reveal in progress — cells cycle and lock at random times |
| `Holding` | All cells settled, waiting for holdDuration or manual dismiss |
| `Dismissing` | Cells randomly unlock back into cycling, then fade out |
| `Done` | Dismiss complete, all cells hidden |

**CellPhase enum** (per-cell animation phase):

| Value | Description |
|-------|-------------|
| `Hidden` | Cell is invisible (pre-reveal or post-dismiss) |
| `Cycling` | Cell is cycling through random glyphs |
| `LockFlash` | Cell just locked onto target — brief yellow-green flash |
| `Settled` | Cell is showing its target glyph (mid green → white pulse → grey) |
| `Dismissing` | Cell is cycling again and fading out |

**Key queries**:
- `GetCell(index)` → `CellState` (per-cell animation state)
- `GetCellCount()` → `int` (number of cells)
- `GetPhase()` → `ScramblePhase` (global animation phase)
- `IsActive()` → `bool` (true if Revealing, Holding, or Dismissing)
- `IsRevealComplete()` → `bool` (true if all non-space cells have settled)
- `IsDismissComplete()` → `bool` (true if all non-space cells are hidden)
- `GetPostRevealTimer()` → `float` (time since all cells settled, drives white pulse)
- `GetRevealTimer()` → `float` (time elapsed in reveal phase)

**Key operations**:
- `Initialize(cellCount, revealDuration, dismissDuration, cycleInterval, flashDuration, holdDuration)` — configures timing parameters and allocates per-cell state
- `MarkSpace(index)` — flags a cell as a space so it stays hidden during reveal
- `StartReveal()` — begins reveal: all non-space cells start cycling, each gets a random lock time in [0, revealDuration]
- `StartDismiss()` — begins dismiss: settled cells get random unlock times, then cycle and fade out
- `Reset()` — returns to Idle with all timers and cells zeroed

**Initialization parameters** (current tuned values):
- HelpHintOverlay: `Initialize(cellCount, 1.5, 1.0, 0.045, 0.15, 3.0)` — 3s hold, default timing
- HotkeyOverlay: `Initialize(cellCount, 1.5, 1.0, 0.045, 0.15, -1.0)` — hold indefinitely (dismissed by keypress)
- UsageDialog: `Initialize(cellCount, 1.8, 1.0, 0.065, 1.0, -1.0)` — slower cycling, longer flash, hold indefinitely

## Relationships

```
UsageText ──────→ SwitchEntry (1:N)

ScrambleRevealEffect ←── HelpHintOverlay (1:1, timing oracle)
ScrambleRevealEffect ←── HotkeyOverlay   (1:1, timing oracle)
ScrambleRevealEffect ←── UsageDialog     (1:1, timing oracle)

HelpHintOverlay ──→ HintCharacter (1:N, flat array, includes margin columns)

HotkeyOverlay ──┬──→ HintCharacter (1:N, flat array, includes margin columns)
                └──→ HotkeyEntry (1:N, hotkey content)

UsageDialog ────┬──→ UsageText (uses for formatted text content)
                └──→ CharPosition (1:N, pre-computed character positions)
```

## Validation Rules

- `HintCharacter.opacity` must be in range [0.0, 1.0]
- `HintCharacter.targetGlyphIndex` must be a valid index into the overlay's glyph set
- `HintCharacter.currentGlyphIndex` must be a valid glyph index OR `SIZE_MAX` (invisible sentinel)
- `HintCharacter.randomGlyphIndex` must be a valid index into the overlay's glyph set
- `CellState.opacity` must be in range [0.0, 1.0]
- `ScrambleRevealEffect.m_cellCount` must match the consumer's character count
- `ScrambleRevealEffect` phase transitions must follow: Idle → Revealing → Holding → Dismissing → Done
- `GetCell(index)` index must be in range [0, cellCount)
- Margin columns (col < MARGIN_COLS or col >= textCols + MARGIN_COLS) must render as invisible when outside cycling zone
- `UsageDialog.m_characterPositions` must contain one entry per non-space character in the formatted text
- `UsageDialog.m_revealedFlags.size() == m_characterPositions.size()`
- `UsageDialog.m_animationPhase` transitions from `Revealing` to `Background` when all characters are revealed
- `CharPosition.x` and `CharPosition.y` must be within the dialog's client area
- `UsageText.switchPrefix` must be either `/` or `-`
- `OverlayPhase` maps from `ScramblePhase`: Revealing→Revealing, Holding→Holding, Dismissing→Dissolving, Idle/Done→Hidden
- Re-trigger (Show()) restarts ScrambleRevealEffect from Idle → Revealing
- Recognized hotkey (Dismiss()) calls ScrambleRevealEffect::StartDismiss() from Revealing/Holding (no-op if already Dismissing/Done)
