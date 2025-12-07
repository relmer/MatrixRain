# Tasks: MatrixRain Screensaver Experience

**Input**: Design documents from `/specs/002-add-screensaver-support/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/

**Tests**: Add test tasks only where TDD or regression coverage is required by the plan. Tests listed below must be authored and executed before implementing dependent code.

**Organization**: Tasks are grouped by user story so each increment remains independently buildable and testable.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Task can proceed in parallel (distinct files, no unmet dependencies)
- **[Story]**: Maps work to a specific user story (US1, US2, ...); omitted for setup/foundational/polish
- Include exact file paths in every task description to avoid ambiguity

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Validate the existing solution before introducing screensaver changes.

- [X] T001 Run baseline debug build via `scripts/Invoke-MatrixRainBuild.ps1` to snapshot current compiler state
- [X] T002 Run baseline test suite with `scripts/Invoke-MatrixRainTests.ps1` to capture pre-change failures

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Establish shared enums, state structures, and build outputs required by all user stories. **All tasks in this phase must complete before story work begins.**

- [X] T003 Define `ScreenSaverMode` enum in `MatrixRainCore/ScreenSaverMode.h`
- [X] T004 Create `ScreenSaverModeContext` struct in `MatrixRainCore/ScreenSaverModeContext.h` with flags for hotkeys, cursor, and exit rules
- [X] T005 Introduce `InputExitState` helpers in `MatrixRainCore/InputExitState.h` for tracking mouse thresholds and input triggers
- [X] T006 Author `ScreenSaverSettings` data contract with defaults and clamp helpers in `MatrixRainCore/ScreenSaverSettings.h`
- [X] T007 Add failing unit coverage for `ScreenSaverSettings` defaults/clamps in `MatrixRainTests/unit/ScreenSaverSettingsTests.cpp`
- [X] T008 Add `.scr` copy `AfterBuild` target to `MatrixRain/MatrixRain.vcxproj` mirroring the executable each configuration
- [X] T009 Update `scripts/Invoke-MatrixRainBuild.ps1` to assert both `MatrixRain.exe` and `MatrixRain.scr` exist post-build

---

## Phase 3: User Story 1 - Launch Screensaver From Windows (Priority: P1) 🎯 MVP

**Goal**: When Windows invokes MatrixRain with `/s`, it must run full screen, hide the cursor, suppress hotkeys, and exit gracefully on user input using stored settings.

**Independent Test**: Execute `MatrixRain.scr /s` and verify the animation launches full screen with persisted settings, hides the cursor, and exits immediately on keyboard or meaningful mouse input while restoring cursor visibility.

### Implementation & Tests for User Story 1 (write tests first)

- [X] T010 [P] [US1] Add failing CLI parsing tests for `/s`, `/p`, `/c`, `/a`, and default execution in `MatrixRainTests/unit/ScreenSaverModeParserTests.cpp`
- [X] T011 [US1] Implement argument parsing and context construction in `MatrixRain/ScreenSaverModeParser.cpp` and wire usage inside `MatrixRain/main.cpp`
- [X] T012 [P] [US1] Add failing exit-threshold unit tests covering cursor visibility and input triggers in `MatrixRainTests/unit/InputExitStateTests.cpp`
- [X] T013 [US1] Implement `InputExitState` handling within `MatrixRainCore/InputSystem.h` and `MatrixRainCore/InputSystem.cpp`
- [X] T014 [P] [US1] Add integration coverage for `/s` mode (cursor hide, hotkey suppression, exit-on-input) in `MatrixRainTests/integration/DisplayModeTests.cpp`
- [X] T015 [US1] Propagate `ScreenSaverModeContext` through `MatrixRainCore/include/matrixrain/Application.h`, `MatrixRainCore/src/Application.cpp`, and `MatrixRainCore/ApplicationState.cpp` to enforce `/s` runtime rules
- [X] T032 [P] [US1] Add failing error-handling tests covering invalid screensaver arguments and DirectX initialization failures in `MatrixRainTests/unit/ScreenSaverErrorHandlingTests.cpp`
- [X] T033 [US1] Surface descriptive message dialogs/log events for invalid arguments and initialization failures in `MatrixRain/ScreenSaverModeParser.cpp` and `MatrixRain/main.cpp`
- [~] T038 [P] [US1] ~~Add failing tests ensuring a second `/s` launch while the saver is active exits immediately with a diagnostic in `MatrixRainTests/integration/DisplayModeTests.cpp`~~ **SKIPPED** - Windows OS already manages screensaver instance lifecycle; mutex-based detection unnecessary and risks leaked locks
- [~] T039 [US1] ~~Implement single-instance detection and diagnostic messaging for concurrent saver launches in `MatrixRain/main.cpp` and `MatrixRainCore/ApplicationState.cpp`~~ **SKIPPED** - Not needed; see T038
- [ ] T045 [P] [US1] Add failing multi-monitor integration tests confirming `/s` sessions cover all displays in `MatrixRainTests/integration/DisplayModeTests.cpp`
- [ ] T046 [US1] Implement multi-monitor rendering support across `MatrixRainCore/src/Application.cpp` and `MatrixRainCore/RenderSystem.cpp`

**Checkpoint**: MatrixRain operates as a Windows screensaver via `/s`, satisfying FR-002, FR-003, and FR-010 without regressing normal mode startup.

---

## Phase 4: User Story 2 - Configure Screensaver Preferences (Priority: P1)

**Goal**: Provide a registry-backed configuration dialog reachable via `/c` so users can adjust density, color, animation speed, and glow settings that persist between sessions.

**Independent Test**: Launch `MatrixRain.scr /c`, adjust sliders/combobox selections, confirm registry updates, and verify the next `/s` launch applies the revised settings.

### Implementation & Tests for User Story 2 (write tests first)

- [X] T016 [P] [US2] Add failing load/save tests for `RegistrySettingsProvider` in `MatrixRainTests/unit/RegistrySettingsProviderTests.cpp`
- [X] T017 [US2] Implement `RegistrySettingsProvider` in `MatrixRainCore/RegistrySettingsProvider.h` and `MatrixRainCore/RegistrySettingsProvider.cpp`
- [X] T018 [US2] Integrate registry loading/saving flows into `MatrixRainCore/ApplicationState.cpp` using the provider
- [X] T019 [P] [US2] Add failing presenter logic tests for the settings dialog in `MatrixRainTests/unit/ConfigDialogControllerTests.cpp`
- [X] T020 [US2] Implement `ConfigDialogController` in `MatrixRainCore/ConfigDialogController.h` and `MatrixRainCore/ConfigDialogController.cpp`
- [X] T021 [US2] Define `IDD_MATRIXRAIN_SAVER_CONFIG` and control IDs in `MatrixRain/MatrixRain.rc` and `MatrixRain/resource.h`
- [X] T022 [US2] Handle `/c` argument in `MatrixRain/main.cpp` by invoking `ConfigDialogController` with registry-backed settings
- [X] T034 [P] [US2] Add failing tests for registry read/write failure paths and user-facing diagnostics in `MatrixRainTests/unit/RegistrySettingsProviderTests.cpp`
- [X] T035 [US2] Implement registry error propagation and user-facing feedback in `MatrixRainCore/RegistrySettingsProvider.cpp` and `MatrixRainCore/ConfigDialogController.cpp`
- [~] T040 [P] [US2] ~~Add failing tests verifying the configuration dialog refuses changes while a screensaver session is active in `MatrixRainTests/unit/ConfigDialogControllerTests.cpp`~~ **SKIPPED** - Windows UI prevents simultaneous screensaver execution and Control Panel settings dialog access; OS enforces mutual exclusion
- [~] T041 [US2] ~~Implement dialog concurrency safeguards and user feedback in `MatrixRainCore/ConfigDialogController.cpp` and `MatrixRainCore/ApplicationState.cpp`~~ **SKIPPED** - Not needed; see T040
- [X] T049 [P] [US2] Add `ConfigDialogSnapshot` struct to `MatrixRainCore/ConfigDialogSnapshot.h` for capturing settings state and ApplicationState reference
- [X] T050 [P] [US2] Add failing tests for live overlay dialog mode (immediate updates, Cancel rollback) in `MatrixRainTests/unit/ConfigDialogControllerTests.cpp`
- [X] T051 [US2] Extend `ConfigDialogController` to support dual modes (modal vs live overlay) with snapshot/restore logic in `MatrixRainCore/ConfigDialogController.cpp`
- [X] T052 [US2] Implement real-time setting propagation from dialog controls to `ApplicationState` during live overlay mode in `MatrixRainCore/ConfigDialogController.cpp`
- [X] T053 [US2] Update `/c` handling in `MatrixRain/main.cpp` to detect presence/absence of HWND in command line: if HWND provided, use it as external Control Panel parent for modal mode; if HWND absent, use `FindWindow()` to locate running MatrixRain instance and obtain its main window HWND for live overlay mode (show error if no instance found)
- [X] T054 [P] [US2] Add failing integration tests for live overlay dialog behavior (topmost z-order, immediate animation updates, Cancel revert, primary monitor centering with multi-monitor fullscreen) in `MatrixRainTests/integration/LiveDialogTests.cpp`
- [X] T055 [US2] Implement WS_EX_TOPMOST dialog styling and parent-to-app-window logic for live overlay mode in `MatrixRainCore/ConfigDialogController.cpp` and `MatrixRain/main.cpp`
- [X] T056 [P] [US2] ~~Add failing error handling tests for `/c` without HWND when app not running (FindWindow returns NULL) or during screensaver sessions (FindWindow finds /s or /p mode window)~~ **NOT APPLICABLE** - architecture changed to initialize own Application instance instead of using FindWindow
- [X] T057 [US2] ~~Implement error dialogs for invalid live overlay mode invocations (no app running, screensaver active)~~ **NOT APPLICABLE** - architecture changed to initialize own Application instance
- [X] T058 [P] [US2] Add failing tests for dialog behavior when MatrixRain application exits while live overlay dialog is open (dialog detects parent window destruction via WM_PARENTNOTIFY or window validation and closes gracefully) in `MatrixRainTests/unit/ConfigDialogControllerTests.cpp`

**Checkpoint**: Users can configure screensaver options via the dialog (modal Control Panel or live overlay), and registry persistence functions for both read and write paths with immediate visual feedback in live mode.

---

## Phase 5: User Story 3 - Continue Using MatrixRain as an App (Priority: P2)

**Goal**: Preserve the existing desktop experience when MatrixRain launches without screensaver arguments, while ensuring hotkey-driven changes persist.

**Independent Test**: Run `MatrixRain.exe`, toggle density/color/debug overlays via hotkeys, restart, and confirm settings persist while debug helpers remain disabled only in screensaver modes.

### Implementation & Tests for User Story 3 (write tests first)

- [ ] T023 [P] [US3] Add regression coverage for normal-mode hotkeys and persistence in `MatrixRainTests/integration/AnimationLoopTests.cpp`
- [ ] T024 [US3] Ensure normal-mode branches retain hotkeys and debug overlays by conditioning mode checks in `MatrixRainCore/ApplicationState.cpp` and `MatrixRainCore/InputSystem.cpp`
- [ ] T025 [US3] Persist hotkey-driven adjustments through `MatrixRainCore/ApplicationState.cpp` and `MatrixRainCore/DensityController.cpp`
- [ ] T047 [P] [US3] Add failing integration tests verifying normal-mode fullscreen spans all monitors in `MatrixRainTests/integration/DisplayModeTests.cpp`
- [ ] T048 [US3] Implement normal-mode multi-monitor fullscreen behavior in `MatrixRainCore/src/Application.cpp` and `MatrixRainCore/RenderSystem.cpp`

**Checkpoint**: Desktop users retain full functionality, and registry persistence works across both modes without leaking screensaver restrictions.

---

## Phase 6: User Story 4 - Preview Within Control Panel (Priority: P3)

**Goal**: Support `/p <HWND>` preview runs that render safely inside the supplied window, keep the cursor visible, and ignore exit-on-input semantics.

**Independent Test**: Use the Windows screensaver control panel preview (or `MatrixRain.scr /p <HWND>`) to verify the animation scales within the target window, leaves hotkeys disabled, and keeps the cursor visible until closed.

### Implementation & Tests for User Story 4 (write tests first)

- [ ] T026 [P] [US4] Add failing preview viewport tests in `MatrixRainTests/unit/RenderSystemPreviewTests.cpp`
- [ ] T027 [US4] Implement preview rendering path across `MatrixRainCore/RenderSystem.cpp` and `MatrixRainCore/Viewport.cpp`
- [ ] T028 [US4] Finalize `/p` handling in `MatrixRain/main.cpp` and `MatrixRainCore/src/Application.cpp` so preview mode omits cursor hiding and exit-on-input
- [ ] T036 [P] [US4] Add failing tests verifying `/a` password-change requests surface the unsupported message in `MatrixRainTests/unit/ScreenSaverModeParserTests.cpp`
- [ ] T037 [US4] Implement `/a` handling with unsupported password-change dialog in `MatrixRain/ScreenSaverModeParser.cpp` and `MatrixRain/main.cpp`

**Checkpoint**: Control panel preview operates reliably without impacting full-screen or desktop modes.

---

## Phase 7: Polish & Cross-Cutting Concerns

**Purpose**: Final documentation, packaging validation, and release readiness.

- [ ] T029 Update screensaver guidance in `specs/002-add-screensaver-support/quickstart.md` with final QA and deployment instructions
- [ ] T030 Refresh `README.md` to document `.scr` distribution and installation steps
- [ ] T031 Run release build validation via `scripts/Invoke-MatrixRainBuild.ps1 -Target Build -Configuration Release -Platform x64` confirming `.scr` and `.exe` parity
- [ ] T042 [P] Instrument startup profiling to verify <500 ms launch time in `MatrixRainTests/perf/StartupPerformanceTests.cpp`
- [ ] T043 [P] Capture frame-time metrics for `/s` sessions hitting 60 FPS in `MatrixRainTests/perf/RenderPerformanceTests.cpp`
- [ ] T044 Incorporate manual QA timing checklist (exit-on-input under 1 second, registry persistence verification) into `specs/002-add-screensaver-support/quickstart.md`

---

## Dependencies & Execution Order

- **Phase 1 → Phase 2**: Complete environment validation before modifying code.
- **Phase 2 → Phases 3-6**: Foundational enums, settings, and build outputs are prerequisites for all user stories.
- **User Story Order**: Execute in priority order (US1 & US2 in parallel once foundation completes, followed by US3, then US4).
- **Polish (Phase 7)**: Runs after desired user stories reach acceptance checkpoints.

### Story Dependency Graph

US1 (P1) ──┐
           ├── US3 (P2) ──┐
US2 (P1) ──┘               ├── Polish
                           └── US4 (P3) ──┘

- US1 and US2 can begin immediately after Phase 2.
- US3 depends on both US1 and US2 to ensure registry persistence and mode gating are stable.
- US4 depends on US1 infrastructure (mode context) and benefits from US2 settings but can start once those are in place.

### Parallel Execution Examples

- **US1**: T010 and T012 can be authored concurrently; T014 integration work can proceed once T013 lands.
- **US2**: T016 and T019 (test scaffolding) can proceed in parallel before their corresponding implementations (T017 and T020).
- **US3**: T023 test updates can run while US2 finishes, enabling rapid validation once US1/US2 merge.
- **US4**: T026 unit tests can start while T027 implementation is drafted, assuming mode context APIs are stable from US1.

## Implementation Strategy

### MVP First (Deliver User Story 1)

1. Finish Phases 1-2 (environment + foundation).
2. Complete Phase 3 (US1) and validate `/s` flows end-to-end.
3. Pause for stakeholder review before progressing.

### Incremental Delivery

1. Deliver US1 (screensaver launch) as MVP.
2. Layer US2 (configuration dialog + registry persistence) to enable customization.
3. Add US3 to guarantee desktop parity and persistence.
4. Finish with US4 preview support and Phase 7 polish for release readiness.

### Team Parallelization

- Developer A: Focus on US1 runtime behaviors after foundation.
- Developer B: Own US2 registry + dialog work in parallel with US1.
- Developer C: Pick up US3 regression tasks once US1/US2 APIs settle, then assist with US4 preview implementation.
- Share Phase 7 polish tasks after all high-priority stories reach checkpoints.
