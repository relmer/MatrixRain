# Implementation Plan: Matrix Rain Visual Effect

**Branch**: `001-matrix-rain` | **Date**: 2025-11-05 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/001-matrix-rain/spec.md`

**Note**: This template is filled in by the `/speckit.plan` command. See `.specify/templates/commands/plan.md` for the execution workflow.

## Summary

Matrix Rain is a real-time visual effect application that renders cascading character animations reminiscent of The Matrix film. The core requirement is to display falling streaks of characters (71 half-width katakana + Latin letters + numerals) with depth perspective, continuous zoom, 3-second fade-out, glow effects, and user controls for density adjustment and display mode switching. Technical approach uses DirectX for hardware-accelerated rendering, achieving 60fps performance with frame-time-independent animation logic to ensure smooth visual effects across varying system loads.

## Technical Context

<!--
  ACTION REQUIRED: Verify and update these baseline settings for the specific feature.
  These defaults reflect the MatrixRain project constitution.
-->

**Language/Version**: C++23 (Visual Studio 2026 minimum, version 18.x)  
**Primary Dependencies**: DirectX 11/12 (D3D11/D3D12, DXGI), Direct2D/DirectWrite for text rendering, Win32 API (USER32 for window management, keyboard input)  
**Storage**: N/A (no persistent data storage required)  
**Testing**: Microsoft C++ Native Unit Test Framework (TDD mandatory - tests before implementation)  
**Target Platform**: Windows 11 with latest SDK (x86-64 only, no Windows 10 support)  
**Rendering Architecture**: Perspective projection with camera movement (authentic depth perception) combined with variable streak speeds based on Z-position (compound depth effect)  
**Project Structure**:

- MatrixRainCore.lib (core library - all functionality, 100% test coverage)
- MatrixRain.exe (entry point only - minimal, no testing required)
- MatrixRainTests (native unit test project)

**Performance Goals**: 60fps rendering at 1080p and higher resolutions, <1 second application startup, <200ms display mode toggle, 3-second character fade with ±50ms precision, <16ms input response for density controls  
**Constraints**: Performance-driven design (DirectX chosen for GPU acceleration), real-time animation requiring frame-time-independent update logic, character glyph rendering requires font/texture atlas generation, zoom wraps at Z=100.0 boundary using modulo to prevent numerical overflow  
**Scale/Scope**: 10-500 concurrent character streaks (density levels 1-10 using formula: targetStreaks = 10 + (level-1)*54), 71 katakana + 52 Latin + 10 numerals = 133 total character glyphs, streak lengths randomized 10-30 characters, viewport adapts to any resolution (windowed 800x600 minimum to 4K+ full-screen), continuous operation for hours without performance degradation

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

Verify compliance with `.specify/memory/constitution.md`:

- [x] **TDD Compliance**: Tests written before implementation for core library, Red-Green-Refactor cycle enforced - All animation logic, rendering systems, input handling, and state management will be test-driven
- [x] **Performance Analysis**: Performance-critical paths identified, benchmark tests planned, C++23 vs WinAPI choice justified - DirectX selected for GPU acceleration (vs GDI/GDI+), critical paths: character rendering loop, fade calculation, depth sorting, spawn frequency management
- [x] **C++23/Windows Platform**: Confirms C++23 features, Win32 API usage, Windows 11 target, latest SDK, VS 2026 minimum - Using C++23 ranges for character iteration, std::expected for error handling, Win32 for window/input, DirectX for rendering
- [x] **Modular Design**: Clear module boundaries, no circular dependencies, testable in isolation - Modules: Animation (streak management), Rendering (DirectX wrapper), Input (keyboard handler), State (density/display mode), no circular dependencies
- [x] **Type Safety**: Modern C++ idioms planned (smart pointers, std::expected, concepts, const correctness) - std::unique_ptr for streak ownership, std::optional for character mutation, concepts for drawable entities, const correctness throughout
- [x] **Library-First Architecture**: All functionality in MatrixRainCore.lib, minimal .exe entry point - Core library contains all rendering, animation, and input logic; .exe only hosts WinMain and calls library initialization
- [x] **Precompiled Headers**: System headers only in pch.h, alphabetically sorted by category, test PCH includes core PCH - DirectX headers, Win32 headers, C++ standard headers all in pch.h; test PCH includes core PCH plus <CppUnitTest.h>
- [x] **Code Formatting**: Whitespace strategy defined (5 lines between functions), declaration alignment planned - Standard formatting rules apply: 5-line separation, tabular alignment with pointer column
- [x] **Commit Discipline**: One task per commit planned, build+test verification before commit - Each implementation task results in one atomic commit after build and test success
- [x] **Build Configuration**: VS 2026 solution/projects, `/W4 /WX` enabled, debug heap configured, VS Code integration (tasks.json, launch.json, per-project c_cpp_properties.json) - Full VS 2026 solution with three projects, per-project IntelliSense configuration, debug heap enabled

**Initial Compliance Assessment**: ✅ PASS - All constitutional requirements satisfied. No violations requiring justification.

## Project Structure

### Documentation (this feature)

```text
specs/[###-feature]/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (/speckit.plan command)
├── data-model.md        # Phase 1 output (/speckit.plan command)
├── quickstart.md        # Phase 1 output (/speckit.plan command)
├── contracts/           # Phase 1 output (/speckit.plan command)
└── tasks.md             # Phase 2 output (/speckit.tasks command - NOT created by /speckit.plan)
```

### Source Code (repository root)
<!--
  ACTION REQUIRED: Adapt the structure below for this specific feature.
  Expand with real module names and paths. Follow Library-First Architecture.
-->

```text
# MatrixRain Project Structure (Library-First Architecture)

MatrixRain.sln                    # Visual Studio solution

MatrixRainCore/                   # Core Static Library (.lib) - ALL functionality
├── MatrixRainCore.vcxproj
├── pch.h                         # Precompiled header: system headers only, alphabetically sorted
├── pch.cpp
├── include/                      # Public headers
│   └── [module_name]/
│       └── [feature].h
├── src/                          # Implementation files
│   ├── [module_name]/
│   │   ├── [feature].cpp
│   │   └── [feature_impl].cpp
│   └── ui/                       # GUI-related code
│       ├── windows/              # Window classes and procedures
│       ├── controls/             # Custom controls
│       └── rendering/            # GDI/Direct2D rendering code
└── .vscode/
    └── c_cpp_properties.json     # IntelliSense config for core library

MatrixRain/                       # Application Executable (.exe) - Entry point ONLY
├── MatrixRain.vcxproj
├── main.cpp                      # WinMain - minimal (<10 lines), calls core library
└── .vscode/
    └── c_cpp_properties.json     # IntelliSense config for app

MatrixRainTests/                  # Native Unit Test Project
├── MatrixRainTests.vcxproj
├── pch.h                         # Includes ../MatrixRainCore/pch.h + <CppUnitTest.h>
├── pch.cpp
├── unit/                         # Unit tests
│   └── [module_name]/
│       └── [feature]Tests.cpp
├── integration/                  # Integration tests
│   └── [feature_flow]Tests.cpp
└── .vscode/
    └── c_cpp_properties.json     # IntelliSense config for tests

.vscode/                          # Workspace-level VS Code configuration
├── tasks.json                    # Build tasks (Debug/Release)
├── launch.json                   # Debug configurations
└── c_cpp_properties.json         # Workspace-level IntelliSense config
```

**Structure Decision**: This feature implements the complete Matrix Rain application with the following module organization:

**MatrixRainCore.lib modules**:

- `src/animation/` - Character streak management, spawn/update/removal logic, fade timing
- `src/rendering/` - DirectX initialization, character texture rendering, glow effects, depth sorting
- `src/input/` - Keyboard input handling (density controls, display mode toggle)
- `src/state/` - Application state management (density level, display mode, viewport dimensions)
- `src/characters/` - Character set management (133 glyphs: 71 katakana + 52 Latin + 10 numerals), random selection
- `include/matrixrain/` - Public API headers for library initialization and shutdown

**MatrixRain.exe**:

- `main.cpp` - WinMain entry point (<10 lines): creates window, initializes core library, runs message loop

**MatrixRainTests modules**:

- `unit/animation/` - Tests for streak lifecycle, fade calculations, mutation probability
- `unit/rendering/` - Tests for depth sorting, viewport transformations, glow intensity calculations
- `unit/input/` - Tests for keyboard event handling, density adjustments, mode toggling
- `unit/state/` - Tests for state transitions, boundary enforcement (min/max density)
- `unit/characters/` - Tests for character set completeness, random distribution, mirroring logic
- `integration/` - End-to-end animation flow tests, display mode switching with state preservation

## Complexity Tracking

> **Fill ONLY if Constitution Check has violations that must be justified**

**Status**: No constitutional violations identified. All requirements satisfied within standard architecture.

The following design decisions were validated against constitutional principles:

- **DirectX 11 Selection**: Performance-First principle satisfied - GPU acceleration required for 60fps target with 10-500 concurrent streaks. Simpler GDI/GDI+ insufficient for real-time blending/glow effects.
- **Texture Atlas Approach**: Performance-First principle satisfied - Pre-rendering 266 glyphs (133 normal + 133 mirrored) to single texture eliminates per-frame rasterization overhead. Direct2D text rendering per-frame would violate 60fps requirement.
- **Five Core Modules**: Modular Architecture principle satisfied - Animation, Rendering, Input, State, Characters modules have clear boundaries with no circular dependencies. Each testable in isolation.

No additional complexity beyond constitutional baseline. Library-First Architecture maintained (3-project structure). No additional layers, patterns, or abstractions required.
