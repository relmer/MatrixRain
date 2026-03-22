# Data Model: Screensaver Install/Uninstall

## Entities

### ScreenSaverMode (enum class) — MODIFIED

Existing enum with two new values added:

| Value | CLI Switch | Description |
|-------|-----------|-------------|
| Normal | *(none)* | Default application mode |
| ScreenSaverFull | `/s` | Full-screen screensaver |
| ScreenSaverPreview | `/p <HWND>` | Preview in Control Panel |
| SettingsDialog | `/c` or `/c:<HWND>` | Configuration dialog |
| PasswordChangeUnsupported | `/a` | Password change (unsupported) |
| HelpRequested | `/?` | Display help text |
| **Install** | `/install` | **NEW** — Install as screensaver |
| **Uninstall** | `/uninstall` | **NEW** — Uninstall screensaver |

### ScreenSaverModeContext (struct) — NO CHANGES

The existing context struct is sufficient. Install/Uninstall modes don't need additional context fields — they don't use `m_previewParentHwnd`, `m_enableHotkeys`, etc. The mode enum value alone drives the dispatcher.

### ScreenSaverInstaller (new class)

Encapsulates all install/uninstall logic. Stateless — all methods are static or operate on parameters only.

| Method | Purpose |
|--------|---------|
| `Install()` | Copy self to System32, invoke desk.cpl InstallScreenSaver |
| `Uninstall()` | Remove .scr from System32, clean up registry if active |
| `IsElevated()` | Check if current process has admin privileges |
| `RequestElevation(switchArg)` | Re-launch self with UAC elevation |

### Registry State (external — HKCU\Control Panel\Desktop)

| Value | Type | Install Behavior | Uninstall Behavior |
|-------|------|-----------------|-------------------|
| `SCRNSAVE.EXE` | REG_SZ | Set by desk.cpl | Clear if points to MatrixRain.scr |
| `ScreenSaveActive` | REG_SZ | Set to "1" by desk.cpl | Set to "0" if MatrixRain was active |
| `ScreenSaveTimeOut` | REG_SZ | Not modified | Not modified |
| `ScreenSaverIsSecure` | REG_SZ | Not modified | Not modified |

### File System State

| Path | Install | Uninstall |
|------|---------|-----------|
| `%SystemRoot%\System32\MatrixRain.scr` | Created (copy of running .exe) | Deleted |
| Running executable (any location) | Source for copy | Unchanged |

## State Transitions

```
                    /install                              /uninstall
Not Installed ──────────────► Installed ──────────────► Not Installed
                  (copy .scr,       │                    (delete .scr,
                   set registry,    │                     clear registry)
                   open CPL)        │
                                    │  /install (upgrade)
                                    ├──────────────────► Installed
                                    │  (overwrite .scr,
                                    │   re-open CPL)
                                    │
                                    │  User switches to
                                    │  different screensaver
                                    ▼
                              Installed but         /uninstall
                              not active     ──────────────► Not Installed
                                                (delete .scr only,
                                                 don't touch registry)
```

## Validation Rules

- Install: Source executable must be readable (`GetModuleFileNameW` succeeds)
- Install: Target directory must be writable (requires elevation)
- Uninstall: Registry comparison must be case-insensitive path match
- Uninstall: Missing .scr file is not an error (informational message)
