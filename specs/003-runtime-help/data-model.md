# Data Model: Runtime Help Overlay and Command-Line Help

**Feature**: 003-runtime-help
**Date**: 2026-03-02

## Entities

### 1. HelpHintOverlay

The GPU-rendered overlay showing the three-line help hint with matrix rain reveal/dissolve animation.

**Lifecycle**: Created once at application startup (Normal mode only). Persists for app lifetime (can be re-triggered). Not created in screensaver modes.

| Field | Type | Description |
|-------|------|-------------|
| `phase` | `OverlayPhase` | Current overlay-level phase |
| `phaseTimer` | `float` | Time elapsed in current phase (seconds) |
| `holdDuration` | `float` | How long to hold at full visibility before dissolve |
| `characters` | `vector<HintCharacter>` | Flat array of all characters in the hint text |
| `rows` | `int` | Number of text rows (3) |
| `cols` | `int` | Number of text columns (max width of formatted text) |
| `boundingRect` | `D2D1_RECT_F` | Screen-space bounding rectangle for occlusion + feathered border |
| `isActive` | `bool` | Whether the overlay is currently visible or animating |

**State transitions**:
```
Hidden → Revealing → Holding → Dissolving → Hidden
  ↑         ↓           ↓          ↓
  └─────────┴───────────┴──────────┘  (re-trigger: unrecognized key)
            ↓           ↓          ↓
            └───────────┴──────────┘  (recognized hotkey: jump to Dissolving)
```

### 2. HintCharacter

Per-character animation state for the overlay hint.

| Field | Type | Description |
|-------|------|-------------|
| `targetGlyphIndex` | `size_t` | Index of the final intended character in the character set |
| `currentGlyphIndex` | `size_t` | Index of the currently displayed character (random during scramble) |
| `phase` | `CharPhase` | Per-character animation phase |
| `opacity` | `float` | Current opacity (1.0 = fully visible, 0.0 = invisible) |
| `scrambleTimer` | `float` | Time remaining in scramble/cycle phase |
| `scrambleInterval` | `float` | Time between glyph changes during scramble |
| `dissolveStartOffset` | `float` | Random delay before this character begins dissolving |
| `row` | `int` | Row position in the hint grid |
| `col` | `int` | Column position in the hint grid |
| `isSpace` | `bool` | True if this position is a space (not rendered, no scramble) |

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
| `Scrambling` | Cycling through random glyphs before reveal |
| `Resolved` | Displaying the final target character at full opacity |
| `DissolveCycling` | Re-cycling through random glyphs at full brightness |
| `DissolveFading` | Cycling glyphs with decreasing opacity |
| `Hidden` | Fully transparent, no longer rendered |

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

## Relationships

```
UsageText ──────→ SwitchEntry (1:N)

HelpHintOverlay ──→ HintCharacter (1:N, flat array)

HelpRainDialog ──┬──→ UsageText (uses for formatted text content)
                 ├──→ CharPosition (1:N, pre-computed character positions)
                 ├──→ RevealStreak (1:N, Phase 1 reveal streaks)
                 └──→ DecorativeStreak (1:N, ambient rain)
```

## Validation Rules

- `HintCharacter.opacity` must be in range [0.0, 1.0]
- `HintCharacter.targetGlyphIndex` must be a valid index into `CharacterConstants::GetAllCodepoints()`
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
- `OverlayPhase` transitions must follow the state machine (no skipping from Hidden to Holding)
- Re-trigger (unrecognized key) from Revealing or Holding resets to `Revealing` (all chars back to Scrambling)
- Re-trigger from Dissolving reverses the dissolve — partially faded characters materialize back toward `Resolved`, fully `Hidden` characters restart from `Scrambling`
- Recognized hotkey transitions to `Dissolving` from `Revealing` or `Holding` (no-op if already `Dissolving`)
