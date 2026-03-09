# 003-runtime-help: Implementation Context

## Branch & Status
- Branch: `003-runtime-help` (MatrixRain repo)
- Build: 0 warnings, 0 errors (/W4 /WX)
- Tests: 318 passing (Debug x64)
- Speckit artifacts: all updated as of 2026-03-07

## Architecture: ScrambleRevealEffect Model

The original spec described per-character random resolve/dissolve (each char has its own scramble timer, settles independently). This was **replaced** with a scramble-reveal model:

- **ScrambleRevealEffect** = shared per-cell timing oracle (no character data ownership)
- Per-cell staggered reveal with random lock times within revealDuration
- Per-cell random dismiss unlock times within dismissDuration
- All non-space cells simultaneously cycle through random glyphs at cycleInterval, then each independently "locks" onto its target character at its random lock time
- Dismiss reverses: cells randomly unlock back into cycling, then fade out
- All three overlays (HelpHintOverlay, HotkeyOverlay, UsageDialog) delegate all timing to ScrambleRevealEffect

## Key Initialization Parameters

| Overlay | CellCount | RevealDur | DismissDur | CycleInt | FlashDur | Hold | MarginCols |
|---------|-----------|-----------|------------|----------|----------|------|------------|
| HelpHint | N | 1.5 | 1.0 | 0.045 | 0.15 | 3.0 | 1 |
| Hotkey   | N | 1.5 | 1.0 | 0.045 | 0.15 | -1.0 (∞) | 2 |
| UsageDialog | N | 1.8 | 1.0 | 0.065 | 1.0 | -1.0 (∞) | N/A |

## CellPhase / CharPhase Enum
- 5 values: `Hidden`, `Cycling`, `LockFlash`, `Settled`, `Dismissing`
- CellPhase (in ScrambleRevealEffect) controls per-cell state
- CharPhase (in overlay HintCharacter) mirrors CellPhase

## Color Model: ComputeScrambleColor
- **Cycling**: dark green (0, 0.33, 0)
- **LockFlash**: yellow (0.8, 1.0, 0.3) decaying to mid green (0, 0.6, 0) over flashDuration
- **Settled pre-pulse**: mid green (0, 0.6, 0)
- **Settled pulse**: ramp to white (0.8s quadratic), hold white (1.5s), ramp to grey 0.75 (1.0s quadratic)
- **Dismissing**: dark green (0, 0.33, 0) — opacity handles fade-out

## randomGlyphIndex
- Deterministic from position: `(charIndex * 7 + 13) % m_allGlyphs.size()`
- Assigned once on first reveal (Hidden → Cycling transition)
- Margin columns and spaces participate in reveal (use SIZE_MAX as currentGlyphIndex sentinel for invisible characters)

## Visual Bugs Fixed (prior session)
1. Right-edge stuck column — head didn't travel far enough, fixed with extended travel distance
2. No dismiss matrix effect — added dismiss phase showing random glyphs before fade
3. Cycling zone too short — changed to full overlay width for extended matrix effect

## Rendering Details (RenderSystem.cpp)
- `RenderHelpHintOverlay()` and `RenderHotkeyOverlay()` are separate functions
- Two-pass rendering: Pass 1 = dark glow halos (8 directional offsets × GLOW_LAYERS), Pass 2 = foreground text
- Color driven by ComputeScrambleColor: CellPhase determines color transitions (dark green → yellow flash → mid green → white pulse → grey)
- Characters with `currentGlyphIndex >= allGlyphs.size()` (including SIZE_MAX) are skipped
- Codepoints > 0xFFFF use surrogate pairs
- `edgePadding = max(0, PADDING - MARGIN_COLS * CHAR_WIDTH)` — margin cols eat into visual padding

## Open Work: Phase 8 Visual Tuning (T113–T118)
- T113: Evaluate reveal speed and per-cell stagger (revealDuration, cycleInterval)
- T114: Evaluate margin column visual effect
- T115: Evaluate cycling random glyphs vs fixed random glyph
- T116: Evaluate dismiss visual quality (Settled → Dismissing → Hidden transitions)
- T117: Evaluate ComputeScrambleColor transitions (yellow flash, white pulse timing, grey level)
- T118: Evaluate margin column counts (1 for help, 2 for hotkey)

## Refactoring Needed
- `RenderSystem::RenderHelpHintOverlay` — needs refactor (large function with duplicated glyph→codepoint→surrogate logic in both passes, shared rendering patterns with `RenderHotkeyOverlay`)

## Key File Locations
- `MatrixRainCore/ScrambleRevealEffect.h/.cpp` — scramble-reveal timing oracle
- `MatrixRainCore/HelpHintOverlay.h/.cpp` — 3-line help hint overlay
- `MatrixRainCore/HotkeyOverlay.h/.cpp` — 9-row hotkey reference overlay
- `MatrixRainCore/RenderSystem.cpp` — RenderHelpHintOverlay (~L1578), RenderHotkeyOverlay
- `MatrixRainTests/unit/ScrambleRevealEffectTests.cpp` — scramble-reveal effect unit tests
- `MatrixRainTests/HelpHintOverlayTests.cpp` — overlay state machine tests
- `specs/003-runtime-help/` — all speckit artifacts (spec, plan, tasks, data-model, contracts, quickstart)
