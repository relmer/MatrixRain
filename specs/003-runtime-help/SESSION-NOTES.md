# 003-runtime-help: Implementation Context

## Branch & Status
- Branch: `003-runtime-help` (MatrixRain repo)
- Build: 0 warnings, 0 errors (/W4 /WX)
- Tests: 318 passing (Debug x64)
- Speckit artifacts: all updated as of 2026-03-07

## Architecture: TextSweepEffect Sweep Model

The original spec described per-character random resolve/dissolve (each char has its own scramble timer, settles independently). This was **replaced** with a horizontal sweep model:

- **TextSweepEffect** = shared per-row timing oracle (no character data ownership)
- Left-to-right reveal sweep, right-to-left dismiss sweep
- Per-row staggered random delays and durations
- Streak zone = full overlay width (`m_cols`) — characters IN the streak show random glyph, characters BEHIND it show target glyph
- Head travels `m_cols - 1 + streakLen` total distance so right-edge characters settle correctly
- Both overlays delegate all timing to TextSweepEffect

## Key Initialization Parameters

| Overlay | Rows | Stagger | DurMin | DurMax | FadeIn | Hold | GlowDecay | MarginCols |
|---------|------|---------|--------|--------|--------|------|-----------|------------|
| HelpHint | 3 | 1.2 | 1.0 | 2.0 | 2.5 | 3.0 | 1.5 | 1 |
| Hotkey   | 9 | 1.2 | 1.0 | 2.0 | 2.5 | -1.0 (∞) | 1.5 | 2 |

## CharPhase Enum (simplified)
- `Resolved` and `Hidden` only (down from 5 values: Scrambling, Resolved, DissolveCycling, DissolveFading, Hidden)

## randomGlyphIndex
- Deterministic from position: `(charIndex * 7 + 13) % m_allGlyphs.size()`
- Assigned once on first reveal (Hidden → Resolved transition)
- Margin columns and spaces participate in sweep (use SIZE_MAX as currentGlyphIndex sentinel for invisible characters)

## Visual Bugs Fixed (prior session)
1. Right-edge stuck column — head didn't travel far enough, fixed with `m_cols-1+streakLen` travel distance
2. No dismiss matrix effect — added right-to-left sweep in dismiss phase showing random glyphs before fade
3. Streak too short — changed to full overlay width (`m_cols`) instead of small fixed value

## Rendering Details (RenderSystem.cpp)
- `RenderHelpHintOverlay()` and `RenderHotkeyOverlay()` are separate functions
- Two-pass rendering: Pass 1 = dark glow halos (8 directional offsets × GLOW_LAYERS), Pass 2 = foreground text
- Green→white color lerp: `r = 1-glow, g = 1-0.2*glow, b = 1-glow` (glow=1 at sweep head → bright green, glow=0 → white)
- Characters with `currentGlyphIndex >= allGlyphs.size()` (including SIZE_MAX) are skipped
- Codepoints > 0xFFFF use surrogate pairs
- `edgePadding = max(0, PADDING - MARGIN_COLS * CHAR_WIDTH)` — margin cols eat into visual padding

## Open Work: Phase 8 Visual Tuning (T113–T118)
- T113: Evaluate sweep speed and per-row stagger range
- T114: Evaluate streak zone length (full width vs shorter)
- T115: Evaluate cycling random glyphs vs fixed random glyph in streak zone
- T116: Evaluate dismiss sweep visual quality
- T117: Evaluate green→white glow decay rate (currently 1.5)
- T118: Evaluate margin column counts (1 for help, 2 for hotkey)

## Refactoring Needed
- `RenderSystem::RenderHelpHintOverlay` — needs refactor (large function with duplicated glyph→codepoint→surrogate logic in both passes, shared rendering patterns with `RenderHotkeyOverlay`)

## Key File Locations
- `MatrixRainCore/TextSweepEffect.h/.cpp` — sweep timing oracle
- `MatrixRainCore/HelpHintOverlay.h/.cpp` — 3-line help hint overlay
- `MatrixRainCore/HotkeyOverlay.h/.cpp` — 9-row hotkey reference overlay
- `MatrixRainCore/RenderSystem.cpp` — RenderHelpHintOverlay (~L1578), RenderHotkeyOverlay
- `MatrixRainTests/TextSweepEffectTests.cpp` — sweep effect unit tests
- `MatrixRainTests/HelpHintOverlayTests.cpp` — overlay state machine tests
- `specs/003-runtime-help/` — all speckit artifacts (spec, plan, tasks, data-model, contracts, quickstart)
