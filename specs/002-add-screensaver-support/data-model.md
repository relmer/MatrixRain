# Data Model – MatrixRain Screensaver Experience

## Entities

### ScreenSaverSettings

| Field | Type | Description | Validation / Defaults |
|-------|------|-------------|------------------------|
| densityPercent | `int` | Character density percentage relative to base configuration | Clamped to `[0, 100]` (0 renders exactly one streak head), default `100`. Stored in registry as `REG_DWORD` and used directly by `DensityController`. |
| colorSchemeKey | `std::wstring` | Stable identifier for configured color palette | Must match a key in `ColorSchemeRepository`; defaults to current application default |
| animationSpeedPercent | `int` | Percentage applied to streak drop cadence relative to historical default | Clamped to `[1, 100]`, default `75` |
| glowIntensityPercent | `int` | Percentage applied to bloom composite intensity (controls overall glow brightness) | Clamped to `[0, 200]`, default `100` |
| glowSizePercent | `int` | Percentage applied to bloom blur radius (controls halo size around glyphs) | Clamped to `[50, 200]`, default `100` |
| startFullscreen | `bool` | Indicates whether the saver forces fullscreen on launch | Default `true`; ignored when running in preview/embedded modes |
| showDebugStats | `bool` | Toggles debug statistics overlay in normal mode | Default `false`; ignored in screensaver modes |
| showFadeTimers | `bool` | Shows per-character fade debug overlay in normal mode | Default `false`; ignored in screensaver modes |
| lastSavedTimestamp | `std::chrono::system_clock::time_point` | Timestamp persisted for audit/telemetry | Optional; populated when registry supports `REG_QWORD`

### ScreenSaverModeContext

| Field | Type | Description |
|-------|------|-------------|
| mode | `ScreenSaverMode` enum | Indicates runtime intent: `Normal`, `ScreenSaverFull`, `ScreenSaverPreview`, `SettingsDialog`, `PasswordChangeUnsupported` |
| previewParentHwnd | `HWND` | Parent window handle for preview mode (`nullptr` otherwise) |
| commandLine | `std::wstring` | Original command-line string for logging/troubleshooting |
| enableHotkeys | `bool` | Determines whether runtime keyboard shortcuts remain active |
| hideCursor | `bool` | Whether the cursor must be hidden while session is active |
| exitOnInput | `bool` | Signals loop to exit on first qualifying input event |
| suppressDebug | `bool` | Indicates debug overlays should remain disabled |

### InputExitState

| Field | Type | Description |
|-------|------|-------------|
| initialMousePosition | `POINT` | Cursor location captured when saver activates |
| exitThresholdPixels | `int` | Movement threshold before treating mouse motion as exit signal |
| keyboardTriggered | `bool` | Tracks whether keyboard input triggered exit |
| mouseTriggered | `bool` | Tracks whether mouse movement triggered exit |

### DistributionArtifacts

| Field | Type | Description |
|-------|------|-------------|
| executablePath | `std::wstring` | Path to `MatrixRain.exe` produced by the build (measured in PowerShell verification) |
| screensaverPath | `std::wstring` | Path to `MatrixRain.scr` copied from executable |
| debugSymbolPath | `std::wstring` | Path to `.pdb` for debugging installations |

## Relationships

- `ScreenSaverSettings` is loaded into the existing `ApplicationState` during startup; when the application runs in any screensaver mode, the settings values override in-memory defaults before rendering begins.
- `ScreenSaverModeContext` is produced by the launcher layer (WinMain) and injected into `Application`/`ApplicationState`. It determines which subsets of `ScreenSaverSettings` are honored (e.g., debug toggles ignored when `suppressDebug` is `true`).
- `InputExitState` is owned by the input system while screensaver mode is active and feeds exit notifications back to the application loop.
- `DistributionArtifacts` belongs to build tooling scripts and is used to validate deliverables post-build (ensuring `.scr` is always in sync with the `.exe`).

## State Transitions

1. **Launch → Mode Detection**: `WinMain` constructs `ScreenSaverModeContext` based on command-line arguments. If mode equals `PasswordChangeUnsupported`, the executable shows the unsupported dialog and exits without touching the rendering pipeline.
2. **Mode Detection → Settings Load**: When the core application starts, it loads `ScreenSaverSettings` from the registry (falling back to defaults if missing) and applies overrides according to `ScreenSaverModeContext` (e.g., forcing fullscreen, disabling debug overlays).
3. **Settings Load → Runtime**: `ApplicationState` initializes subsystems with the resolved settings. If `hideCursor` is `true`, the input system hides the cursor and initializes `InputExitState` for movement tracking.
4. **Runtime → Exit**: While in screensaver modes, either keyboard input or mouse movement beyond `exitThresholdPixels` sets the corresponding flag in `InputExitState` and dispatches an exit event, causing the main loop to terminate and restore the cursor.
5. **Configuration Dialog → Persistence**: When the dialog controller receives confirmation, it updates `ScreenSaverSettings` in memory, persists them via `RegistrySettingsProvider`, and raises an event prompting running sessions to reload if necessary.
