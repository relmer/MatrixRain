# Contract: MatrixRain Screensaver CLI

## Overview

MatrixRain supports Windows screensaver invocation semantics. The `.scr` binary shares the same bits as `MatrixRain.exe`; behavior depends on the command-line arguments supplied by the shell or user.

## Commands

### `/s`

| Aspect | Details |
|--------|---------|
| Purpose | Start the screensaver in full-screen mode on the primary monitor |
| Input | None |
| Behavior | App hides cursor, disables hotkeys, monitors input via `InputExitState`, and exits on qualifying keyboard or mouse activity |
| Exit Codes | `0` on graceful exit; non-zero on initialization failure |

### `/p <HWND>`

| Aspect | Details |
|--------|---------|
| Purpose | Render preview inside the HWND supplied by `desk.cpl` |
| Input | `HWND` (unsigned integer passed as decimal string) |
| Behavior | App re-parents its render window to the supplied HWND, scales viewport to fit, keeps cursor visible, ignores exit-on-input |
| Exit Codes | `0` on success; `2` if the HWND is invalid or parenting fails |

### `/c`

| Aspect | Details |
|--------|---------|
| Purpose | Display the configuration dialog for user preferences |
| Input | Optional HWND for parent window (ignored if zero) |
| Behavior | Launches modal dialog using current registry-backed `ScreenSaverSettings`. On acceptance, writes settings back to the registry and returns control to caller |
| Exit Codes | `0` after dialog closes normally; `1` on initialization error |

### `/a <HWND>`

| Aspect | Details |
|--------|---------|
| Purpose | Password change request (unsupported) |
| Input | Parent HWND supplied by Windows |
| Behavior | Shows a message dialog indicating that password changes are not supported and exits immediately |
| Exit Codes | `0` after showing the dialog |

### No Arguments

| Aspect | Details |
|--------|---------|
| Purpose | Run MatrixRain as a normal desktop application |
| Behavior | Behavior matches existing pre-feature implementation, honoring registry settings and hotkeys |
| Exit Codes | Existing codes preserved |

## Error Handling

- Unknown arguments trigger a help-style message and exit with code `3`.
- Registry or DirectX initialization failures use EHM macros to propagate `HRESULT` values, which are converted to non-zero exit codes.
