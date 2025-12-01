# Quickstart – MatrixRain Screensaver Experience

## Prerequisites

- Visual Studio 2026 (v18.x) with C++ Desktop workload and latest Windows 11 SDK
- Windows 11 x64 workstation with DirectX 11 runtime
- PowerShell 7+ (for repository scripts)

## Build the Solution

```powershell
# Debug build (produces MatrixRain.exe and MatrixRain.scr)
& "scripts/Invoke-MatrixRainBuild.ps1" -Target Build -Configuration Debug -Platform x64

# Release build
& "scripts/Invoke-MatrixRainBuild.ps1" -Target Build -Configuration Release -Platform x64
```

The updated MSBuild `AfterBuild` target copies `MatrixRain.exe` to `MatrixRain.scr`. Verify both artifacts exist in `MatrixRain\x64\<Config>\`.

## Run the Application

```powershell
# Normal desktop mode
& "MatrixRain\x64\Debug\MatrixRain.exe"

# Screensaver full-screen mode
& "MatrixRain\x64\Debug\MatrixRain.scr" /s

# Control panel preview (replace 123456 with a valid HWND during testing)
& "MatrixRain\x64\Debug\MatrixRain.scr" /p 123456

# Configuration dialog
& "MatrixRain\x64\Debug\MatrixRain.scr" /c
```

## Registry Settings

Per-user options are stored under `HKCU\Software\relmer\MatrixRain`. Deleting the key restores defaults. Debug toggles remain persisted but are ignored whenever `ScreenSaverMode` suppresses them.

## Testing

```powershell
# Build + run unit/integration tests
& "scripts/Invoke-MatrixRainTests.ps1" -Configuration Debug -Platform x64 -Parallel
```

New tests cover argument parsing, registry persistence, mode flags, multi-monitor rendering in both screensaver and normal fullscreen modes, and configuration dialog validation. Ensure tests are run (and fail) before adding implementation, per project TDD requirements.

## Manual QA Checklist

1. Launch `.scr /s`, verify cursor hides and exit occurs on key press or meaningful mouse movement.
2. With at least two monitors attached, run `.scr /s` and confirm both displays render synchronized MatrixRain visuals without letterboxing or unused screens.
3. Use `/p <HWND>` from the control panel preview and confirm the animation stays within the preview window and ignores hotkeys.
4. Change options in the configuration dialog (`/c`), including density, color, animation speed, glow intensity, and glow size; reopen the saver and confirm the applied visuals match the new settings.
5. In normal mode, toggle fullscreen, verify it spans all monitors, then adjust settings with hotkeys, restart, and confirm registry persistence.
6. Confirm `.scr` artifact regenerates after every build configuration (Debug/Release) and matches the `.exe` timestamp.
```}]}၎