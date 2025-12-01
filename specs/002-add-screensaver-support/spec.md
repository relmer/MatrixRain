# Feature Specification: MatrixRain Screensaver Experience

**Feature Branch**: `002-add-screensaver-support`  
**Created**: 2025-11-30  
**Status**: Draft  
**Input**: User description: "We're going to turn this app into a full-fledged Windows 11 screen saver. It will support all of the required cmdline args that screensavers handle. We're going to continue to build it as an exe, but we'll need an additional step that copies the exe to an scr, so we'll have both. This will make it easy for people to run as a regular app or install as a screensaver without requiring the user to understand how to use an scr outside of the screensaver scenario, nor understand that they would have to rename the exe to scr for use in that scenario. When run without any of the screensaver args, the program will function just as it does today. It won't treat mouse/keyboard input as a signal to exit (other than the esc key which exits today). It will need a configuration dialog to allow configuring options for screensaver usage. The state of these options will be stored in the registry. When the app opens (both normally and as a screensaver), it will read the registry settings and use them, overriding the defaults. When run in normal mode, any configuration changes made via the keyboard hotkeys will be preserved in the registry. Configuration settings related to debug helpers (statistics display, character fade timers) will not have any effect when running in screensaver mode. The setting to run in windowed mode will not have any effect in screensaver mode. The ability to drag the window to reposition it will not have any effect in screensaver mode nor in standard mode if in fullscreen mode. In screensaver mode, the mouse cursor will be hidden and restored when the app closes. When running in screensaver demo mode (windowed within the screensaver control panel), the mouse cursor will not be hidden, and keyboard/mouse input will not exit the screensaver demo mode. In any screensaver mode, none of the hotkeys will be functional--configuration of screensaver mode is restricted solely to the settings stored by the config dialog."

## Clarifications

### Session 2025-11-30

- Q: How should the `/a` password change argument be handled? → A: Show an unsupported password-change message and exit immediately.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Launch Screensaver From Windows (Priority: P1)

A Windows user selects MatrixRain as their active screensaver and expects it to behave like a native Windows 11 screensaver when Windows invokes it with `/s`.

**Why this priority**: Without reliable fullscreen screensaver execution the feature fails its primary purpose for end users.

**Independent Test**: Trigger MatrixRain via the Windows screensaver timeout or by running `MatrixRain.scr /s` and observe that it adopts stored settings, hides the cursor, and exits cleanly when the user interacts.

**Acceptance Scenarios**:

1. **Given** Windows launches `MatrixRain.scr` with `/s`, **When** the screensaver starts, **Then** it renders full screen using the latest saved configuration and hides the mouse cursor.
2. **Given** the screensaver is running full screen, **When** the user presses any key or moves the mouse, **Then** the screensaver exits and the cursor is restored without leaving artifacts.

---

### User Story 2 - Configure Screensaver Preferences (Priority: P1)

A user opens the MatrixRain screensaver settings dialog (via `/c` or the Windows control panel) to tailor visual behavior for screensaver use and expects changes to persist.

**Why this priority**: Configuration control is essential so users can adjust intensity, colors, and behavior before leaving the animation unattended.

**Independent Test**: Launch the configuration dialog, adjust options, save, and verify the updated settings apply on the next screensaver launch regardless of session.

**Acceptance Scenarios**:

1. **Given** the user opens the settings dialog, **When** they change visual parameters (e.g., density, color theme, animation speed) and confirm, **Then** the selections are written to the registry and acknowledged with a success response.
2. **Given** updated settings exist in the registry, **When** the screensaver subsequently runs in any mode, **Then** it applies those settings without requiring further user input.

---

### User Story 3 - Continue Using MatrixRain as an App (Priority: P2)

An enthusiast launches `MatrixRain.exe` directly for interactive enjoyment, expects all hotkeys and debug helpers to behave as they do today, and wants their adjustments to persist for future sessions.

**Why this priority**: Maintaining the existing desktop experience ensures the new screensaver capability does not degrade current users' workflows.

**Independent Test**: Run the executable without screensaver arguments, exercise hotkeys, and confirm the resulting preferences are saved and reused while screensaver-specific constraints remain isolated.

**Acceptance Scenarios**:

1. **Given** MatrixRain is launched without screensaver arguments, **When** the user toggles density, color, or debug overlays via hotkeys, **Then** the current session behaves exactly as before and changes persist in the registry for the next launch.
2. **Given** MatrixRain is running in normal mode, **When** the user attempts screensaver-only actions (e.g., toggling window mode while fullscreen), **Then** the application maintains current behavior and communicates any ignored setting gracefully.

---

### User Story 4 - Preview Within Control Panel (Priority: P3)

A user previews MatrixRain inside the Windows screensaver dialog (`/p <HWND>`) and expects a safe, windowed preview that does not steal input focus or exit unexpectedly.

**Why this priority**: A reliable preview reduces user confusion and mirrors standard Windows screensaver behavior.

**Independent Test**: Invoke `MatrixRain.scr /p <windowHandle>` or use the control panel preview button and verify the animation runs windowed, ignores hotkeys, and keeps the cursor visible until closed manually.

**Acceptance Scenarios**:

1. **Given** the control panel requests a preview, **When** MatrixRain runs in the provided window, **Then** it scales to the preview bounds, keeps the cursor visible, and ignores exit-on-input behavior.

---

### Edge Cases

- Missing or corrupted registry settings must fall back to documented defaults without blocking startup.
- Unsupported or malformed command-line arguments must yield a graceful error message and exit without launching the animation.
- Running the screensaver on multi-monitor setups must either span all monitors consistently or document single-monitor limitations.
- When both `.exe` and `.scr` are present, double-launch detection must prevent multiple concurrent full-screen sessions.
- If the configuration dialog is opened while a screensaver instance is already active, the dialog must refuse changes or queue them to avoid conflicting writes.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Application MUST provide both `MatrixRain.exe` and `MatrixRain.scr` artifacts in the distribution output, with `.scr` reflecting the same binary as the executable.
- **FR-002**: Screensaver entry point MUST recognize standard Windows arguments (`/s`, `/c`, `/p <HWND>`, `/a <HWND> <data>`) and route to the corresponding behaviors.
- **FR-003**: When launched with `/s`, the application MUST open full screen on the active display, hide the mouse cursor, suppress all hotkeys, and exit immediately on user keyboard or mouse input.
- **FR-004**: When launched with `/p <HWND>`, the application MUST render within the provided preview window, retain the mouse cursor, and ignore input meant to dismiss full-screen sessions.
- **FR-005**: When launched with `/c`, the application MUST present a configuration dialog that exposes screensaver-ready options (density, color scheme, animation speed, glow intensity, glow size, fullscreen preference) and allows saving or cancelling changes.
- **FR-006**: Configuration dialog MUST persist selections to a dedicated registry location scoped per user and return success/failure feedback to the caller.
- **FR-007**: At startup in any mode, the application MUST load registry values and apply them, overriding built-in defaults only for settings explicitly stored.
- **FR-008**: While running without screensaver arguments, the application MUST behave exactly as the current desktop app, including honoring hotkeys, debug helpers, and window dragging where permitted, and persisting resulting changes back to the registry.
- **FR-009**: While running with any screensaver argument, the application MUST disable debug overlays, windowed-mode toggles, and window dragging regardless of stored preference.
- **FR-010**: Upon exit from any screensaver mode, the application MUST restore the mouse cursor state and release any system resources acquired solely for screensaver operation.
- **FR-011**: The build process MUST fail clearly if the `.scr` copy step cannot complete, ensuring releases never omit the screensaver artifact.
- **FR-012**: Error conditions (registry write failure, invalid arguments, permission issues) MUST be surfaced via user-friendly dialogs or logged events so administrators can diagnose issues.
- **FR-013**: When invoked with `/a`, the application MUST display a clear message that MatrixRain does not support Windows password changes and exit without showing the animation.

### Key Entities *(include if feature involves data)*

- **ScreenSaverSettings**: Logical collection of user-configurable options (density, color scheme, animation speed, glow intensity, glow size, fullscreen preference, debug helper enable flags) stored per user in the registry with validation and default values.
- **RuntimeModeContext**: In-memory state describing how the application was launched (normal exe, `/s`, `/p`, `/c`, `/a`), including rules for input handling, cursor visibility, feature enablement, and exit criteria.
- **DistributionArtifacts**: Representation of the deliverables created by the build (exe and scr), including metadata required for installers, documentation, and automated copying.

## Assumptions

- Configuration dialog will expose the same visual controls currently manipulated via hotkeys (density, speed, color) plus new glow intensity/size sliders and fullscreen/windowed preference; additional options can be added later without changing scope.
- Registry storage will reside under the current user's hive and does not require administrative elevation for read/write operations.
- Multi-monitor support will mirror the application's existing fullscreen behavior (span if already supported; otherwise primary monitor only) and this behavior will be documented if unchanged.
- Naming convention: when creating symbols or files, combine the words as `ScreenSaver` (PascalCase) to keep terminology consistent with Windows conventions.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Installing a new build produces both `.exe` and `.scr` assets in the release folder on 100% of build runs.
- **SC-002**: In usability testing, 90% of users can configure settings via the dialog and observe their changes in the next screensaver run without assistance.
- **SC-003**: During QA, the screensaver exits within 1 second of user input in `/s` mode and never exits unexpectedly during `/p` preview sessions across five consecutive test runs.
- **SC-004**: Normal application mode retains all existing hotkey functionality with zero regressions reported in regression testing compared to the previous release.
