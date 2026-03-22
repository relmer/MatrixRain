# Tasks: Screensaver Install/Uninstall via Command Line

**Input**: Design documents from `/specs/005-screensaver-install/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/api.md, quickstart.md

**Tests**: Required — TDD per project constitution. Tests MUST be written and FAIL before implementation.

**Organization**: Tasks grouped by user story for independent implementation and testing.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

---

## Phase 1: Setup

**Purpose**: Add new enum values and extend command-line parsing infrastructure — shared by all stories.

- [X] T001 Add `Install` and `Uninstall` values to `ScreenSaverMode` enum in `MatrixRainCore/ScreenSaverMode.h`
- [X] T002 Create `ScreenSaverInstaller.h` class declaration (static methods: `Install`, `Uninstall`, `IsElevated`, `RequestElevation`) in `MatrixRainCore/ScreenSaverInstaller.h`
- [X] T003 Create stub `ScreenSaverInstaller.cpp` with empty method bodies returning `E_NOTIMPL` in `MatrixRainCore/ScreenSaverInstaller.cpp`
- [X] T004 Add `ScreenSaverInstaller.h` and `ScreenSaverInstaller.cpp` to `MatrixRainCore/MatrixRainCore.vcxproj`
- [X] T005 Add `ScreenSaverInstallerTests.cpp` to `MatrixRainTests/MatrixRainTests.vcxproj`
- [X] T006 Verify solution builds with new files: run `Build Debug (current arch)` task

**Checkpoint**: Solution compiles with new enum values and stub class. No behavior changes yet.

---

## Phase 2: Foundational — Multi-Character Switch Parsing

**Purpose**: Extend `ParseCommandLine` to recognize `/install` and `/uninstall` switches. This MUST complete before US1/US2 can dispatch these modes.

**⚠️ CRITICAL**: No user story implementation can begin until parsing works.

### Tests (write FIRST, verify they FAIL)

- [X] T007 [P] Write test `ParseCommandLine_SlashInstall_ReturnsInstallMode` in `MatrixRainTests/unit/ScreenSaverModeParserTests.cpp`
- [X] T008 [P] Write test `ParseCommandLine_SlashUninstall_ReturnsUninstallMode` in `MatrixRainTests/unit/ScreenSaverModeParserTests.cpp`
- [X] T009 [P] Write test `ParseCommandLine_DashInstall_ReturnsInstallMode` in `MatrixRainTests/unit/ScreenSaverModeParserTests.cpp`
- [X] T010 [P] Write test `ParseCommandLine_DashUninstall_ReturnsUninstallMode` in `MatrixRainTests/unit/ScreenSaverModeParserTests.cpp`
- [X] T011 [P] Write test `ParseCommandLine_SlashInstall_CaseInsensitive` in `MatrixRainTests/unit/ScreenSaverModeParserTests.cpp`
- [X] T012 [P] Write test `ParseCommandLine_SlashUninstall_CaseInsensitive` in `MatrixRainTests/unit/ScreenSaverModeParserTests.cpp`
- [X] T013 Write test `ParseCommandLine_ExistingSingleCharSwitches_StillWork` regression test in `MatrixRainTests/unit/ScreenSaverModeParserTests.cpp` (verify `/s`, `/c`, `/p`, `/a`, `/?` still parse correctly after multi-char refactor)
- [X] T014 Build and run tests — verify new tests FAIL, existing tests PASS

### Implementation

- [X] T015 Extend `ParseCommandLine` in `MatrixRainCore/ScreenSaverModeParser.cpp` to collect multi-character switch words after prefix, match "install"/"uninstall" (case-insensitive) before falling through to single-character logic per research.md R1
- [X] T016 Build and run ALL tests — verify new parsing tests PASS and all existing tests still PASS

**Checkpoint**: `/install` and `/uninstall` are parsed into correct `ScreenSaverMode` values. All existing switch parsing unchanged.

---

## Phase 3: User Story 1 — Install as Screensaver (Priority: P1) 🎯 MVP

**Goal**: `MatrixRain.exe /install` copies itself to `%SystemRoot%\System32\MatrixRain.scr`, invokes `desk.cpl InstallScreenSaver` to set it as active screensaver and open Screen Saver Settings dialog, with automatic UAC elevation.

**Independent Test**: Run `MatrixRain.exe /install` → UAC prompt → `.scr` file appears in System32, Screen Saver Settings dialog opens with MatrixRain selected.

### Tests (write FIRST, verify they FAIL)

- [X] T017 [P] [US1] Write test `IsElevated_ReturnsBoolean` in `MatrixRainTests/unit/ScreenSaverInstallerTests.cpp` — verify `IsElevated()` returns `true` or `false` without crashing (non-elevated test runner returns `false`)
- [X] T018 [P] [US1] Write test `Install_NotElevated_ReturnsAccessDenied` in `MatrixRainTests/unit/ScreenSaverInstallerTests.cpp` — verify `Install()` returns `E_ACCESSDENIED` or appropriate error when called without elevation (test runner is not elevated)
- [X] T019 [US1] Build and run tests — verify new tests FAIL (stubs return `E_NOTIMPL`)

### Implementation

- [X] T020 [US1] Implement `IsElevated()` in `MatrixRainCore/ScreenSaverInstaller.cpp` using `OpenProcessToken` + `GetTokenInformation(TokenElevation)` per research.md R3
- [X] T021 [US1] Implement `RequestElevation(pszSwitch)` in `MatrixRainCore/ScreenSaverInstaller.cpp` using `ShellExecuteExW` with `lpVerb = L"runas"` per research.md R3
- [X] T022 [US1] Implement `Install()` in `MatrixRainCore/ScreenSaverInstaller.cpp`: (1) check elevation with `IsElevated()`, return `E_ACCESSDENIED` if not elevated, (2) `GetModuleFileNameW` to get source exe path, (3) `GetSystemDirectoryW` + `\MatrixRain.scr` for target path, (4) if source == target, skip copy (handles running from installed `.scr`), (5) `CopyFileW` with overwrite per research.md R5, (6) invoke `rundll32.exe desk.cpl,InstallScreenSaver` with target path per research.md R2
- [X] T024 [US1] Build and run tests — verify `IsElevated` and `Install` tests PASS

**Checkpoint**: `/install` copies `.scr` to System32 (skipping self-copy when source==target) and opens Screen Saver Settings. UAC elevation works. Manually verify via quickstart.md steps 1–3.

---

## Phase 4: User Story 2 — Uninstall Screensaver (Priority: P1)

**Goal**: `MatrixRain.exe /uninstall` removes the `.scr` file from System32 and cleans up registry entries if MatrixRain is the active screensaver.

**Independent Test**: After install, run `MatrixRain.exe /uninstall` → `.scr` removed from System32, registry `SCRNSAVE.EXE` cleared if it pointed to MatrixRain.

### Tests (write FIRST, verify they FAIL)

- [X] T025 [P] [US2] Write test `Uninstall_NotElevated_ReturnsAccessDenied` in `MatrixRainTests/unit/ScreenSaverInstallerTests.cpp` — verify `Uninstall()` returns `E_ACCESSDENIED` or appropriate error when called without elevation
- [X] T026 [P] [US2] Write test `Uninstall_ScrFileNotPresent_ReturnsSOK` in `MatrixRainTests/unit/ScreenSaverInstallerTests.cpp` — verify `Uninstall()` returns `S_OK` or `S_FALSE` when `.scr` file doesn't exist (graceful "not installed" case per FR-009)
- [X] T026a [P] [US2] Write test `Uninstall_MatrixRainActive_ClearsRegistryEntries` using mock registry interface — verify that when mock `SCRNSAVE.EXE` points to `MatrixRain.scr`, uninstall clears `SCRNSAVE.EXE` and sets `ScreenSaveActive` to `"0"` (FR-013, FR-014)
- [X] T026b [P] [US2] Write test `Uninstall_DifferentScreensaverActive_LeavesRegistryUntouched` using mock registry interface — verify that when mock `SCRNSAVE.EXE` points to a different `.scr`, uninstall does NOT modify registry values (FR-015)
- [X] T026c [P] [US2] Write test `Uninstall_RegistryKeyMissing_DoesNotCrash` using mock registry interface — verify graceful handling when `SCRNSAVE.EXE` value does not exist
- [X] T027 [US2] Build and run tests — verify new tests FAIL (stubs return `E_NOTIMPL`)

### Implementation

- [X] T028 [US2] Implement `Uninstall()` in `MatrixRainCore/ScreenSaverInstaller.cpp`: (1) check elevation, return `E_ACCESSDENIED` if not elevated, (2) build target path `%SystemRoot%\System32\MatrixRain.scr`, (3) if file doesn't exist return `S_FALSE` with informational message, (4) `DeleteFileW` to remove `.scr`, (5) read `HKCU\Control Panel\Desktop\SCRNSAVE.EXE` and compare case-insensitively to target path per research.md R4, (6) if match: delete `SCRNSAVE.EXE` value and set `ScreenSaveActive` to `"0"`, (7) if different screensaver: leave registry untouched per FR-015
- [X] T029 [US2] Build and run ALL tests — verify uninstall tests PASS and all previous tests still PASS

**Checkpoint**: `/uninstall` removes `.scr` and cleans registry correctly. Manually verify via quickstart.md steps 4–6.

---

## Phase 5: User Story 3 — Help Text Updated (Priority: P2)

**Goal**: `/install` and `/uninstall` appear in help output when user runs `MatrixRain.exe /?`.

**Independent Test**: Run `MatrixRain.exe /?` and verify `/install` and `/uninstall` are listed with descriptions.

### Tests (write FIRST, verify they FAIL)

- [X] T030 [P] [US3] Write test `GetFormattedText_ContainsInstallSwitch` in `MatrixRainTests/unit/UsageTextTests.cpp` — verify help text contains "install" with its description
- [X] T031 [P] [US3] Write test `GetFormattedText_ContainsUninstallSwitch` in `MatrixRainTests/unit/UsageTextTests.cpp` — verify help text contains "uninstall" with its description
- [X] T032 [US3] Build and run tests — verify new tests FAIL

### Implementation

- [X] T033 [US3] Update `UsageText` class to support multi-character switches (may need to adjust `SwitchEntry` struct or add a parallel data structure) in `MatrixRainCore/UsageText.h` and `MatrixRainCore/UsageText.cpp`
- [X] T034 [US3] Add `/install` and `/uninstall` entries with descriptions to the switch list in `MatrixRainCore/UsageText.cpp`
- [X] T035 [US3] Build and run ALL tests — verify help text tests PASS and all previous tests still PASS

**Checkpoint**: Help text shows install/uninstall switches. All user stories are independently functional.

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Final validation, error handling review, dispatch integration, and cleanup.

- [ ] T036 Review error messages for Install/Uninstall — ensure clear user-facing messages per FR-010 (console output with message box fallback if no console attached) in `MatrixRainCore/ScreenSaverInstaller.cpp`
- [ ] T036a Add Install/Uninstall dispatch in `MatrixRain/main.cpp`: before `app.Initialize`, check for `ScreenSaverMode::Install` or `ScreenSaverMode::Uninstall`, call `IsElevated()`/`RequestElevation()` or `Install()`/`Uninstall()` per contracts/api.md dispatch contract
- [ ] T037 Build Release configuration and verify clean build: run `Build Release (current arch)` task
- [ ] T038 Run full test suite in both Debug and Release: run `Build + Test Debug (current arch)` and `Build + Test Release (current arch)` tasks
- [ ] T039 Run quickstart.md full manual verification sequence (install → verify file → verify registry → uninstall → verify removed → verify registry cleaned → help text check)

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — start immediately
- **Foundational (Phase 2)**: Depends on Phase 1 — BLOCKS all user stories
- **US1 Install (Phase 3)**: Depends on Phase 2 — MVP target
- **US2 Uninstall (Phase 4)**: Depends on Phase 2 — can run in parallel with US1 (different methods in same file, but T023 dispatch is shared)
- **US3 Help Text (Phase 5)**: Depends on Phase 1 only — can run in parallel with US1/US2
- **Polish (Phase 6)**: Depends on all user stories complete

### Within Each User Story

1. Tests MUST be written and FAIL before implementation begins
2. Core logic before dispatch integration
3. Build and verify after each implementation task

### Parallel Opportunities

**Phase 2 Tests** (all different test methods, same file — can be written together):
```
T007, T008, T009, T010, T011, T012 — all parser tests in parallel
```

**Phase 3 + Phase 5 Tests** (different files — fully parallel):
```
T017, T018 — installer tests     |  T030, T031 — usage text tests
```

**US1 + US2 Implementation** (different methods, manageable in parallel):
```
T020–T022 — Install logic        |  T028 — Uninstall logic
```
Note: T036a (main.cpp dispatch) handles both modes and is in Phase 6, after both Install and Uninstall are implemented.

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup (T001–T006)
2. Complete Phase 2: Foundational parsing (T007–T016)
3. Complete Phase 3: User Story 1 — Install (T017–T024)
4. **STOP and VALIDATE**: `/install` works end-to-end with UAC elevation
5. This alone delivers core value — users can install MatrixRain as a screensaver

### Incremental Delivery

1. Setup + Foundational → Parsing infrastructure ready
2. User Story 1 (Install) → Test independently → **MVP shipped**
3. User Story 2 (Uninstall) → Test independently → Clean removal works
4. User Story 3 (Help Text) → Test independently → Discoverability complete
5. Polish → Full manual verification → Release-ready

### Files Modified/Created Summary

| File | Action | Phase |
|------|--------|-------|
| `MatrixRainCore/ScreenSaverMode.h` | MODIFY | Phase 1 |
| `MatrixRainCore/ScreenSaverInstaller.h` | CREATE | Phase 1 |
| `MatrixRainCore/ScreenSaverInstaller.cpp` | CREATE | Phase 1, 3, 4 |
| `MatrixRainCore/MatrixRainCore.vcxproj` | MODIFY | Phase 1 |
| `MatrixRainCore/ScreenSaverModeParser.cpp` | MODIFY | Phase 2 |
| `MatrixRainCore/UsageText.h` | MODIFY | Phase 5 |
| `MatrixRainCore/UsageText.cpp` | MODIFY | Phase 5 |
| `MatrixRain/main.cpp` | MODIFY | Phase 3 |
| `MatrixRainTests/unit/ScreenSaverModeParserTests.cpp` | MODIFY | Phase 2 |
| `MatrixRainTests/unit/ScreenSaverInstallerTests.cpp` | CREATE | Phase 3, 4 |
| `MatrixRainTests/unit/UsageTextTests.cpp` | MODIFY | Phase 5 |
| `MatrixRainTests/MatrixRainTests.vcxproj` | MODIFY | Phase 1 |

---

## Notes

- [P] tasks = different files, no dependencies — safe to parallelize
- [Story] label maps task to specific user story for traceability
- UAC elevation and System32 file operations cannot be tested in unit tests running under normal privileges — unit tests validate the elevation-check logic and error paths; manual verification covers the privileged happy path
- Registry operations tested EXCLUSIVELY via mock interface — tests MUST NEVER interact with the actual Windows registry
- `desk.cpl InstallScreenSaver` handles all registry writes for install; only uninstall needs manual registry cleanup
- Commit after each task or logical group per constitution commit discipline
