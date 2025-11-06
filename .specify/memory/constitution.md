<!--
Sync Impact Report:
- Version change: 1.1.1 → 1.1.2
- Modified principles: 
  * VIII. Code Formatting and Style - Clarified pointer/reference alignment rules in declaration blocks
- Modified sections: None
- Added sections: None
- Removed sections: None
- Templates requiring updates:
  ✅ plan-template.md - Verified compatible
  ✅ spec-template.md - Verified compatible
  ✅ tasks-template.md - Verified compatible
- Follow-up TODOs: None
-->

# MatrixRain Constitution

## Core Principles

### I. Test-Driven Development (NON-NEGOTIABLE)

All code in the **core static library** MUST be developed using Test-Driven Development methodology. Tests are written first, must fail initially, then implementation makes them pass. The Red-Green-Refactor cycle is strictly enforced:

- **Red**: Write a test that fails (demonstrates missing functionality)
- **Green**: Write minimal code to make the test pass
- **Refactor**: Improve code quality while keeping tests green

**Scope Exemption**: The application entry point (WinMain/main in the .exe project) is exempt from TDD requirements. This function should contain minimal logic (typically <10 lines) that simply initializes and invokes the core library.

**Rationale**: TDD ensures correctness, maintainability, and prevents regressions. Tests serve as living documentation and enable confident refactoring. The minimal app entry point requires no testing as it contains no business logic.

**Compliance**: No core library implementation work begins until tests are written and verified to fail. All pull requests must include tests that were written before the implementation code. Unit tests link against the core static library (.lib) to avoid duplicate compilation.

### II. Performance-First Architecture

Performance is a primary design constraint, not an afterthought. When choosing between equivalent implementations (e.g., C++23 standard library vs Windows API), the decision MUST be based on measured performance characteristics for the specific use case.

- Benchmark critical paths before and after changes
- Profile-guided optimization for hot paths
- Zero-copy operations preferred where applicable
- Cache-friendly data structures and algorithms
- Minimize allocations in performance-critical sections

**Rationale**: MatrixRain is a real-time GUI application where performance directly impacts user experience. Sluggish rendering or UI responsiveness is unacceptable.

**Compliance**: Performance-sensitive code paths must include benchmark tests. Design documents must justify performance trade-offs when choosing between implementation approaches.

### III. C++23 and Windows Native Platform

The project targets modern C++23 features on the Windows 11 platform exclusively using Visual Studio 2026 (version 18.x) as the minimum requirement. All code MUST:

- Compile with C++23 standard (`/std:c++latest`)
- Target 64-bit x64 architecture only (no 32-bit support)
- Target Windows 11 with the latest Windows SDK (no Windows 10 or older SDK support)
- Use Windows API for platform-specific functionality (GUI, threading, file I/O)
- Leverage C++23 features: modules, ranges, concepts, coroutines, std::expected, etc.
- Follow Windows-specific conventions (UTF-16 for Win32 APIs, COM where applicable)
- When choosing between C++23 standard library and Windows API equivalents, select based on measured performance for the specific use case

**Rationale**: Modern C++ provides safety, expressiveness, and performance. Windows API integration is essential for native GUI development. 64-bit removes memory limitations. Windows 11 and the latest SDK provide modern APIs and capabilities. VS 2026 is required for complete C++23 feature support.

**Compliance**: All source files must compile with `/std:c++latest`. Platform detection must verify Windows 11 x64 at build time. Performance-critical path choices between C++23 and WinAPI must be documented with benchmark justification.

### IV. Modular Architecture

Code MUST be organized into well-defined modules with clear boundaries and minimal coupling:

- Each module has a single, well-defined responsibility
- Interfaces defined using C++20/23 concepts where appropriate
- Dependencies flow in one direction (no circular dependencies)
- Modules are independently testable
- Public APIs documented with purpose and usage examples

**Rationale**: Modularity enables parallel development, easier testing, and maintainability. Clear boundaries prevent coupling and make the codebase navigable.

**Compliance**: Design reviews must verify module boundaries. Build system must enforce dependency direction. Each module requires interface documentation.

### V. Type Safety and Modern C++ Idioms

Leverage C++'s type system to catch errors at compile time. MUST follow modern C++ best practices:

- Prefer `std::unique_ptr`, `std::shared_ptr` over raw pointers
- Use `std::optional` for optional values, `std::expected` for error handling
- Employ `constexpr` and `consteval` for compile-time computation
- Use `std::span` for non-owning views of contiguous data
- Apply `const` correctness throughout
- Prefer `enum class` over plain `enum`
- Use concepts to constrain template parameters

**Rationale**: Type safety eliminates entire classes of runtime errors. Modern idioms make intent clear and reduce cognitive load. Compile-time checking is faster than runtime debugging.

**Compliance**: Code reviews must verify adherence to modern C++ guidelines. Static analysis tools must be configured to enforce these practices.

### VI. Library-First Architecture

The application MUST be split into two distinct components:

- **Core Static Library** (`.lib`): Contains ALL application functionality, business logic, algorithms, and testable code. This is where development effort is concentrated.
- **Application Executable** (`.exe`): Contains ONLY the entry point (WinMain/main). Should be minimal (<10 lines) and simply initialize/invoke the core library. No business logic permitted.

**Rationale**: This separation enables efficient testing without recompiling source files for both the app and tests. Unit tests link against the core library directly. The thin app layer ensures fast build times and focuses testing effort on actual functionality.

**Compliance**: Code reviews must reject any business logic in the .exe project. All pull requests adding functionality must target the core library. The app project's only responsibility is program entry.

### VII. Precompiled Headers Strategy

Precompiled headers MUST be used to improve build performance. All system headers MUST follow these rules:

- **System headers** (angle brackets `<>`) MUST be included ONLY in `pch.h`
- **Never** include system headers in other source files (`.cpp`) or project headers (`.h`)
- Headers within `pch.h` MUST be alphabetically sorted within category groups:
  - C++ standard library headers (e.g., `<algorithm>`, `<memory>`, `<string>`)
  - Windows SDK headers (e.g., `<windows.h>`, `<windowsx.h>`)
  - DirectX headers (if used)
  - Other third-party library headers
- Alphabetical sorting may be suspended only when header dependencies require specific ordering
- **Unit test PCH**: The test project's `pch.h` MUST `#include` the core library's `pch.h`, then add only test-framework-specific system headers (e.g., `<CppUnitTest.h>`)

**Rationale**: Centralizing system headers in PCH eliminates redundant parsing, reduces build times, and prevents header order issues. Reusing the core library PCH in tests avoids duplication and ensures consistency.

**Compliance**: Code reviews must reject any `#include <...>` in non-PCH files. Build warnings about PCH usage must be resolved. Automated checks should scan for violations.

### VIII. Code Formatting and Style

Code MUST follow these formatting conventions for readability and consistency:

**Whitespace**:

- Use blank lines liberally to separate logical chunks within functions
- Use **5 blank lines** between functions or other dissimilar top-level constructs (e.g., class definitions, function definitions)
- Separate conceptually distinct blocks of code within a function with 1-2 blank lines

**Declaration/Definition Alignment**:

When multiple related declarations or definitions appear consecutively, format them as a table with aligned columns:

- Type (leftmost)
- Pointer/reference symbol (`*`, `&`) **as a separate column** if present on ANY line in the block
- Variable/function name
- Assignment operator (`=`) if present  
- Initializer/value (rightmost)

**Critical Rule**: If ANY declaration in a contiguous block has a pointer/reference symbol, ALL declarations in that block MUST align with the pointer/reference column (non-pointer types skip that column with whitespace).

**Example**:

```cpp
int                    width       = 800;
int                    height      = 600;
std::unique_ptr<Foo>   fooPtr      = std::make_unique<Foo>();
const char           * windowTitle = "MatrixRain";
```

Note: The pointer `*` is aligned as a separate column. Non-pointer types (`int`, `std::unique_ptr<Foo>`) have whitespace in the pointer column to maintain alignment.

**Rationale**: Consistent column alignment makes pointer/reference declarations visually distinct and prevents the pointer symbol from being visually attached to either the type or the variable name, reducing cognitive errors.

**Compliance**: Code reviews should check formatting. Consider using `.clang-format` or similar tooling to automate alignment where feasible.

### IX. Commit Discipline

Each task implementation MUST result in exactly one commit. Commits must be atomic and focused:

- **After implementing a task**: Build the code, run all tests, verify success, THEN commit
- **One task = One commit**: Do not bundle multiple tasks into a single commit
- **No partial work commits**: Each commit must leave the codebase in a buildable, testable state
- Commit messages should reference the task ID and briefly describe what was implemented

**Rationale**: Atomic commits enable precise history bisection, easier rollbacks, and clear change tracking. Each commit represents a logical unit of work that can be understood, reviewed, and reverted independently.

**Compliance**: Pull requests with multi-task commits will be rejected. CI/CD should verify that each commit in a PR builds and passes tests individually.

## Technical Platform Requirements

**Language**: C++23  
**Compiler**: Visual Studio 2026 (version 18.x) minimum - do not target older versions  
**Architecture**: x86-64 (64-bit) exclusively  
**Target OS**: Windows 11 exclusively with latest Windows SDK - no Windows 10 support  
**Build System**: Visual Studio solution (.sln) and project files (.vcxproj)  
**Testing Framework**: Microsoft C++ Native Unit Test Framework (`<CppUnitTest.h>`)  
**UI Framework**: Win32 API (GDI, GDI+, or Direct2D/DirectWrite for hardware acceleration)  
**Threading Model**: C++20 coroutines and/or Windows thread pool  
**Character Encoding**: UTF-16 for Windows API interop, UTF-8 internally where feasible

**Project Structure**:

- **MatrixRainCore** (static library, `.lib`): All application functionality
- **MatrixRain** (executable, `.exe`): Minimal entry point only
- **MatrixRainTests** (native unit test project): Links against MatrixRainCore.lib

**Performance Baselines** (to be refined during development):

- UI frame rate: Target 60fps minimum for smooth animations
- Startup time: <500ms from launch to visible window
- Memory footprint: TBD based on feature set
- Input latency: <16ms for user interactions

## Build and IDE Configuration

**Visual Studio Solution**:

- Generate and maintain `.sln` and `.vcxproj` files for Visual Studio 2026 (minimum)
- Configure all projects for C++23 (`/std:c++latest`)
- Enable precompiled headers for all projects
- Configure output directories to separate Debug/Release builds
- Link test project against core library (avoid duplicate compilation)
- Set warning level 4 (`/W4`) and treat warnings as errors (`/WX`) for all projects
- Debug configuration: Enable debug heap, heap corruption checking, memory leak detection

**VS Code Integration**:

- Generate `.vscode/tasks.json` for build tasks (Debug/Release configurations)
- Generate `.vscode/launch.json` for debugging the executable and tests
- Generate `.vscode/c_cpp_properties.json` at the workspace root AND in each project subdirectory
- Each project subdirectory's `c_cpp_properties.json` MUST specify correct include paths for that project
- IntelliSense configuration must match Visual Studio project settings (`/std:c++latest`, includes, defines)

**Configuration Requirements**:

- Each project subdirectory needs its own `c_cpp_properties.json` for accurate IntelliSense
- Include paths must be project-specific (core library includes differ from app/test includes)
- Defines must match build configuration (e.g., `_DEBUG`, `NDEBUG`, `WIN32`, `_WINDOWS`, `_CRTDBG_MAP_ALLOC`)
- Compiler path must point to Visual Studio 2026 MSVC toolset (minimum version 18.x)

## Development Standards

**Code Quality Gates**:

- All code must pass static analysis (e.g., clang-tidy, MSVC analyzer)
- MUST compile with warning level 4 (`/W4`) and treat warnings as errors (`/WX`)
- Warning-free compilation is MANDATORY - no warnings permitted
- Code coverage target: TBD (to be defined after initial test suite established)
- All public APIs must have documentation comments

**Debug Build Requirements**:

- Debug builds MUST use the debug heap allocator
- Enable additional heap corruption checking in Debug configuration
- Use `_CRTDBG_MAP_ALLOC` to enable detailed memory leak detection
- Configure Debug CRT to report memory leaks on program exit

**Workflow**:

1. Feature specification with test scenarios (see `.specify/templates/spec-template.md`)
2. Implementation plan with architecture review (see `.specify/templates/plan-template.md`)
3. Write tests (verify they fail)
4. Implement feature (make tests pass)
5. Refactor for quality/performance
6. Code review verifying constitution compliance
7. Merge to main branch

**Review Requirements**:

- At least one approver for all changes
- Reviewer must verify TDD compliance (tests before implementation)
- Reviewer must verify performance considerations for critical paths
- Reviewer must verify modern C++ idiom usage

**Documentation**:

- Public APIs documented with purpose, parameters, return values, and exceptions
- Complex algorithms explained with rationale
- Performance characteristics noted for critical paths
- Design decisions captured in architecture decision records (ADRs) when significant

## Governance

This constitution supersedes all other development practices and conventions. All contributors must understand and follow these principles.

**Amendment Process**:

- Proposed amendments must be documented with rationale
- Major changes (principle additions/removals) require team consensus
- Amendments must include impact analysis on existing code
- Version must be updated following semantic versioning

**Compliance Enforcement**:

- All pull requests must be reviewed for constitutional compliance
- Automated checks (linters, static analysis, architecture tests) enforce technical standards
- Complexity exceptions must be explicitly justified and approved
- Deviations require documented rationale and approval

**Versioning Policy**:

- MAJOR: Backward-incompatible principle changes, fundamental architecture shifts
- MINOR: New principles added, significant guidance expansions
- PATCH: Clarifications, wording improvements, non-semantic refinements

**Version**: 1.1.2 | **Ratified**: 2025-11-05 | **Last Amended**: 2025-11-05
