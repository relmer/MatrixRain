# Feature Specification: Runtime Help Overlay and Command-Line Help

**Feature Branch**: `003-runtime-help`  
**Created**: 2026-03-02  
**Status**: In Progress  
**Input**: User description: "Add command-line help display (/?/-?) that dumps all supported switches, similar to TCDir. When MatrixRain starts in normal mode, show a centered three-line help hint (Settings/Enter, ?/Help, Esc/Exit) using a matrix rain character reveal effect — characters cycle through random glyphs before settling on the final character. Re-show on unrecognized key press. Do not show in any screensaver mode. Pressing Enter opens the config dialog; pressing ? shows a usage dialog listing command-line switches."

## Clarifications

### Session 2026-03-02

- Q: Should recognized hotkeys (Space, C, S, +/-, etc.) dismiss the help hint, and if so how? → A: All recognized hotkeys dismiss the hint via the rain dissolve effect (not instantly).
- Q: Should the spec formally declare out-of-scope items? → A: No — recreational project, not worried about scope creep.

### Session 2026-03-03

- Q: Console output from a GUI subsystem app — workable? → A: No. `AttachConsole(ATTACH_PARENT_PROCESS)` is fundamentally broken for `/SUBSYSTEM:WINDOWS` apps: the shell doesn't wait, the prompt renders mid-output, cursor positioning fails, and `DISABLE_NEWLINE_AUTO_RETURN` leaks to the parent terminal. The entire console output approach is abandoned.
- Q: What replaces the console rain effect for `/?`? → A: A custom graphical dialog window with its own D3D/D2D rendering that displays matrix rain revealing the usage text — the same visual effect, but rendered via GPU in a standalone window instead of ANSI codes in the parent terminal.
- Q: Should the `?` key usage dialog also use the graphical rain dialog? → A: No — the `?` key renders hotkey information directly on the main window as an in-app overlay. `/?` uses `UsageDialog` for command-line switches.

### Session 2026-03-04

- Q: Should the dialog use a monospace or proportional font? → A: Proportional (Segoe UI). Text columns are aligned via explicit x-offsets from DirectWrite, not character grid cells. The rain reveal uses per-character positioning via `HitTestTextPosition` with a randomized queue — no column grid needed.
- Q: Should hotkeys be in the `/?` dialog? → A: No. The `/?` dialog shows command-line switches only (Options + Screensaver Options). Hotkeys are shown by the `?` key as an in-app overlay rendered directly on the main MatrixRain window.
- Q: Should the `?` key open a dialog? → A: No. The `?` key renders hotkey information directly on the main window as an in-app overlay, not in a separate dialog. This is distinct from `/?` which uses `UsageDialog`.
- Q: How does the usage dialog reveal work with proportional font? → A: Per-character. Pre-compute (x, y) for every non-space character. Each character cycles through random glyphs and locks into its target at a random time within the reveal duration. The same scramble-reveal model used by the overlays.
- Q: Dialog sizing? → A: 2x text bounding box, capped at 80% of primary monitor work area per dimension. Text block centered in window.
- Q: Should `/c` be in the Screensaver Options section? → A: No. `/c` (show settings dialog) stays in general Options. Only `/s`, `/p`, `/a` are in Screensaver Options.
- Q: Unicode symbols? → A: All Unicode characters go in `UnicodeSymbols.h` with named constants — same pattern as TCDir.

### Session 2026-03-06 — Scramble-Reveal Animation Redesign

- The original per-character random resolve/dissolve model was replaced during implementation with a scramble-reveal model.
- **Reveal**: Characters appear as cycling random glyphs that lock into their target glyphs with per-cell staggered timing. A yellow flash marks each lock, followed by a white pulse once all cells have settled.
- **Dismiss**: Characters transition back to cycling random glyphs and fade out with staggered timing.
- **ScrambleRevealEffect**: Shared timing oracle used by `HelpHintOverlay`, `HotkeyOverlay`, and `UsageDialog`. Manages per-cell staggered timing, reveal/hold/dismiss phase transitions, and opacity. Overlays query per-cell state to drive character animation.
- **Margin columns**: Invisible columns added on each side (`MARGIN_COLS=1` for HelpHintOverlay, `MARGIN_COLS=2` for HotkeyOverlay) so the reveal enters/exits the visible area smoothly rather than starting/ending abruptly at the first/last text character.
- **Random glyph assignment**: Each character is assigned one random glyph index on first reveal (deterministic from position). Characters in the cycling phase display this fixed random glyph.
- **CharPhase**: Maps from `CellPhase` — Hidden, Cycling, LockFlash, Settled, Dismissing. The scramble-reveal timing oracle manages all per-cell state.
- **Color sequence**: Driven by `ComputeScrambleColor` based on `CellPhase` and the active rain color scheme (including color cycling mode):
  - Cycling: 70% scheme color intensity
  - LockFlash: white decaying to full scheme color over flash duration
  - Settled pre-pulse: full scheme color
  - Pulse ramp up (0.33s): scheme color → white (quadratic ease-in)
  - Pulse hold (0.75s): white (1, 1, 1)
  - Pulse ramp down (0.5s): white → grey (0.75, 0.75, 0.75) (quadratic ease-out)
  - Dismissing: 70% scheme color intensity
- **Known visual tuning areas (in progress)**:
  - Reveal speed and per-cell timing distribution
  - Flash duration (yellow lock flash)
  - Dismiss timing relative to fade-out opacity

### Session 2026-03-07 through 2026-03-18 — Rendering & Input Overhaul

- **DPI awareness**: Added per-monitor DPI awareness (`WM_DPICHANGED`). Overlay atlas recreated at current DPI scale for 1:1 texel-to-pixel mapping. Character scale in constant buffer adjusted by `m_dpiScale`. Reference viewport height (1080) is DPI-adjusted.
- **Bloom/glow system**: Added multi-pass Gaussian blur (3 passes of 13-tap horizontal + vertical). Bloom extract shader uses `smoothstep(0.1, 0.6, brightness)` with max-component metric. Composite shader uses exponential soft-saturation `1 - exp(-bloom * intensity)` to prevent dense areas from clipping. Glow size slider wired to blur shader `glowSize` uniform.
- **Proportional font rendering**: Overlay text switched from monospace grid to proportional positioning. Two-column layout with right-aligned key column and left-aligned description column. Per-character x-positions computed via `advanceWidth` from DirectWrite glyph metrics. `CalculateColumnAlignedTextPositions` handles margin, key, gap, and description columns.
- **Overlay background**: Opaque overlay background replaced with feathered semi-transparent per-row halos (D2D `FillRoundedRectangle` with layered opacity, later replaced by D3D11 SDF shader in session 2026-03-19).
- **Input system refactoring**: `Application::OnKeyDown` converted from if/else chain to switch statement. Key processing delegation to `InputSystem::ProcessKeyDown` for C, S, Space, +/- keys. `IsAltEnterPressed` removed; `OnSysKeyDown` checks `wParam == VK_RETURN` directly. Modifier key filter (Shift, Ctrl, Alt, Win) prevents spurious help hint display.
- **Overlay mutual exclusivity**: Help hint and hotkey overlays are mutually exclusive — showing one dismisses the other. Auto-dismiss timers: help hint 2.7s hold, hotkey overlay 5.4s hold.
- **Overlay behavior**: Pressing `?` during dissolve restarts the overlay via `Show()`. Any non-modifier key dismisses the active overlay. Overlays are not created in screensaver modes (pointers left null).
- **UsageDialog GPU pipeline**: Custom D2D rain in UsageDialog replaced with full `RenderSystem`/`AnimationSystem` GPU render pipeline. Dialog has its own D3D11 device, swap chain, and bloom post-processing.
- **ScrambleRevealEffect refactoring**: Constructor-based initialization with const config members (`revealDuration`, `dismissDuration`, `cycleInterval`, `flashDuration`, `holdDuration`). `SetCellCount` separate from construction. `Update` factored into `UpdateRevealing`, `UpdateHolding`, `UpdateDismissing`. `AdvanceCycleTimer` and `FadeOpacity` helper methods extracted.

### Session 2026-03-19 — Rendering Pipeline & Density Overhaul

- Overlay rendering moved to scene texture before bloom — bloom post-processing provides the glow effect for free, eliminating N per-character glow layer instances.
- D2D feathered rounded-rect background replaced with a D3D11 fullscreen SDF pixel shader (signed distance field to per-row rounded rects with quadratic falloff). Single draw call, zero FPS impact.
- Halo row rects cached per overlay (invalidated on resize / DPI change).
- `Render()` 12-parameter signature replaced with `RenderParams` struct using C++20 designated initializers.
- Dead `pOcclusionRect` parameter removed from `Render()` and `UpdateInstanceBuffer()` — rain occlusion was never used.
- Dead `m_pointSamplerState` removed (linear sampler produces identical results at 1:1 texel-to-pixel).
- Scramble cycle timers staggered per-cell with random initial offset in `[0, cycleInterval)` to prevent synchronized glyph changes.
- Cycle interval tuned from 0.10s to 0.25s for overlays.
- Cycling/dismissing character brightness increased from 50% to 70% scheme color.
- Default rain density reduced from 80% to 50%; streak multiplier from 4.0x to 2.4x (new 50% = old 30%).
- `ScreenSaverSettings::DEFAULT_DENSITY_PERCENT` aligned to 50 (config dialog Reset button).
- Config dialog cancel bug fixed: `OnCancel()` now clears `m_hConfigDialog` before `DestroyWindow()` so Enter and Escape resume working.
- `CharacterInstanceData` and `ConstantBufferData` moved from public to private in `RenderSystem`.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Runtime Help Hint on Startup (Priority: P2)

A user launches MatrixRain in normal mode (no screensaver arguments) and sees a brief, visually integrated help hint in the center of the screen showing available key bindings. The hint uses a scramble-reveal effect — characters appear as cycling random matrix glyphs that lock into their target characters with per-cell staggered timing, accompanied by a yellow flash and white pulse. After a brief hold, a dismiss animation reverses the effect: characters transition back to cycling random glyphs and fade out with staggered timing. The message area has a feathered border and rain streaks pass behind it, keeping the message legible without interrupting the surrounding animation.

**Why this priority**: Provides visual discoverability for first-time users. Deferred behind CLI help (US3) because the overlay involves more unknowns (D2D rendering, per-character animation, rain occlusion) while the CLI path is self-contained and lower-risk.

**Independent Test**: Launch `MatrixRain.exe` with no arguments and observe that a centered help hint appears with the matrix reveal effect, displays three key bindings in two left-aligned columns, holds briefly, then dissolves with staggered per-character rain cycling and fade-out. Per-row halo backgrounds should darken behind each text line for readability.

**Acceptance Scenarios**:

1. **Given** MatrixRain is launched without screensaver arguments, **When** the application starts and the rain animation begins, **Then** a three-line help hint appears centered on the screen showing:
   ```
   Settings      Enter
       Help      ?
       Exit      Esc
   ```
   The left column (labels) is right-justified and the right column (keys) is left-justified, with the entire block centered horizontally and vertically on screen.

2. **Given** the help hint is appearing, **When** the reveal animation runs, **Then** characters appear as cycling random matrix rain glyphs (from `CharacterConstants`) that lock into their target glyphs with per-cell staggered timing. Each character locks with a yellow flash, and once all characters have settled, a white pulse washes across the text. Invisible margin columns on each side ensure the reveal enters and exits the visible area smoothly.

3. **Given** the help hint is fully revealed and the hold time has elapsed, **When** the dismiss phase begins, **Then** characters transition back to cycling random glyphs and fade out with per-cell staggered timing. Each row has staggered timing so the message dissolves organically rather than disappearing all at once. Characters that have been dismissed fully fade to transparent.

4. **Given** the help hint is visible on screen, **When** rain streaks fall through the area occupied by the message, **Then** the per-row rounded-rect halos (rendered via D3D11 SDF pixel shader) darken the background behind each text line, maintaining readability. The halo opacity scales with character opacity so it fades in and out with the text. Rain streaks continue to render normally — they are not occluded.

5. **Given** the help hint has fully dissolved, **When** all characters have faded to transparent, **Then** the halo background also fades to fully transparent and the animation area appears unobstructed.

6. **Given** MatrixRain is launched with any screensaver argument (`/s`, `/p`, `/c`, `/c:<HWND>`, `/a`), **When** the application starts, **Then** no help hint is displayed.

---

### User Story 2 - Re-show Help Hint on Unrecognized Key (Priority: P2)

A user presses a key that is not mapped to any existing hotkey. Instead of being silently ignored, the help hint reappears with the same matrix reveal animation, reminding the user of the available controls.

**Why this priority**: Complements the startup overlay (US1). Deferred to same priority since it depends on US1 being implemented first.

**Independent Test**: Wait for the startup hint to disappear, press a key that is not a recognized hotkey (e.g., `X`), and observe that the help hint reappears with the matrix reveal effect and fades away again.

**Acceptance Scenarios**:

1. **Given** the help hint has faded away, **When** the user presses a key that is not a recognized hotkey, **Then** the help hint reappears with the same matrix reveal animation, hold, and dissolve behavior as the initial display.

2. **Given** the help hint is currently visible, **When** the user presses an unrecognized key, **Then**:
   - If the hint is in the **reveal** or **hold** phase: the animation resets and replays from the beginning (ScrambleRevealEffect restarts).
   - If the hint is in the **dismiss** phase: the ScrambleRevealEffect restarts a fresh reveal.

3. **Given** the user presses a recognized hotkey (Space, C, S, +, -, `, Escape, Alt+Enter, Enter, ?), **When** the help hint is visible, **Then** the hotkey performs its normal function AND triggers the dissolve phase of the help hint (characters begin rain-cycling and fading with staggered timing). If the hint is not visible, the hotkey simply performs its normal function without triggering the hint.

---

### User Story 3 - Command-Line Help Display with Usage Dialog (Priority: P1)

A user runs `MatrixRain.exe /?` (or `-?`) from any context — command prompt, double-click, shortcut, etc. A custom graphical dialog window (`UsageDialog`) appears with a black background and matrix rain animation. The dialog shows command-line switches only (Options and Screensaver Options sections) — no hotkeys. Text is rendered in proportional font (Segoe UI) with columnar alignment via explicit x-offsets. Characters use the same scramble-reveal animation as the overlays — each non-space character's (x, y) position is pre-computed, and characters cycle through random glyphs before locking into their targets with per-cell staggered timing, accompanied by a yellow flash and white pulse. Background matrix rain fills the entire window. The dialog remains open until the user dismisses it (Enter, Escape, or close button), then the application exits.

**Why this priority**: Foundational UX — establishes `UsageText` and the usage dialog infrastructure. Fewest GPU unknowns compared to the overlay (US1) since the dialog has its own isolated rendering context with no existing render loop to integrate with.

**Independent Test**: Run `MatrixRain.exe /?` from any context. Observe a graphical dialog with scramble-reveal animation revealing the formatted usage text (switches only, no hotkeys). After all text is revealed, verify the dialog stays open with ambient rain continuing throughout the window. Dismiss the dialog and verify the application exits cleanly.

**Acceptance Scenarios**:

1. **Given** a user runs `MatrixRain.exe /?` from any context (terminal, double-click, shortcut), **When** the application processes the argument, **Then** a custom graphical dialog window (`UsageDialog`) appears with a black background and matrix rain animation that reveals the formatted help text showing command-line switches only (Options and Screensaver Options).

2. **Given** a user runs `MatrixRain.exe -?`, **When** the application processes the argument, **Then** the same usage dialog is displayed as for `/?`.

3. **Given** the scramble-reveal effect is revealing usage text in the dialog, **When** a character's random lock time arrives, **Then** that character locks into its target glyph with a yellow flash. The character remains in its settled color from that point forward.

4. **Given** the full usage text layout has been computed, **When** the scramble-reveal begins, **Then** every non-space character starts cycling through random glyphs and independently locks into its target at a random time within the reveal duration.

5. **Given** the help output is displayed, **Then** it includes: the application name and version, general options (`/c`, `/?`), and screensaver options (`/s`, `/p`, `/a`) with a brief description of each. Hotkeys are NOT displayed. All switches use the same prefix style (`/` or `-`) that the user used to invoke help.

6. **Given** the scramble-reveal effect is running in the dialog, **When** characters are cycling, **Then** they display random matrix glyphs (from the existing character set) in dark green, consistent with the matrix rain visual language. Background matrix rain also fills the entire window.

7. **Given** the scramble-reveal effect has completed revealing all characters, **When** the user presses Enter, Escape, or clicks the dialog's close button, **Then** the dialog closes and the application exits cleanly. The user may also dismiss the dialog at any time during the reveal animation.

8a. **Given** the reveal phase has completed, **When** the usage text is fully visible, **Then** ambient matrix rain continues at reduced density/speed throughout the entire window — rain passes through the text area, not just around it.

8b. **Given** the resolved usage text is displayed, **Then** each text character has a feathered dark glow effect (concentric shadow layers) that creates a local contrast zone, ensuring readability even when rain passes through the text area.

9. **Given** the usage dialog is displayed, **Then** the dialog window is sized to 2x the text bounding box (capped at 80% of primary monitor work area per dimension), and is centered on the primary monitor.

---

### User Story 4 - Open Settings Dialog via Enter Key (Priority: P2)

A user presses Enter during normal mode to open the configuration dialog, providing an intuitive way to access settings that was previously only available via the `/c` command-line switch.

**Why this priority**: The help hint advertises "Settings Enter" — the Enter key binding must work for the hint to be accurate. This completes the discoverability loop.

**Independent Test**: Launch MatrixRain normally, press Enter, and verify the configuration dialog opens as a live overlay with the same behavior as `/c` (modeless, changes apply immediately).

**Acceptance Scenarios**:

1. **Given** MatrixRain is running in normal mode, **When** the user presses Enter, **Then** the configuration dialog opens as a live modeless overlay, identical in behavior to launching with `/c` (topmost, immediate effect, OK saves, Cancel reverts).

2. **Given** the configuration dialog is already open, **When** the user presses Enter again, **Then** nothing happens (a second dialog is not created).

3. **Given** the help hint is visible when Enter is pressed, **When** the configuration dialog opens, **Then** the help hint begins its dissolve phase (rain-cycling and fading with staggered timing).

---

### User Story 5 - Show Hotkey Reference via ? Key (Priority: P2)

A user presses `?` during normal mode to see a hotkey reference rendered directly on the main MatrixRain window as an in-app overlay — NOT in a separate dialog. The overlay lists all runtime hotkeys with descriptions, providing an in-app reference without leaving the animation. This is distinct from `/?` which shows command-line switches in a standalone `UsageDialog`.

**Why this priority**: The help hint advertises "? Help" — the ? key binding must work. Provides comprehensive hotkey reference beyond the three-line hint.

**Independent Test**: Launch MatrixRain normally, press `?`, and verify a hotkey reference overlay appears directly on the main window showing all runtime hotkeys with descriptions.

**Acceptance Scenarios**:

1. **Given** MatrixRain is running in normal mode, **When** the user presses `?` (Shift+/ or the `?` key), **Then** a hotkey reference overlay appears directly on the main window showing all supported runtime hotkeys with descriptions. No separate dialog window is created.

2. **Given** the hotkey overlay is visible, **When** the user presses any non-modifier key (recognized or unrecognized), **Then** the overlay begins its dissolve phase and the animation continues.

3. **Given** the hotkey overlay is dissolving, **When** the user presses `?` again, **Then** the overlay restarts with a fresh reveal animation.

4. **Given** the hotkey overlay is visible in Holding or Revealing phase, **When** the user presses `?` again, **Then** the overlay begins its dissolve phase.

5. **Given** no overlay is active, **When** the user presses `?`, **Then** the hotkey overlay appears. The hotkey overlay auto-dismisses after 5.4 seconds at full visibility.

---

### Edge Cases

- If the console is not attached (double-click launch), `/?` help text must still be visible to the user — the usage dialog works identically regardless of launch context.
- The matrix reveal effect must work correctly regardless of screen resolution or DPI scaling.
- Rain continues to render normally behind overlay text during all phases. Per-row rounded-rect halos provide a darkened background for readability — they are driven by the D3D11 SDF halo shader and fade in/out with character opacity.
- During the dissolve phase, partially-dissolved characters (still cycling through rain glyphs but fading) must remain legible against the feathered background until their opacity drops below a visible threshold.
- Rapidly pressing unrecognized keys should not stack multiple help hint instances — only one hint is active at a time, resetting on each trigger.
- The help hint bounding area must have a feathered border that blends seamlessly with the surrounding rain — no hard rectangular edges.
- If the user presses a recognized hotkey while the hint is in any phase (reveal, hold, or dissolve), the hotkey must still execute its action.
- Window resize or display mode change (Alt+Enter) while the help hint is visible must reposition/resize the hint to remain centered.
- If a dismiss is in progress and the hint is retriggered (unrecognized key), the ScrambleRevealEffect restarts a fresh reveal from the beginning.
- The usage dialog (`/?`) must render correctly on high-DPI displays with no blurry text or misaligned rain characters.
- The usage dialog must be centered on the primary monitor and sized appropriately for the text content. If the text content exceeds the monitor dimensions, it should be scaled or scrollable.
- Closing the usage dialog via Alt+F4, close button, Enter, or Escape must all behave identically — clean dismiss.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: All command-line switches MUST accept both `/` and `-` as the switch prefix interchangeably (e.g., `/s` and `-s` are equivalent, `/?` and `-?` are equivalent). Switch characters MUST be case-insensitive.
- **FR-002**: Application MUST accept `/?` and `-?` as command-line arguments to display formatted help text listing all supported switches and their descriptions, then exit without launching the animation. The help output MUST present all switches using the same prefix style (`/` or `-`) that the user used to invoke help. The help text MUST be displayed in a custom usage dialog where scramble-reveal animation reveals the text (see FR-015).
- **FR-003**: When launched in Normal mode (no screensaver arguments), the application MUST display a three-line help hint centered on screen showing key bindings in two columns: labels right-justified in the left column, key names left-justified in the right column.
- **FR-004**: The help hint MUST use a scramble-reveal effect. Characters appear as cycling random glyphs that lock into their target glyphs with per-cell staggered timing (`ScrambleRevealEffect`). Each character locks with a yellow flash, and a white pulse marks all cells settling. Invisible margin columns extend the reveal past the visible text edges for smooth entry/exit.
- **FR-005**: The help hint MUST dismiss automatically after the reveal animation completes, with a 2.7 second hold at full visibility before dismissal begins. Characters transition back to cycling random glyphs and fade out with per-cell staggered timing. The overlay becomes hidden when all characters have fully faded.
- **FR-006**: When the user presses any key that is not mapped to a recognized hotkey, the application MUST re-display the help hint with the full matrix reveal animation.
- **FR-007**: The help hint MUST NOT appear when MatrixRain is launched with any screensaver argument (`/s`, `/p`, `/c`, `/c:<HWND>`, `/a`). Overlay objects (`HelpHintOverlay`, `HotkeyOverlay`) are not created in screensaver modes — null pointers gate all overlay code paths.
- **FR-008**: Pressing Enter in Normal mode MUST open the configuration dialog as a live modeless overlay, with the same behavior as launching with `/c`.
- **FR-009**: Pressing `?` (Shift+/) in Normal mode MUST display a hotkey reference overlay rendered directly on the main window, listing all runtime hotkeys with brief descriptions. This is an in-app overlay — NOT a separate dialog window.
- **FR-010**: Only one help hint instance may be active at a time. Triggering the hint while it is in the reveal or hold phase MUST restart the animation from the beginning. Triggering while in the dismiss phase MUST restart a fresh reveal.
- **FR-011**: The help hint bounding area MUST have per-row rounded-rect halos rendered via a D3D11 fullscreen SDF pixel shader (signed distance to rounded rects with quadratic falloff). Single draw call per overlay. Halo opacity is driven by the maximum character opacity across all non-space characters, with a tunable peak darkness (`haloMaxDark = 0.85`). Each row gets an independent halo sized to its text content. Halo row rects are cached per overlay and invalidated on resize or DPI change.
- **FR-012**: Overlays render to the scene texture before bloom post-processing. Rain streaks render first, then halo backgrounds, then overlay text — all to the same scene texture. Bloom then extracts and composites, providing a natural glow on the overlay text for free. Rain is NOT occluded — it renders under the halo darkening.
- **FR-013**: The help hint MUST remain centered after window resize or display mode transitions.
- **FR-014**: Recognized hotkeys MUST continue to function normally regardless of whether the help hint is visible. Any non-modifier keypress (excluding Shift, Ctrl, Alt, and Win keys) MUST dismiss both the help hint and hotkey overlays. Standalone modifier keys MUST be ignored entirely — they do not dismiss overlays or trigger the help hint. The help hint and hotkey overlay are mutually exclusive — only one may be active at a time.
- **FR-015**: When `/?` or `-?` is invoked, the usage text MUST be displayed in a custom graphical dialog (`UsageDialog`) — a standalone window with its own rendering context, black background, proportional font (Segoe UI), and scramble-reveal animation:
  - **Content**: Command-line switches only (Options and Screensaver Options sections). No hotkeys.
  - **Animation**: Uses `ScrambleRevealEffect` — the same per-cell timing oracle as the overlays. Pre-compute (x, y) for every non-space character. Characters cycle through random glyphs and independently lock into their targets at random times within the reveal duration. Each lock is marked by a yellow flash; a white pulse washes across all text once everything has settled. Background matrix rain fills the entire window.
  - **Post-reveal**: Background rain continues at reduced density/speed. Rain passes through the text area, not just around it.
  - **Resolved Text**: Each revealed character is rendered with a feathered dark glow effect (concentric shadow layers at decreasing opacity, same technique as the FPS counter overlay). The glow creates a local contrast zone ensuring readability even when rain passes through the text area.
  - **Sizing**: 2x text bounding box, capped at 80% of primary monitor work area per dimension. Text centered in window.
  - The dialog remains open until dismissed by the user (Enter, Escape, or close button). Dismissing the dialog exits the application.

### Key Entities

- **HelpHintOverlay**: The on-screen three-line help message with scramble-reveal animation. Uses `ScrambleRevealEffect` as a per-cell timing oracle for staggered character reveals. Per-character state includes target glyph index, current glyph index (random during cycling, target otherwise), a single assigned random glyph index, phase (Hidden/Cycling/LockFlash/Settled/Dismissing), opacity, and color (driven by `ComputeScrambleColor`). Invisible margin columns (`MARGIN_COLS=1`) extend the reveal past text edges. Per-row SDF halo backgrounds provide readability. Overall lifecycle: Hidden \u2192 Revealing \u2192 Holding \u2192 Dismissing \u2192 Hidden.
- **UsageDialog**: A custom graphical window used to display command-line switches (no hotkeys) with scramble-reveal animation and background matrix rain. Has its own rendering context, black background, proportional font (Segoe UI). Pre-computes per-character (x, y) positions via DirectWrite. Uses `ScrambleRevealEffect` for the same per-cell animation as the overlays. Background rain fills the entire window. Provides feathered dark glow on resolved text for readability. Used by `/?` invocation only — the `?` key uses an in-app overlay instead.
- **UsageText**: The shared module that builds formatted help text content — switch names, arguments, and descriptions grouped into Options and Screensaver Options sections. Provides text formatting and a plain text view. Used by `UsageDialog` for its content. Does NOT contain hotkeys (those are managed by the in-app hotkey overlay).
- **CommandLineHelp**: The orchestration module for `/?`/`-?` handling — detects the switch prefix, creates a `UsageDialog`, and exits the process after the dialog is dismissed.

## Assumptions

- The matrix rain character reveal effect uses the same glyph set as the existing rain animation (half-width katakana and other characters defined in `CharacterConstants`).
- The animation timing (scramble duration per character during reveal, hold time before dissolve, dissolve stagger intervals, per-character cycling and fade durations) will be tuned during implementation for visual appeal; the spec does not prescribe exact durations.
- The `?` key is detected as `VK_OEM_2` with Shift held, consistent with standard US keyboard layout. International keyboard layouts may vary.
- The Enter key binding for settings does not conflict with any existing hotkey (Enter is currently unhandled).
- The help hint text content ("Settings", "Enter", "?", "Help", "Esc", "Exit") is in English and not localized.
- The usage dialog creates its own rendering context, independent of the main application's render pipeline. For `/?`, no main window exists; for `?` key, the overlay renders on the main window (not a dialog).
- The usage dialog window is non-resizable and sized to fit the usage text content with appropriate padding.
- Console output (stdout, AttachConsole, ANSI escape codes) is NOT used for help display. The previous console-based approach was abandoned because GUI subsystem apps cannot make the shell wait for their output.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001** *(design goal — not automatable)*: 100% of first-time users can identify at least two available controls (Settings, Help, or Exit) within 10 seconds of launching MatrixRain in normal mode, without consulting external documentation.
- **SC-002**: Running `MatrixRain.exe /?` displays the usage dialog, reveals all usage text through the scramble-reveal effect within 5 seconds, and the dialog is visually correct with all characters properly placed and no visual artifacts. Dismissing the dialog exits the application cleanly.
- **SC-003**: The help hint matrix reveal effect completes, holds, and dissolves fully within 10 seconds of application startup, without dropped frames or visual glitches across tested resolutions (4K desktop, Surface Laptop 6 13", Surface Pro ARM). SDF halo backgrounds and bloom glow render without visual artifacts during all phases.
- **SC-004**: Pressing an unrecognized key triggers the help hint re-display within one rendered frame, with the animation restarting or reversing cleanly depending on the current phase.
- **SC-005**: All existing hotkeys (Space, C, S, +, -, `, Escape, Alt+Enter) continue to function identically to the previous release, with zero regressions.
- **SC-006**: The help hint is not displayed in any screensaver mode across all tested invocation methods (`/s`, `/p <HWND>`, `/c`, `/c:<HWND>`, `/a`).
