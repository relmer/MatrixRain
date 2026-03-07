# Data Model: Runtime Help Overlay and Command-Line Help

**Feature**: 003-runtime-help
**Date**: 2026-03-02

## Entities

### 1. HelpHintOverlay

The GPU-rendered overlay showing the three-line help hint with horizontal sweep reveal/dismiss animation.

**Lifecycle**: Created once at application startup (Normal mode only). Persists for app lifetime (can be re-triggered). Not created in screensaver modes.

| Field | Type | Description |
|-------|------|-------------|
| `m_sweep` | `TextSweepEffect` | Per-row timing oracle for reveal/hold/dismiss sweep |
| `m_chars` | `vector<HintCharacter>` | Flat array of all characters in the hint text (including margin columns) |
| `m_rows` | `int` | Number of text rows (3) |
| `m_cols` | `int` | Total columns including margins (`m_textCols + 2 * MARGIN_COLS`) |
| `m_textCols` | `int` | Number of text columns (max width of formatted text, excluding margins) |
| `m_boundingRect` | `D2D1_RECT_F` | Screen-space bounding rectangle for occlusion + feathered border |
| `m_textLines` | `vector<wstring>` | The three lines of hint text |
| `m_allGlyphs` | `vector<uint32_t>` | Glyph set for random glyph assignment (from CharacterConstants + extras) |

**State transitions** (managed by TextSweepEffect):
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
| `currentGlyphIndex` | `size_t` | Index of the currently displayed glyph (random during sweep streak, target otherwise, `SIZE_MAX` for invisible) |
| `randomGlyphIndex` | `size_t` | Fixed random glyph assigned once on first reveal (deterministic from position) |
| `phase` | `CharPhase` | Per-character phase (Resolved or Hidden) |
| `opacity` | `float` | Current opacity (0.0–1.0, driven by TextSweepEffect::GetOpacity) |
| `glowIntensity` | `float` | Green→white transition (1.0 = bright green, 0.0 = settled white, driven by TextSweepEffect::GetGlowIntensity) |
| `row` | `int` | Row position in the character grid |
| `col` | `int` | Column position in the character grid (includes margin offset) |
| `isSpace` | `bool` | True if this position is a space (renders as invisible when not in streak zone) |

**Notes**:
- Characters in the sweep streak zone display `randomGlyphIndex`
- Characters past the streak zone display `targetGlyphIndex` (or `SIZE_MAX` if space)
- Margin columns always render as invisible (`SIZE_MAX`) when outside streak zone

### 3. OverlayPhase (enum class)

| Value | Description |
|-------|-------------|
| `Hidden` | Overlay not visible; rain passes through normally |
| `Revealing` | Characters are scrambling and resolving to final glyphs |
| `Holding` | All characters resolved; message fully visible |
| `Dissolving` | Characters are randomly cycling and fading out |

### 4. CharPhase (enum class)

| Value | Description |
|-------|-------------|
| `Resolved` | Character is visible (opacity > 0); displaying target glyph or random glyph depending on sweep position |
| `Hidden` | Fully transparent, not rendered |

### 5. HelpRainDialog

A custom graphical window with its own D3D11/D2D rendering context that displays command-line switches (Options + Screensaver Options) via a two-phase matrix rain animation. Text is rendered in proportional font (Segoe UI) with per-character positioning via DirectWrite. Phase 1 (reveal) spawns streaks from a randomized per-character queue to reveal text over ~3 seconds. Phase 2 (background) continues ambient decorative rain at reduced density/speed throughout the entire window. Rain fills the entire window — streaks pass through the text area, not just around it. Used by `/?`/`-?` invocation only — the `?` key uses an in-app overlay instead.

**Lifecycle**: Created on demand when `/?`/`-?` is detected. Runs its own message loop and render loop. Destroyed when the user dismisses the dialog (Enter, Escape, close button). Application exits after dismissal.

| Field | Type | Description |
|-------|------|-------------|
| `revealQueue` | `vector<size_t>` | Shuffled indices into `characterPositions` — drain order for reveal streaks |
| `revealQueueIndex` | `size_t` | Next index to spawn from the reveal queue |
| `characterPositions` | `vector<CharPosition>` | Pre-computed (x, y) pixel position for every non-space character (see sub-entity) |
| `revealedFlags` | `vector<bool>` | Per-character flag: true once the reveal streak has locked in this character |
| `activeRevealStreaks` | `vector<RevealStreak>` | Currently falling reveal streaks (see sub-entity) |
| `decorativeStreaks` | `vector<DecorativeStreak>` | Active decorative rain streaks (see sub-entity) |
| `animationPhase` | `enum` | `Revealing` or `Background` — which rain phase is active |
| `phaseTimer` | `float` | Time elapsed in current phase (seconds) |
| `textBoundingBox` | `D2D1_RECT_F` | Bounding box of the formatted text block (from DirectWrite layout) |
| `windowWidth` | `int` | Dialog window client area width in pixels |
| `windowHeight` | `int` | Dialog window client area height in pixels |
| `hWnd` | `HWND` | The dialog window handle |
| `device` | `ComPtr<ID3D11Device>` | D3D11 device for this dialog's rendering |
| `swapChain` | `ComPtr<IDXGISwapChain>` | Swap chain for this dialog's window |
| `d2dRenderTarget` | `ComPtr<ID2D1RenderTarget>` | D2D render target for text and rain drawing |
| `textFormat` | `ComPtr<IDWriteTextFormat>` | Proportional text format (Segoe UI, DPI-scaled) |
| `textLayout` | `ComPtr<IDWriteTextLayout>` | DirectWrite text layout for position computation |
| `textBrush` | `ComPtr<ID2D1SolidColorBrush>` | White/near-white brush for resolved usage text |
| `glowBrush` | `ComPtr<ID2D1SolidColorBrush>` | Black brush with variable opacity for feathered glow |
| `headBrush` | `ComPtr<ID2D1SolidColorBrush>` | Bright green/white brush for rain head characters |
| `trailBrush` | `ComPtr<ID2D1SolidColorBrush>` | Dimming green brush for rain trail characters |

### 5a. CharPosition (sub-entity of HelpRainDialog)

Pre-computed pixel position for a single non-space character in the usage text.

| Field | Type | Description |
|-------|------|-------------|
| `charIndex` | `size_t` | Index into the original text string |
| `x` | `float` | Pixel x-position (from `HitTestTextPosition`) |
| `y` | `float` | Pixel y-position (from `HitTestTextPosition`) |
| `character` | `wchar_t` | The actual character to display when revealed |

### 5b. RevealStreak (sub-entity of HelpRainDialog)

A short streak spawned at a specific character's x-position to reveal that character. Falls downward; when the head reaches the target y-position, the character locks in.

| Field | Type | Description |
|-------|------|-------------|
| `targetCharIndex` | `size_t` | Index into `characterPositions` — which character this streak reveals |
| `pixelX` | `float` | Pixel x-position (same as the target character's x) |
| `headPixelY` | `float` | Current pixel y-position of the streak head |
| `targetPixelY` | `float` | Target y-position where the character locks in |
| `speed` | `float` | Pixels per second |
| `leadCells` | `int` | Number of glyph cells above the head (default: `kStreakLeadCells = 4`) |
| `trailCells` | `int` | Number of dimming cells below the head (default: `kStreakTrailCells = 3`) |
| `glyphIndices` | `vector<size_t>` | Random glyph indices for head and trail characters |
| `revealed` | `bool` | True once the head has passed the target y-position |

### 5c. DecorativeStreak (sub-entity of HelpRainDialog)

An ambient rain streak for visual atmosphere. Spawns at random pixel x-positions (no column grid). During Phase 1, runs at the same density/speed as reveal streaks. During Phase 2, density and speed are reduced via tunable multipliers.

| Field | Type | Description |
|-------|------|-------------|
| `pixelX` | `float` | Random pixel x-position across the window width |
| `headPixelY` | `float` | Current pixel y-position of the streak head |
| `trailLength` | `int` | Number of trailing characters (5-10) |
| `speed` | `float` | Pixels per second |
| `glyphIndices` | `vector<size_t>` | Random glyph indices for head and trail characters |

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

### 9. TextSweepEffect

Shared per-row horizontal sweep timing oracle. Manages reveal (left-to-right), hold, and dismiss (right-to-left) phases with per-row staggered delays/durations. Both `HelpHintOverlay` and `HotkeyOverlay` delegate all timing to this class.

**Lifecycle**: Owned by each overlay instance. Initialized with row count and timing parameters. Drives the overlay's entire animation lifecycle.

| Field | Type | Description |
|-------|------|-------------|
| `m_phase` | `SweepPhase` | Current phase: Idle, Revealing, Holding, Dismissing, Done |
| `m_revealTimer` | `float` | Time elapsed during reveal phase |
| `m_holdStartTime` | `float` | Time when hold phase began |
| `m_dismissTimer` | `float` | Time elapsed during dismiss phase |
| `m_fadeInSpeed` | `float` | Speed of opacity fade-in during reveal |
| `m_holdDuration` | `float` | Hold time before auto-dismiss (-1.0 = hold indefinitely) |
| `m_glowDecaySpeed` | `float` | Rate at which glow decays from 1.0 (green) to 0.0 (white) |
| `m_rowCount` | `int` | Number of rows |
| `m_rowRevealDelays` | `vector<float>` | Per-row random delay before reveal starts |
| `m_rowRevealDurations` | `vector<float>` | Per-row random duration for reveal sweep |
| `m_rowDismissDelays` | `vector<float>` | Per-row random delay before dismiss starts |
| `m_rowDismissDurations` | `vector<float>` | Per-row random duration for dismiss sweep |
| `m_staggerRange` | `float` | Range of per-row delay randomization |
| `m_durationMin` | `float` | Minimum sweep duration per row |
| `m_durationMax` | `float` | Maximum sweep duration per row |

**SweepPhase enum**:

| Value | Description |
|-------|-------------|
| `Idle` | Not started |
| `Revealing` | Left-to-right sweep in progress |
| `Holding` | Reveal complete, waiting for holdDuration |
| `Dismissing` | Right-to-left sweep in progress |
| `Done` | Dismiss complete |

**Key queries**:
- `GetRevealProgress(row)` → 0..1 (how far the reveal sweep has progressed on this row)
- `GetDismissProgress(row)` → 0..1 (how far the dismiss sweep has progressed on this row)
- `GetOpacity(row, normCol)` → 0..1 (combined reveal fade-in and dismiss fade-out)
- `GetGlowIntensity(row, normCol)` → 0..1 (green→white decay since reveal)
- `IsPositionRevealed(row, normCol)` → bool
- `IsPositionDismissed(row, normCol)` → bool

**Initialization parameters** (current values):
- HelpHintOverlay: `Initialize(3, 1.2, 1.0, 2.0, 2.5, 3.0, 1.5)` — 3 rows, 3s hold
- HotkeyOverlay: `Initialize(9, 1.2, 1.0, 2.0, 2.5, -1.0, 1.5)` — 9 rows, hold indefinitely (dismissed by keypress)

## Relationships

```
UsageText ──────→ SwitchEntry (1:N)

TextSweepEffect ←── HelpHintOverlay (1:1, timing oracle)
TextSweepEffect ←── HotkeyOverlay   (1:1, timing oracle)

HelpHintOverlay ──→ HintCharacter (1:N, flat array, includes margin columns)

HotkeyOverlay ──┬──→ HintCharacter (1:N, flat array, includes margin columns)
                └──→ HotkeyEntry (1:N, hotkey content)

HelpRainDialog ──┬──→ UsageText (uses for formatted text content)
                 ├──→ CharPosition (1:N, pre-computed character positions)
                 ├──→ RevealStreak (1:N, Phase 1 reveal streaks)
                 └──→ DecorativeStreak (1:N, ambient rain)
```

## Validation Rules

- `HintCharacter.opacity` must be in range [0.0, 1.0]
- `HintCharacter.glowIntensity` must be in range [0.0, 1.0]
- `HintCharacter.targetGlyphIndex` must be a valid index into the overlay's glyph set
- `HintCharacter.currentGlyphIndex` must be a valid glyph index OR `SIZE_MAX` (invisible sentinel)
- `HintCharacter.randomGlyphIndex` must be a valid index into the overlay's glyph set
- `TextSweepEffect.m_rowCount` must match the overlay's row count
- `TextSweepEffect` phase transitions must follow: Idle → Revealing → Holding → Dismissing → Done
- `GetRevealProgress(row)` and `GetDismissProgress(row)` must return values in [0.0, 1.0]
- `GetOpacity(row, normCol)` must return values in [0.0, 1.0]
- Margin columns (col < MARGIN_COLS or col >= textCols + MARGIN_COLS) must render as invisible when outside sweep streak zone
- `HelpRainDialog.characterPositions` must contain one entry per non-space character in the formatted text
- `HelpRainDialog.revealQueue` must be a permutation of indices [0, characterPositions.size())
- `HelpRainDialog.revealedFlags.size() == characterPositions.size()`
- `HelpRainDialog.animationPhase` transitions from `Revealing` to `Background` when all characters in `revealedFlags` are true
- `CharPosition.x` and `CharPosition.y` must be within the dialog's client area
- `RevealStreak.targetPixelY` must match the corresponding `CharPosition.y`
- `RevealStreak.pixelX` must match the corresponding `CharPosition.x`
- `DecorativeStreak.pixelX` must be in range [0, windowWidth]
- `DecorativeStreak.speed` must be positive
- `DecorativeStreak.trailLength` must be in range [5, 10]
- `UsageText.switchPrefix` must be either `/` or `-`
- `OverlayPhase` maps from `SweepPhase`: Revealing→Revealing, Holding→Holding, Dismissing→Dissolving, Idle/Done→Hidden
- Re-trigger (Show()) restarts TextSweepEffect from Idle → Revealing
- Recognized hotkey (Dismiss()) calls TextSweepEffect::StartDismiss() from Revealing/Holding (no-op if already Dismissing/Done)
