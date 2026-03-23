# Feature Specification: Screensaver Install/Uninstall via Command Line

**Feature Branch**: `005-screensaver-install`  
**Created**: 2026-03-21  
**Status**: Draft  
**Input**: User description: "Add command-line switch to install or uninstall MatrixRain as a screensaver"

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Install as Screensaver (Priority: P1)

A user who has downloaded MatrixRain wants to register it as a Windows screensaver. They run `MatrixRain.exe /install` from the command line (or double-click a shortcut configured with that flag). The application copies itself as a `.scr` file to the appropriate system directory, sets MatrixRain as the active screensaver, and opens the Windows Screen Saver Settings dialog — matching the standard behavior of right-clicking an `.scr` file in Explorer and choosing "Install". The user can then configure timeout and other screensaver preferences before closing the dialog.

**Why this priority**: This is the core value proposition — without install, users must manually copy files to System32, which is error-prone and intimidating for non-technical users.

**Independent Test**: Run `MatrixRain.exe /install` and verify that the `.scr` file is placed in the correct system directory, MatrixRain is set as the active screensaver, and the Screen Saver Settings dialog opens with MatrixRain selected.

**Acceptance Scenarios**:

1. **Given** MatrixRain is not installed as a screensaver, **When** the user runs `MatrixRain.exe /install`, **Then** the `.scr` file is copied to the system screensaver directory, MatrixRain is set as the active screensaver, and the Screen Saver Settings dialog opens with MatrixRain selected.
2. **Given** MatrixRain is already installed as a screensaver, **When** the user runs `MatrixRain.exe /install`, **Then** the existing `.scr` file is overwritten with the current version, MatrixRain remains selected, and the Screen Saver Settings dialog opens (enabling upgrades).
3. **Given** the user does not have sufficient privileges, **When** the user runs `MatrixRain.exe /install`, **Then** the application requests elevation (UAC prompt) or displays a clear error message explaining that administrator privileges are required.
4. **Given** the user runs `MatrixRain.exe /install`, **When** the file copy and screensaver selection complete, **Then** the Screen Saver Settings control panel opens and the application exits after the dialog is launched.

---

### User Story 2 - Uninstall Screensaver (Priority: P1)

A user who previously installed MatrixRain as a screensaver wants to remove it. They run `MatrixRain.exe /uninstall` from the command line. The application removes the `.scr` file from the system screensaver directory and confirms removal. MatrixRain no longer appears in the Windows screensaver picker.

**Why this priority**: Uninstall is equally critical to install — users must be able to cleanly remove the screensaver without manually navigating to System32.

**Independent Test**: After a successful install, run `MatrixRain.exe /uninstall` and verify the `.scr` file is removed and MatrixRain no longer appears in Windows Screen Saver Settings.

**Acceptance Scenarios**:

1. **Given** MatrixRain is installed as a screensaver, **When** the user runs `MatrixRain.exe /uninstall`, **Then** the `.scr` file is removed from the system screensaver directory and a success message is displayed.
2. **Given** MatrixRain is not installed as a screensaver, **When** the user runs `MatrixRain.exe /uninstall`, **Then** the application displays a message indicating it was not installed (not an error, just informational).
3. **Given** the user does not have sufficient privileges, **When** the user runs `MatrixRain.exe /uninstall`, **Then** the application requests elevation (UAC prompt) or displays a clear error message explaining that administrator privileges are required.
4. **Given** MatrixRain is currently set as the active screensaver, **When** the user runs `MatrixRain.exe /uninstall`, **Then** the `.scr` file is removed and Windows reverts to no screensaver selected.
5. **Given** the user runs `MatrixRain.exe /uninstall`, **When** uninstallation completes, **Then** the application exits.

---

### User Story 3 - Help Text Updated (Priority: P2)

A user runs `MatrixRain.exe /?` and sees the install/uninstall switches documented alongside the existing command-line options, so they know these capabilities exist.

**Why this priority**: Discoverability is important but secondary to the actual install/uninstall functionality working correctly.

**Independent Test**: Run `MatrixRain.exe /?` and verify that `/install` and `/uninstall` are listed with clear descriptions.

**Acceptance Scenarios**:

1. **Given** the user runs `MatrixRain.exe /?`, **When** the help text is displayed, **Then** `/install` and `/uninstall` are listed with brief descriptions of their purpose.

---

### Edge Cases

- **Read-only System32 / third-party security**: Covered by FR-010 — if `CopyFileW` or `DeleteFileW` fails, a clear error message is displayed.
- **Foreign `MatrixRain.scr` already in System32**: Covered by FR-008 — install always overwrites. The user accepts this by choosing to install.
- **Running `/install` from the `.scr` file itself**: When launched as `MatrixRain.scr` from System32, `GetModuleFileNameW` returns the System32 path, so `CopyFileW` would copy a file onto itself. Install MUST detect when source and target paths are identical and skip the copy step (proceed directly to the `desk.cpl` invocation).
- **Multi-user systems**: Each user's `HKCU` registry is independent. User A installing doesn't affect User B. However, the `.scr` file in System32 is shared — User B uninstalling removes the file for all users. This matches standard Windows screensaver behavior and is a known limitation.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The application MUST accept `/install` (or `-install`) as a command-line switch to install MatrixRain as a Windows screensaver.
- **FR-002**: The application MUST accept `/uninstall` (or `-uninstall`) as a command-line switch to remove MatrixRain from the Windows screensaver list.
- **FR-003**: Install MUST copy the running executable itself to the Windows system screensaver directory (`%SystemRoot%\System32\MatrixRain.scr`), renaming it with the `.scr` extension. This is self-contained — no dependency on an adjacent `.scr` file.
- **FR-004**: Install MUST invoke `desk.cpl,InstallScreenSaver` with the installed `.scr` path, which sets MatrixRain as the active screensaver (writing `SCRNSAVE.EXE` and `ScreenSaveActive` to the registry) and opens the Screen Saver Settings dialog — matching the standard Windows `.scr` right-click "Install" behavior. No manual registry writes are needed for install. This preserves the user's existing `ScreenSaveTimeOut` and `ScreenSaverIsSecure` preferences.
- **FR-005**: Uninstall MUST remove the `.scr` file from the system screensaver directory.
- **FR-006**: Both operations MUST require and verify administrator privileges, since writing to System32 requires elevation.
- **FR-007**: If the process is not elevated, the application MUST re-launch itself with elevated privileges (triggering UAC) and pass through the original command-line switch.
- **FR-008**: Install MUST overwrite an existing `.scr` file if one is already present (to support version upgrades).
- **FR-009**: Uninstall MUST handle the case where the `.scr` file does not exist gracefully (informational message, not an error).
- **FR-010**: Both operations MUST display a clear failure message to the user if an error occurs (via the console or a message box).
- **FR-011**: After install completes, the application MUST exit after the `InstallScreenSaver` call launches the Screen Saver Settings dialog (it does not remain running or launch the screensaver itself).
- **FR-012**: After uninstall completes, the application MUST exit without launching the screensaver or showing the main window.
- **FR-013**: Uninstall MUST clear the `SCRNSAVE.EXE` registry value (under `HKCU\Control Panel\Desktop`) if it currently points to the MatrixRain `.scr` file (do not clear it if the user has since switched to a different screensaver).
- **FR-014**: Uninstall MUST set `ScreenSaveActive` to `"0"` when removing the active MatrixRain screensaver.
- **FR-015**: Uninstall MUST NOT modify `SCRNSAVE.EXE` or `ScreenSaveActive` if a different screensaver is currently selected (only remove the `.scr` file from System32).
- **FR-016**: The help text (`/?`) MUST document `/install`, `/uninstall`, and `/force` switches.
- **FR-017**: Install MUST check for Group Policy and MDM/Intune policies that would block screensaver activation before requesting elevation. If a blocking policy is detected, the install MUST display an error with the specific policy details and abort.
- **FR-018**: The `/force` switch MUST skip the policy check during install, allowing installation on managed devices where the user accepts the limitation.
- **FR-019**: The `/s` (screensaver) mode MUST always force fullscreen regardless of the `StartFullscreen` registry setting, and MUST NOT persist the fullscreen state back to the registry.
- **FR-020**: The config dialog opened by the screensaver CPL (`/c` with parent HWND) MUST hide the fullscreen checkbox, since screensaver mode always forces fullscreen.
- **FR-021**: The `.scr` file MUST include a STRINGTABLE resource with string ID 1 set to "MatrixRain" for the Screen Saver Settings CPL to display the correct name.

### Key Entities

- **Screensaver Registry Settings**: Windows stores screensaver configuration under `HKCU\Control Panel\Desktop`. The key values are:
  - `SCRNSAVE.EXE` (REG_SZ) — Full path to the active `.scr` file
  - `ScreenSaveActive` (REG_SZ) — `"1"` = screensaver enabled, `"0"` = disabled
  - `ScreenSaveTimeOut` (REG_SZ) — Idle timeout in seconds before screensaver starts
  - `ScreenSaverIsSecure` (REG_SZ) — `"1"` = password required on resume, `"0"` = no password
- **Screensaver File**: The `.scr` file placed in `%SystemRoot%\System32\` that Windows discovers for the screensaver picker dropdown.

### Assumptions

- Placing `.scr` files in `%SystemRoot%\System32\` makes them visible in the screensaver picker dropdown. Additionally, setting `SCRNSAVE.EXE` in the registry is required to make a specific screensaver the active selection.
- The `.exe` and `.scr` are binary-identical (an `.scr` is just an `.exe` with a different extension), so the running executable can copy itself directly as the `.scr` file.
- UAC elevation is the standard mechanism for obtaining write access to System32 on supported Windows versions.
- The `/install` and `/uninstall` switches follow the same prefix convention (`/` or `-`) as existing switches (`/s`, `/c`, `/p`).
- Output messages for install/uninstall use console output (since these are command-line operations), with a fallback to a message box if no console is attached.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A user can install MatrixRain as a screensaver with a single command, and the Screen Saver Settings dialog opens with MatrixRain selected and ready to configure.
- **SC-002**: A user can uninstall MatrixRain as a screensaver with a single command and confirm it no longer appears in Windows Screen Saver Settings.
- **SC-003**: A non-administrator user is prompted for elevation (UAC) automatically — no manual "Run as Administrator" step is required.
- **SC-004**: Upgrading (running `/install` over a previous installation) preserves user's `ScreenSaveTimeOut` and `ScreenSaverIsSecure` preferences.
- **SC-005**: After uninstall, if MatrixRain was the active screensaver, the screensaver is disabled and no orphaned registry entries remain pointing to a deleted `.scr` file.
- **SC-006**: The install/uninstall operations complete and exit cleanly without leaving orphan processes or windows.
- **SC-007**: Help text accurately documents the new switches, and users can discover them via `/?`.

## Clarifications

### Session 2026-03-21

- Q: Where does the install source `.scr` come from? → A: The running `.exe` copies itself to System32 as `MatrixRain.scr` (self-contained, no sibling `.scr` dependency).
- Q: What if the `.scr` file is locked during uninstall? → A: Not possible — the screensaver exits on any mouse/keyboard input, and the user must use input to run the uninstall command. Edge case removed.
- Q: How should registry operations be tested? → A: Use a mock registry interface (abstraction) exclusively. Tests MUST NEVER interact with the actual Windows registry. The `ScreenSaverInstaller` accepts a registry interface for testability; production code uses the real Windows Registry API implementation.
