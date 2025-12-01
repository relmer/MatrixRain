# Research: MatrixRain Screensaver Experience

## Windows Screensaver Invocation

- **Decision**: Implement a dedicated `ScreenSaverMode` detector that interprets `/s`, `/p <HWND>`, `/c`, and `/a <HWND>` following Win32 screensaver conventions. `/p` spawns a child window hosted inside the provided HWND; `/a` displays a "password change not supported" message and exits.
- **Rationale**: Windows Shell treats `.scr` binaries as executables and dispatches these standardized arguments. Handling them explicitly keeps behavior aligned with built-in screensavers and avoids undefined input states.
- **Alternatives considered**: Ignoring unknown switches (risks runaway processes launched by `desk.cpl`); reusing existing app CLI parser (lacks HWND-aware logic and would require risky refactors).

## Registry Storage Location

- **Decision**: Persist settings under `HKCU\Software\relmer\MatrixRain`, storing values per user with defaults mirrored in code. Each option (density, color scheme, animation speed, glow intensity, glow size, fullscreen flag, debug toggles) maps to its own named value.
- **Rationale**: User-context hive avoids elevation and matches Windows guidance for screensaver personalization. Namespacing with the publisher prevents collisions with other apps.
- **Alternatives considered**: `HKLM` (requires admin rights, unsuitable for per-user preferences); custom config files (adds file I/O complexity and diverges from Windows control panel expectations).

## Build Artifact Duplication

- **Decision**: Add a post-build MSBuild target in `MatrixRain.vcxproj` that copies `$(TargetPath)` to `$(TargetDir)MatrixRain.scr` for every configuration/platform.
- **Rationale**: MSBuild targets keep VS and command-line builds in sync and guarantee `.scr` output exists alongside the `.exe` without manual steps.
- **Alternatives considered**: Manual scripts (risk human error); separate `.scr` project (duplicates linking configuration and complicates maintenance).

## Configuration Dialog Technology

- **Decision**: Create a modal dialog resource (`IDD_MATRIXRAIN_SAVER_CONFIG`) implemented with classic Win32 dialog procedures inside `MatrixRainCore` UI namespace, exposing a thin façade callable from both `/c` and normal mode hotkeys.
- **Rationale**: Dialog resources integrate cleanly with `desk.cpl` expectations, reuse existing Win32 toolchain, and keep logic testable by isolating data binding away from dialog procedure glue.
- **Alternatives considered**: Custom DirectX overlay (incompatible with control panel preview); external settings app (forces users out of standard workflow).

## Screensaver Input Exit Handling

- **Decision**: In `/s` mode, track initial mouse position and dismiss the saver only after detecting either keyboard input or mouse movement exceeding a small threshold (e.g., 5 px) to avoid exiting on synthetic `WM_MOUSEMOVE`. Cursor visibility is managed via `ShowCursor` with balanced calls.
- **Rationale**: Built-in savers guard against spurious messages; implementing the same threshold prevents immediate exit while honoring quick user intent.
- **Alternatives considered**: Polling raw input (unnecessary complexity); ignoring mouse movement (breaks expected exit behavior).

## Multi-Monitor Rendering

- **Decision**: Drive MatrixRain across all attached monitors in both `/s` screensaver mode and normal fullscreen sessions by enumerating IDXGI outputs and presenting synchronized swap chains per display.
- **Rationale**: Native Windows savers blanket every monitor; leaving secondary displays idle breaks immersion and fails enterprise signage scenarios.
- **Alternatives considered**: Restricting to the primary monitor (violates spec); cloning the primary view via OS display settings (hands control to the user and complicates support).
