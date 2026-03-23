# Implementation Plan: Runtime Help Overlay and Command-Line Help

**Branch**: `003-runtime-help` | **Date**: 2026-03-02 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/003-runtime-help/spec.md`

## Summary

Add runtime help overlay with matrix rain reveal/dissolve effects (GPU-rendered via D3D11 instancing with D2D atlas) and command-line help (`/?`/`-?`) via a custom usage dialog window with its own D3D/D2D rendering context. Adds Enter key → config dialog, ? key → in-app hotkey overlay (rendered to scene texture before bloom for free glow), unrecognized key → re-show hint. Overlay background halos rendered via D3D11 SDF pixel shader (signed distance to per-row rounded rects). All rendering is GPU-based — no console output, no ANSI escape codes.

## Technical Context

**Language/Version**: C++23 (`/std:c++latest`), Visual Studio 2026 (v18.x)
**Primary Dependencies**: Direct3D 11.1, Direct2D 1.1, DirectWrite, Win32 API
**Storage**: N/A (no persistence; registry settings handled by existing `RegistrySettingsProvider`)
**Testing**: Microsoft C++ Native Unit Test Framework (`CppUnitTest.h`), VSTest runner
**Target Platform**: Windows 11 x64
**Project Type**: Single solution — 3 projects (MatrixRainCore.lib, MatrixRain.exe, MatrixRainTests.dll)
**Performance Goals**: 60 FPS render loop unaffected by overlay; usage dialog renders at 60 FPS independently
**Constraints**: Overlays render to scene texture before bloom (single-threaded render loop); CLI help (`/?`) runs before main window creation; usage dialog has its own D3D/D2D rendering context
**Scale/Scope**: ~15 FRs, 5 user stories; touches input handling, render system, command-line parser; all new code in MatrixRainCore.lib

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*
*Post-design re-evaluation: All 9 principles confirmed PASS — no new violations from Phase 1 design decisions.*

| # | Principle | Status | Notes |
|---|-----------|--------|-------|
| I | Test-Driven Development | PASS | All new classes (`HelpHintOverlay`, `UsageDialog`, `UsageText`) in MatrixRainCore.lib — testable via TDD. Overlay and dialog animation state is pure logic (timers, phase transitions, per-cell state) — fully unit-testable without D2D. |
| II | Performance-First Architecture | PASS | Overlay rendered to scene texture before bloom — bloom provides glow for free (zero additional draw calls for glow layers). Per-row halo backgrounds rendered via a single fullscreen SDF shader pass. Halo row rects cached per overlay. Per-character state is flat arrays (cache-friendly). Usage dialog has its own lightweight D3D/D2D context. `?` key overlay rendered inline in the main render loop. |
| III | C++23 and Windows Native Platform | PASS | Uses C++23, targets Windows 11 x64, Win32 API, D3D11/D2D1 for all rendering. No console output or ANSI escape codes. |
| IV | Modular Architecture | PASS | New modules: `HelpHintOverlay` (overlay state machine + per-char animation), `UsageDialog` (usage dialog with scramble-reveal renderer), `UsageText` (shared text/layout). Clear boundaries, one responsibility each. No circular deps. |
| V | Type Safety and Modern C++ | PASS | `enum class` for overlay phases, `std::optional` for stagger offsets, `std::span` for character grid views, `constexpr` for timing constants. |
| VI | Library-First Architecture | PASS | All new logic in MatrixRainCore.lib. |
| VII | Precompiled Headers Strategy | PASS | No new system headers needed — existing pch.h already includes `<chrono>`, `<random>`, `<string>`, `<vector>`, `<algorithm>`, Windows.h, D2D, DirectWrite. |
| VIII | Code Formatting and Style | PASS | Will follow column-aligned declarations, 5-blank-line spacing, pointer column alignment per constitution. |
| IX | Commit Discipline | PASS | One task = one commit. Each commit builds and passes all tests. |

## Project Structure

### Documentation (this feature)

```text
specs/003-runtime-help/
├── plan.md              # This file
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output
├── quickstart.md        # Phase 1 output
├── contracts/           # Phase 1 output
│   ├── help-hint-overlay.md
│   ├── usage-dialog.md
│   ├── usage-text.md
│   └── command-line-help.md
├── checklists/
│   └── requirements.md  # Quality checklist
└── tasks.md             # Phase 2 output (created by /speckit.tasks)
```

### Source Code (repository root)

```text
MatrixRainCore/                    # Static library — all new feature code here
├── HelpHintOverlay.h/cpp          # NEW: Overlay state machine + per-character animation (3-line hint)
├── UsageOverlay.h/cpp             # NEW: Usage text overlay for /? mode (GPU-rendered, replaces UsageDialog)
├── UsageDialog.h/cpp              # DEPRECATED: Old D2D usage dialog (to be removed after unified Overlay refactor)
├── HotkeyOverlay.h/cpp            # NEW: In-app hotkey reference overlay (? key, rendered on main window)
├── ScrambleRevealEffect.h/cpp     # NEW: Shared per-cell scramble-reveal timing oracle (reveal/hold/dismiss)
├── UsageText.h/cpp                # NEW: Command-line switch text content + formatting
├── CommandLineHelp.h/cpp          # NEW: /? orchestration (legacy — /? now routes through Application)
├── UnicodeSymbols.h               # NEW: Named constants for Unicode characters (em dash, etc.)
├── CommandLine.h/cpp    # MODIFIED: Add /? parsing, prefix detection
├── Application.h/cpp              # MODIFIED: Enter/? keys, overlay show/dismiss, HelpRequested mode
├── InputSystem.h/cpp              # MODIFIED: Unrecognized key → help hint trigger
├── RenderSystem.h/cpp             # MODIFIED: Render overlay to scene texture, SDF halo shader, RenderParams struct
├── AnimationSystem.h/cpp          # MODIFIED: Overlay character support
├── ApplicationState.h             # MODIFIED: Help hint enabled flag
└── ...existing files...

MatrixRain/                        # Executable — minimal changes
├── main.cpp                       # MODIFIED: CLI help early exit before window creation
├── UnicodeSymbols.h               # NEW: Named Unicode constants (shared header)
└── ...existing files...

MatrixRainTests/                   # Test project
├── unit/
│   ├── HelpHintOverlayTests.cpp   # NEW: Overlay state machine tests
│   ├── UsageDialogTests.cpp       # NEW: Scramble-reveal animation + character position tests
│   ├── HotkeyOverlayTests.cpp     # NEW: ? key overlay state tests
│   ├── ScrambleRevealEffectTests.cpp  # NEW: Scramble-reveal timing oracle tests
│   ├── UsageTextTests.cpp          # NEW: Text formatting + prefix detection tests
│   ├── CommandLineHelpTests.cpp    # NEW: Orchestration tests
│   └── ...existing tests...
└── ...existing files...
```

**Structure Decision**: Single solution with 3 projects (MatrixRainCore.lib, MatrixRain.exe, MatrixRainTests.dll). All new logic in the core library per Constitution VI. `ScrambleRevealEffect` is the shared timing oracle for all overlay reveal/dismiss animations — drives per-cell staggered scramble-reveal with configurable timing parameters. `HelpHintOverlay`, `HotkeyOverlay`, and `UsageDialog` delegate all timing to `ScrambleRevealEffect` and render characters based on its queries. `UsageDialog` creates its own D3D11/D2D rendering context for the usage dialog — independent of the main application's render pipeline. The `?` key hotkey overlay (`HotkeyOverlay`) renders directly on the main window via the existing D2D render pipeline. Six new module pairs (`.h`/`.cpp`) plus `UnicodeSymbols.h` in the core library, six new test files. Minimal changes to the .exe project (main.cpp early exit only).

## Complexity Tracking

No constitution violations — all gates passed. No complexity exceptions needed.
