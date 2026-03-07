# Quickstart: Runtime Help Overlay and Command-Line Help

**Feature**: 003-runtime-help
**Date**: 2026-03-04

## What This Feature Adds

1. **Runtime help hint overlay** — A three-line help message appears on-screen when MatrixRain starts in Normal mode, showing key bindings (Settings/Enter, Help/?, Exit/Esc). Uses a horizontal sweep reveal effect (a left-to-right sweep head moves across each row with per-row staggered timing — characters in the sweep's streak zone show a random glyph, characters behind the streak show their target glyph). Dismiss uses a mirrored right-to-left sweep. Rain streaks pass behind the message area.

2. **Re-show on unrecognized key** — Pressing any unmapped key re-triggers the help hint animation.

3. **Hotkey-triggered dissolve** — Any recognized hotkey dismisses the hint via the dissolve effect.

4. **Command-line help (`/?`/`-?`)** — Displays formatted usage text (command-line switches only — Options and Screensaver Options, no hotkeys) in a custom graphical rain dialog window with its own D3D/D2D rendering. Uses proportional font (Segoe UI) with per-character queued reveal and two independent streak pools (reveal + decorative). Works identically regardless of launch context.

5. **Enter key opens config dialog** — Opens the settings dialog as a live modeless overlay.

6. **? key shows hotkey reference overlay** — Renders hotkey information directly on the main MatrixRain window as an in-app overlay. This is NOT a dialog — it's rendered inline on the main window. Distinct from `/?` which shows switches in a standalone `HelpRainDialog`.

## Key Architecture Decisions

- **All new code in MatrixRainCore.lib** — follows Library-First Architecture (Constitution VI)
- **All rendering is GPU-based** — D2D/DirectWrite for the in-app overlay, hotkey overlay, and graphical rain dialog. No console output, no ANSI escape codes, no `AttachConsole`.
- **TextSweepEffect** — shared per-row horizontal sweep timing oracle, used by both HelpHintOverlay and HotkeyOverlay
- **Flat parallel arrays** for per-character overlay state — cache-friendly, TDD-friendly
- **`HelpRainDialog`** is a custom window with its own D3D11/D2D rendering context — independent of the main app's render pipeline. Used by `/?` only. Uses proportional font (Segoe UI), per-character queued reveal, two independent streak pools.
- **`HotkeyOverlay`** renders hotkey reference directly on the main window — used by `?` key only. NOT `HelpRainDialog`.
- **`UsageText`** is used by `HelpRainDialog` — single source of truth for command-line switch content (NOT hotkeys, NOT the runtime hint overlay)
- **Graphical rain dialog runs before window creation** for `/?` — has its own D3D/D2D context, no dependency on main app initialization
- **State machine** for overlay phases: Hidden → Revealing → Holding → Dismissing → Hidden (driven by TextSweepEffect::SweepPhase)

## New Files

| File | Project | Purpose |
|------|---------|---------|
| `HelpHintOverlay.h/cpp` | MatrixRainCore | Overlay state machine + sweep-driven per-character animation |
| `TextSweepEffect.h/cpp` | MatrixRainCore | Shared per-row horizontal sweep timing oracle |
| `HelpRainDialog.h/cpp` | MatrixRainCore | Custom graphical rain dialog with D3D/D2D rendering (`/?` only) |
| `HotkeyOverlay.h/cpp` | MatrixRainCore | In-app hotkey reference overlay (`?` key only) |
| `UsageText.h/cpp` | MatrixRainCore | Command-line switch text content + formatting (no hotkeys) |
| `CommandLineHelp.h/cpp` | MatrixRainCore | /? orchestration — creates HelpRainDialog, exits process |
| `UnicodeSymbols.h` | MatrixRain | Named constants for Unicode characters (em dash, etc.) |
| `HelpHintOverlayTests.cpp` | MatrixRainTests | Unit tests for overlay state machine |
| `TextSweepEffectTests.cpp` | MatrixRainTests | Unit tests for sweep timing oracle |
| `HelpRainDialogTests.cpp` | MatrixRainTests | Unit tests for reveal queue, character positions, animation state |
| `HotkeyOverlayTests.cpp` | MatrixRainTests | Unit tests for hotkey overlay state machine |
| `UsageTextTests.cpp` | MatrixRainTests | Unit tests for text formatting, prefix detection, section grouping |
| `CommandLineHelpTests.cpp` | MatrixRainTests | Unit tests for orchestration |

## Modified Files

| File | Change |
|------|--------|
| `ScreenSaverModeParser.cpp` | Add `?` switch parsing, detect prefix style |
| `Application.cpp` | Add Enter/? key handling in `OnKeyDown`, show/dismiss overlay |
| `InputSystem.cpp/h` | Route unrecognized keys to trigger help hint |
| `RenderSystem.cpp/h` | Render `HelpHintOverlay` and `HotkeyOverlay` (D2D), rain occlusion |
| `AnimationSystem.cpp/h` | Occlusion rect for rain streaks passing behind overlay |
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

# Manual test: CLI help (graphical rain dialog)
.\x64\Debug\MatrixRain.exe /?       # Graphical rain dialog — switches only, proportional font

# Manual test: screensaver mode (no hint)
.\x64\Debug\MatrixRain.exe /s       # No help hint
```

## Implementation Order

Priority: CLI help (US3) is P1 MVP; overlay (US1/US2) is P2.

1. `UnicodeSymbols.h` — named Unicode constants (em dash, etc.)
2. `UsageText` — text content and section-grouped formatting (foundation for dialog)
3. `HelpRainDialog` — custom graphical window with D3D/D2D rendering + per-character queued reveal + two independent streak pools
4. `CommandLineHelp` — orchestration for `/?` using `HelpRainDialog`
5. `HelpHintOverlay` — overlay state machine (pure logic, no rendering)
6. Render integration — D2D rendering of overlay, rain occlusion, feathered border
7. Input integration — Enter/? keys, unrecognized key → show, hotkey → dismiss
8. `HotkeyOverlay` — in-app overlay for `?` key (rendered directly on main window, NOT dialog)
