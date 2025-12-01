# Implementation Plan: MatrixRain Screensaver Experience

**Branch**: `002-add-screensaver-support` | **Date**: 2025-11-30 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/specs/002-add-screensaver-support/spec.md`

## Summary

Extend MatrixRain into a Windows 11-compatible screensaver while preserving the desktop app. Implement full screensaver argument handling (`/s`, `/p`, `/c`, `/a`), persist configuration in the registry, surface a Win32 settings dialog, and adjust runtime behavior (cursor hiding, input exit, hotkey suppression). Build pipeline must emit both `.exe` and `.scr` artifacts from the same binary.

## Technical Context

**Language/Version**: C++26 (MSVC preview `/std:c++latest`)  
**Primary Dependencies**: Win32 API, DirectX 11 rendering stack, Project EHM macros, Microsoft C++ Native Unit Test Framework  
**Storage**: Windows Registry (`HKCU\Software\relmer\MatrixRain`) for per-user settings  
**Testing**: Microsoft C++ Native Unit Test Framework (MatrixRainTests project)  
**Target Platform**: Windows 11 x64 desktop  
**Project Type**: Native Win32 application split into `MatrixRainCore` static library + thin `MatrixRain` executable  
**Performance Goals**: Maintain 60 FPS animation baseline with no regressions to startup time (<500 ms)  
**Constraints**: Must use EHM macros for error handling, preserve existing hotkey experience outside screensaver mode, hide cursor reliably in `/s` mode  
**Scale/Scope**: Single-user desktop deployment, distributed via existing build artifacts

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- **G1 – TDD Enforcement (MatrixRainCore)**: All new runtime branches and persistence helpers will land in `MatrixRainCore` with failing unit tests added first in `MatrixRainTests`. Entry-point wiring in `MatrixRain` remains a thin delegator. **Status: PASS**
- **G2 – Performance-First**: Plan maintains existing DirectX render loop with profiling hooks; registry and dialog logic run off the render hot path. Full-screen mode changes avoid additional per-frame allocations. **Status: PASS**
- **G3 – Platform & Toolchain Compliance**: Work targets Visual Studio 2026 (C++26 preview) and Win32/DirectX stack already configured in the solution; no cross-platform deviations introduced. **Status: PASS**
- **G4 – Modular Architecture**: Screensaver behaviors extend the core application state machine and controller classes without cross-module leakage. New dialog code isolated behind dedicated module boundaries. **Status: PASS**
- **G5 – Formatting & PCH Conventions**: All new Windows headers continue to live in project `pch.h`, and column alignment / blank-line rules will be followed per global instructions. **Status: PASS**

## Project Structure

### Documentation (this feature)

```text
specs/002-add-screensaver-support/
├── plan.md              # Implementation plan (this file)
├── research.md          # Phase 0 research outputs
├── data-model.md        # Phase 1 domain model
├── quickstart.md        # Phase 1 implementation quickstart
├── contracts/           # Phase 1 API/ABI contracts
└── tasks.md             # Phase 2 task backlog (created by /speckit.tasks)
```

### Source Code (repository root)
<!--
  ACTION REQUIRED: Replace the placeholder tree below with the concrete layout
  for this feature. Delete unused options and expand the chosen structure with
  real paths (e.g., apps/admin, packages/something). The delivered plan must
  not include Option labels.
-->

```text
MatrixRain.sln

MatrixRain/                  # Executable shell (WinMain dispatch)
├── main.cpp
├── MatrixRain.vcxproj
├── pch.h / pch.cpp
└── resource files (icons, .rc)

MatrixRainCore/              # Core library (logic + rendering)
├── include/matrixrain/      # Public headers
├── src/                     # Implementation (animation, rendering, state, ui)
├── pch.h / pch.cpp
└── MatrixRainCore.vcxproj

MatrixRainTests/             # Native test project (links to MatrixRainCore.lib)
├── unit/ + integration/ suites
├── pch.h / pch.cpp
└── MatrixRainTests.vcxproj

scripts/                     # Build/test scripts (PowerShell)
assets/                      # Shared media resources
```

**Structure Decision**: Continue leveraging the existing Visual Studio solution structure (core static library + thin exe + test project). New screensaver configuration logic will be implemented inside `MatrixRainCore` (with headers under `include/matrixrain/state/` or `ui/` as appropriate) while the executable gains only minimal command-line routing.

## Complexity Tracking

> **Fill ONLY if Constitution Check has violations that must be justified**

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| *(none)*  | —          | —                                   |

## Phase 0 – Research Summary

Reference: [research.md](./research.md)

1. **Argument Handling Blueprint** – Confirmed Win32 screensaver switches and committed to a small dispatcher that distinguishes `/s`, `/p`, `/c`, and `/a`. Preview mode uses the supplied HWND as parent; password change requests surface the unsupported dialog chosen during clarification.
2. **Registry Layout** – Settings persist under `HKCU\Software\relmer\MatrixRain`, with separate named values for density, color scheme, animation speed, glow intensity, glow size, fullscreen flag, and debug toggles. Defaults mirror current in-memory values to keep normal mode behavior unchanged when the hive is absent.
3. **Build Artifact Duplication** – MSBuild post-build step will copy `MatrixRain.exe` to `MatrixRain.scr` for every configuration to keep VS/VS Code workflows aligned.
4. **Configuration Dialog Approach** – A Win32 dialog resource with a dedicated controller in `MatrixRainCore::ui` will own data binding, enabling reuse from both `/c` invocation and in-app entry points.
5. **Input Exit Policy** – `/s` sessions exit on keyboard input or mouse movement beyond a minimal threshold, ensuring we do not exit immediately on synthetic `WM_MOUSEMOVE`. Cursor visibility managed via balanced `ShowCursor` calls.

All research uncertainties resolved; no outstanding clarification items remain.

## Phase 1 – Design Outline

### Runtime Mode Flow

- Introduce a `ScreenSaverMode` enum (`Normal`, `ScreenSaverFull`, `ScreenSaverPreview`, `SettingsDialog`, `PasswordChangeUnsupported`).
- Add a `ScreenSaverModeContext` struct carrying launch arguments, preview parent HWND, and feature flags (hotkeys enabled, cursor hidden, exit-on-input).
- Extend `ApplicationState` to accept an injected mode context; screensaver modes disable debug overlays, force fullscreen, and prevent drag repositioning, while normal mode fullscreen expands across all monitors using the same swap chain orchestration.
- Ensure screensaver fullscreen mode enumerates all attached monitors and expands the render surface across every display, keeping animations synchronized.
- For preview mode, create a child render target sized to the control panel window; reuse the existing rendering pipeline with a constrained viewport.

### Configuration Persistence

- Create `RegistrySettingsProvider` inside `matrixrain::state`, responsible for serializing/deserializing `ScreenSaverSettings` via Win32 registry APIs wrapped with EHM macros.
- Settings struct fields: `density`, `colorSchemeId`, `animationSpeedPercent`, `glowIntensityPercent`, `glowSizePercent`, `startFullscreen`, `showDebugStats`, `showFadeTimers`. Debug flags are ignored when mode suppresses them but remain stored for normal sessions.
- Provide validation helpers that clamp numeric ranges and fall back to defaults when registry values are missing or malformed.

### Dialog and UX

- Add resource script entries for `IDD_MATRIXRAIN_SAVER_CONFIG` with standard controls (sliders or spin controls for density, speed, glow intensity, glow size; combobox for color scheme; checkbox for fullscreen; informational text showing that debug toggles are ignored in screensaver mode).
- Implement a `ConfigDialogController` class that loads current settings, populates UI controls, and commits changes on OK. The dialog procedure delegated to the controller to keep Win32 message handling minimal and testable.
- When invoked via `/c`, dialog runs modally and exits the process after user action. When invoked from normal mode hotkey, reuse the same controller but stay modeless if the existing UX expects it.

### Build & Distribution

- Amend `MatrixRain.vcxproj` with a custom `AfterBuild` target performing `Copy` from `$(TargetPath)` to `$(TargetDir)MatrixRain.scr` (overwriting the file when necessary) and failing the build if the copy fails.
- Update `scripts/Invoke-MatrixRainBuild.ps1` to validate that both `.exe` and `.scr` exist before reporting success.

### Testing Strategy

- Add unit tests covering argument parsing, registry round-trips, and mode-specific feature suppression.
- Add integration tests simulating transitions between modes using high-level application APIs, verifying cursor state requests and exit signals.
 - Validate multi-monitor rendering by exercising `/s` sessions and normal fullscreen toggles across dual-display configurations.
- Document manual QA checklist (to be included in `quickstart.md`) covering Windows screen saver control panel flows, preview embedding, cursor hide/restore, and `.scr` deployment.

### Constitution Re-check

Post-design, all gates remain satisfied: logic continues to reside in the core library, the test-first plan covers new functionality, and rendering hot paths remain unchanged aside from mode flags.

## Phase 2 – Implementation Roadmap (Preliminary)

1. **Command-line Dispatch & Mode Context**

- Write failing tests for argument parsing and mode determination.
- Implement `ScreenSaverModeParser` and integrate into `WinMain` to forward context into `MatrixRainCore` startup.

2. **Registry-backed Settings Provider**

- Author tests for default loads, round-trip persistence, and malformed data recovery.
- Implement provider with EHM-wrapped registry helpers and connect to existing configuration system, including translating glow percentages to shader-friendly values.

3. **Screensaver Runtime Adjustments**

 - Add tests ensuring debug features and window dragging respect mode flags, plus coverage that glow controls propagate to the rendering pipeline.
 - Implement cursor hide/show, exit-on-input threshold, preview viewport sizing, and dynamic bloom parameter updates within the render loop/application state.
 - Detect and drive multi-monitor swap chains so `/s` sessions cover every attached display and normal mode fullscreen mirrors that behavior when toggled.

4. **Configuration Dialog**

- Create dialog resource and controller unit tests (logic separated from Win32 message pump) covering new glow sliders.
- Wire `/c` path and in-app hotkey to shared controller ensuring registry persistence on confirmation and real-time preview updates for glow intensity/size where feasible.

5. **Build Artifact Duplication & CI Check**

Update `.vcxproj` and PowerShell build script, add verification that `.scr` exists post-build.

6. **Manual Verification & Documentation**

- Document QA steps in `quickstart.md`, including Windows screen saver control panel flows and multi-monitor expectations.

Detailed task slicing will continue in `/speckit.tasks` after design artifacts are approved.
