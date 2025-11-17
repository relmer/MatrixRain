# Tasks: Matrix Rain Visual Effect

**Input**: Design documents from `/specs/001-matrix-rain/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, quickstart.md

**Tests**: Tests are MANDATORY for all core library code (MatrixRainCore). Use Test-Driven Development: write tests first, verify they fail, then implement. The application entry point (MatrixRain.exe) does not require tests as it contains minimal logic.

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

**Library-First Architecture**: All functionality goes in MatrixRainCore (static library). The MatrixRain.exe project contains only the minimal entry point (WinMain). Unit tests link against the core library to avoid duplicate compilation.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

## Path Conventions

- **MatrixRainCore/**: Core static library (.lib) - all functionality
- **MatrixRain/**: Application executable (.exe) - entry point only
- **MatrixRainTests/**: Native unit test project

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Project initialization and Visual Studio solution structure

- [x] T001 Create Visual Studio 2026 solution MatrixRain.sln at repository root
- [x] T002 Create MatrixRainCore static library project with /std:c++latest /W4 /WX configuration
- [x] T003 Create MatrixRain executable project with minimal configuration
- [x] T004 Create MatrixRainTests native unit test project with reference to MatrixRainCore
- [x] T005 [P] Configure MatrixRainCore precompiled header MatrixRainCore/pch.h with DirectX 11, Direct2D, Win32 headers
- [x] T006 [P] Configure MatrixRainCore precompiled header implementation MatrixRainCore/pch.cpp
- [x] T007 [P] Configure MatrixRainTests precompiled header MatrixRainTests/pch.h including MatrixRainCore/pch.h and CppUnitTest.h
- [x] T008 [P] Configure MatrixRainTests precompiled header implementation MatrixRainTests/pch.cpp
- [x] T009 [P] Create .vscode/c_cpp_properties.json for MatrixRainCore IntelliSense configuration
- [x] T010 [P] Create .vscode/c_cpp_properties.json for MatrixRain IntelliSense configuration
- [x] T011 [P] Create .vscode/c_cpp_properties.json for MatrixRainTests IntelliSense configuration
- [x] T012 [P] Create .vscode/tasks.json with Debug and Release build tasks for solution
- [x] T013 [P] Create .vscode/launch.json with debug configurations for MatrixRain.exe
- [x] T014 Create directory structure MatrixRainCore/include/matrixrain/ for public API headers
- [x] T015 [P] Create directory structure MatrixRainCore/src/characters/ for character management
- [x] T016 [P] Create directory structure MatrixRainCore/src/rendering/ for DirectX rendering
- [x] T017 [P] Create directory structure MatrixRainCore/src/animation/ for streak animation
- [x] T018 [P] Create directory structure MatrixRainCore/src/input/ for keyboard input
- [x] T019 [P] Create directory structure MatrixRainCore/src/state/ for application state
- [x] T020 [P] Create directory structure MatrixRainTests/unit/characters/ for character tests
- [x] T021 [P] Create directory structure MatrixRainTests/unit/rendering/ for rendering tests
- [x] T022 [P] Create directory structure MatrixRainTests/unit/animation/ for animation tests
- [x] T023 [P] Create directory structure MatrixRainTests/unit/input/ for input tests
- [x] T024 [P] Create directory structure MatrixRainTests/unit/state/ for state tests
- [x] T025 [P] Create directory structure MatrixRainTests/integration/ for integration tests

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core infrastructure that MUST be complete before ANY user story can be implemented

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [x] T026 Create Vector2 struct in MatrixRainCore/include/matrixrain/Math.h with x, y float components
- [x] T027 Create Vector3 struct in MatrixRainCore/include/matrixrain/Math.h with x, y, z float components
- [x] T028 Create Color4 struct in MatrixRainCore/include/matrixrain/Math.h with r, g, b, a float components
- [x] T029 Create Matrix4x4 struct in MatrixRainCore/include/matrixrain/Math.h with orthographic projection support
- [x] T030 Implement Math utility functions in MatrixRainCore/src/Math.cpp for vector operations
- [x] T031 Create GlyphInfo struct in MatrixRainCore/include/matrixrain/CharacterSet.h with uvMin, uvMax, codepoint, mirrored fields
- [x] T032 Create CharacterSet singleton class declaration in MatrixRainCore/include/matrixrain/CharacterSet.h
- [x] T033 Define character constants in MatrixRainCore/src/characters/CharacterConstants.cpp with 71 katakana codepoints
- [x] T034 Add Latin letter constants (A-Z, a-z) in MatrixRainCore/src/characters/CharacterConstants.cpp
- [x] T035 Add numeral constants (0-9) in MatrixRainCore/src/characters/CharacterConstants.cpp
- [x] T036 Create high-resolution timer utility in MatrixRainCore/src/Timer.cpp using QueryPerformanceCounter

**Checkpoint**: Foundation ready - user story implementation can now begin in parallel

---

## Phase 3: User Story 1 - View Animated Matrix Rain (Priority: P1) 🎯 MVP

**Goal**: Display falling character streaks with depth perspective, fade-out, glow effects, and continuous zoom

**Independent Test**: Launch application and observe green glowing characters falling down screen with proper fade-out over 3 seconds

### Tests for User Story 1 (TDD - Write FIRST)

> **NOTE: Write these tests FIRST, ensure they FAIL before implementation**

- [x] T037 [P] [US1] Test CharacterInstance initialization in MatrixRainTests/unit/characters/CharacterInstanceTests.cpp
- [x] T038 [P] [US1] Test CharacterInstance brightness fade over 3 seconds in MatrixRainTests/unit/characters/CharacterInstanceTests.cpp
- [x] T039 [P] [US1] Test CharacterStreak initialization with random length 10-30 in MatrixRainTests/unit/animation/CharacterStreakTests.cpp
- [x] T040 [P] [US1] Test CharacterStreak velocity scaling by depth in MatrixRainTests/unit/animation/CharacterStreakTests.cpp
- [x] T041 [P] [US1] Test CharacterStreak update with delta time in MatrixRainTests/unit/animation/CharacterStreakTests.cpp
- [x] T042 [P] [US1] Test CharacterStreak character mutation 5% probability in MatrixRainTests/unit/animation/CharacterStreakTests.cpp
- [x] T043 [P] [US1] Test Viewport projection matrix generation in MatrixRainTests/unit/state/ViewportTests.cpp
- [x] T044 [P] [US1] Test Viewport resize handling in MatrixRainTests/unit/state/ViewportTests.cpp
- [ ] T045 [P] [US1] Test depth sorting back-to-front order in MatrixRainTests/unit/rendering/DepthSortingTests.cpp
- [ ] T046 [P] [US1] Test zoom wrapping at Z=100 boundary in MatrixRainTests/unit/animation/ZoomTests.cpp
- [ ] T047 [P] [US1] Test character color transitions white→green for leading character in MatrixRainTests/unit/animation/ColorTransitionTests.cpp

### Implementation for User Story 1

#### Character Set and Texture Atlas

- [x] T048 [US1] Implement CharacterSet::Initialize in MatrixRainCore/src/characters/CharacterSet.cpp to create 2048x2048 atlas texture
- [x] T049 [US1] Implement Direct2D render target creation for texture atlas in MatrixRainCore/src/characters/CharacterSet.cpp
- [x] T050 [US1] Implement DirectWrite text rendering for 133 normal glyphs to atlas in MatrixRainCore/src/characters/CharacterSet.cpp
- [x] T051 [US1] Implement mirrored glyph rendering for 133 additional entries in MatrixRainCore/src/characters/CharacterSet.cpp
- [x] T052 [US1] Implement CharacterSet::GetRandomGlyph in MatrixRainCore/src/characters/CharacterSet.cpp with uniform distribution
- [x] T053 [US1] Store UV coordinates for all 266 glyphs in CharacterSet::glyphs array in MatrixRainCore/src/characters/CharacterSet.cpp
- [ ] T053a [US1] Create D3D11 texture resource from Direct2D bitmap in MatrixRainCore/src/characters/CharacterSet.cpp and expose ID3D11ShaderResourceView for rendering

#### Data Models

- [x] T054 [P] [US1] Create CharacterInstance class in MatrixRainCore/include/matrixrain/CharacterInstance.h with glyphIndex, color, brightness, scale, positionOffset fields
- [x] T055 [P] [US1] Implement CharacterInstance::Update in MatrixRainCore/src/animation/CharacterInstance.cpp for fade calculation
- [x] T056 [P] [US1] Create CharacterStreak class in MatrixRainCore/include/matrixrain/CharacterStreak.h with position, velocity, length, characters, timers
- [x] T057 [US1] Implement CharacterStreak::Spawn in MatrixRainCore/src/animation/CharacterStreak.cpp with random initialization
- [ ] T057a [US1] Implement character mutation logic in CharacterStreak::Update: for each visible character (brightness > 0.0), generate random float [0.0, 1.0], if < 0.05 then call CharacterSet::GetRandomGlyph and replace character glyph
- [x] T058 [US1] Implement CharacterStreak::Update in MatrixRainCore/src/animation/CharacterStreak.cpp for position, fade, mutation
- [x] T059 [US1] Implement CharacterStreak::ShouldDespawn in MatrixRainCore/src/animation/CharacterStreak.cpp for viewport bounds check
- [x] T060 [P] [US1] Create Viewport class in MatrixRainCore/include/matrixrain/Viewport.h with width, height, projection fields
- [x] T061 [US1] Implement Viewport::UpdateProjection in MatrixRainCore/src/state/Viewport.cpp for orthographic matrix generation

#### Animation System

- [x] T062 [US1] Create AnimationSystem class in MatrixRainCore/include/matrixrain/AnimationSystem.h with streaks vector, zoom velocity
- [x] T063 [US1] Implement AnimationSystem::Initialize in MatrixRainCore/src/animation/AnimationSystem.cpp
- [x] T064 [US1] Implement AnimationSystem::Update in MatrixRainCore/src/animation/AnimationSystem.cpp for streak updates and zoom
- [x] T065 [US1] Implement AnimationSystem::SpawnStreak in MatrixRainCore/src/animation/AnimationSystem.cpp with random X/Z position
- [x] T066 [US1] Implement AnimationSystem::DespawnOffscreenStreaks in MatrixRainCore/src/animation/AnimationSystem.cpp
- [x] T067 [US1] Implement AnimationSystem::ApplyZoom in MatrixRainCore/src/animation/AnimationSystem.cpp with modulo wrapping at Z=100

#### DirectX 11 Rendering

- [x] T068 [US1] Create RenderSystem class in MatrixRainCore/include/matrixrain/RenderSystem.h with D3D11 device, context, swap chain
- [x] T069 [US1] Implement RenderSystem::Initialize in MatrixRainCore/src/rendering/RenderSystem.cpp for D3D11 device creation
- [x] T070 [US1] Implement RenderSystem::CreateSwapChain in MatrixRainCore/src/rendering/RenderSystem.cpp for DXGI swap chain
- [x] T071 [US1] Implement RenderSystem::CreateRenderTargetView in MatrixRainCore/src/rendering/RenderSystem.cpp
- [x] T072 [US1] Create vertex shader inline in MatrixRainCore/src/rendering/RenderSystem.cpp for instanced quad rendering
- [x] T073 [US1] Create pixel shader inline in MatrixRainCore/src/rendering/RenderSystem.cpp for texture sampling, glow, fade
- [x] T074 [US1] Implement shader compilation in MatrixRainCore/src/rendering/RenderSystem.cpp for VS and PS
- [x] T075 [US1] Implement instance buffer creation in MatrixRainCore/src/rendering/RenderSystem.cpp for character attributes
- [x] T076 [US1] Implement RenderSystem::SortStreaksByDepth in MatrixRainCore/src/rendering/RenderSystem.cpp using std::stable_sort
- [x] T077 [US1] Implement RenderSystem::UpdateInstanceBuffer in MatrixRainCore/src/rendering/RenderSystem.cpp for character data upload
- [x] T078 [US1] Implement RenderSystem::Render in MatrixRainCore/src/rendering/RenderSystem.cpp for draw instanced calls
- [x] T079 [US1] Implement RenderSystem::Present in MatrixRainCore/src/rendering/RenderSystem.cpp for swap chain present

#### Win32 Window and Main Loop

- [x] T080 [US1] Create Application class in MatrixRainCore/include/matrixrain/Application.h with window handle, systems
- [x] T081 [US1] Implement Application::Initialize in MatrixRainCore/src/Application.cpp for window creation, system initialization
- [x] T082 [US1] Implement Application::Run in MatrixRainCore/src/Application.cpp for message loop with delta time calculation
- [x] T083 [US1] Implement Application::Update in MatrixRainCore/src/Application.cpp to call AnimationSystem::Update
- [x] T084 [US1] Implement Application::Render in MatrixRainCore/src/Application.cpp to call RenderSystem::Render
- [x] T085 [US1] Implement Application::Shutdown in MatrixRainCore/src/Application.cpp for cleanup
- [x] T086 [US1] Implement window procedure in MatrixRainCore/src/Application.cpp for WM_DESTROY, WM_SIZE messages
- [x] T087 [US1] Create WinMain entry point in MatrixRain/main.cpp calling Application::Initialize, Run, Shutdown (<10 lines per constitution requirement)

#### Integration

- [x] T088 [US1] Wire CharacterSet initialization in Application::Initialize in MatrixRainCore/src/Application.cpp
- [x] T089 [US1] Wire AnimationSystem initialization with Viewport reference in MatrixRainCore/src/Application.cpp
- [ ] T090 [US1] Wire RenderSystem initialization with CharacterSet texture SRV from T053a in MatrixRainCore/src/Application.cpp
- [ ] T091 [US1] Verify integration test for complete animation loop in MatrixRainTests/integration/AnimationLoopTests.cpp

**Checkpoint**: At this point, User Story 1 should be fully functional - launch app and see Matrix rain animation with all visual effects

---

## Phase 4: User Story 2 - Control Rain Density (Priority: P2)

**Goal**: Allow users to increase/decrease number of falling streaks via +/- keys with min/max bounds

**Independent Test**: Launch app, press + key multiple times to see more streaks, press - key to see fewer streaks

### Tests for User Story 2 (TDD - Write FIRST)

- [ ] T092 [P] [US2] Test DensityController initialization with level 5 default (target 262 streaks) in MatrixRainTests/unit/state/DensityControllerTests.cpp
- [ ] T093 [P] [US2] Test DensityController level increase to max 10 (target 496 streaks) using formula: targetStreaks = 10 + (level-1)*54 in MatrixRainTests/unit/state/DensityControllerTests.cpp
- [ ] T094 [P] [US2] Test DensityController level decrease to min 1 (target 10 streaks) using formula: targetStreaks = 10 + (level-1)*54 in MatrixRainTests/unit/state/DensityControllerTests.cpp
- [ ] T095 [P] [US2] Test DensityController spawn probability calculation based on current streak count vs target in MatrixRainTests/unit/state/DensityControllerTests.cpp
- [ ] T096 [P] [US2] Test DensityController bounds enforcement in MatrixRainTests/unit/state/DensityControllerTests.cpp
- [ ] T097 [P] [US2] Test InputSystem keyboard event processing for VK_ADD in MatrixRainTests/unit/input/InputSystemTests.cpp
- [ ] T098 [P] [US2] Test InputSystem keyboard event processing for VK_SUBTRACT in MatrixRainTests/unit/input/InputSystemTests.cpp
- [ ] T099 [P] [US2] Test InputSystem keyboard event processing for VK_OEM_PLUS in MatrixRainTests/unit/input/InputSystemTests.cpp
- [ ] T100 [P] [US2] Test InputSystem keyboard event processing for VK_OEM_MINUS in MatrixRainTests/unit/input/InputSystemTests.cpp

### Implementation for User Story 2

- [ ] T101 [P] [US2] Create DensityController class in MatrixRainCore/include/matrixrain/DensityController.h with level (1-10), GetTargetStreakCount() method implementing formula: targetStreaks = 10 + (level-1)*54
- [ ] T102 [US2] Implement DensityController::Initialize in MatrixRainCore/src/state/DensityController.cpp with level 5 default (262 target streaks)
- [ ] T103 [US2] Implement DensityController::IncreaseLevel in MatrixRainCore/src/state/DensityController.cpp with max bound check (level <= 10)
- [ ] T104 [US2] Implement DensityController::DecreaseLevel in MatrixRainCore/src/state/DensityController.cpp with min bound check
- [ ] T105 [US2] Implement DensityController::UpdateSpawnLogic in MatrixRainCore/src/state/DensityController.cpp for spawn probability
- [ ] T106 [US2] Implement DensityController::ShouldSpawnStreak in MatrixRainCore/src/state/DensityController.cpp based on active count
- [ ] T107 [P] [US2] Create InputSystem class in MatrixRainCore/include/matrixrain/InputSystem.h with keyboard state tracking
- [ ] T108 [US2] Implement InputSystem::ProcessKeyDown in MatrixRainCore/src/input/InputSystem.cpp for WM_KEYDOWN handling
- [ ] T109 [US2] Implement InputSystem::OnDensityIncrease in MatrixRainCore/src/input/InputSystem.cpp calling DensityController
- [ ] T110 [US2] Implement InputSystem::OnDensityDecrease in MatrixRainCore/src/input/InputSystem.cpp calling DensityController
- [ ] T111 [US2] Wire InputSystem initialization in Application::Initialize in MatrixRainCore/src/Application.cpp
- [ ] T112 [US2] Wire DensityController to AnimationSystem in MatrixRainCore/src/Application.cpp
- [ ] T113 [US2] Add WM_KEYDOWN message handling to window procedure in MatrixRainCore/src/Application.cpp
- [ ] T114 [US2] Integrate DensityController::UpdateSpawnLogic into AnimationSystem::Update in MatrixRainCore/src/animation/AnimationSystem.cpp
- [ ] T115 [US2] Verify integration test for density changes in MatrixRainTests/integration/DensityControlTests.cpp

**Checkpoint**: At this point, User Stories 1 AND 2 should both work - animation runs with controllable density via +/- keys

---

## Phase 5: User Story 3 - Toggle Display Mode (Priority: P3)

**Goal**: Allow users to switch between windowed and fullscreen modes via Alt+Enter without disrupting animation

**Independent Test**: Launch app in windowed mode, press Alt+Enter to go fullscreen, press Alt+Enter again to return to windowed

### Tests for User Story 3 (TDD - Write FIRST)

- [ ] T116 [P] [US3] Test ApplicationState display mode initialization to Windowed in MatrixRainTests/unit/state/ApplicationStateTests.cpp
- [ ] T117 [P] [US3] Test ApplicationState ToggleDisplayMode transition Windowed→Fullscreen in MatrixRainTests/unit/state/ApplicationStateTests.cpp
- [ ] T118 [P] [US3] Test ApplicationState ToggleDisplayMode transition Fullscreen→Windowed in MatrixRainTests/unit/state/ApplicationStateTests.cpp
- [ ] T119 [P] [US3] Test RenderSystem swap chain recreation for fullscreen in MatrixRainTests/unit/rendering/SwapChainTests.cpp
- [ ] T120 [P] [US3] Test RenderSystem swap chain recreation for windowed in MatrixRainTests/unit/rendering/SwapChainTests.cpp
- [ ] T121 [P] [US3] Test InputSystem Alt+Enter key combination detection in MatrixRainTests/unit/input/InputSystemTests.cpp
- [ ] T122 [P] [US3] Test animation state preservation during display mode switch in MatrixRainTests/integration/DisplayModeTests.cpp

### Implementation for User Story 3

- [ ] T123 [P] [US3] Create ApplicationState class in MatrixRainCore/include/matrixrain/ApplicationState.h with displayMode enum, densityController, viewport
- [ ] T124 [US3] Implement ApplicationState::Initialize in MatrixRainCore/src/state/ApplicationState.cpp with Windowed default
- [ ] T125 [US3] Implement ApplicationState::ToggleDisplayMode in MatrixRainCore/src/state/ApplicationState.cpp
- [ ] T126 [US3] Implement RenderSystem::RecreateSwapChain in MatrixRainCore/src/rendering/RenderSystem.cpp for mode transitions
- [ ] T127 [US3] Implement RenderSystem::SetFullscreen in MatrixRainCore/src/rendering/RenderSystem.cpp calling IDXGISwapChain::SetFullscreenState
- [ ] T128 [US3] Implement RenderSystem::SetWindowed in MatrixRainCore/src/rendering/RenderSystem.cpp with window style restoration
- [ ] T129 [US3] Implement InputSystem::IsAltEnterPressed in MatrixRainCore/src/input/InputSystem.cpp checking VK_RETURN with Alt modifier
- [ ] T130 [US3] Wire ApplicationState to Application class in MatrixRainCore/src/Application.cpp
- [ ] T131 [US3] Add Alt+Enter handling to window procedure in MatrixRainCore/src/Application.cpp calling ApplicationState::ToggleDisplayMode
- [ ] T132 [US3] Ensure Viewport updates on display mode switch in MatrixRainCore/src/state/Viewport.cpp
- [ ] T133 [US3] Verify swap chain recreation preserves render target view in MatrixRainCore/src/rendering/RenderSystem.cpp
- [ ] T134 [US3] Verify integration test for complete display mode toggle cycle in MatrixRainTests/integration/DisplayModeTests.cpp

**Checkpoint**: All user stories should now be independently functional - full Matrix rain with density controls and display mode toggle

---

## Phase 6: Additional Window Controls & User Experience

**Purpose**: Window manipulation, keyboard shortcuts, and user experience improvements (FR-021 through FR-024)

### Tests for Additional Controls (TDD - Write FIRST)

- [ ] T135 [P] Test window centering on primary monitor at startup in MatrixRainTests/unit/ui/WindowTests.cpp
- [ ] T136 [P] Test WM_NCHITTEST handling for borderless window drag in MatrixRainTests/unit/ui/WindowTests.cpp
- [ ] T137 [P] Test ESC key handling for application exit in MatrixRainTests/unit/input/InputSystemTests.cpp
- [ ] T138 [P] Test Space key handling for pause/resume in MatrixRainTests/unit/input/InputSystemTests.cpp
- [ ] T139 [P] Test pause state freezes animation updates in MatrixRainTests/unit/animation/AnimationSystemTests.cpp
- [ ] T140 [P] Test pause state preserves rendering in MatrixRainTests/unit/rendering/RenderSystemTests.cpp

### Implementation for Additional Controls

- [ ] T141 [P] Implement window centering in Application::Initialize in MatrixRainCore/src/Application.cpp using GetSystemMetrics for screen dimensions
- [ ] T142 [P] Implement WM_NCHITTEST handler in window procedure to return HTCAPTION for client area drag in MatrixRainCore/src/Application.cpp
- [ ] T143 [P] Add ESC key handler (VK_ESCAPE) in InputSystem::ProcessKeyDown to call Application::Shutdown in MatrixRainCore/src/input/InputSystem.cpp
- [ ] T144 [P] Add Space key handler (VK_SPACE) in InputSystem::ProcessKeyDown to toggle pause state in MatrixRainCore/src/input/InputSystem.cpp
- [ ] T145 [P] Add pause state flag to ApplicationState class in MatrixRainCore/include/matrixrain/ApplicationState.h
- [ ] T146 Implement ApplicationState::TogglePause in MatrixRainCore/src/state/ApplicationState.cpp
- [ ] T147 Modify Application::Update to skip AnimationSystem::Update when paused in MatrixRainCore/src/Application.cpp
- [ ] T148 Ensure RenderSystem::Render continues when paused (maintains last frame) in MatrixRainCore/src/rendering/RenderSystem.cpp
- [ ] T149 Verify integration test for pause/resume cycle in MatrixRainTests/integration/PauseResumeTests.cpp

**Checkpoint**: Window manipulation and control shortcuts complete - users can drag window, pause/resume, and exit with ESC

---

## Phase 7: Polish & Cross-Cutting Concerns

**Purpose**: Final quality assurance and performance validation

- [ ] T150 [P] Add frame time instrumentation to Application::Run in MatrixRainCore/src/Application.cpp for performance monitoring
- [ ] T151 [P] Add memory leak detection via debug heap in MatrixRainCore/src/Application.cpp
- [ ] T152 [P] Verify /W4 /WX compliance across all MatrixRainCore source files
- [ ] T153 [P] Run static analysis on MatrixRainCore source files
- [ ] T154 [P] Add error handling for D3D11CreateDevice failure in MatrixRainCore/src/rendering/RenderSystem.cpp
- [ ] T155 [P] Add error handling for swap chain creation failure in MatrixRainCore/src/rendering/RenderSystem.cpp
- [ ] T156 [P] Add error handling for shader compilation failure in MatrixRainCore/src/rendering/RenderSystem.cpp
- [ ] T157 [P] Add error handling for DirectWrite initialization failure in MatrixRainCore/src/characters/CharacterSet.cpp
- [ ] T158 [P] Add logging for key application events in MatrixRainCore/src/Application.cpp
- [ ] T159 [P] Optimize instance buffer updates to use MAP_WRITE_DISCARD in MatrixRainCore/src/rendering/RenderSystem.cpp
- [ ] T160 [P] Profile depth sorting performance with 500 streaks in Release build
- [ ] T161 [P] Profile texture atlas creation time in Release build
- [ ] T162 [P] Verify 60fps performance with 500 streaks at 1080p
- [ ] T163 [P] Verify 3-second fade timing accuracy ±50ms
- [ ] T164 [P] Verify zoom effect visible over 60 seconds
- [ ] T165 [P] Verify display mode toggle completes in <200ms
- [ ] T166 [P] Verify application startup time <1 second
- [ ] T167 [P] Run memory profiler for 1-hour continuous operation
- [ ] T168 [P] Add XML documentation comments to public API headers in MatrixRainCore/include/matrixrain/
- [ ] T169 [P] Update specs/001-matrix-rain/quickstart.md with actual build instructions
- [ ] T170 Run through quickstart.md validation on clean machine
- [ ] T171 Final smoke test of all user stories plus new controls (window drag, pause, ESC)
- [ ] T172 Commit discipline audit - verify one task per commit, all builds passing

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies - can start immediately
- **Foundational (Phase 2)**: Depends on Setup completion - BLOCKS all user stories
- **User Story 1 (Phase 3)**: Depends on Foundational phase - Core MVP animation
- **User Story 2 (Phase 4)**: Depends on Foundational phase - Can start after foundational, but logically follows US1 for testing
- **User Story 3 (Phase 5)**: Depends on Foundational phase - Can start after foundational, but logically follows US1 for testing
- **Polish (Phase 6)**: Depends on all user stories being complete

### User Story Dependencies

- **User Story 1 (P1)**: Can start after Foundational (Phase 2) - No dependencies on other stories - **THIS IS THE MVP**
- **User Story 2 (P2)**: Can start after Foundational (Phase 2) - Integrates with US1 AnimationSystem but independently testable
- **User Story 3 (P3)**: Can start after Foundational (Phase 2) - Integrates with US1 RenderSystem but independently testable

### Within Each User Story

- **TDD Discipline**: Tests MUST be written and FAIL before implementation
- Models before services/systems
- Core systems before integration points
- Integration verification at end of each story
- Story complete and tested before moving to next priority

### Parallel Opportunities

- **Phase 1 Setup**: Tasks T005-T025 marked [P] can all run in parallel (different directories/files)
- **Phase 2 Foundational**: Tasks T026-T035 marked [P] can run in parallel (math types, character constants)
- **User Story 1 Tests**: Tasks T037-T047 marked [P] can all run in parallel (different test files)
- **User Story 1 Models**: Tasks T054, T060 marked [P] can run in parallel (different model files)
- **User Story 2 Tests**: Tasks T092-T100 marked [P] can all run in parallel (different test files)
- **User Story 2 Implementation**: Tasks T101, T107 marked [P] can run in parallel (DensityController and InputSystem)
- **User Story 3 Tests**: Tasks T116-T122 marked [P] can all run in parallel (different test files)
- **User Story 3 Implementation**: Task T123 can start in parallel with other US3 tasks
- **Polish Phase**: Tasks T135-T154 marked [P] can run in parallel (different concerns)

### User Stories Can Run in Parallel (with sufficient team)

Once Foundational phase completes:

- Developer A: User Story 1 (T037-T091)
- Developer B: User Story 2 (T092-T115) - starts after foundational
- Developer C: User Story 3 (T116-T134) - starts after foundational

However, US2 and US3 are easier to test if US1 is complete first, so sequential order (P1→P2→P3) recommended for small teams.

---

## Parallel Example: User Story 1 Models

```bash
# These tasks can run simultaneously (different files):
T054: Create CharacterInstance.h
T056: Create CharacterStreak.h  
T060: Create Viewport.h

# These tasks can also run simultaneously (different files):
T055: Implement CharacterInstance.cpp
T061: Implement Viewport.cpp
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup (T001-T025) - ~2-3 hours
2. Complete Phase 2: Foundational (T026-T036) - ~2-3 hours
3. Complete Phase 3: User Story 1 (T037-T091) - ~20-30 hours
4. **STOP and VALIDATE**: Test User Story 1 independently
   - Launch app → See Matrix rain animation
   - Verify 60fps performance
   - Verify 3-second fade timing
   - Verify depth perspective visible
   - Verify continuous zoom over 60 seconds
5. Deploy/demo MVP if ready

**MVP Delivers**: Core value proposition - the iconic Matrix rain visual effect

### Incremental Delivery

1. **Foundation** (Phase 1 + Phase 2): ~4-6 hours → VS solution ready, foundation complete
2. **MVP** (Phase 3 - US1): ~20-30 hours → Full animated Matrix rain → **DEMO**
3. **User Controls** (Phase 4 - US2): ~4-6 hours → Add +/- density controls → **DEMO**
4. **Display Flexibility** (Phase 5 - US3): ~4-6 hours → Add Alt+Enter fullscreen toggle → **DEMO**
5. **Polish** (Phase 6): ~8-12 hours → Performance optimization, error handling, documentation → **SHIP**

Total estimated effort: **40-60 hours** for complete feature

### Parallel Team Strategy

With 3 developers after foundational phase completes:

- **Dev A (Senior)**: User Story 1 (Core rendering pipeline, DirectX, shaders) - Critical path
- **Dev B (Mid)**: User Story 2 (Input system, density controller) - Can start after foundational
- **Dev C (Mid)**: User Story 3 (Display mode toggle, swap chain recreation) - Can start after foundational

Note: US2 and US3 developers should coordinate with US1 developer for integration points (AnimationSystem, RenderSystem)

---

## Notes

- **[P] tasks** = different files, no dependencies, can run in parallel
- **[Story] label** maps task to specific user story for traceability
- Each user story should be independently completable and testable
- **TDD Mandatory**: Write tests FIRST for core library code, verify they FAIL before implementing
- **Commit Discipline**: After completing each task:
  1. Build solution (verify no compile errors)
  2. Run ALL tests (verify all pass)
  3. Commit ONE task with descriptive message
  4. Never bundle multiple tasks into one commit
- **One Task = One Commit**: Each task T### results in exactly one commit
- **No Partial Commits**: Each commit must leave codebase buildable and all tests passing
- Stop at any checkpoint to validate story independently
- **Performance Validation**: Use VS Performance Profiler in Release build to verify 60fps with 500 streaks
- **Constitution Compliance**: All tasks respect Library-First Architecture, TDD, Performance-First principles
- Avoid: vague tasks, same file conflicts, cross-story dependencies that break independence
