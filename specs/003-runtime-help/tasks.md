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

## Phase 2: User Story 3 — Command-Line Help with Graphical Rain Dialog (Priority: P1) 🎯 MVP

**Goal**: `MatrixRain.exe /?` displays a custom graphical window with matrix rain animation that reveals the usage text, then exits after the user dismisses it.

**Independent Test**: Run `MatrixRain.exe /?` → custom window opens with matrix rain animation revealing usage text → press Enter or Esc to dismiss → app exits. Run `MatrixRain.exe -?` → same behavior with `-` prefix.

### Superseded Tasks (Console Approach Abandoned)

The following tasks were completed under the original console-based approach but are now superseded by the graphical rain dialog:

- [X] ~~T008 ConsoleRainEffect construction tests — SUPERSEDED (replaced by HelpRainDialog)~~
- [X] ~~T009 ConsoleRainEffect shutdown test — SUPERSEDED (replaced by HelpRainDialog)~~
- [X] ~~T014 ConsoleRainEffect constructor — SUPERSEDED (replaced by HelpRainDialog)~~
- [X] ~~T015 IsInteractiveConsole/EnableVTProcessing — SUPERSEDED (no console detection)~~
- [X] ~~T016 ConsoleRainEffect::Run() — SUPERSEDED (replaced by HelpRainDialog render loop)~~
- [X] ~~T017 AttachParentConsole — SUPERSEDED (no console attachment)~~
- [X] ~~T018 DisplayCommandLineHelp orchestration — SUPERSEDED (simplified, no console paths)~~
- [X] ~~T019 ConsoleCtrlHandler — SUPERSEDED (no Ctrl+C handling needed)~~

### Retained Tests for User Story 3

> Previously completed and still valid:

- [X] T002 [P] [US3] Test UsageText construction with `/` prefix
- [X] T003 [P] [US3] Test UsageText construction with `-` prefix
- [X] T004 [P] [US3] Test UsageText::GetPlainText() returns all lines joined with CRLF, no ANSI codes
- [X] T005 [P] [US3] Test UsageText::DetectSwitchPrefix()
- [X] T010 [P] [US3] Test ScreenSaverModeParser handles `/?` and `-?` arguments

### Superseded Tests for User Story 3

- [X] ~~T006 Test UsageText::BuildTextGrid() — SUPERSEDED (proportional font, no text grid)~~
- [X] ~~T007 Test UsageText::BuildColumnActivityMap() — SUPERSEDED (no column activity map, random pixel x)~~

### Retained Implementation for User Story 3

> Previously completed and still valid:

- [X] T011 [US3] Implement UsageText constructor and queries
- [X] T012 [US3] Implement UsageText::DetectSwitchPrefix()
- [X] T020 [US3] Add `/?`/`-?` parsing to ScreenSaverModeParser
- [X] T021 [US3] Wire CLI help early exit in main.cpp

### Superseded Implementation for User Story 3

- [X] ~~T013 Implement UsageText::BuildTextGrid() and BuildColumnActivityMap() — SUPERSEDED (proportional font, no text grid or column map)~~

### New Tests for HelpRainDialog

> **Write tests FIRST, verify they FAIL, then implement**

- [ ] T067 [P] [US3] Test HelpRainDialog window sizing — ComputeWindowSize() returns 2x text bounding box dimensions (from IDWriteTextLayout metrics), capped at 80% of primary monitor work area per dimension — in MatrixRainTests/unit/HelpRainDialogTests.cpp
- [ ] T068 [P] [US3] Test HelpRainDialog Phase 1 (reveal) animation state — pre-compute CharPosition for every non-space character, shuffle into revealQueue, spawn RevealStreak at each character's (x, y), character locks in when streak head reaches targetPixelY — in MatrixRainTests/unit/HelpRainDialogTests.cpp
- [ ] T069 [P] [US3] Test HelpRainDialog::IsRevealComplete() returns true when all revealedFlags are true — in MatrixRainTests/unit/HelpRainDialogTests.cpp
- [ ] T083 [P] [US3] Test HelpRainDialog Phase 2 (background) — after reveal completes, decorative streaks density and speed reduce via tunable multipliers (kPhase2DensityMultiplier, kPhase2SpeedMultiplier), decorative streaks never lock in characters — in MatrixRainTests/unit/HelpRainDialogTests.cpp
- [ ] T085 [P] [US3] Test HelpRainDialog reveal queue drain rate — verify queue drains completely within kRevealDurationSeconds (3.0f), fixed rate = N characters / duration — in MatrixRainTests/unit/HelpRainDialogTests.cpp
- [ ] T086 [P] [US3] Test UsageText section grouping — GetFormattedText() produces "Options:" section with /c and /?, "Screensaver Options:" section with /s, /p, /a — no hotkeys — in MatrixRainTests/unit/UsageTextTests.cpp

### New Implementation for HelpRainDialog

- [ ] T087 [US3] Create UnicodeSymbols.h in MatrixRain/ — namespace UnicodeSymbols with static constexpr WCHAR constants for em dash (\u2014) and other Unicode symbols, following TCDir's pattern
- [ ] T088 [US3] Update UsageText to produce section-grouped content — "Options:" section (/c, /?), "Screensaver Options:" section (/s, /p, /a), no hotkeys, em dash from UnicodeSymbols.h, remove BuildTextGrid()/BuildColumnActivityMap() — in MatrixRainCore/UsageText.h/.cpp
- [ ] T070 [US3] Implement HelpRainDialog class declaration in MatrixRainCore/HelpRainDialog.h — window handle, D3D11 device/swap chain, D2D render target, revealQueue, characterPositions (vector<CharPosition>), revealedFlags, activeRevealStreaks, decorativeStreaks, D2D brushes (text/glow/head/trail), textLayout (IDWriteTextLayout), animation phase enum (Revealing/Background), tunable constants
- [ ] T071 [US3] Implement HelpRainDialog window creation in MatrixRainCore/HelpRainDialog.cpp — register WNDCLASS, create popup window with WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU (close button, non-resizable), size to 2x text bounding box capped at 80% work area, center on primary monitor
- [ ] T072 [US3] Implement HelpRainDialog D3D11 and D2D initialization in MatrixRainCore/HelpRainDialog.cpp — create device, swap chain, DXGI surface, D2D render target, DirectWrite Segoe UI proportional text format (14-16pt DPI-scaled), IDWriteTextLayout for formatted text, create brushes: white text brush, black glow brush (variable opacity), bright green head brush, dim green trail brush
- [ ] T089 [US3] Implement HelpRainDialog character position computation in MatrixRainCore/HelpRainDialog.cpp — iterate every non-space character in formatted text, call IDWriteTextLayout::HitTestTextPosition to get (x, y), build characterPositions vector, shuffle indices into revealQueue, compute drain rate = N / kRevealDurationSeconds
- [ ] T073 [US3] Implement HelpRainDialog Phase 1 render logic in MatrixRainCore/HelpRainDialog.cpp — drain revealQueue at fixed rate, spawn RevealStreak at each dequeued character's (x, y) with kStreakLeadCells above and kStreakTrailCells below, lock in character when head reaches targetPixelY (set revealedFlags), render streak heads (bright green/white random glyph) and trails (dimming green), render resolved characters with feathered dark glow (DrawFeatheredGlow technique), render decorative streaks at random pixel x across full window at same density/speed, transition to Phase 2 when all revealedFlags true
- [ ] T084 [US3] Implement HelpRainDialog Phase 2 render logic in MatrixRainCore/HelpRainDialog.cpp — reduce decorative streak density via kPhase2DensityMultiplier (0.15f) and increase speed via kPhase2SpeedMultiplier (1.5f), continue rendering resolved text with feathered glow, rain fills entire window — streaks pass through text area
- [ ] T074 [US3] Implement HelpRainDialog input handling in MatrixRainCore/HelpRainDialog.cpp — WndProc handles WM_KEYDOWN (VK_RETURN, VK_ESCAPE to dismiss), WM_CLOSE, WM_DESTROY, close button (X)
- [ ] T075 [US3] Implement HelpRainDialog::Show() in MatrixRainCore/HelpRainDialog.cpp — show window, run PeekMessage-based message pump with render-on-idle pattern, block until dismissed
- [ ] T076 [US3] Update CommandLineHelp::DisplayCommandLineHelp() in MatrixRainCore/CommandLineHelp.cpp — create UsageText, create HelpRainDialog with const UsageText&, call Show(), no console attachment or detection
- [ ] T077 [US3] Remove dead console code — delete ConsoleRainEffect.h/.cpp, remove AttachParentConsole/ConsoleCtrlHandler from CommandLineHelp, remove BuildTextGrid/BuildColumnActivityMap from UsageText if still present, remove AnsiCodes.h if unused elsewhere

**Checkpoint**: `/?` and `-?` launch a custom graphical window with proportional font, per-character queued reveal (~3s), two independent streak pools (reveal + decorative), feathered dark glow, switches only (Options + Screensaver Options, no hotkeys). Enter/Esc dismisses. No console output. MVP complete.

---

## Phase 3: User Story 1 — Runtime Help Hint on Startup (Priority: P2)

**Goal**: First-time user sees a centered three-line help hint with matrix rain reveal/dissolve effect. Rain streaks pass behind the message area with feathered border.

**Independent Test**: Launch `MatrixRain.exe` with no arguments → centered help hint appears with matrix reveal effect, holds briefly, dissolves with staggered per-character rain cycling and fade-out. Rain streaks pass behind the message area.

### Setup for Overlay Types

- [X] T022 [P] [US1] Define OverlayPhase enum class (Hidden, Revealing, Holding, Dissolving) and CharPhase enum class (Scrambling, Resolved, DissolveCycling, DissolveFading, Hidden) in MatrixRainCore/HelpHintOverlay.h
- [X] T023 [P] [US1] Define HintCharacter struct (targetGlyphIndex, currentGlyphIndex, phase, opacity, scrambleTimer, scrambleInterval, dissolveStartOffset, row, col, isSpace) in MatrixRainCore/HelpHintOverlay.h

### Tests for User Story 1

> **Write tests FIRST, verify they FAIL, then implement**

- [X] T024 [P] [US1] Test HelpHintOverlay initial state: phase is Hidden, IsActive() returns false, characters empty — in MatrixRainTests/unit/HelpHintOverlayTests.cpp
- [X] T025 [P] [US1] Test Show() transitions phase to Revealing, IsActive() returns true, characters initialized to Scrambling with staggered timers (spaces marked isSpace) — in MatrixRainTests/unit/HelpHintOverlayTests.cpp
- [X] T026 [P] [US1] Test Update() during Revealing: scrambling characters cycle currentGlyphIndex, characters resolve to targetGlyphIndex when scrambleTimer expires — in MatrixRainTests/unit/HelpHintOverlayTests.cpp
- [X] T027 [P] [US1] Test Revealing→Holding transition: once all characters are Resolved, overlay phase becomes Holding — in MatrixRainTests/unit/HelpHintOverlayTests.cpp
- [X] T028 [P] [US1] Test Holding→Dissolving transition: after holdDuration elapses, overlay phase becomes Dissolving, characters begin transitioning to DissolveCycling with staggered dissolveStartOffset — in MatrixRainTests/unit/HelpHintOverlayTests.cpp
- [X] T029 [P] [US1] Test Dissolving→Hidden transition: all characters progress DissolveCycling→DissolveFading→Hidden, once all Hidden the overlay phase becomes Hidden — in MatrixRainTests/unit/HelpHintOverlayTests.cpp
- [X] T030 [P] [US1] Test UpdateLayout() computes centered bounding rect for given viewport dimensions — in MatrixRainTests/unit/HelpHintOverlayTests.cpp
- [X] T031 [P] [US1] Test GetCharacters() returns span with correct row/col layout matching the three-line hint format — in MatrixRainTests/unit/HelpHintOverlayTests.cpp

### Implementation for User Story 1

- [X] T032 [US1] Implement HelpHintOverlay constructor, Show(), Hide(), and IsActive()/GetPhase()/GetRows()/GetCols() in MatrixRainCore/HelpHintOverlay.cpp — initialize 3-line hint text grid ("Settings Enter" / "Help ?" / "Exit Esc") with right-justified left column and left-justified right column
- [X] T033 [US1] Implement HelpHintOverlay::Update() reveal logic in MatrixRainCore/HelpHintOverlay.cpp — scramble cycling, per-character resolution with staggered timers, Revealing→Holding transition
- [X] T034 [US1] Implement HelpHintOverlay::Update() hold and dissolve logic in MatrixRainCore/HelpHintOverlay.cpp — hold timer countdown, Dissolving phase with per-character DissolveCycling→DissolveFading→Hidden, overlay Hidden when all characters done
- [X] T035 [US1] Implement HelpHintOverlay::UpdateLayout() and GetBoundingRect() in MatrixRainCore/HelpHintOverlay.cpp — center bounding rect in viewport, recompute on resize
- [X] T036 [US1] Implement HelpHintOverlay::GetCharacters() returning span<const HintCharacter> in MatrixRainCore/HelpHintOverlay.cpp
- [X] T037 [US1] Add HelpHintOverlay member to Application, call Show() on startup in Normal mode only (skip for /s, /p, /c, /a) in MatrixRainCore/Application.cpp — wire UpdateLayout() on resize
- [X] T038 [US1] Add HelpHintOverlay::Update() call in render loop (under m_renderMutex) in MatrixRainCore/Application.cpp
- [X] T039 [US1] Render HelpHintOverlay in RenderSystem — iterate GetCharacters(), draw each non-hidden character using D2D/DirectWrite with current glyph and opacity, draw feathered border using DrawFeatheredGlow() — in MatrixRainCore/RenderSystem.cpp
- [X] T040 [US1] Add rain occlusion in AnimationSystem — pass HelpHintOverlay::GetBoundingRect() as exclusion zone, skip rendering rain streak characters inside the rect when overlay IsActive() — in MatrixRainCore/AnimationSystem.cpp and MatrixRainCore/RenderSystem.cpp
- [X] T041 [US1] Add help hint enabled flag to ApplicationState in MatrixRainCore/ApplicationState.h — set based on screensaver mode in Application startup

**Checkpoint**: Help hint overlay displays on startup with full reveal/hold/dissolve animation, rain occlusion, and feathered border. Screensaver modes suppressed.

---

## Phase 4: User Story 2 — Re-show Help Hint on Unrecognized Key (Priority: P2)

**Goal**: Pressing any unmapped key re-triggers the help hint. Recognized hotkeys dismiss via dissolve.

**Independent Test**: Wait for hint to dissolve → press `X` → hint reappears. Press Space while hint is visible → hint dissolves, Space action executes.

### Tests for User Story 2

- [X] T042 [P] [US2] Test Show() while Revealing resets all characters to Scrambling (re-trigger from reveal phase) — in MatrixRainTests/unit/HelpHintOverlayTests.cpp
- [X] T043 [P] [US2] Test Show() while Holding resets to Revealing (re-trigger from hold phase) — in MatrixRainTests/unit/HelpHintOverlayTests.cpp
- [X] T044 [P] [US2] Test Show() while Dissolving reverses the dissolve — characters at DissolveFading reverse opacity trend and materialize back, characters at DissolveCycling transition to Scrambling, fully Hidden characters restart from Scrambling — in MatrixRainTests/unit/HelpHintOverlayTests.cpp
- [X] T045 [P] [US2] Test Dismiss() from Revealing transitions to Dissolving — in MatrixRainTests/unit/HelpHintOverlayTests.cpp
- [X] T046 [P] [US2] Test Dismiss() from Holding transitions to Dissolving — in MatrixRainTests/unit/HelpHintOverlayTests.cpp
- [X] T047 [P] [US2] Test Dismiss() from Dissolving is a no-op (does not restart dissolve) — in MatrixRainTests/unit/HelpHintOverlayTests.cpp
- [X] T048 [P] [US2] Test Dismiss() from Hidden is a no-op — in MatrixRainTests/unit/HelpHintOverlayTests.cpp

### Implementation for User Story 2

- [X] T049 [US2] Implement HelpHintOverlay::Dismiss() in MatrixRainCore/HelpHintOverlay.cpp — transition to Dissolving from Revealing/Holding, no-op from Dissolving/Hidden
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

**Goal**: Pressing `?` displays a hotkey reference overlay rendered directly on the main MatrixRain window — NOT in a separate dialog. This is distinct from `/?` which uses `HelpRainDialog`.

**Independent Test**: Launch MatrixRain → press `?` → hotkey reference overlay appears directly on the main window showing all runtime hotkeys with descriptions. Press any key → overlay begins dissolving. Help hint dissolves when `?` is pressed.

### Superseded Tasks (Win32 Dialog Approach Abandoned)

- [X] ~~T058 Add usage dialog resource template — SUPERSEDED (using in-app overlay instead of dialog)~~
- [X] ~~T059 Implement usage dialog proc — SUPERSEDED (using in-app overlay instead of dialog)~~

### Superseded Tasks (HelpRainDialog Approach Abandoned for ? Key)

- [X] ~~T078 Wire ? key to HelpRainDialog — SUPERSEDED (? key uses in-app overlay, not HelpRainDialog)~~
- [X] ~~T079 Remove old usage dialog resource template — SUPERSEDED (already cleaned up)~~

### Retained Tests for User Story 5

- [X] T056 [P] [US5] Test that `?` key (VK_OEM_2 with Shift) triggers usage display in Application::OnKeyDown
- [X] T057 [P] [US5] Test that `?` key is recognized as a hotkey (calls Dismiss on overlay, not Show)

### New Tests for User Story 5 (In-App Overlay)

> **Write tests FIRST, verify they FAIL, then implement**

- [ ] T090 [P] [US5] Test HotkeyOverlay initial state: phase is Hidden, IsActive() returns false — in MatrixRainTests/unit/HotkeyOverlayTests.cpp
- [ ] T091 [P] [US5] Test HotkeyOverlay::Show() transitions phase to Visible, builds hotkey list with all runtime hotkeys and descriptions — in MatrixRainTests/unit/HotkeyOverlayTests.cpp
- [ ] T092 [P] [US5] Test HotkeyOverlay::Dismiss() transitions phase to Dissolving, no-op if already Hidden — in MatrixRainTests/unit/HotkeyOverlayTests.cpp
- [ ] T093 [P] [US5] Test HotkeyOverlay::Update() during Dissolving: opacity decreases, transitions to Hidden when fully faded — in MatrixRainTests/unit/HotkeyOverlayTests.cpp

### New Implementation for User Story 5 (In-App Overlay)

- [ ] T094 [US5] Implement HotkeyOverlay class declaration in MatrixRainCore/HotkeyOverlay.h — phase enum (Hidden/Visible/Dissolving), hotkey list, opacity, bounding rect, positioning
- [ ] T095 [US5] Implement HotkeyOverlay state machine and Update() in MatrixRainCore/HotkeyOverlay.cpp — Show()/Dismiss()/Update() with dissolve animation
- [ ] T096 [US5] Render HotkeyOverlay in RenderSystem — draw hotkey list with D2D/DirectWrite, feathered dark glow for readability, centered on screen — in MatrixRainCore/RenderSystem.cpp
- [ ] T097 [US5] Wire ? key to HotkeyOverlay in Application::OnKeyDown — show overlay when pressed, guard against duplicate, call HelpHintOverlay::Dismiss() — in MatrixRainCore/Application.cpp

### Retained Implementation

- [X] T060 [US5] Handle VK_OEM_2 (with Shift check) in Application::OnKeyDown — open usage display, guard against duplicate, call HelpHintOverlay::Dismiss()

**Checkpoint**: `?` key shows hotkey reference overlay directly on the main window. No separate dialog. Overlay has dissolve animation. Help hint dissolves when `?` is pressed.

---

## Phase 7: Polish & Cross-Cutting Concerns

**Purpose**: Cleanup, build integration, and final validation

### Superseded Tasks

- [X] ~~T061 Console rain timing test — SUPERSEDED (no console rain)~~
- [X] ~~T062 Narrow terminal test — SUPERSEDED (no console rain)~~
- [X] ~~T063 AnsiCodes.h vcxproj — SUPERSEDED (AnsiCodes.h not needed for this feature)~~

### Active Tasks

- [ ] T080 Add HelpRainDialog.h/.cpp, HotkeyOverlay.h/.cpp, UnicodeSymbols.h to MatrixRainCore.vcxproj and HelpRainDialogTests.cpp, HotkeyOverlayTests.cpp to MatrixRainTests.vcxproj — verify build with `MSBuild MatrixRain.sln /p:Configuration=Debug /p:Platform=x64`
- [ ] T081 Run full test suite and fix any failures — `vstest.console.exe x64\Debug\MatrixRainTests.dll`
- [ ] T082 Run manual validation scenarios — overlay startup, `/?` graphical dialog (switches only, proportional font, per-character reveal, two pools), `-?` graphical dialog, screensaver mode suppression, Enter key config dialog, `?` key hotkey overlay (in-app, not dialog)

---

## Dependencies & Execution Order

### Phase Dependencies

- **User Story 3 (Phase 2)**: No external dependencies — **MVP, start here**. Retained tasks (UsageText, ScreenSaverModeParser, main.cpp wiring) already complete. New work: UsageText section grouping (T086–T088), HelpRainDialog (T067–T077, T083–T085, T089)
- **User Story 1 (Phase 3)**: Independent of US3 — already complete
- **User Story 2 (Phase 4)**: Depends on User Story 1 (needs HelpHintOverlay with Show/Dismiss) — already complete
- **User Story 4 (Phase 5)**: Depends on User Story 2 (needs Dismiss() wired for recognized hotkeys) — already complete
- **User Story 5 (Phase 6)**: Independent of US3 (uses its own HotkeyOverlay, NOT HelpRainDialog) — new tasks T090–T097
- **Polish (Phase 7)**: Depends on all user stories being complete — new tasks T080–T082

### Remaining Work Dependency Graph

```
US3 HelpRainDialog (T067–T077, T083–T089) 🎯 MVP
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

## Parallel Example: HelpRainDialog (MVP)

```
# Tests (all parallel — independent methods):
T067: Window sizing test (2x bounding box, 80% cap)
T068: Phase 1 reveal animation state test (queued per-character reveal)
T069: IsRevealComplete() test (all revealedFlags true)
T083: Phase 2 decorative rain test (density/speed back off)
T085: Reveal queue drain rate test (guaranteed 3s)
T086: UsageText section grouping test (Options + Screensaver Options)

# Implementation (sequential — each builds on previous):
T087: Create UnicodeSymbols.h (em dash, etc.)
T088: Update UsageText (section grouping, remove grid/map)
T070: Class declaration (HelpRainDialog.h) — revealQueue, characterPositions, two pool types
T071: Window creation (2x bounding box, 80% cap, centered)
T072: D3D11/D2D initialization (Segoe UI proportional, DPI-scaled)
T089: Character position computation (HitTestTextPosition, shuffle queue)
T073: Phase 1 render — drain reveal queue, spawn streaks, feathered glow
T084: Phase 2 render — decorative rain density/speed back off
T074: Input handling (WndProc)
T075: Show() message pump
T076: Update CommandLineHelp orchestration
T077: Remove dead console code
```

## Post-MVP: US5 (Hotkey Overlay) + Polish

```
After HelpRainDialog is working for /?:

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

### MVP First (User Story 3 — HelpRainDialog)

1. Create UnicodeSymbols.h (T087) and update UsageText for section grouping (T088)
2. Write HelpRainDialog tests (T067–T069, T083, T085) — verify they fail
3. Implement HelpRainDialog class (T070–T075, T089, T084) — window (with close button), D3D/D2D (Segoe UI proportional), character position computation (HitTestTextPosition), Phase 1 queued reveal (two independent pools + feathered glow), Phase 2 decorative rain (density/speed back off), input, Show()
4. Update CommandLineHelp orchestration (T076) — wire HelpRainDialog into `/?` path
5. Remove dead console code (T077) — clean up ConsoleRainEffect, console attachment, Ctrl+C handler, BuildTextGrid/BuildColumnActivityMap
6. **STOP and VALIDATE**: `/?` launches graphical rain dialog with proportional font, per-character queued reveal (~3s), two independent streak pools, switches only (no hotkeys), enter/esc/close dismisses, app exits

### Incremental Delivery

1. US3 HelpRainDialog → `/?` works with two-pool graphical rain dialog and proportional font → **MVP!**
2. US5 → `?` key shows hotkey reference as in-app overlay on the main window (NOT dialog)
3. Polish → Files in vcxproj, test suite, manual validation

---

## Notes

- [P] tasks = different files or independent test methods, no dependencies
- [Story] label maps task to specific user story for traceability
- Constitution I (TDD): ALL core library code follows Red-Green-Refactor
- Constitution VI (Library-First): ALL logic in MatrixRainCore.lib — .exe only gets main.cpp wiring
- Constitution IX (Commit Discipline): One task = one commit, each commit builds and passes all tests
- HelpRainDialog creates its own D3D11 device/swap chain — independent of main app's rendering context
- HelpRainDialog uses proportional font (Segoe UI 14-16pt DPI-scaled) with per-character positioning via IDWriteTextLayout::HitTestTextPosition
- HelpRainDialog has two independent streak pools: reveal (queued per-character, guaranteed 3s) and decorative (random pixel x, backs off in Phase 2)
- Rain fills the entire window — no column grid, random pixel x-positions, streaks pass through text area
- Resolved text uses feathered dark glow (DrawFeatheredGlow technique) for readability against rain
- `/?` content: switches only (Options + Screensaver Options), no hotkeys
- `?` key: in-app overlay (HotkeyOverlay) on the main window, NOT HelpRainDialog
- UsageText serves `/?` only — HelpHintOverlay has its own hardcoded 3-line layout, HotkeyOverlay has its own hotkey list
- UnicodeSymbols.h provides named constants for em dash and other Unicode characters
- Superseded tasks (T001, T008–T009, T014–T019, T058–T059, T061–T063) remain listed for traceability but are struck through
