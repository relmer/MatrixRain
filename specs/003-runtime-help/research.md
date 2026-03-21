# Research: Runtime Help Overlay and Command-Line Help

**Feature**: 003-runtime-help
**Date**: 2026-03-02

## R1: ~~ANSI VT Console Output for Console Rain Effect~~ (SUPERSEDED)

**Status**: SUPERSEDED — Console output from a GUI subsystem app is fundamentally broken. Shell doesn't wait for `/SUBSYSTEM:WINDOWS` apps, causing prompt rendering mid-output, cursor positioning failures, and `DISABLE_NEWLINE_AUTO_RETURN` leaking to the parent terminal. Replaced by R8 (Graphical Rain Dialog).

~~**Decision**: Use ANSI VT escape sequences via `SetConsoleMode` with `ENABLE_VIRTUAL_TERMINAL_PROCESSING` for all console-based matrix rain rendering.~~

## R2: ~~Interactive Console Detection~~ (SUPERSEDED)

**Status**: SUPERSEDED — No longer needed. The graphical rain dialog renders GPU-based matrix rain regardless of launch context. No console detection, no pipe detection.

~~**Decision**: Use `GetConsoleMode` as the primary detection method for interactive console vs pipe/file redirect.~~

## R3: ~~Console Attachment for GUI Subsystem App~~ (SUPERSEDED)

**Status**: SUPERSEDED — `AttachConsole(ATTACH_PARENT_PROCESS)` is fundamentally broken for GUI subsystem apps. The shell returns immediately after launching a `/SUBSYSTEM:WINDOWS` app, so any console output races with the shell prompt. The new approach uses a custom graphical dialog window (R8) that works identically regardless of launch context (terminal, double-click, shortcut).

~~**Decision**: Use `AttachConsole (ATTACH_PARENT_PROCESS)` to reuse the launching terminal.~~

## R4: ~~Ctrl+C Signal Handling During Console Rain~~ (SUPERSEDED)

**Status**: SUPERSEDED — No console rain effect exists. The graphical rain dialog uses standard Win32 window message handling (WM_CLOSE, WM_KEYDOWN for Escape/Enter) for dismissal.

~~**Decision**: Use `SetConsoleCtrlHandler` with `std::atomic<bool>` flag.~~

## R5: D2D Text Layout for Centered Overlay

**Decision**: Use `IDWriteTextLayout` to measure help hint text bounds, then position the D2D draw origin to center the text block on screen.

**Rationale**: DirectWrite's `IDWriteTextLayout` provides precise text metrics (width, height) needed for centering. The existing `RenderFPSCounter` already uses `IDWriteTextFormat` with `DrawText`, so extending to use `CreateTextLayout` for measurement is a natural progression.

**Alternatives considered**:
- Hardcode pixel offsets based on known font size: Fragile across DPI scales and font changes. Rejected.
- Use GDI `GetTextExtentPoint32`: Would require a GDI device context — the app is D2D-only. Rejected.

**Key details**:
- Create `IDWriteTextLayout` from the help text using existing `IDWriteFactory::CreateTextLayout`
- Call `GetMetrics()` to get `DWRITE_TEXT_METRICS` (width, height, line count)
- Center: `x = (viewportWidth - metrics.width) / 2`, `y = (viewportHeight - metrics.height) / 2`
- For the two-column layout (right-justified labels, left-justified keys), use two separate text layouts or a single layout with per-character positioning
- Re-compute position on window resize (FR-013)

## R6: Overlay Per-Character Animation State

**Decision**: Implement per-character animation as flat parallel arrays (glyph index, opacity, phase, timer) indexed by position in the text grid. Phase transitions managed by a simple state machine.

**Rationale**: Flat arrays are cache-friendly (Constitution II: Performance-First) and trivially testable without rendering (Constitution I: TDD). The state machine pattern maps naturally to the spec's reveal → hold → dissolve lifecycle.

**Alternatives considered**:
- Per-character objects with virtual dispatch: Heap-heavy, poor cache locality for small state. Rejected.
- Single animation curve applied uniformly: Can't achieve per-character stagger. Rejected.

**Key details**:
- Cell phase enum (`CellPhase`): `Hidden`, `Cycling`, `LockFlash`, `Settled`, `Dismissing`
- Each cell has: phase, lockTime, flashTimer, flashDuration, cycleTimer, opacity, needsCycle, isSpace
- Overlay-level phase (`ScramblePhase`): `Idle`, `Revealing`, `Holding`, `Dismissing`, `Done`
- Reveal: all cells start `Hidden`, transition to `Cycling` (random glyphs), then `LockFlash` (yellow flash at lock time), then `Settled` (resolved character)
- Dismiss: cells transition `Settled` → `Dismissing` (random glyphs, fading) → `Hidden`
- Color driven by `ComputeScrambleColor`: Cycling=dark green, LockFlash=yellow→mid green, Settled=mid green→white pulse→grey, Dismissing=dark green
- Update logic is pure (no rendering calls) — fully unit-testable

## R7: ~~Console Rain Character Grid and Column Mapping~~ (SUPERSEDED)

**Status**: SUPERSEDED — The column activity mapping and monospace text grid concepts are no longer used. The new design uses proportional font (Segoe UI) with per-character positioning via `IDWriteTextLayout::HitTestTextPosition`. Rain streaks spawn at random pixel x-positions (no column grid). Characters are revealed via a randomized per-character queue, not column-based reveal.

~~**Decision**: Pre-compute the usage text as a 2D grid (rows × columns), scan each column for any non-space character to build a column activity bitmap, then animate only active columns.~~

**Key concepts still applicable**:
- Character set for random glyphs: from `CharacterConstants` (half-width katakana, Latin, numerals)
- Pre-computing text layout before animation start
- Separation of text formatting from animation logic (independently testable)

## R8: Graphical Rain Dialog for Help Display

**Decision**: Replace console-based help output with a custom graphical dialog window (`UsageDialog`) that has its own D3D11/D2D rendering context and displays command-line switches via a two-phase GPU-rendered scramble-reveal animation with proportional font and per-cell reveal driven by `ScrambleRevealEffect`.

**Rationale**: Console output from a `/SUBSYSTEM:WINDOWS` app is fundamentally broken — the shell doesn't wait for the process, causing output to race with the next shell prompt. A graphical dialog avoids all console issues, provides a visually impressive presentation consistent with the application's identity, and works identically regardless of launch context (terminal, double-click, shortcut, etc.).

**Alternatives considered**:
- MessageBox: Simple and reliable, but loses the visual identity of the application. Used temporarily as an interim solution during development. Rejected as final approach.
- Console subsystem with GUI window creation: Would make the shell wait, but causes a console flash on double-click launch and changes the app's fundamental subsystem. Rejected.
- `start /wait` wrapper script: Requires shipping a separate script and user knowledge. Rejected.
- Monospace font with column-based reveal: Original approach — used column activity mapping and per-column reveal streaks. Replaced by proportional font with per-character queued reveal for better visual quality.

**Key details**:

*Content:*
- Command-line switches only — Options (`/c`, `/?`) and Screensaver Options (`/s`, `/p`, `/a`)
- No hotkeys in the `/?` dialog (hotkeys are shown by the `?` key as an in-app overlay)
- Section headers: "Options:" and "Screensaver Options:"

*Font and text layout:*
- Proportional font: Segoe UI, 14-16pt, DPI-scaled via `IDWriteTextFormat`
- Per-character positioning: `IDWriteTextLayout::HitTestTextPosition` computes (x, y) for every character
- Columnar alignment for switch names and descriptions uses explicit x-offsets, not character grid cells
- Text columns flow naturally with proportional metrics — no padding to uniform width

*Window and GPU setup:*
- `UsageDialog` creates a standalone top-level window (not a child of the main app window)
- Window has `WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU` style — non-resizable, with close button
- Background is solid black
- The window creates its own `ID3D11Device`, `IDXGISwapChain`, `ID2D1RenderTarget` — minimal setup, no 3D geometry, no compute shaders, no bloom
- Window sizing: 2x text bounding box (from `IDWriteTextLayout::GetMetrics()`), capped at 80% of primary monitor work area per dimension. Text block centered in window.
- High-DPI: use per-monitor DPI awareness (already enabled for the main app)

*Two-phase animation with ScrambleRevealEffect:*
- **Phase 1 — Scramble-reveal**: Each non-space character cell starts `Hidden`, transitions to `Cycling` (random glyphs in dark green), receives a random lock time within `kRevealDuration = 1.8f`. At lock time, cell enters `LockFlash` (yellow flash decaying to mid green over `kFlashDuration = 1.0f`), then `Settled` (resolved character, pulsing mid green → white → grey via `ComputeScrambleColor`). `holdDuration = -1.0f` (hold indefinitely until dialog dismissed).
- **Background rain**: Runs independently throughout both phases via `AnimationSystem`. During Phase 1, normal density. After reveal completes, density reduced via `kBgDensityMultiplier = 0.15f` and speed increased via `kPhase2SpeedMultiplier = 1.5f`.
- **Phase transition**: Phase 1 → Phase 2 when `ScrambleRevealEffect::IsRevealComplete()` returns true (all cells `Settled`).
- **Rain fills the entire window** — passes through the text area, not just around it. Text is "floating in rain, not in a clear bubble."

*Text rendering:*
- Resolved text is rendered with color driven by `ComputeScrambleColor` (mid green → white pulse → grey for settled cells)
- Each resolved character gets a feathered dark glow effect — concentric shadow layers at decreasing opacity (same technique as the main app's `DrawFeatheredGlow` for the FPS counter overlay). This creates a local contrast zone ensuring readability even when rain passes through the text area.

*Dialog lifecycle:*
- Dialog runs its own message loop (`PeekMessage`-based for render-on-idle) — blocks the calling thread until dismissed
- Used by `/?` only — dismiss exits the process
- The `?` key uses a separate in-app overlay on the main window (NOT `UsageDialog`)
- Dismiss triggers: Enter, Escape, close button (X), Alt+F4

*Tunable constants:*
- `kRevealDuration = 1.8f` — time window for all cells to lock in
- `kCycleInterval = 0.065f` — random glyph change interval during cycling
- `kFlashDuration = 1.0f` — yellow→green flash duration at lock-in
- `kBgDensityMultiplier = 0.15f` — background rain density reduction in Phase 2
- `kPhase2SpeedMultiplier = 1.5f` — rain speed factor in Phase 2
