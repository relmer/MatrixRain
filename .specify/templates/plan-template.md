# Implementation Plan: [FEATURE]

**Branch**: `[###-feature-name]` | **Date**: [DATE] | **Spec**: [link]
**Input**: Feature specification from `/specs/[###-feature-name]/spec.md`

**Note**: This template is filled in by the `/speckit.plan` command. See `.specify/templates/commands/plan.md` for the execution workflow.

## Summary

[Extract from feature spec: primary requirement + technical approach from research]

## Technical Context

<!--
  ACTION REQUIRED: Verify and update these baseline settings for the specific feature.
  These defaults reflect the MatrixRain project constitution.
-->

**Language/Version**: C++23 (Visual Studio 2026 minimum, version 18.x)  
**Primary Dependencies**: Win32 API (USER32, GDI32, or Direct2D/DirectWrite), [feature-specific libs or NEEDS CLARIFICATION]  
**Storage**: [if applicable, e.g., file system, registry, SQLite or N/A]  
**Testing**: Microsoft C++ Native Unit Test Framework (TDD mandatory - tests before implementation)  
**Target Platform**: Windows 11 with latest SDK (x86-64 only, no Windows 10 support)  
**Project Structure**:

- MatrixRainCore.lib (core library - all functionality, 100% test coverage)
- MatrixRain.exe (entry point only - minimal, no testing required)
- MatrixRainTests (native unit test project)

**Performance Goals**: 60fps UI rendering, <500ms startup, <16ms input latency, [feature-specific goals or NEEDS CLARIFICATION]  
**Constraints**: Performance-driven design, C++23 vs Windows API choice based on benchmarks, precompiled headers mandatory, [feature-specific or NEEDS CLARIFICATION]  
**Scale/Scope**: [feature-specific, e.g., number of UI components, data volume, concurrent operations or NEEDS CLARIFICATION]

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

Verify compliance with `.specify/memory/constitution.md`:

- [ ] **TDD Compliance**: Tests written before implementation for core library, Red-Green-Refactor cycle enforced
- [ ] **Performance Analysis**: Performance-critical paths identified, benchmark tests planned, C++23 vs WinAPI choice justified
- [ ] **C++23/Windows Platform**: Confirms C++23 features, Win32 API usage, Windows 11 target, latest SDK, VS 2026 minimum
- [ ] **Modular Design**: Clear module boundaries, no circular dependencies, testable in isolation
- [ ] **Type Safety**: Modern C++ idioms planned (smart pointers, std::expected, concepts, const correctness)
- [ ] **Library-First Architecture**: All functionality in MatrixRainCore.lib, minimal .exe entry point
- [ ] **Precompiled Headers**: System headers only in pch.h, alphabetically sorted by category, test PCH includes core PCH
- [ ] **Code Formatting**: Whitespace strategy defined (5 lines between functions), declaration alignment planned
- [ ] **Commit Discipline**: One task per commit planned, build+test verification before commit
- [ ] **Build Configuration**: VS 2026 solution/projects, `/W4 /WX` enabled, debug heap configured, VS Code integration (tasks.json, launch.json, per-project c_cpp_properties.json)

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

**Structure Decision**: [Document the specific modules and directories for this feature,
e.g., "This feature adds a new 'animation' module under MatrixRainCore/src/animation/ with
corresponding tests in MatrixRainTests/unit/animation/. All functionality goes in the core
library; the .exe simply calls the library's initialization function."]

## Complexity Tracking

> **Fill ONLY if Constitution Check has violations that must be justified**

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| [e.g., 4th project] | [current need] | [why 3 projects insufficient] |
| [e.g., Repository pattern] | [specific problem] | [why direct DB access insufficient] |
