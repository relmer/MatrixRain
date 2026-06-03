# Feature Specification: Multi-Monitor User Control and GPU Efficiency

**Feature Branch**: `006-multimon-gpu-efficiency`  
**Created**: 2026-06-03  
**Status**: Draft  
**Input**: User description: "Improve MatrixRain v1.4: make multi-monitor rendering optional (default on); add a GPU adapter selection so users on hybrid laptops can pick integrated vs discrete; respond appropriately to monitors and GPUs being added or removed while running (current behavior leaves a 90% GPU load after undocking a Surface Book 3); and reduce overall GPU usage on hybrid hardware through a frame-rate cap on high-refresh monitors and a graphics quality preset spectrum (Low / Medium / High / Custom) with advanced controls behind a disclosure toggle and accessible information tips."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Adapt immediately when monitors are added or removed (Priority: P1)

A Surface Book 3 user is running MatrixRain across both the external monitor and the laptop screen. They undock the laptop, disconnecting the external monitor. MatrixRain continues running on the remaining laptop screen and immediately stops consuming GPU work for the disconnected monitor. When they redock, the external monitor begins displaying MatrixRain again without restarting the application.

**Why this priority**: This is a correctness defect today. After an undock, MatrixRain continues rendering at ~90% GPU into a non-existent monitor; on this hardware it can never recover until the user restarts the application. Fixing this is a blocking quality issue for hybrid-laptop users, who are also the population most affected by all other items in this feature.

**Independent Test**: With MatrixRain running on a multi-monitor setup, physically disconnect one monitor (or disable it in Display Settings) and verify that within a few seconds GPU utilization for MatrixRain drops to a level consistent with rendering only to the remaining monitors. Reconnect the monitor and verify MatrixRain resumes rendering to it without manual intervention.

**Acceptance Scenarios**:

1. **Given** MatrixRain is running spanning two monitors at high GPU load, **When** the user disconnects one monitor, **Then** within 1 second only the remaining monitor continues to be rendered and GPU utilization drops to a level comparable to having started in the new topology.
2. **Given** MatrixRain is running on a single monitor after a disconnect, **When** the user reconnects an additional monitor, **Then** MatrixRain begins rendering to it within 1 second without requiring restart.
3. **Given** MatrixRain is rendering on a specific GPU and that GPU's driver is reset or it becomes temporarily unavailable, **When** the device-loss event occurs, **Then** MatrixRain recovers automatically and resumes rendering without user intervention or visible error dialogs.

---

### User Story 2 - Choose whether MatrixRain spans all monitors (Priority: P1)

A multi-monitor user opens MatrixRain's configuration dialog and toggles off the "render on all monitors" option because they only want the screensaver on their primary display. The change takes effect immediately on the running instance and persists across restarts.

**Why this priority**: Multi-monitor rendering is currently always-on. Some users find this distracting (gaming/working secondary monitor, ambient lighting on a TV in the same room) or simply want lower GPU cost. Today they have no opt-out. Making it a setting (default on) is small but unblocks a real user choice.

**Independent Test**: Open the configuration dialog on a multi-monitor system, locate the "render on all monitors" control, toggle it off, dismiss the dialog. Verify MatrixRain immediately stops rendering on non-primary monitors. Reopen the dialog and confirm the setting persisted; toggle it back on and verify all monitors resume rendering.

**Acceptance Scenarios**:

1. **Given** MatrixRain is running on multiple monitors with the multi-monitor setting on (default), **When** the user opens the configuration dialog and turns the setting off, **Then** within 1 second only the primary monitor continues rendering and the secondary monitors return to their normal desktop content.
2. **Given** MatrixRain was last configured with the multi-monitor setting off, **When** the application is launched, **Then** it renders only on the primary monitor.
3. **Given** a single-monitor system, **When** the user opens the configuration dialog, **Then** the multi-monitor control is present but has no effect on behavior (always renders on the one available monitor).

---

### User Story 3 - Pick which GPU MatrixRain uses on hybrid laptops (Priority: P2)

A user on a Surface Book 3, Surface Laptop Studio 2, or NVIDIA Optimus / AMD PowerXpress laptop opens MatrixRain's configuration dialog. They see a list of the GPUs available on their machine by their real names (e.g., "Intel Iris Xe Graphics", "NVIDIA GeForce RTX 3050 Ti"), with the system default annotated. They select the integrated GPU to extend battery life. MatrixRain begins running on that GPU and the change persists across restarts.

**Why this priority**: On hybrid laptops, MatrixRain currently always runs on whichever GPU the operating system chose. There is no way for the user to prefer the lower-power integrated GPU when on battery, or the higher-performance discrete GPU when plugged in. The previous priority items (US1, US2) have higher urgency: US1 fixes a defect that affects this same hardware, US2 is the smallest power lever. With those addressed, this becomes the next-most-impactful user control.

**Independent Test**: On a hybrid-laptop system with two or more rendering-capable adapters, open the configuration dialog and verify the GPU list shows the real adapter names with "(default)" appended to the system default. Select a non-default adapter, dismiss the dialog. Verify (via Task Manager's GPU view) that MatrixRain switches to using the selected GPU. Restart MatrixRain and verify it starts on the selected GPU. Choose a GPU that doesn't exist (by editing the saved setting to a fake name), restart, and verify MatrixRain falls back to the default adapter without error.

**Acceptance Scenarios**:

1. **Given** a system with multiple non-software GPU adapters, **When** the user opens the configuration dialog, **Then** the GPU selection control lists each adapter by its real name and the system default adapter is annotated with "(default)".
2. **Given** software/basic-render adapters exist on the system, **When** the user views the GPU selection list, **Then** those adapters are not included in the list.
3. **Given** the user has selected a specific GPU, **When** the dialog is dismissed, **Then** MatrixRain begins rendering on that GPU within 1 second on the running instance and continues to do so on subsequent launches.
4. **Given** a previously selected GPU is no longer present on the system at launch (removed, disabled, or driver uninstalled), **When** MatrixRain starts, **Then** it falls back to the system default adapter and starts successfully without showing an error dialog.

---

### User Story 4 - Cap frame rate on high-refresh monitors (Priority: P2)

A user with a 144Hz laptop display starts MatrixRain. Rather than rendering 144 frames per second per monitor (wasteful for a screensaver), MatrixRain limits itself to 60 frames per second on that monitor, substantially reducing GPU work. A user on a 60Hz monitor sees no change from prior behavior.

**Why this priority**: High-refresh displays are common on modern laptops, and rendering a screensaver at 144 FPS is gratuitously expensive — every frame pays the full rendering and bloom cost. A simple per-monitor cap at 60 FPS for refresh > 60Hz is the largest guaranteed GPU saving available with no visible quality impact for this content. Lower priority than US1/US2 only because it is invisible to users (no UI to surface it) and its impact is felt as cooler hardware rather than as a feature.

**Independent Test**: On a system with a >60Hz monitor, run MatrixRain with debug statistics enabled and verify the FPS counter on that monitor reports approximately 60. On a system with a 60Hz monitor, verify the FPS counter reports approximately 60 (the existing behavior). Compare GPU utilization on the high-refresh case before and after the change to confirm a substantial reduction.

**Acceptance Scenarios**:

1. **Given** a monitor whose native refresh rate is greater than 60Hz, **When** MatrixRain renders to that monitor, **Then** rendering is limited to approximately 60 frames per second on that monitor.
2. **Given** a monitor whose native refresh rate is 60Hz or less, **When** MatrixRain renders to that monitor, **Then** rendering continues at the monitor's native refresh rate as before, without any added overhead from the limiter.
3. **Given** a multi-monitor setup with mixed refresh rates (e.g., 60Hz primary and 144Hz secondary), **When** MatrixRain renders to both, **Then** the 60Hz monitor runs at 60 FPS and the 144Hz monitor also runs at approximately 60 FPS, each evaluated independently.

---

### User Story 5 - Tune graphics quality with presets and an advanced disclosure (Priority: P3)

A user opens MatrixRain's configuration dialog and sees a "Quality" preset selector offering Low, Medium, and High. They pick Low for battery savings or High for the richest look. A more advanced user enables an "advanced graphics settings" toggle that reveals individual controls for the number of glow passes, the resolution of the glow buffer, and the smoothness of the glow blur. As they adjust any individual control, the preset selector automatically switches to "Custom" and remembers those values for next time they pick Custom. Each quality control has an "ⓘ" indicator next to it; hovering the mouse over it or tabbing to it and pressing Space/Enter shows a short description of the control and its GPU performance impact (Significant / Moderate / Small).

**Why this priority**: This is the most powerful efficiency lever (resolution divisor alone can be ~4x cheaper) but it is also the largest UI surface in this feature. It is lower priority than US1-US4 because: US1 fixes a defect; US2/US3 are simpler controls that unlock immediate user choice; US4 captures the largest fixed win without any UI work. US5 is the long-tail quality/perf control surface that benefits power-conscious users who want to dial in their own tradeoff.

**Independent Test**: Open the configuration dialog, locate the Quality preset selector, verify Low/Medium/High options are present. Switch through them and verify visible changes to glow appearance. Enable the advanced toggle and verify three additional controls appear (Passes, Resolution, Smoothness). Move any one of them and verify the preset selector switches to Custom. Switch the preset back to High; verify all advanced controls snap to the High preset's values. Switch to Custom again and verify the previously-customized values are restored. Hover the mouse over each "ⓘ" indicator and verify a tooltip appears with descriptive text ending in one of "Significant GPU performance impact.", "Moderate GPU performance impact.", or "Small GPU performance impact.". Tab to each "ⓘ" indicator and press Space; verify the same tooltip appears.

**Acceptance Scenarios**:

1. **Given** the configuration dialog is open, **When** the user views the Graphics section, **Then** a Quality preset selector is visible with at least Low, Medium, and High options.
2. **Given** the user has not enabled the advanced toggle, **When** the dialog is shown, **Then** the advanced graphics controls are not visible and the dialog occupies only the space needed for the always-visible controls.
3. **Given** the user enables the advanced toggle, **When** the toggle is checked, **Then** the dialog grows to reveal the advanced controls and the existing OK/Cancel/Reset buttons reposition appropriately; unchecking the toggle reverses this.
4. **Given** any named preset is selected, **When** the user moves an advanced control, **Then** the preset selector automatically switches to "Custom" and the current advanced control values are remembered as the "last custom" set.
5. **Given** a named preset is selected, **When** the user selects a different named preset, **Then** all advanced controls snap to that preset's defined values.
6. **Given** the user has previously used Custom and saved a custom set of values, **When** the user selects "Custom" from the preset selector after having moved through named presets, **Then** the advanced controls restore to the saved custom values.
7. **Given** the user has never used Custom in this dialog session and there is no saved custom set, **When** the user selects "Custom" from the preset selector, **Then** the advanced controls remain at their current values.
8. **Given** the existing Glow Intensity slider, **When** the user moves it to 0%, **Then** the glow effect is fully disabled (not merely darker) and the slider's value indicator reads "0% (glow disabled)".
9. **Given** any quality-related control with an "ⓘ" indicator, **When** the user hovers the mouse over the indicator OR tabs to it with the keyboard and presses Space or Enter, **Then** a tooltip appears containing descriptive text and ending with exactly one of "Significant GPU performance impact.", "Moderate GPU performance impact.", or "Small GPU performance impact.".
10. **Given** a fresh installation with no saved quality preset, **When** the user launches MatrixRain on a system with a discrete GPU, **Then** the High preset is applied; on a system with only an integrated GPU and a modest pixel load, the Medium preset is applied; on a system with an integrated GPU driving multiple high-resolution monitors, the Low preset is applied.

---

### Edge Cases

- A user toggles the "render on all monitors" setting off while MatrixRain is currently spanning multiple monitors: the secondary monitor windows must be cleanly removed within 1 second and not leave stale or frozen content on those displays.
- A monitor is added or removed while MatrixRain is running in screensaver mode (`/s`): MatrixRain must adapt and keep running rather than exiting, the same as in normal display mode.
- A user toggles the "render on all monitors" setting off and then immediately back on: MatrixRain must end in the fully-spanning state without leaving any monitor stuck in an in-between state.
- The user's previously selected GPU has been removed, disabled, or replaced between MatrixRain runs: MatrixRain must start successfully on the default adapter without an error dialog and without losing the user's other preferences.
- The user's previously selected GPU is removed *while MatrixRain is running*: MatrixRain must detect the loss and continue running on the default adapter.
- A laptop is suspended and resumed while MatrixRain is running: MatrixRain must recover gracefully (the GPU and monitors may have effectively been "removed and re-added" from MatrixRain's perspective).
- The system reports a monitor whose native refresh rate cannot be determined: MatrixRain must fall back to standard vertical-sync behavior rather than refusing to render or capping incorrectly.
- The user selects "Custom" before ever having moved an advanced control: the advanced controls must stay at their current values (whatever named preset they reflect), and from that point any subsequent adjustment becomes the new "last custom".
- A configuration dialog is open while a monitor is removed: the dialog must remain functional and dismissible; new MatrixRain behavior takes effect after dialog dismissal.
- The user cancels (rather than OKs) the configuration dialog after having adjusted multiple controls in advanced mode: all live previews are reverted and persisted settings are unchanged.

## Requirements *(mandatory)*

### Functional Requirements

#### Multi-monitor user control

- **FR-001**: System MUST provide a user-visible setting in the configuration dialog to enable or disable multi-monitor spanning.
- **FR-002**: Multi-monitor spanning MUST default to enabled for new installations and for existing users who have never seen this setting.
- **FR-003**: Toggling the multi-monitor setting MUST take effect on the running application within 1 second, without restart.
- **FR-004**: System MUST persist the multi-monitor setting across application restarts.

#### Runtime topology and device-loss response

- **FR-005**: System MUST detect monitor connection and disconnection events while running.
- **FR-006**: When monitors are added or removed, system MUST adapt within 1 second so that rendering resources for absent monitors are released and rendering resources for newly-present monitors are created. This MUST happen in both normal display mode and screensaver mode (`/s`).
- **FR-007**: When the active GPU becomes unavailable (driver reset, removal, sleep/resume, or other device-loss event), system MUST detect the loss and re-initialize on an available adapter (falling back to the system default if the previously-chosen adapter is no longer present), without showing an error dialog and without exiting.
- **FR-008**: System MUST collapse a burst of topology-change notifications into a single re-initialization rather than re-initializing once per notification.

#### GPU adapter selection

- **FR-009**: System MUST provide a user-visible control in the configuration dialog to select which GPU is used for rendering, listing each available rendering-capable adapter by its real, human-readable name.
- **FR-010**: GPU selection MUST mark the system's default adapter by appending "(default)" to its name in the list.
- **FR-011**: GPU selection MUST exclude software adapters (e.g., Microsoft Basic Render Driver) from the list.
- **FR-012**: GPU selection MUST default to "system default adapter" for new installations.
- **FR-013**: The selected GPU MUST persist across application restarts identified by adapter description.
- **FR-014**: If the previously selected adapter is not present at startup, system MUST silently fall back to the system default adapter and continue starting.
- **FR-015**: Changing the GPU selection MUST take effect on the running application within 1 second, without restart.
- **FR-016**: All monitors rendered by the application MUST use the user's selected GPU; the selection is application-wide, not per-monitor.

#### Frame-rate efficiency

- **FR-017**: For any monitor whose native refresh rate exceeds 60Hz, system MUST limit rendering on that monitor to approximately 60 frames per second.
- **FR-018**: For any monitor whose native refresh rate is 60Hz or less, system MUST continue to render synchronized to vertical refresh as today, with no measurable per-frame overhead from the limiter.
- **FR-019**: The frame-rate limit applies independently per monitor; mixed-refresh-rate multi-monitor setups MUST each follow the rule above.

#### Graphics quality presets and advanced controls

- **FR-020**: System MUST provide a Quality preset control with at least the named options "Low", "Medium", and "High", plus a "Custom" option that is selected automatically (never directly by the user) whenever the advanced controls' values do not match any named preset.
- **FR-021**: Selecting a named Quality preset MUST set all advanced graphics controls to that preset's predefined values within 1 second of the selection.
- **FR-022**: The "High" preset's values MUST produce the same visual result as the current (pre-feature) default rendering, so existing users who never touch the new controls see no change.
- **FR-023**: Adjusting any individual advanced graphics control MUST automatically switch the Quality preset to "Custom" and update an in-memory "last custom" snapshot of all advanced control values.
- **FR-024**: Selecting "Custom" from the preset control MUST restore the saved "last custom" values if any have been saved previously; otherwise the advanced controls MUST remain at their current values.
- **FR-025**: System MUST persist both the Quality preset name and (when the preset is "Custom" or when a custom set has been used at least once) the "last custom" advanced values across application restarts.

#### Advanced graphics controls — visibility and individual controls

- **FR-026**: System MUST provide an "advanced graphics settings" toggle control. When this toggle is off, the advanced graphics controls MUST NOT be visible and the configuration dialog MUST occupy only the space needed for the always-visible controls.
- **FR-027**: When the advanced toggle is turned on, the configuration dialog MUST dynamically grow to reveal the advanced controls; existing dialog controls below the advanced section (OK, Cancel, Reset) MUST reposition appropriately. Turning the toggle off MUST reverse this.
- **FR-028**: System MUST provide an advanced control for the number of glow passes (integer, 4 discrete positions from minimum to maximum) with labeled tick positions and a value indicator showing the current value.
- **FR-029**: System MUST provide an advanced control for the glow buffer resolution with 4 discrete positions labeled "Eighth", "Quarter", "Half", "Full" from minimum to maximum, defaulting to "Half".
- **FR-030**: System MUST provide an advanced control for the glow blur smoothness with 3 discrete positions labeled "Low", "Medium", "High" from minimum to maximum, defaulting to "High".
- **FR-031**: The existing Glow Intensity slider MUST own the on/off state of the glow effect: when its value is set to 0%, the glow effect MUST be fully disabled (no glow processing performed at all), and the slider's value indicator MUST read "0% (glow disabled)" instead of "0%".

#### Configuration dialog — common conventions

- **FR-032**: All percentage sliders in the configuration dialog MUST display unlabeled tick marks. Tick frequency MUST be chosen so that a tick falls at the midpoint of each slider's range where the range allows an integer midpoint, with a target of approximately 21 ticks across the range.
- **FR-033**: All discrete-position sliders (the three advanced graphics controls) MUST display a label beneath each tick position indicating the value at that tick.
- **FR-034**: Every new quality-related control (Quality preset, advanced toggle, advanced controls, Glow Intensity, Glow Size) MUST have an information indicator (an "ⓘ" / lowercase 'i' in a circle) immediately associated with it.
- **FR-035**: Information indicators MUST be reachable both by mouse hover and by keyboard focus (Tab). Activating an indicator (mouse hover, or Space/Enter while focused) MUST display a tooltip containing the indicator's descriptive text.
- **FR-036**: Every information tip's text MUST consist of one or more descriptive sentences followed by exactly one of the standardized perf-impact sentences: "Significant GPU performance impact.", "Moderate GPU performance impact.", or "Small GPU performance impact.".

#### First-run defaults

- **FR-037**: On first run with no saved Quality preset, system MUST select a default preset based on the system's available GPU class and the total resolution of currently-connected monitors:
  - Systems with a discrete GPU available: "High".
  - Systems with only integrated GPUs and a modest total monitor pixel count: "Medium".
  - Systems with only integrated GPUs and a heavy total monitor pixel count (e.g., multiple high-resolution monitors): "Low".
- **FR-038**: On first run with no saved GPU selection, system MUST default to the system default adapter (same behavior as omitting any selection).
- **FR-039**: On first run with no saved multi-monitor setting, system MUST default to multi-monitor spanning enabled.

### Key Entities *(include if feature involves data)*

- **Multi-monitor setting**: A single yes/no preference whether MatrixRain renders on all monitors or only the primary. Defaults to yes.
- **Selected GPU adapter**: A persistent identifier (human-readable adapter description) of the user's preferred rendering adapter, or empty meaning "use system default". Used to look up the actual adapter at startup and after device-loss recovery; falls back to system default if not found.
- **Quality preset**: The currently-selected named preset (one of "Low", "Medium", "High", "Custom"). Determines the values of the advanced graphics controls when set to a named preset.
- **Advanced graphics control values**: A group consisting of (passes, resolution, smoothness, glow intensity). Driven by the Quality preset's predefined values when a named preset is selected; freely editable when Custom is selected.
- **Last custom values**: A persisted snapshot of the most recent set of advanced control values the user actually customized. Restored when the user re-selects "Custom" after having used a named preset. Does not exist until the user has customized at least once.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: After disconnecting an external monitor on a hybrid laptop while MatrixRain is running on both, the application's GPU utilization within 1 second drops to a level comparable (within 10%) to launching MatrixRain fresh with only the remaining monitor connected.
- **SC-002**: After reconnecting a previously-disconnected monitor, MatrixRain begins rendering to that monitor within 1 second without requiring the application to be restarted.
- **SC-003**: A user on a hybrid laptop can change the selected GPU through the configuration dialog and observe (via the operating system's GPU view) MatrixRain switch to using the newly-selected GPU within 1 second, without restarting the application.
- **SC-004**: On a 144Hz monitor with default settings, MatrixRain renders at no more than 65 frames per second per monitor and reduces GPU work for that monitor by at least 50% relative to v1.3 baseline behavior on the same hardware.
- **SC-005**: On a Surface-class integrated-GPU laptop running MatrixRain with default settings on its built-in display, GPU utilization is no more than 70% of the v1.3 baseline for the same hardware and content.
- **SC-006**: Every information tip in the configuration dialog can be read both by hovering with a mouse and by tabbing to its indicator and pressing Space or Enter; both methods produce the same text.
- **SC-007**: When the previously-saved GPU adapter is not present at application start, the application starts successfully on the system default adapter without showing any error dialog.
- **SC-008**: Existing users who upgrade and never open the configuration dialog see no visible change in the rendered output of MatrixRain (the "High" quality preset is visually identical to the v1.3 default).
- **SC-009**: A user can switch between named quality presets in the configuration dialog and observe the visual change on all active monitors within 1 second of selection, without restarting the application.
- **SC-010**: After customizing the advanced graphics controls and selecting a different named preset, the user can return to "Custom" and find their previously-customized values restored exactly.

## Assumptions

- Target operating systems are Windows 10 version 1803 or later, and Windows 11. On these versions, the operating system's modern GPU-preference APIs are sufficient to route MatrixRain's rendering to the user's chosen adapter on NVIDIA Optimus, AMD PowerXpress, and Surface-class hybrid systems. Legacy driver-export mechanisms for forcing the discrete GPU are not employed in this version of the feature; they may be added later if real-world testing on a target hybrid configuration shows the OS-level APIs are insufficient.
- The DWM compositor runs on the integrated GPU on hybrid laptops, and presenting from the discrete GPU therefore incurs a cross-adapter copy at present time. This is expected, normal, and the discrete GPU's compute headroom is assumed to far exceed that copy cost.
- A user who selects the discrete GPU expects MatrixRain's process to be routed to it; the operating system's per-application GPU preference (if set by the user separately) is assumed to either match or be deliberately overridden. We do not modify the per-application OS preference automatically.
- Single-monitor users retain their current behavior exactly: no visible change, no new dialog footprint that meaningfully shifts the layout for the single-monitor case beyond the additional graphics-quality and information-tip controls.
- Users who have not opened the configuration dialog continue to see MatrixRain's current default visual quality with no perceptible regression (the "High" preset is calibrated to be visually equivalent to the v1.3 default).
- Adapter human-readable descriptions are stable enough across reboots and driver updates to be used as the persistent identifier for the user's GPU selection. A driver update that renames an adapter would be treated as the adapter being absent and would silently fall back to the default; this is acceptable.
- Monitor refresh rates queried from the operating system are correct and stable while a given monitor is connected; if the OS cannot report a refresh rate for a monitor, fallback to standard vertical-sync rendering is acceptable.
- The configuration dialog continues to use the operating system's standard Win32 dialog rendering and accessibility behaviors; no third-party UI framework is introduced. The information-tip indicator may be a small owner-drawn control to achieve the required appearance (a lowercase "i" in a circle) while remaining keyboard-focusable.
- The "heavy" pixel-count threshold used to choose between Medium and Low first-run presets on integrated GPUs is a tunable constant; its initial value (approximately 16 million pixels — e.g., two 4K monitors, or one 8K monitor) is acceptable and can be revisited based on real-world feedback.
- Existing test infrastructure (the project's 354-test unit test suite, pure-helper + in-memory provider pattern) is the appropriate place for behavioral tests of the new pure helpers introduced by this feature. The Win32 dialog, information-tip control, owner-draw rendering, real hot-plug behavior, and real-hardware GPU utilization measurements are validated by manual QA on the user's hybrid-laptop hardware.
