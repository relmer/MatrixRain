# Tasks: Runtime Help Overlay and Command-Line Help

**Input**: Design documents from `/specs/003-runtime-help/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/, quickstart.md

**Tests**: Required — Constitution I mandates TDD for all core library code. Tests are written first and must fail before implementation.

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Project initialization — no additional infrastructure needed (console approach abandoned)

### Superseded Tasks

- [X] ~~T001 Copy AnsiCodes.h from TCDir — SUPERSEDED (no ANSI console output; graphical dialog uses D3D11/D2D instead)~~

**Checkpoint**: No setup phase required — proceed directly to Phase 2

---

## Phase 2: User Story 3 — Command-Line Help with Usage Dialog (Priority: P1) 🎯 MVP

**Goal**: `MatrixRain.exe /?` displays a custom graphical window with scramble-reveal animation that reveals the usage text, then exits after the user dismisses it.

**Independent Test**: Run `MatrixRain.exe /?` → custom window opens with scramble-reveal animation revealing usage text → press Enter or Esc to dismiss → app exits. Run `MatrixRain.exe -?` → same behavior with `-` prefix.

### Superseded Tasks (Console Approach Abandoned)

The following tasks were completed under the original console-based approach but are now superseded by the usage dialog:

- [X] ~~T008 ConsoleRainEffect construction tests — SUPERSEDED (replaced by UsageDialog)~~
- [X] ~~T009 ConsoleRainEffect shutdown test — SUPERSEDED (replaced by UsageDialog)~~
- [X] ~~T014 ConsoleRainEffect constructor — SUPERSEDED (replaced by UsageDialog)~~
- [X] ~~T015 IsInteractiveConsole/EnableVTProcessing — SUPERSEDED (no console detection)~~
- [X] ~~T016 ConsoleRainEffect::Run() — SUPERSEDED (replaced by UsageDialog render loop)~~
- [X] ~~T017 AttachParentConsole — SUPERSEDED (no console attachment)~~
- [X] ~~T018 DisplayCommandLineHelp orchestration — SUPERSEDED (simplified, no console paths)~~
- [X] ~~T019 ConsoleCtrlHandler — SUPERSEDED (no Ctrl+C handling needed)~~

### Retained Tests for User Story 3

> Previously completed and still valid:

- [X] T002 [P] [US3] Test UsageText construction with `/` prefix
- [X] T003 [P] [US3] Test UsageText construction with `-` prefix
- [X] T004 [P] [US3] Test UsageText::GetPlainText() returns all lines joined with CRLF, no ANSI codes
- [X] T005 [P] [US3] Test UsageText::DetectSwitchPrefix()
- [X] T010 [P] [US3] Test CommandLine handles `/?` and `-?` arguments

### Superseded Tests for User Story 3

- [X] ~~T006 Test UsageText::BuildTextGrid() — SUPERSEDED (proportional font, no text grid)~~
- [X] ~~T007 Test UsageText::BuildColumnActivityMap() — SUPERSEDED (no column activity map, random pixel x)~~

### Retained Implementation for User Story 3

> Previously completed and still valid:

- [X] T011 [US3] Implement UsageText constructor and queries
- [X] T012 [US3] Implement UsageText::DetectSwitchPrefix()
- [X] T020 [US3] Add `/?`/`-?` parsing to CommandLine
- [X] T021 [US3] Wire CLI help early exit in main.cpp

### Superseded Implementation for User Story 3

- [X] ~~T013 Implement UsageText::BuildTextGrid() and BuildColumnActivityMap() — SUPERSEDED (proportional font, no text grid or column map)~~

### New Tests for UsageDialog

> **Write tests FIRST, verify they FAIL, then implement**

- [X] T067 [P] [US3] Test UsageDialog window sizing — ComputeWindowSize() returns 2x text bounding box dimensions (from IDWriteTextLayout metrics), capped at 80% of primary monitor work area per dimension — in MatrixRainTests/unit/UsageDialogTests.cpp
- [X] T068 [P] [US3] Test UsageDialog scramble-reveal animation state — pre-compute CharPosition for every non-space character, characters cycle through random glyphs and lock at random times via ScrambleRevealEffect — in MatrixRainTests/unit/UsageDialogTests.cpp
- [X] T069 [P] [US3] Test UsageDialog::IsRevealComplete() returns true when all revealedFlags are true — in MatrixRainTests/unit/UsageDialogTests.cpp
- [X] T083 [P] [US3] Test UsageDialog Phase 2 (background) — after reveal completes, background matrix rain density reduces via kBgDensityMultiplier (0.15f), speed increases via kPhase2SpeedMultiplier (1.5f) — in MatrixRainTests/unit/UsageDialogTests.cpp
- [X] T085 [P] [US3] Test UsageDialog scramble-reveal timing — verify reveal completes within kRevealDuration (1.8f) — in MatrixRainTests/unit/UsageDialogTests.cpp
- [X] T086 [P] [US3] Test UsageText section grouping — GetFormattedText() produces "Options:" section with /c and /?, "Screensaver Options:" section with /s, /p, /a — no hotkeys — in MatrixRainTests/unit/UsageTextTests.cpp

### New Implementation for UsageDialog

- [X] T087 [US3] Create UnicodeSymbols.h in MatrixRain/ — namespace UnicodeSymbols with static constexpr WCHAR constants for em dash (\u2014) and other Unicode symbols, following TCDir's pattern
- [X] T088 [US3] Update UsageText to produce section-grouped content — "Options:" section (/c, /?), "Screensaver Options:" section (/s, /p, /a), no hotkeys, em dash from UnicodeSymbols.h, remove BuildTextGrid()/BuildColumnActivityMap() — in MatrixRainCore/UsageText.h/.cpp
- [X] T070 [US3] Implement UsageDialog class declaration in MatrixRainCore/UsageDialog.h — window handle, D3D11 device/swap chain, D2D render target, characterPositions (vector<CharPosition>), revealedFlags, ScrambleRevealEffect, D2D brushes, textLayout (IDWriteTextLayout), animation phase enum (Revealing/Background), tunable constants
- [X] T071 [US3] Implement UsageDialog window creation in MatrixRainCore/UsageDialog.cpp — register WNDCLASS, create popup window with WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU (close button, non-resizable), size to 2x text bounding box capped at 80% work area, center on primary monitor
- [X] T072 [US3] Implement UsageDialog D3D11 and D2D initialization in MatrixRainCore/UsageDialog.cpp — create device, swap chain, DXGI surface, D2D render target, DirectWrite Segoe UI proportional text format (14-16pt DPI-scaled), IDWriteTextLayout for formatted text, create brushes: white text brush, black glow brush (variable opacity), bright green head brush, dim green trail brush
- [X] T089 [US3] Implement UsageDialog character position computation in MatrixRainCore/UsageDialog.cpp — iterate every non-space character in formatted text, call IDWriteTextLayout::HitTestTextPosition to get (x, y), build characterPositions vector with randomCharacter field
- [X] T073 [US3] Implement UsageDialog scramble-reveal animation in MatrixRainCore/UsageDialog.cpp — ScrambleRevealEffect drives per-cell reveal (characters cycle through random glyphs and lock at random times), ComputeScrambleColor drives color transitions (dark green → yellow flash → mid green → white pulse → grey), render resolved characters with feathered dark glow, render background matrix rain, transition to Phase 2 when all characters revealed
- [X] T084 [US3] Implement UsageDialog Phase 2 render logic in MatrixRainCore/UsageDialog.cpp — reduce background rain density via kBgDensityMultiplier (0.15f) and increase speed via kPhase2SpeedMultiplier (1.5f), continue rendering resolved text with feathered glow, rain fills entire window
- [X] T074 [US3] Implement UsageDialog input handling in MatrixRainCore/UsageDialog.cpp — WndProc handles WM_KEYDOWN (VK_RETURN, VK_ESCAPE to dismiss), WM_CLOSE, WM_DESTROY, close button (X)
- [X] T075 [US3] Implement UsageDialog::Show() in MatrixRainCore/UsageDialog.cpp — show window, run PeekMessage-based message pump with render-on-idle pattern, block until dismissed
- [X] T076 [US3] Update CommandLineHelp::DisplayCommandLineHelp() in MatrixRainCore/CommandLineHelp.cpp — create UsageText, create UsageDialog with const UsageText&, call Show(), no console attachment or detection
- [X] T077 [US3] Remove dead console code — delete ConsoleRainEffect.h/.cpp, remove AttachParentConsole/ConsoleCtrlHandler from CommandLineHelp, remove BuildTextGrid/BuildColumnActivityMap from UsageText if still present, remove AnsiCodes.h if unused elsewhere

**Checkpoint**: `/?` and `-?` launch a custom graphical window with proportional font, scramble-reveal animation (characters cycle through random glyphs and lock at random times via ScrambleRevealEffect), background matrix rain, feathered dark glow, switches only (Options + Screensaver Options, no hotkeys). Enter/Esc dismisses. No console output. MVP complete.

---

## Phase 3: User Story 1 — Runtime Help Hint on Startup (Priority: P2)

**Goal**: First-time user sees a centered three-line help hint with matrix rain reveal/dissolve effect. Rain streaks pass behind the message area with feathered border.

**Independent Test**: Launch `MatrixRain.exe` with no arguments → centered help hint appears with matrix reveal effect, holds briefly, dissolves with staggered per-character rain cycling and fade-out. Rain streaks pass behind the message area.

### Setup for Overlay Types

- [X] T022 [P] [US1] Define OverlayPhase enum class (Hidden, Revealing, Holding, Dissolving) and CharPhase enum class (Hidden, Cycling, LockFlash, Settled, Dismissing) in MatrixRainCore/HelpHintOverlay.h — *CharPhase originally Scrambling/Resolved/DissolveCycling/DissolveFading; replaced by CellPhase-mapped values in Phase 3a*
- [X] T023 [P] [US1] Define HintCharacter struct (targetGlyphIndex, currentGlyphIndex, randomGlyphIndex, phase, opacity, row, col, isSpace) in MatrixRainCore/HelpHintOverlay.h — *originally had scrambleTimer/scrambleInterval/dissolveStartOffset; timing now managed by ScrambleRevealEffect*

### Tests for User Story 1

> **Write tests FIRST, verify they FAIL, then implement**

- [X] T024 [P] [US1] Test HelpHintOverlay initial state: phase is Hidden, IsActive() returns false, characters empty — in MatrixRainTests/unit/HelpHintOverlayTests.cpp
- [X] T025 [P] [US1] Test Show() transitions phase to Revealing, IsActive() returns true, characters initialized to Cycling via ScrambleRevealEffect (spaces marked isSpace) — in MatrixRainTests/unit/HelpHintOverlayTests.cpp
- [X] T026 [P] [US1] Test Update() during Revealing: cycling characters display randomGlyphIndex, characters lock to targetGlyphIndex when ScrambleRevealEffect cell reaches LockFlash/Settled — in MatrixRainTests/unit/HelpHintOverlayTests.cpp
- [X] T027 [P] [US1] Test Revealing→Holding transition: once all cells are Settled, overlay phase becomes Holding — in MatrixRainTests/unit/HelpHintOverlayTests.cpp
- [X] T028 [P] [US1] Test Holding→Dissolving transition: after holdDuration elapses, overlay phase becomes Dissolving, ScrambleRevealEffect begins dismiss with staggered cell timing — in MatrixRainTests/unit/HelpHintOverlayTests.cpp
- [X] T029 [P] [US1] Test Dissolving→Hidden transition: all cells progress Dismissing→Hidden (CellPhase), once all Hidden the overlay phase becomes Hidden — in MatrixRainTests/unit/HelpHintOverlayTests.cpp
- [X] T030 [P] [US1] Test UpdateLayout() computes centered bounding rect for given viewport dimensions — in MatrixRainTests/unit/HelpHintOverlayTests.cpp
- [X] T031 [P] [US1] Test GetCharacters() returns span with correct row/col layout matching the three-line hint format — in MatrixRainTests/unit/HelpHintOverlayTests.cpp

### Implementation for User Story 1

- [X] T032 [US1] Implement HelpHintOverlay constructor, Show(), Hide(), and IsActive()/GetPhase()/GetRows()/GetCols() in MatrixRainCore/HelpHintOverlay.cpp — initialize 3-line hint text grid ("Settings Enter" / "Help ?" / "Exit Esc") with right-justified left column and left-justified right column
- [X] T033 [US1] Implement HelpHintOverlay::Update() reveal logic in MatrixRainCore/HelpHintOverlay.cpp — delegates to ScrambleRevealEffect::Update(), per-cell Cycling→LockFlash→Settled, Revealing→Holding transition
- [X] T034 [US1] Implement HelpHintOverlay::Update() hold and dissolve logic in MatrixRainCore/HelpHintOverlay.cpp — hold timer countdown, Dissolving phase with per-cell Settled→Dismissing→Hidden (CellPhase), overlay Hidden when all cells done
- [X] T035 [US1] Implement HelpHintOverlay::UpdateLayout() and GetBoundingRect() in MatrixRainCore/HelpHintOverlay.cpp — center bounding rect in viewport, recompute on resize
- [X] T036 [US1] Implement HelpHintOverlay::GetCharacters() returning span<const HintCharacter> in MatrixRainCore/HelpHintOverlay.cpp
- [X] T037 [US1] Add HelpHintOverlay member to Application, call Show() on startup in Normal mode only (skip for /s, /p, /c, /a) in MatrixRainCore/Application.cpp — wire UpdateLayout() on resize
- [X] T038 [US1] Add HelpHintOverlay::Update() call in render loop (under m_renderMutex) in MatrixRainCore/Application.cpp
- [X] T039 [US1] Render HelpHintOverlay in RenderSystem — iterate GetCharacters(), draw each non-hidden character using D2D/DirectWrite with current glyph and opacity, draw feathered border using DrawFeatheredGlow() — in MatrixRainCore/RenderSystem.cpp
- [X] T040 [US1] Add rain occlusion in AnimationSystem — pass HelpHintOverlay::GetBoundingRect() as exclusion zone, skip rendering rain streak characters inside the rect when overlay IsActive() — in MatrixRainCore/AnimationSystem.cpp and MatrixRainCore/RenderSystem.cpp
- [X] T041 [US1] Add help hint enabled flag to ApplicationState in MatrixRainCore/ApplicationState.h — set based on screensaver mode in Application startup

**Checkpoint**: Help hint overlay displays on startup with full reveal/hold/dismiss animation, rain occlusion, and feathered border. Screensaver modes suppressed.

---

## Phase 3a: ScrambleRevealEffect — Shared Timing Oracle

**Goal**: Extract per-cell staggered timing into a reusable class used by HelpHintOverlay, HotkeyOverlay, and UsageDialog. Replaces the original per-character random resolve/dissolve model.

### Completed Tasks

- [X] T098 [P] Test ScrambleRevealEffect initial state: phase is Idle, IsActive() returns false — in MatrixRainTests/unit/ScrambleRevealEffectTests.cpp
- [X] T099 [P] Test ScrambleRevealEffect::StartReveal() transitions to Revealing, cells begin Cycling phase — in MatrixRainTests/unit/ScrambleRevealEffectTests.cpp
- [X] T100 [P] Test ScrambleRevealEffect per-cell opacity: 1.0 during reveal, fades to 0.0 during dismiss — in MatrixRainTests/unit/ScrambleRevealEffectTests.cpp
- [X] T101 [P] Test ScrambleRevealEffect CellPhase transitions: Cycling → LockFlash → Settled during reveal — in MatrixRainTests/unit/ScrambleRevealEffectTests.cpp
- [X] T102 [P] Test ScrambleRevealEffect hold phase: after reveal completes, holds for holdDuration before auto-dismissing — in MatrixRainTests/unit/ScrambleRevealEffectTests.cpp
- [X] T103 [P] Test ScrambleRevealEffect dismiss phase: cells transition Settled → Dismissing → Hidden — in MatrixRainTests/unit/ScrambleRevealEffectTests.cpp
- [X] T104 Implement ScrambleRevealEffect class with Initialize(), StartReveal(), StartDismiss(), Reset(), Update(), GetCell(), and per-cell queries — in MatrixRainCore/ScrambleRevealEffect.h/.cpp
- [X] T105 Refactor HelpHintOverlay to delegate timing to ScrambleRevealEffect — remove per-character scramble/dissolve timers, use scramble-reveal Update() — in MatrixRainCore/HelpHintOverlay.h/.cpp
- [X] T106 Refactor HotkeyOverlay to delegate timing to ScrambleRevealEffect — same pattern as HelpHintOverlay — in MatrixRainCore/HotkeyOverlay.h/.cpp

### Completed Enhancements

- [X] T107 Add margin columns to HelpHintOverlay (MARGIN_COLS=1) for smooth reveal entry/exit — in MatrixRainCore/HelpHintOverlay.h/.cpp
- [X] T108 Add margin columns to HotkeyOverlay (MARGIN_COLS=2) for smooth reveal entry/exit — in MatrixRainCore/HotkeyOverlay.h/.cpp
- [X] T109 Update RenderSystem to handle margin/space columns (skip rendering when currentGlyphIndex >= allGlyphs.size()) — in MatrixRainCore/RenderSystem.cpp
- [X] T110 Replace headCol tracking with continuous head position from GetRevealProgress() — head extends past grid edge for smooth right-side settling — in HelpHintOverlay.cpp and HotkeyOverlay.cpp
- [X] T111 Add dismiss effect: random glyphs using GetDismissProgress() — in HelpHintOverlay.cpp and HotkeyOverlay.cpp
- [X] T112 Set cycling zone to full overlay width (m_cols) for extended matrix effect — in HelpHintOverlay.cpp and HotkeyOverlay.cpp

**Checkpoint**: All overlays use ScrambleRevealEffect for timing. Per-cell staggered scramble-reveal with margin columns and dismiss effect. Build succeeds (0W/0E), 318 tests pass.

---

## Phase 4: User Story 2 — Re-show Help Hint on Unrecognized Key (Priority: P2)

**Goal**: Pressing any unmapped key re-triggers the help hint. Recognized hotkeys dismiss via dissolve.

**Independent Test**: Wait for hint to dissolve → press `X` → hint reappears. Press Space while hint is visible → hint dissolves, Space action executes.

### Tests for User Story 2

- [X] T042 [P] [US2] Test Show() while Revealing resets ScrambleRevealEffect (re-trigger from reveal phase) — in MatrixRainTests/unit/HelpHintOverlayTests.cpp
- [X] T043 [P] [US2] Test Show() while Holding resets to Revealing (re-trigger from hold phase) — in MatrixRainTests/unit/HelpHintOverlayTests.cpp
- [X] T044 [P] [US2] Test Show() while Dismissing restarts ScrambleRevealEffect with a fresh reveal — in MatrixRainTests/unit/HelpHintOverlayTests.cpp
- [X] T045 [P] [US2] Test Dismiss() from Revealing transitions to Dissolving — in MatrixRainTests/unit/HelpHintOverlayTests.cpp
- [X] T046 [P] [US2] Test Dismiss() from Holding transitions to Dissolving — in MatrixRainTests/unit/HelpHintOverlayTests.cpp
- [X] T047 [P] [US2] Test Dismiss() from Dissolving is a no-op (does not restart dissolve) — in MatrixRainTests/unit/HelpHintOverlayTests.cpp
- [X] T048 [P] [US2] Test Dismiss() from Hidden is a no-op — in MatrixRainTests/unit/HelpHintOverlayTests.cpp

### Implementation for User Story 2

- [X] T049 [US2] Implement HelpHintOverlay::Dismiss() in MatrixRainCore/HelpHintOverlay.cpp — calls ScrambleRevealEffect::StartDismiss() from Revealing/Holding, no-op from Dissolving/Hidden
- [X] T050 [US2] Route unrecognized keys to HelpHintOverlay::Show() — modify InputSystem::ProcessKeyDown to call Show() for any key not in the recognized set, in MatrixRainCore/InputSystem.cpp
- [X] T051 [US2] Route recognized hotkeys to HelpHintOverlay::Dismiss() — modify Application::OnKeyDown to call Dismiss() when processing any recognized hotkey (Space, C, S, +, -, backtick, Escape, Alt+Enter), in MatrixRainCore/Application.cpp

**Checkpoint**: Unrecognized keys re-show hint, recognized hotkeys dismiss via dissolve, hotkey actions still execute normally.

---

## Phase 5: User Story 4 — Open Settings Dialog via Enter Key (Priority: P2)

**Goal**: Pressing Enter opens the config dialog as a live modeless overlay.

**Independent Test**: Launch MatrixRain → press Enter → config dialog opens as modeless overlay. Press Enter again → no second dialog. Help hint dissolves when Enter is pressed.

### Tests for User Story 4

- [X] T052 [P] [US4] Test that Enter key (VK_RETURN) triggers config dialog open in Application::OnKeyDown — in MatrixRainTests/unit/InputSystemTests.cpp (or ApplicationTests if exists)
- [X] T053 [P] [US4] Test that Enter key is recognized as a hotkey (calls Dismiss on overlay, not Show) — in MatrixRainTests/unit/HelpHintOverlayTests.cpp

### Implementation for User Story 4

- [X] T054 [US4] Handle VK_RETURN in Application::OnKeyDown — open config dialog as modeless overlay (same behavior as /c), guard against opening twice — in MatrixRainCore/Application.cpp
- [X] T055 [US4] Ensure Enter calls HelpHintOverlay::Dismiss() as a recognized hotkey — in MatrixRainCore/Application.cpp

**Checkpoint**: Enter key opens config dialog and dismisses help hint via dissolve.

---

## Phase 6: User Story 5 — Show Hotkey Reference via ? Key (Priority: P2)

**Goal**: Pressing `?` displays a hotkey reference overlay rendered directly on the main MatrixRain window — NOT in a separate dialog. This is distinct from `/?` which uses `UsageDialog`.

**Independent Test**: Launch MatrixRain → press `?` → hotkey reference overlay appears directly on the main window showing all runtime hotkeys with descriptions. Press any key → overlay begins dissolving. Help hint dissolves when `?` is pressed.

### Superseded Tasks (Win32 Dialog Approach Abandoned)

- [X] ~~T058 Add usage dialog resource template — SUPERSEDED (using in-app overlay instead of dialog)~~
- [X] ~~T059 Implement usage dialog proc — SUPERSEDED (using in-app overlay instead of dialog)~~

### Superseded Tasks (UsageDialog Approach Abandoned for ? Key)

- [X] ~~T078 Wire ? key to UsageDialog — SUPERSEDED (? key uses in-app overlay, not UsageDialog)~~
- [X] ~~T079 Remove old usage dialog resource template — SUPERSEDED (already cleaned up)~~

### Retained Tests for User Story 5

- [X] T056 [P] [US5] Test that `?` key (VK_OEM_2 with Shift) triggers usage display in Application::OnKeyDown
- [X] T057 [P] [US5] Test that `?` key is recognized as a hotkey (calls Dismiss on overlay, not Show)

### New Tests for User Story 5 (In-App Overlay)

> **Write tests FIRST, verify they FAIL, then implement**

- [X] T090 [P] [US5] Test HotkeyOverlay initial state: phase is Hidden, IsActive() returns false — in MatrixRainTests/unit/HotkeyOverlayTests.cpp
- [X] T091 [P] [US5] Test HotkeyOverlay::Show() transitions phase to Visible, builds hotkey list with all runtime hotkeys and descriptions — in MatrixRainTests/unit/HotkeyOverlayTests.cpp
- [X] T092 [P] [US5] Test HotkeyOverlay::Dismiss() transitions phase to Dissolving, no-op if already Hidden — in MatrixRainTests/unit/HotkeyOverlayTests.cpp
- [X] T093 [P] [US5] Test HotkeyOverlay::Update() during Dissolving: opacity decreases, transitions to Hidden when fully faded — in MatrixRainTests/unit/HotkeyOverlayTests.cpp

### New Implementation for User Story 5 (In-App Overlay)

- [X] T094 [US5] Implement HotkeyOverlay class declaration in MatrixRainCore/HotkeyOverlay.h — phase enum (Hidden/Visible/Dissolving), hotkey list, opacity, bounding rect, positioning
- [X] T095 [US5] Implement HotkeyOverlay state machine and Update() in MatrixRainCore/HotkeyOverlay.cpp — Show()/Dismiss()/Update() with dissolve animation
- [X] T096 [US5] Render HotkeyOverlay in RenderSystem — draw hotkey list with D2D/DirectWrite, feathered dark glow for readability, centered on screen — in MatrixRainCore/RenderSystem.cpp
- [X] T097 [US5] Wire ? key to HotkeyOverlay in Application::OnKeyDown — show overlay when pressed, guard against duplicate, call HelpHintOverlay::Dismiss() — in MatrixRainCore/Application.cpp

### Retained Implementation

- [X] T060 [US5] Handle VK_OEM_2 (with Shift check) in Application::OnKeyDown — open usage display, guard against duplicate, call HelpHintOverlay::Dismiss()

**Checkpoint**: `?` key shows hotkey reference overlay directly on the main window. No separate dialog. Overlay has scramble-reveal animation matching HelpHintOverlay. Help hint dissolves when `?` is pressed.

---

## Phase 8: Visual Tuning — Animation Quality (Priority: P2)

**Goal**: Tune the scramble-reveal effect for all overlays until the visual quality matches the desired matrix rain aesthetic.

**Status**: IN PROGRESS — Scramble-reveal architecture is working (reveal + dismiss + margins), but visual quality needs iteration.

### Known Issues (from manual testing 2026-03-06)

- [X] T113 [US1/US5] Evaluate and tune reveal speed and per-cell stagger — tuned to revealDuration=1.5, cycleInterval=0.25, flashDuration=1.0. Cycle timers staggered per-cell with random offset.
- [X] T114 [US1/US5] Evaluate margin column visual effect — margins provide smooth entry/exit. MARGIN_COLS=1 for help hint, MARGIN_COLS=2 for hotkey/usage.
- [X] T115 [US1/US5] Evaluate whether characters in cycling phase should cycle through multiple random glyphs — kept cycling at cycleInterval with staggered timers for smooth visual.
- [X] T116 [US1/US5] Evaluate dismiss visual quality — dismiss at 1.0s duration looks natural with staggered per-cell fade.
- [X] T117 [US1/US5] Evaluate ComputeScrambleColor transitions — cycling/dismissing at 70% scheme color, flash decays to full scheme, pulse ramp 0.33/0.75/0.5s.
- [X] T118 [US1/US5] Evaluate margin column counts — MARGIN_COLS=1 for help hint, MARGIN_COLS=2 for hotkey and usage overlays. Provides good visual entry/exit.

**Checkpoint**: Scramble-reveal animations are visually polished and match the desired matrix rain aesthetic.

---

## Phase 7: Polish & Cross-Cutting Concerns

**Purpose**: Cleanup, build integration, and final validation

### Superseded Tasks

- [X] ~~T061 Console rain timing test — SUPERSEDED (no console rain)~~
- [X] ~~T062 Narrow terminal test — SUPERSEDED (no console rain)~~
- [X] ~~T063 AnsiCodes.h vcxproj — SUPERSEDED (AnsiCodes.h not needed for this feature)~~

### Active Tasks

- [X] T080 Add UsageDialog.h/.cpp, HotkeyOverlay.h/.cpp, UnicodeSymbols.h to MatrixRainCore.vcxproj and UsageDialogTests.cpp, HotkeyOverlayTests.cpp to MatrixRainTests.vcxproj — verify build with `MSBuild MatrixRain.sln /p:Configuration=Debug /p:Platform=x64`
- [X] T081 Run full test suite and fix any failures — `vstest.console.exe x64\Debug\MatrixRainTests.dll`
- [X] T082 Run manual validation scenarios — overlay startup, `/?` usage dialog (switches only, proportional font, scramble-reveal animation), `-?` usage dialog, screensaver mode suppression, Enter key config dialog, `?` key hotkey overlay (in-app, not dialog)

---

## Dependencies & Execution Order

### Phase Dependencies

- **User Story 3 (Phase 2)**: No external dependencies — **MVP, start here**. Retained tasks (UsageText, CommandLine, main.cpp wiring) already complete. New work: UsageText section grouping (T086–T088), UsageDialog (T067–T077, T083–T085, T089)
- **User Story 1 (Phase 3)**: Independent of US3 — already complete
- **User Story 2 (Phase 4)**: Depends on User Story 1 (needs HelpHintOverlay with Show/Dismiss) — already complete
- **User Story 4 (Phase 5)**: Depends on User Story 2 (needs Dismiss() wired for recognized hotkeys) — already complete
- **User Story 5 (Phase 6)**: Independent of US3 (uses its own HotkeyOverlay, NOT UsageDialog) — new tasks T090–T097
- **Polish (Phase 7)**: Depends on all user stories being complete — new tasks T080–T082

### Remaining Work Dependency Graph

```
US3 UsageDialog (T067–T077, T083–T089) 🎯 MVP
   └──→ Polish (T080–T082)

US5 HotkeyOverlay (T090–T097) [independent of US3]
   └──→ Polish (T080–T082)
```

### Within New Tasks

- Tests MUST be written and FAIL before implementation
- Per Constitution I: Red → Green → Refactor cycle
- Core logic before integration wiring

### Parallel Opportunities

- **Phase 2 new tests**: T067–T069, T083, T085–T086 all [P] — independent test methods
- **Phase 2 implementation**: T087–T088 first (UnicodeSymbols, UsageText update), then T070–T075, T089, T084 sequential (each builds on previous), T076–T077 after T075
- **Phase 6 new tests**: T090–T093 all [P] — independent test methods
- **Phase 6 implementation**: T094–T097 sequential

---

## Parallel Example: UsageDialog (MVP)

```
# Tests (all parallel — independent methods):
T067: Window sizing test (2x bounding box, 80% cap)
T068: Scramble-reveal animation state test (per-cell cycling and lock)
T069: IsRevealComplete() test (all revealedFlags true)
T083: Phase 2 background rain test (density/speed back off)
T085: Scramble-reveal timing test (completes within kRevealDuration)
T086: UsageText section grouping test (Options + Screensaver Options)

# Implementation (sequential — each builds on previous):
T087: Create UnicodeSymbols.h (em dash, etc.)
T088: Update UsageText (section grouping, remove grid/map)
T070: Class declaration (UsageDialog.h) — characterPositions, ScrambleRevealEffect
T071: Window creation (2x bounding box, 80% cap, centered)
T072: D3D11/D2D initialization (Segoe UI proportional, DPI-scaled)
T089: Character position computation (HitTestTextPosition)
T073: Scramble-reveal animation — per-cell cycling, lock, ComputeScrambleColor
T084: Phase 2 render — background rain density/speed back off
T074: Input handling (WndProc)
T075: Show() message pump
T076: Update CommandLineHelp orchestration
T077: Remove dead console code
```

## Post-MVP: US5 (Hotkey Overlay) + Polish

```
After UsageDialog is working for /?:

# US5 Tests (all parallel):
T090: HotkeyOverlay initial state
T091: Show() transitions and hotkey list
T092: Dismiss() transitions
T093: Update() dissolve animation

# US5 Implementation (sequential):
T094: HotkeyOverlay class declaration
T095: State machine and Update()
T096: Render in RenderSystem
T097: Wire ? key to HotkeyOverlay

# Polish:
T080: Add new files to vcxproj
T081: Run full test suite
T082: Manual validation
```

---

## Implementation Strategy

### MVP First (User Story 3 — UsageDialog)

1. Create UnicodeSymbols.h (T087) and update UsageText for section grouping (T088)
2. Write UsageDialog tests (T067–T069, T083, T085) — verify they fail
3. Implement UsageDialog class (T070–T075, T089, T084) — window (with close button), D3D/D2D (Segoe UI proportional), character position computation (HitTestTextPosition), scramble-reveal animation (ScrambleRevealEffect + ComputeScrambleColor + background rain), input, Show()
4. Update CommandLineHelp orchestration (T076) — wire UsageDialog into `/?` path
5. Remove dead console code (T077) — clean up ConsoleRainEffect, console attachment, Ctrl+C handler, BuildTextGrid/BuildColumnActivityMap
6. **STOP and VALIDATE**: `/?` launches usage dialog with proportional font, scramble-reveal animation (per-cell cycling and lock via ScrambleRevealEffect), switches only (no hotkeys), enter/esc/close dismisses, app exits

### Incremental Delivery

1. US3 UsageDialog → `/?` works with scramble-reveal usage dialog and proportional font → **MVP!**
2. US5 → `?` key shows hotkey reference as in-app overlay on the main window (NOT dialog)
3. Polish → Files in vcxproj, test suite, manual validation

---

## Notes

- [P] tasks = different files or independent test methods, no dependencies
- [Story] label maps task to specific user story for traceability
- Constitution I (TDD): ALL core library code follows Red-Green-Refactor
- Constitution VI (Library-First): ALL logic in MatrixRainCore.lib — .exe only gets main.cpp wiring
- Constitution IX (Commit Discipline): One task = one commit, each commit builds and passes all tests
- UsageDialog creates its own D3D11 device/swap chain — independent of main app's rendering context
- UsageDialog uses proportional font (Segoe UI 14-16pt DPI-scaled) with per-character positioning via IDWriteTextLayout::HitTestTextPosition
- UsageDialog uses ScrambleRevealEffect for per-cell scramble-reveal animation with background matrix rain
- Rain fills the entire window — no column grid, random pixel x-positions, streaks pass through text area
- Resolved text uses feathered dark glow (DrawFeatheredGlow technique) for readability against rain
- `/?` content: switches only (Options + Screensaver Options), no hotkeys
- `?` key: in-app overlay (HotkeyOverlay) on the main window, NOT UsageDialog
- UsageText serves `/?` only — HelpHintOverlay has its own hardcoded 3-line layout, HotkeyOverlay has its own hotkey list
- UnicodeSymbols.h provides named constants for em dash and other Unicode characters
- Superseded tasks (T001, T008–T009, T014–T019, T058–T059, T061–T063) remain listed for traceability but are struck through

### Architectural Note: ScrambleRevealEffect (added 2026-03-06)

The original spec described per-character random resolve/dissolve (scramble timers, random selection for dissolve, per-character DissolveCycling/DissolveFading phases). During implementation this was replaced with a scramble-reveal model:

- **ScrambleRevealEffect** is a shared timing oracle with per-cell staggered timing
- **Reveal**: all non-space cells start cycling through random glyphs, each locks onto its target at a random time within revealDuration
- **Dismiss**: settled cells randomly unlock back into cycling, then fade out
- **CellPhase** has 5 values: Hidden, Cycling, LockFlash, Settled, Dismissing
- **CharPhase** (overlay-level) mirrors CellPhase: Hidden, Cycling, LockFlash, Settled, Dismissing
- **Color**: ComputeScrambleColor drives per-cell color — Cycling=dark green, LockFlash=yellow→mid green, Settled=mid green→white pulse→grey, Dismissing=dark green
- **Margin columns**: invisible columns on each side for smooth reveal entry/exit at edges
- All overlays' `Update()` methods share identical reveal logic: query per-cell state from ScrambleRevealEffect to determine phase, glyph, and color
