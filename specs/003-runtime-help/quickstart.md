# Quickstart: Runtime Help Overlay and Command-Line Help

**Feature**: 003-runtime-help
**Date**: 2026-03-04

## What This Feature Adds

1. **Runtime help hint overlay** — A three-line help message appears on-screen when MatrixRain starts in Normal mode, showing key bindings (Settings/Enter, Help/?, Exit/Esc). Uses a scramble-reveal effect (characters appear as random cycling glyphs that lock into their target glyphs with per-cell staggered timing). Dismiss reverses the animation. Per-row rounded-rect halos (D3D11 SDF shader) darken the background for readability.

2. **Re-show on unrecognized key** — Pressing any unmapped key re-triggers the help hint animation.

3. **Hotkey-triggered dissolve** — Any recognized hotkey dismisses the hint via the dissolve effect.

4. **Command-line help (`/?`/`-?`)** — Displays formatted usage text (command-line switches only — Options and Screensaver Options, no hotkeys) in a custom usage dialog window with its own D3D/D2D rendering. Uses proportional font (Segoe UI) with scramble-reveal animation (ScrambleRevealEffect) and background matrix rain. Works identically regardless of launch context.

5. **Enter key opens config dialog** — Opens the settings dialog as a live modeless overlay.

6. **? key shows hotkey reference overlay** — Renders hotkey information directly on the main MatrixRain window as an in-app overlay. This is NOT a dialog — it's rendered inline on the main window. Distinct from `/?` which shows switches in a standalone `UsageDialog`.

## Key Architecture Decisions

- **All new code in MatrixRainCore.lib** — follows Library-First Architecture (Constitution VI)
- **All rendering is GPU-based** — D3D11 instancing for overlay characters (rendered to scene texture before bloom for free glow), D3D11 SDF pixel shader for halo backgrounds, D2D/DirectWrite for FPS counter and text measurement. No console output, no ANSI escape codes, no `AttachConsole`.
- **ScrambleRevealEffect** — shared per-cell scramble-reveal timing oracle, used by HelpHintOverlay, HotkeyOverlay, and UsageDialog
- **Flat parallel arrays** for per-character overlay state — cache-friendly, TDD-friendly
- **`UsageDialog`** is a custom window with its own D3D11/D2D rendering context — independent of the main app's render pipeline. Used by `/?` only. Uses proportional font (Segoe UI), scramble-reveal animation via ScrambleRevealEffect.
- **`HotkeyOverlay`** renders hotkey reference directly on the main window — used by `?` key only. NOT `UsageDialog`.
- **`UsageText`** is used by `UsageDialog` — single source of truth for command-line switch content (NOT hotkeys, NOT the runtime hint overlay)
- **Usage dialog runs before window creation** for `/?` — has its own D3D/D2D context, no dependency on main app initialization
- **State machine** for overlay phases: Hidden → Revealing → Holding → Dismissing → Hidden (driven by ScrambleRevealEffect::ScramblePhase)

## New Files

| File | Project | Purpose |
|------|---------|---------|
| `HelpHintOverlay.h/cpp` | MatrixRainCore | Overlay state machine + scramble-reveal per-character animation |
| `ScrambleRevealEffect.h/cpp` | MatrixRainCore | Shared per-cell scramble-reveal timing oracle |
| `UsageDialog.h/cpp` | MatrixRainCore | Custom usage dialog with D3D/D2D rendering and scramble-reveal (`/?` only) |
| `HotkeyOverlay.h/cpp` | MatrixRainCore | In-app hotkey reference overlay (`?` key only) |
| `UsageText.h/cpp` | MatrixRainCore | Command-line switch text content + formatting (no hotkeys) |
| `CommandLineHelp.h/cpp` | MatrixRainCore | /? orchestration — creates UsageDialog, exits process |
| `UnicodeSymbols.h` | MatrixRain | Named constants for Unicode characters (em dash, etc.) |
| `HelpHintOverlayTests.cpp` | MatrixRainTests | Unit tests for overlay state machine |
| `ScrambleRevealEffectTests.cpp` | MatrixRainTests | Unit tests for scramble-reveal timing oracle |
| `UsageDialogTests.cpp` | MatrixRainTests | Unit tests for scramble-reveal animation, character positions, animation state |
| `HotkeyOverlayTests.cpp` | MatrixRainTests | Unit tests for hotkey overlay state machine |
| `UsageTextTests.cpp` | MatrixRainTests | Unit tests for text formatting, prefix detection, section grouping |
| `CommandLineHelpTests.cpp` | MatrixRainTests | Unit tests for orchestration |

## Modified Files

| File | Change |
|------|--------|
| `CommandLine.cpp` | Add `?` switch parsing, detect prefix style |
| `Application.cpp` | Add Enter/? key handling in `OnKeyDown`, show/dismiss overlay |
| `InputSystem.cpp/h` | Route unrecognized keys to trigger help hint |
| `RenderSystem.cpp/h` | Render overlays to scene texture before bloom, SDF halo shader, `RenderParams` struct |
| `AnimationSystem.cpp/h` | Overlay character support |
| `ApplicationState.h` | Add help hint enabled flag |
| `main.cpp` | Call `CommandLineHelp` before window creation for `/?` |

## Build & Test

```powershell
# Build
& "C:\Program Files\Microsoft Visual Studio\18\Enterprise\MSBuild\Current\Bin\MSBuild.exe" MatrixRain.sln /p:Configuration=Debug /p:Platform=x64

# Run tests
vstest.console.exe x64\Debug\MatrixRainTests.dll

# Manual test: overlay
.\x64\Debug\MatrixRain.exe          # Normal mode — expect hint overlay

# Manual test: CLI help (usage dialog)
.\x64\Debug\MatrixRain.exe /?       # Usage dialog — switches only, proportional font

# Manual test: screensaver mode (no hint)
.\x64\Debug\MatrixRain.exe /s       # No help hint
```

## Implementation Order

Priority: CLI help (US3) is P1 MVP; overlay (US1/US2) is P2.

1. `UnicodeSymbols.h` — named Unicode constants (em dash, etc.)
2. `UsageText` — text content and section-grouped formatting (foundation for dialog)
3. `UsageDialog` — custom graphical window with D3D/D2D rendering + scramble-reveal animation (ScrambleRevealEffect + ComputeScrambleColor + background matrix rain)
4. `CommandLineHelp` — orchestration for `/?` using `UsageDialog`
5. `HelpHintOverlay` — overlay state machine (pure logic, no rendering)
6. Render integration — overlay rendering to scene texture before bloom, SDF halo backgrounds
7. Input integration — Enter/? keys, unrecognized key → show, hotkey → dismiss
8. `HotkeyOverlay` — in-app overlay for `?` key (rendered directly on main window, NOT dialog)
