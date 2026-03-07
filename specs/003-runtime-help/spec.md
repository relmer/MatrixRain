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
- Q: Should the `?` key usage dialog also use the graphical rain dialog? → A: Yes — both `/?` and `?` launch the same custom graphical rain dialog for consistency.

### Session 2026-03-04

- Q: Should the dialog use a monospace or proportional font? → A: Proportional (Segoe UI). Text columns are aligned via explicit x-offsets from DirectWrite, not character grid cells. The rain reveal uses per-character positioning via `HitTestTextPosition` with a randomized queue — no column grid needed.
- Q: Should hotkeys be in the `/?` dialog? → A: No. The `/?` dialog shows command-line switches only (Options + Screensaver Options). Hotkeys are shown by the `?` key as an in-app overlay rendered directly on the main MatrixRain window.
- Q: Should the `?` key open a dialog? → A: No. The `?` key renders hotkey information directly on the main window as an in-app overlay, not in a separate dialog. This is distinct from `/?` which uses `HelpRainDialog`.
- Q: How does the rain reveal work with proportional font? → A: Per-character. Pre-compute (x, y) for every non-space character, shuffle into a queue, spawn a short streak at each character's x-position that locks in the character when the head reaches its y-position. Characters materialize scattered across the text like random raindrops.
- Q: Dialog sizing? → A: 2x text bounding box, capped at 80% of primary monitor work area per dimension. Text block centered in window.
- Q: Are rain streaks constrained to columns? → A: No. Both reveal and decorative streaks spawn at exact pixel x-positions (same as the main app). No column grid. Streaks overlap naturally.
- Q: How does reveal pacing work? → A: Two independent streak pools. Pool 1 (reveal): exactly N streaks, drains at a fixed rate to guarantee 3s completion. Pool 2 (decorative): same density/speed during Phase 1, backs off in Phase 2. Pools are completely independent.
- Q: Should `/c` be in the Screensaver Options section? → A: No. `/c` (show settings dialog) stays in general Options. Only `/s`, `/p`, `/a` are in Screensaver Options.
- Q: Unicode symbols? → A: All Unicode characters go in `UnicodeSymbols.h` with named constants — same pattern as TCDir.

### Session 2026-03-06 — Sweep-Based Animation Redesign

- The original per-character random resolve/dissolve model was replaced during implementation with a horizontal sweep model.
- **Reveal**: A left-to-right sweep crosses each row. Behind the sweep head, a wide streak zone (full overlay width) displays random glyphs. As the sweep passes each column, the character transitions from random → target glyph with a green→white color fade (`glowIntensity`). The head extends past the right edge so rightmost characters settle smoothly.
- **Dismiss**: A right-to-left sweep (mirror of reveal) crosses each row. The sweep head leads a streak zone of random glyphs. As it passes each column, the character transitions back to a random glyph and fades out.
- **TextSweepEffect**: Shared timing oracle used by both `HelpHintOverlay` and `HotkeyOverlay`. Manages per-row staggered delays, reveal/hold/dismiss phase transitions, opacity, and glow intensity. Overlays query `GetRevealProgress(row)`, `GetDismissProgress(row)`, `GetOpacity(row, normCol)`, `GetGlowIntensity(row, normCol)` to drive character state.
- **Margin columns**: Invisible columns added on each side (`MARGIN_COLS=1` for HelpHintOverlay, `MARGIN_COLS=2` for HotkeyOverlay) so the sweep enters/exits the visible area smoothly rather than starting/ending abruptly at the first/last text character.
- **Random glyph assignment**: Each character is assigned one random glyph index on first reveal (deterministic from position). Characters in the streak zone show this fixed random glyph, not a rapidly cycling animation.
- **CharPhase simplified**: Reduced from 5 values (Scrambling, Resolved, DissolveCycling, DissolveFading, Hidden) to 2 (Resolved, Hidden). The sweep timing oracle handles all intermediate states.
- **Known visual tuning areas (in progress)**:
  - Sweep speed and per-row stagger range
  - Streak zone length (currently = full overlay width)
  - Glow decay speed (green→white transition rate)
  - Dismiss sweep timing relative to fade-out opacity
  - Whether characters should visually cycle through multiple random glyphs or show one fixed glyph in the streak zone

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Runtime Help Hint on Startup (Priority: P2)

A user launches MatrixRain in normal mode (no screensaver arguments) and sees a brief, visually integrated help hint in the center of the screen showing available key bindings. The hint uses a horizontal sweep reveal effect — a left-to-right sweep crosses each row with per-row stagger, and behind the sweep head a wide streak zone displays random matrix glyphs. As the sweep passes each column position, the character transitions from a random glyph to its target character with a green→white color fade. After a brief hold, a right-to-left dismiss sweep mirrors the reveal: a streak zone of random glyphs sweeps across each row, and as it passes each column the character transitions back to a random glyph and fades out. The message area has a feathered border and rain streaks pass behind it, keeping the message legible without interrupting the surrounding animation.

**Why this priority**: Provides visual discoverability for first-time users. Deferred behind CLI help (US3) because the overlay involves more unknowns (D2D rendering, per-character animation, rain occlusion) while the CLI path is self-contained and lower-risk.

**Independent Test**: Launch `MatrixRain.exe` with no arguments and observe that a centered help hint appears with the matrix reveal effect, displays three key bindings in two left-aligned columns, holds briefly, then dissolves with staggered per-character rain cycling and fade-out. Rain streaks should visibly pass behind the message area.

**Acceptance Scenarios**:

1. **Given** MatrixRain is launched without screensaver arguments, **When** the application starts and the rain animation begins, **Then** a three-line help hint appears centered on the screen showing:
   ```
   Settings      Enter
       Help      ?
       Exit      Esc
   ```
   The left column (labels) is right-justified and the right column (keys) is left-justified, with the entire block centered horizontally and vertically on screen.

2. **Given** the help hint is appearing, **When** the reveal sweep runs, **Then** a horizontal left-to-right sweep progresses across each row (with per-row staggered timing). The sweep head leads a wide streak zone of random matrix rain glyphs (from `CharacterConstants`). As the sweep front passes each column position, the character transitions from a random glyph to its target glyph. Characters just revealed glow green and transition to white as they settle (green → white color fade driven by `glowIntensity`). The streak zone spans the full overlay width, so the head extends well past the right edge before all characters have settled. Invisible margin columns on each side ensure the sweep enters and exits the visible area smoothly.

3. **Given** the help hint is fully revealed and the hold time has elapsed, **When** the dismiss phase begins, **Then** a horizontal right-to-left sweep progresses across each row (mirroring the reveal). The dismiss sweep head leads a wide streak zone of random glyphs sweeping from right to left. As the sweep front passes each column, the character transitions from its target glyph back to a random glyph, then fades out with decreasing opacity. Each row has staggered timing so the message dissolves organically rather than disappearing all at once. Characters that have been swept past fully fade to transparent.

4. **Given** the help hint is visible on screen, **When** rain streaks fall through the area occupied by the message, **Then** the streaks pass behind the message's bounding area and are not visible within it. The message area has a feathered border (soft gradient at the edges) that blends smoothly with the surrounding rain, similar to the glow effect used by the performance counter overlay.

5. **Given** the help hint has fully dissolved, **When** all characters have faded to transparent, **Then** rain streaks in the formerly occluded area become visible again and the animation continues unobstructed.

6. **Given** MatrixRain is launched with any screensaver argument (`/s`, `/p`, `/c`, `/c:<HWND>`, `/a`), **When** the application starts, **Then** no help hint is displayed.

---

### User Story 2 - Re-show Help Hint on Unrecognized Key (Priority: P2)

A user presses a key that is not mapped to any existing hotkey. Instead of being silently ignored, the help hint reappears with the same matrix reveal animation, reminding the user of the available controls.

**Why this priority**: Complements the startup overlay (US1). Deferred to same priority since it depends on US1 being implemented first.

**Independent Test**: Wait for the startup hint to disappear, press a key that is not a recognized hotkey (e.g., `X`), and observe that the help hint reappears with the matrix reveal effect and fades away again.

**Acceptance Scenarios**:

1. **Given** the help hint has faded away, **When** the user presses a key that is not a recognized hotkey, **Then** the help hint reappears with the same matrix reveal animation, hold, and dissolve behavior as the initial display.

2. **Given** the help hint is currently visible, **When** the user presses an unrecognized key, **Then**:
   - If the hint is in the **reveal** or **hold** phase: the animation resets and replays from the beginning (TextSweepEffect restarts).
   - If the hint is in the **dismiss** phase: the TextSweepEffect restarts a fresh reveal sweep.

3. **Given** the user presses a recognized hotkey (Space, C, S, +, -, `, Escape, Alt+Enter, Enter, ?), **When** the help hint is visible, **Then** the hotkey performs its normal function AND triggers the dissolve phase of the help hint (characters begin rain-cycling and fading with staggered timing). If the hint is not visible, the hotkey simply performs its normal function without triggering the hint.

---

### User Story 3 - Command-Line Help Display with Graphical Rain Dialog (Priority: P1)

A user runs `MatrixRain.exe /?` (or `-?`) from any context — command prompt, double-click, shortcut, etc. A custom graphical dialog window appears with a black background and a two-phase matrix rain animation. The dialog shows command-line switches only (Options and Screensaver Options sections) — no hotkeys. Text is rendered in proportional font (Segoe UI) with columnar alignment via explicit x-offsets. In Phase 1 (reveal), characters materialize through a randomized per-character queue — each non-space character's (x, y) position is pre-computed, the queue is shuffled, and streaks spawn at each character's position to lock it in as the head passes through. A separate decorative rain pool fills the entire window at the same density. The reveal completes in ~3 seconds. In Phase 2 (background), decorative rain continues throughout the entire window at reduced density/speed, with resolved text readable via feathered dark glow. The dialog remains open until the user dismisses it (Enter, Escape, or close button), then the application exits.

**Why this priority**: Foundational UX — establishes `UsageText` and the graphical rain dialog infrastructure. Fewest GPU unknowns compared to the overlay (US1) since the dialog has its own isolated rendering context with no existing render loop to integrate with.

**Independent Test**: Run `MatrixRain.exe /?` from any context. Observe a graphical dialog with matrix rain streaks revealing the formatted usage text (switches only, no hotkeys). After all text is revealed (~3 seconds), verify the dialog stays open with ambient rain continuing throughout the window. Dismiss the dialog and verify the application exits cleanly.

**Acceptance Scenarios**:

1. **Given** a user runs `MatrixRain.exe /?` from any context (terminal, double-click, shortcut), **When** the application processes the argument, **Then** a custom graphical dialog window appears with a black background and matrix rain animation that reveals the formatted help text showing command-line switches only (Options and Screensaver Options).

2. **Given** a user runs `MatrixRain.exe -?`, **When** the application processes the argument, **Then** the same graphical rain dialog is displayed as for `/?`.

3. **Given** the rain effect is revealing usage text in the graphical dialog, **When** a reveal streak's head reaches a character's y-position, **Then** that character locks in at its final color/intensity. The character remains constant from that point forward as the streak continues past it.

4. **Given** the full usage text layout has been computed, **When** the rain effect begins, **Then** every non-space character has exactly one entry in the randomized reveal queue, and each spawns its own reveal streak at that character's x-position.

5. **Given** the help output is displayed, **Then** it includes: the application name and version, general options (`/c`, `/?`), and screensaver options (`/s`, `/p`, `/a`) with a brief description of each. Hotkeys are NOT displayed. All switches use the same prefix style (`/` or `-`) that the user used to invoke help.

6. **Given** the rain effect is running in the dialog, **When** each reveal streak falls, **Then** the streak head displays a bright character (random matrix glyph from the existing character set) and trailing characters dim behind it, using green colors consistent with the matrix rain visual language. A separate decorative rain pool also fills the entire window at random pixel x-positions.

7. **Given** the rain effect has completed revealing all characters, **When** the user presses Enter, Escape, or clicks the dialog's close button, **Then** the dialog closes and the application exits cleanly. The user may also dismiss the dialog at any time during the reveal animation.

8a. **Given** the reveal phase has completed, **When** the usage text is fully visible, **Then** ambient matrix rain continues at reduced density/speed throughout the entire window — rain passes through the text area, not just around it.

8b. **Given** the resolved usage text is displayed, **Then** each text character has a feathered dark glow effect (concentric shadow layers) that creates a local contrast zone, ensuring readability even when rain passes through the text area.

9. **Given** the graphical rain dialog is displayed, **Then** the dialog window is sized to 2x the text bounding box (capped at 80% of primary monitor work area per dimension), and is centered on the primary monitor.

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

A user presses `?` during normal mode to see a hotkey reference rendered directly on the main MatrixRain window as an in-app overlay — NOT in a separate dialog. The overlay lists all runtime hotkeys with descriptions, providing an in-app reference without leaving the animation. This is distinct from `/?` which shows command-line switches in a standalone `HelpRainDialog`.

**Why this priority**: The help hint advertises "? Help" — the ? key binding must work. Provides comprehensive hotkey reference beyond the three-line hint.

**Independent Test**: Launch MatrixRain normally, press `?`, and verify a hotkey reference overlay appears directly on the main window showing all runtime hotkeys with descriptions.

**Acceptance Scenarios**:

1. **Given** MatrixRain is running in normal mode, **When** the user presses `?` (Shift+/ or the `?` key), **Then** a hotkey reference overlay appears directly on the main window showing all supported runtime hotkeys with descriptions. No separate dialog window is created.

2. **Given** the hotkey overlay is visible, **When** the user presses any key (recognized or unrecognized), **Then** the overlay begins its dissolve phase and the animation continues.

3. **Given** the hotkey overlay is visible, **When** the user presses `?` again, **Then** nothing happens (the overlay does not restart or duplicate).

4. **Given** the help hint is visible when `?` is pressed, **When** the hotkey overlay appears, **Then** the help hint begins its dissolve phase (rain-cycling and fading with staggered timing).

---

### Edge Cases

- If the console is not attached (double-click launch), `/?` help text must still be visible to the user — the graphical rain dialog works identically regardless of launch context.
- The matrix reveal effect must work correctly regardless of screen resolution or DPI scaling.
- Rain streaks must pass behind the message area during all phases (reveal, hold, dissolve). The occlusion region disappears only after the last character has fully dissolved.
- During the dissolve phase, partially-dissolved characters (still cycling through rain glyphs but fading) must remain legible against the feathered background until their opacity drops below a visible threshold.
- Rapidly pressing unrecognized keys should not stack multiple help hint instances — only one hint is active at a time, resetting on each trigger.
- The help hint bounding area must have a feathered border that blends seamlessly with the surrounding rain — no hard rectangular edges.
- If the user presses a recognized hotkey while the hint is in any phase (reveal, hold, or dissolve), the hotkey must still execute its action.
- Window resize or display mode change (Alt+Enter) while the help hint is visible must reposition/resize the hint to remain centered.
- If a dismiss sweep is in progress and the hint is retriggered (unrecognized key), the TextSweepEffect restarts a fresh reveal sweep from the beginning.
- The graphical rain dialog (`/?` and `?` key) must render correctly on high-DPI displays with no blurry text or misaligned rain characters.
- The graphical rain dialog must be centered on the primary monitor and sized appropriately for the text content. If the text content exceeds the monitor dimensions, it should be scaled or scrollable.
- Closing the graphical rain dialog via Alt+F4, close button, Enter, or Escape must all behave identically — clean dismiss.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: All command-line switches MUST accept both `/` and `-` as the switch prefix interchangeably (e.g., `/s` and `-s` are equivalent, `/?` and `-?` are equivalent). Switch characters MUST be case-insensitive.
- **FR-002**: Application MUST accept `/?` and `-?` as command-line arguments to display formatted help text listing all supported switches and their descriptions, then exit without launching the animation. The help output MUST present all switches using the same prefix style (`/` or `-`) that the user used to invoke help. The help text MUST be displayed in a custom graphical rain dialog where matrix rain streaks reveal the text (see FR-015).
- **FR-003**: When launched in Normal mode (no screensaver arguments), the application MUST display a three-line help hint centered on screen showing key bindings in two columns: labels right-justified in the left column, key names left-justified in the right column.
- **FR-004**: The help hint MUST use a horizontal sweep reveal effect. A left-to-right sweep crosses each row with per-row staggered timing (`TextSweepEffect`). Behind the sweep head, a wide streak zone displays random matrix rain glyphs. As the sweep passes each column, the character resolves from a random glyph to its target glyph with a green→white color transition. Invisible margin columns extend the sweep past the visible text edges for smooth entry/exit.
- **FR-005**: The help hint MUST dismiss automatically after the reveal animation completes, with a brief hold at full visibility before dismissal begins. A right-to-left sweep (mirroring the reveal) crosses each row with per-row staggered timing. The sweep head leads a wide streak zone of random glyphs. As the sweep passes each column, the character transitions back to a random glyph and fades out with decreasing opacity. The overlay becomes hidden when all characters have fully faded.
- **FR-006**: When the user presses any key that is not mapped to a recognized hotkey, the application MUST re-display the help hint with the full matrix reveal animation.
- **FR-007**: The help hint MUST NOT appear when MatrixRain is launched with any screensaver argument (`/s`, `/p`, `/c`, `/c:<HWND>`, `/a`).
- **FR-008**: Pressing Enter in Normal mode MUST open the configuration dialog as a live modeless overlay, with the same behavior as launching with `/c`.
- **FR-009**: Pressing `?` (Shift+/) in Normal mode MUST display a hotkey reference overlay rendered directly on the main window, listing all runtime hotkeys with brief descriptions. This is an in-app overlay — NOT a separate dialog window.
- **FR-010**: Only one help hint instance may be active at a time. Triggering the hint while it is in the reveal or hold phase MUST restart the animation from the beginning. Triggering while in the dismiss phase MUST restart a fresh reveal.
- **FR-011**: The help hint bounding area MUST have a feathered border (soft gradient at edges) that blends smoothly into the surrounding rain, similar to the glow effect used by the performance counter overlay.
- **FR-012**: Rain streaks MUST pass behind the help hint's bounding area while the message is visible. Streaks entering the message area are occluded and not rendered within it. When the message fully dissolves, occluded streak characters become visible again.
- **FR-013**: The help hint MUST remain centered after window resize or display mode transitions.
- **FR-014**: Recognized hotkeys MUST continue to function normally regardless of whether the help hint is visible. If the help hint is currently in the reveal or hold phase when a recognized hotkey is pressed, the hint MUST immediately transition to the dissolve phase (rain-cycling and fading with staggered timing). If the hint is already dissolving, the hotkey does not restart or accelerate the dissolve.
- **FR-015**: When `/?` or `-?` is invoked, the usage text MUST be displayed in a custom graphical rain dialog (`HelpRainDialog`) — a standalone window with its own D3D/D2D rendering context, black background, proportional font (Segoe UI), and a two-phase matrix rain animation with two independent streak pools:
  - **Content**: Command-line switches only (Options and Screensaver Options sections). No hotkeys.
  - **Phase 1 (Reveal)**: Pre-compute (x, y) for every non-space character, shuffle into a randomized queue, spawn reveal streaks at each character's x-position. Reveal pool drains at a fixed rate calibrated for 3-second completion. Decorative pool runs independently at the same density/speed across the full window at random pixel x-positions. Rain fills the entire window — no column grid.
  - **Phase 2 (Background)**: After reveal completes, decorative rain density/speed backs off via tunable multipliers. Rain continues throughout the entire window — passes through the text area, not just around it.
  - **Resolved Text**: Each revealed character is rendered in white/near-white with a feathered dark glow effect (concentric shadow layers at decreasing opacity, same technique as the FPS counter overlay). The glow creates a local contrast zone ensuring readability even when rain passes through the text area.
  - **Sizing**: 2x text bounding box, capped at 80% of primary monitor work area per dimension. Text centered in window.
  - The dialog remains open until dismissed by the user (Enter, Escape, or close button). Dismissing the dialog exits the application.

### Key Entities

- **HelpHintOverlay**: The on-screen three-line help message with horizontal sweep reveal/dismiss animation. Uses `TextSweepEffect` as a per-row timing oracle for staggered horizontal sweeps. Per-character state includes target glyph index, current glyph index (random during sweep streak zone, target otherwise), a single assigned random glyph index, phase (Resolved/Hidden), opacity, and glow intensity (green→white transition). Invisible margin columns (`MARGIN_COLS=1`) extend the sweep past text edges. Bounding area with feathered border for rain streak occlusion. Overall lifecycle: Hidden → Revealing → Holding → Dismissing → Hidden.
- **HelpRainDialog**: A custom graphical window used to display command-line switches (no hotkeys) with a two-phase matrix rain animation. Has its own D3D11/D2D rendering context, black background, proportional font (Segoe UI), and two independent streak pools (reveal + decorative). Pre-computes per-character (x, y) positions via DirectWrite, shuffles into a randomized reveal queue that drains over ~3 seconds. Decorative rain spawns at random pixel x-positions across the full window. Provides feathered dark glow on resolved text for readability. Used by `/?` invocation only — the `?` key uses an in-app overlay instead.
- **UsageText**: The shared module that builds formatted help text content — switch names, arguments, and descriptions grouped into Options and Screensaver Options sections. Provides text formatting and a plain text view. Used by `HelpRainDialog` for its content. Does NOT contain hotkeys (those are managed by the in-app hotkey overlay).
- **CommandLineHelp**: The orchestration module for `/?`/`-?` handling — detects the switch prefix, creates a `HelpRainDialog`, and exits the process after the dialog is dismissed.

## Assumptions

- The matrix rain character reveal effect uses the same glyph set as the existing rain animation (half-width katakana and other characters defined in `CharacterConstants`).
- The animation timing (scramble duration per character during reveal, hold time before dissolve, dissolve stagger intervals, per-character cycling and fade durations) will be tuned during implementation for visual appeal; the spec does not prescribe exact durations.
- The `?` key is detected as `VK_OEM_2` with Shift held, consistent with standard US keyboard layout. International keyboard layouts may vary.
- The Enter key binding for settings does not conflict with any existing hotkey (Enter is currently unhandled).
- The help hint text content ("Settings", "Enter", "?", "Help", "Esc", "Exit") is in English and not localized.
- The graphical rain dialog creates its own D3D11/D2D rendering context, independent of the main application's render pipeline. For `/?`, no main window exists; for `?` key, the dialog is a separate top-level window.
- The graphical rain dialog window is non-resizable and sized to fit the usage text content with appropriate padding.
- Console output (stdout, AttachConsole, ANSI escape codes) is NOT used for help display. The previous console-based approach was abandoned because GUI subsystem apps cannot make the shell wait for their output.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001** *(design goal — not automatable)*: 100% of first-time users can identify at least two available controls (Settings, Help, or Exit) within 10 seconds of launching MatrixRain in normal mode, without consulting external documentation.
- **SC-002**: Running `MatrixRain.exe /?` displays the graphical rain dialog, reveals all usage text through the matrix rain effect within 5 seconds, and the dialog is visually correct with all characters properly placed and no visual artifacts. Dismissing the dialog exits the application cleanly.
- **SC-003**: The help hint matrix reveal effect completes, holds, and dissolves fully within 10 seconds of application startup, without dropped frames or visual glitches across tested resolutions (4K desktop, Surface Laptop 6 13", Surface Pro ARM). Rain streaks pass behind the message area without visual artifacts during all phases.
- **SC-004**: Pressing an unrecognized key triggers the help hint re-display within one rendered frame, with the animation restarting or reversing cleanly depending on the current phase.
- **SC-005**: All existing hotkeys (Space, C, S, +, -, `, Escape, Alt+Enter) continue to function identically to the previous release, with zero regressions.
- **SC-006**: The help hint is not displayed in any screensaver mode across all tested invocation methods (`/s`, `/p <HWND>`, `/c`, `/c:<HWND>`, `/a`).
