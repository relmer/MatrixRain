# Implementation Plan: Screensaver Install/Uninstall via Command Line

**Branch**: `005-screensaver-install` | **Date**: 2026-03-21 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/005-screensaver-install/spec.md`

## Summary

Add `/install` and `/uninstall` command-line switches to MatrixRain. Install copies the running executable to `%SystemRoot%\System32\MatrixRain.scr` and invokes `desk.cpl,InstallScreenSaver` to set it as active and open the Screen Saver Settings dialog. Uninstall removes the `.scr` file, cleans up registry entries if MatrixRain is still the active screensaver, and exits. Both operations self-elevate via UAC when needed.

## Technical Context

**Language/Version**: C++23 (`/std:c++latest`), Visual Studio 2026 (v18.x)
**Primary Dependencies**: Win32 API (CopyFileW, DeleteFileW, ShellExecuteExW for UAC, rundll32 for desk.cpl), Windows Registry API (RegOpenKeyExW, RegQueryValueExW, RegSetValueExW, RegDeleteValueW)
**Storage**: Windows Registry (`HKCU\Control Panel\Desktop`) — read/write for uninstall, read-only check for install
**Testing**: Microsoft C++ Native Unit Test Framework (`<CppUnitTest.h>`)
**Target Platform**: Windows 11 x64 exclusively
**Project Type**: Single solution — MatrixRainCore (.lib), MatrixRain (.exe), MatrixRainTests
**Performance Goals**: N/A — single-shot CLI operations, no frame rate or latency concerns
**Constraints**: Must run elevated (System32 write access); self-elevation via ShellExecuteEx "runas"
**Scale/Scope**: ~2 new source files in MatrixRainCore, 2 new enum values, ~1 new test file

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Status | Notes |
|-----------|--------|-------|
| I. Test-Driven Development | PASS | New ScreenSaverInstaller class in core lib, tested via unit tests. Entry point dispatch in main.cpp exempt per constitution. |
| II. Performance-First | PASS | Single-shot file copy + registry write — no hot paths. No benchmarks needed. |
| III. C++23 and Windows Native | PASS | Uses Win32 API (CopyFileW, ShellExecuteExW, Registry APIs). C++23 features where appropriate. |
| IV. Modular Architecture | PASS | New ScreenSaverInstaller module with single responsibility. No circular dependencies. |
| V. Type Safety | PASS | Uses enum class for modes, HRESULT for error handling, filesystem paths as wstring. |
| VI. Library-First | PASS | All install/uninstall logic in MatrixRainCore. main.cpp only dispatches on mode. |
| VII. Precompiled Headers | PASS | No new system headers needed beyond what's in pch.h. |
| VIII. Code Formatting | PASS | Will follow column alignment, 5-blank-line separation, etc. |
| IX. Commit Discipline | PASS | One task = one commit. |

All gates pass. No violations to justify.

## Project Structure

### Documentation (this feature)

```text
specs/005-screensaver-install/
├── spec.md              # Feature specification
├── plan.md              # This file
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output
├── quickstart.md        # Phase 1 output
├── contracts/           # Phase 1 output
│   └── api.md           # Public API contracts
├── checklists/
│   └── requirements.md  # Quality checklist
└── tasks.md             # Phase 2 output (from /speckit.tasks)
```

### Source Code (repository root)

```text
MatrixRainCore/                          # Core static library
├── ScreenSaverMode.h                    # MODIFY: Add Install, Uninstall enum values
├── CommandLine.cpp            # MODIFY: Parse "install" and "uninstall" multi-char switches
├── CommandLine.h              # MODIFY: (if needed for new declarations)
├── ScreenSaverInstaller.cpp             # NEW: Install/uninstall logic
├── ScreenSaverInstaller.h              # NEW: ScreenSaverInstaller class declaration
├── UsageText.cpp                        # MODIFY: Add /install and /uninstall to help text
└── pch.h                                # VERIFY: No new system headers needed

MatrixRain/                              # Application executable
└── main.cpp                             # MODIFY: Dispatch Install/Uninstall modes (early exit path)

MatrixRainTests/                         # Test project
└── unit/
    ├── CommandLineTests.cpp   # MODIFY: Add tests for /install and /uninstall parsing
    ├── ScreenSaverInstallerTests.cpp    # NEW: Unit tests for install/uninstall logic
    └── UsageTextTests.cpp               # MODIFY: Verify new switches in help text
```

**Structure Decision**: Follows existing patterns — new module (`ScreenSaverInstaller`) in MatrixRainCore, mode dispatch in main.cpp, tests in MatrixRainTests/unit/.
