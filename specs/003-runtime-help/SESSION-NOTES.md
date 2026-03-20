# 003-runtime-help: Implementation Context

## Branch & Status
- Branch: `003-runtime-help` (MatrixRain repo)
- Build: 0 warnings, 0 errors (/W4 /WX)
- Tests: 315 passing (Debug ARM64)
- Speckit artifacts: all updated as of 2026-03-19

## Architecture: ScrambleRevealEffect Model

The original spec described per-character random resolve/dissolve (each char has its own scramble timer, settles independently). This was **replaced** with a scramble-reveal model:

- **ScrambleRevealEffect** = shared per-cell timing oracle (no character data ownership)
- Per-cell staggered reveal with random lock times within revealDuration
- Per-cell random dismiss unlock times within dismissDuration
- All non-space cells simultaneously cycle through random glyphs at cycleInterval, then each independently "locks" onto its target character at its random lock time
- Dismiss reverses: cells randomly unlock back into cycling, then fade out
- All three overlays (HelpHintOverlay, HotkeyOverlay, UsageDialog) delegate all timing to ScrambleRevealEffect

## Key Initialization Parameters

| Overlay | RevealDur | DismissDur | CycleInt | FlashDur | Hold | MarginCols |
|---------|-----------|------------|----------|----------|------|------------|
| HelpHint | 2.5 | 1.0 | 0.25 | 1.0 | 2.7 | 1 |
| Hotkey   | 2.5 | 1.0 | 0.25 | 1.0 | 5.4 | 2 |
| UsageDialog | 1.8 | 0.0 | 0.065 | 1.0 | -1.0 (∞) | N/A (D2D — pending migration) |

## CellPhase / CharPhase Enum
- 5 values: `Hidden`, `Cycling`, `LockFlash`, `Settled`, `Dismissing`
- CellPhase (in ScrambleRevealEffect) controls per-cell state
- CharPhase (in overlay HintCharacter) mirrors CellPhase

## Color Model: ComputeScrambleColor
- **Cycling**: 70% scheme color intensity
- **LockFlash**: white decaying to full scheme color over flashDuration
- **Settled pre-pulse**: full scheme color
- **Settled pulse**: ramp to white (0.33s quadratic), hold white (0.75s), ramp to grey 0.75 (0.5s quadratic)
- **Dismissing**: 70% scheme color intensity — opacity handles fade-out

## randomGlyphIndex
- Deterministic from position: `(charIndex * 7 + 13) % m_allGlyphs.size()`
- Assigned once on first reveal (Hidden → Cycling transition)
- Margin columns and spaces participate in reveal (use SIZE_MAX as currentGlyphIndex sentinel for invisible characters)

## Visual Bugs Fixed (prior session)
1. Right-edge stuck column — head didn't travel far enough, fixed with extended travel distance
2. No dismiss matrix effect — added dismiss phase showing random glyphs before fade
3. Cycling zone too short — changed to full overlay width for extended matrix effect

## Rendering Details (RenderSystem.cpp)
- Overlays render to scene texture BEFORE bloom — bloom provides free glow
- `RenderTwoColumnOverlay()` is the shared entry point for both HelpHint and Hotkey overlays
- Per-row SDF halo backgrounds via D3D11 fullscreen pixel shader (signed distance to rounded rects)
- Halo row rects cached per overlay, invalidated on resize/DPI change
- GPU instancing for overlay characters (one foreground instance per visible char)
- `RenderParams` struct replaces old 12-parameter `Render()` signature
- `Render()` renders rain → overlays → bloom → FPS counter
- `BuildOverlayInstances` creates instance data; `RenderOverlayInstances` uploads and draws
- Proportional font rendering via Segoe UI overlay atlas with per-character advance widths
- `ComputeOverlayLayout` handles two-column centering and proportional positioning
- Dead code removed: pOcclusionRect, m_pointSamplerState, glow layers, OVERLAY_MAX_GLOW_ALPHA

## Density Defaults
- Default density: 50% (was 80%)
- Streak multiplier: 2.4x (was 4.0x) — new 50% = old 30%
- `ScreenSaverSettings::DEFAULT_DENSITY_PERCENT = 50`
- `DensityController::DEFAULT_PERCENTAGE = 50`

## Open Work: UsageDialog GPU Migration

**Decision**: Replace UsageDialog's separate window + duplicate rendering stack with a special mode of the main Application window.

**Approach**:
- `/? ` becomes a mode flag on Application (new ScreenSaverMode or similar)
- Window sizes to 2x text bounding box (capped at 80% work area) instead of fullscreen
- New `UsageOverlay` class (modeled on `HotkeyOverlay`) builds HintCharacter arrays from UsageText content
- Rendered via existing GPU instancing + SDF halo + bloom pipeline
- Rain density held at 0% until overlay reveal completes, then background rain starts
- Only Enter/Esc dismiss (exits app) — no other hotkeys
- Help hint overlay not shown in this mode

**What gets deleted**:
- `UsageDialog` class entirely (UsageDialog.h, UsageDialog.cpp)
- `CharPosition` struct
- `DrawCharacterGlow` D2D per-character glow rendering
- `RenderTextOverlay` D2D text rendering
- Duplicate RenderSystem/AnimationSystem/CharacterSet initialization
- D2D text brushes (m_textBrush, m_glowBrush, m_tracerBrush)

**What gets created**:
- `UsageOverlay` class (like HotkeyOverlay but with UsageText content)
- Application mode handling for /?

**Atlas expansion** (DONE — committed):
- Added: . ( ) [ ] : © — to overlay codepoints
- Total glyphs: 281 (was 273)

**Key insight**: HotkeyOverlay already renders proportional Segoe UI text via GPU instancing. The overlay atlas has all needed glyphs. UsageOverlay is structurally identical — just different text content.

## Config Dialog Cancel Bug (FIXED)
- `OnCancel()` called `CancelLiveMode()` before `DestroyWindow()`, clearing isLiveMode flag
- `OnDestroy()` then skipped `SetConfigDialog(nullptr)` because `IsLiveMode()` returned false
- Result: `m_hConfigDialog` permanently pointed to destroyed HWND, blocking Enter and Escape
- Fix: clear dialog handle in `OnCancel()` before `DestroyWindow()` while Application pointer is accessible

## Key File Locations
- `MatrixRainCore/ScrambleRevealEffect.h/.cpp` — scramble-reveal timing oracle
- `MatrixRainCore/HelpHintOverlay.h/.cpp` — 3-line help hint overlay
- `MatrixRainCore/HotkeyOverlay.h/.cpp` — 9-row hotkey reference overlay
- `MatrixRainCore/RenderSystem.cpp` — RenderHelpHintOverlay (~L1578), RenderHotkeyOverlay
- `MatrixRainTests/unit/ScrambleRevealEffectTests.cpp` — scramble-reveal effect unit tests
- `MatrixRainTests/HelpHintOverlayTests.cpp` — overlay state machine tests
- `specs/003-runtime-help/` — all speckit artifacts (spec, plan, tasks, data-model, contracts, quickstart)
