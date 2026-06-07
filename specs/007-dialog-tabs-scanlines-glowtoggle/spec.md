# Feature Specification: Settings Dialog Overhaul, Scanlines & Glow Toggle (v1.5)

**Feature Branch**: `007-dialog-tabs-scanlines-glowtoggle`
**Created**: 2026-02-23
**Last Updated**: 2026-02-23 (clarifications recorded: palette persistence unconditional; Custom re-click reopens chooser; pinned `Performance (%u fps, %u%% GPU)` format with `--` placeholder; Glow/Scanlines enabled controls demoted to standard checkboxes — Win11 pill deferred; OK/Cancel/X dismissal semantics enumerated)
**Status**: Implemented
**Base Branch**: `006-multimon-gpu-efficiency` (v1.4 multimon/GPU work — required infrastructure)
**Input**: User description: "MatrixRain v1.5 — settings dialog overhaul (two-tab property sheet, live perf readout), CRT scanlines visual effect, explicit Glow on/off toggle (Win11 XAML-style), custom color picker, and complete removal of the orphaned fade-timer debug overlay."

## Clarifications

### Session 2026-02-23

- Q: Should the `CustomColorPalette` (the 16 user-editable swatches inside `ChooseColorW`) participate in the outer property-sheet Cancel rollback, or are palette edits unconditionally persisted? → A: Palette edits persist regardless of outer Cancel. Only the active `CustomColor` participates in snapshot/rollback. (Affects FR-004, FR-035, Edge Cases, SC-011.)
- Q: When the Color combo's active selection is already "Custom" and the user clicks it again, should the chooser re-open? → A: Yes — re-selecting "Custom…" while Custom is already active MUST re-open `ChooseColorW`, pre-populated with the current `CustomColor`. The combo handler uses a subclass-based watcher: subclass the combo and check `CB_GETDROPPEDSTATE` on `WM_LBUTTONUP` / `WM_KEYUP(VK_RETURN)` to detect same-item commit reliably across themes. (Affects FR-029.)
- Q: What is the exact format of the Performance tab title, and what is shown when a field is unavailable? → A: Locked format string `Performance (%u fps, %u%% GPU)` (mixed-case is intentional: lowercase `fps` matches Task Manager; uppercase `GPU` is an acronym). When either field is unavailable, the corresponding integer is rendered as `0` (e.g. `Performance (0 fps, 0% GPU)`). The format string remains uniformly `%u`/`%u` — no placeholder token. Format string lives in the .rc string table; dialog code loads it once. (Affects FR-009, FR-012, Edge Case #1.)
- Q: Should the Glow Enabled and Scanlines Enabled controls be implemented as the Win11/WinUI-style owner-drawn pill toggle, or as standard checkboxes? → A: Use standard `BS_AUTOCHECKBOX` checkboxes for both in v1.5. Defer the Win11 pill visual to a future iteration. Standard checkboxes give keyboard/focus/MSAA behavior automatically via comctl32 v6. (Affects FR-014, FR-028a, US2/US3 prose, Key Entities, Assumptions, and adds a Deferred entry.)
- Q: What are the precise commit-vs-rollback semantics for OK, Cancel, X, and Alt+F4? → A: **OK** commits the (already-live) values to the registry and closes. **Cancel**, **X**, and **Alt+F4** are equivalent: rollback to snapshot, close. No explicit `WM_CLOSE` handler is required — the default dialog proc routes `SC_CLOSE` through `IDCANCEL` → `OnCancel(hDlg)` → controller `CancelLiveMode()`. (Affects FR-004, adds new FR, Edge Cases, SC-011.)

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Reorganized Two-Tab Settings Dialog (Priority: P1)

A user opens the screensaver settings dialog and is presented with a property sheet split into two logical tabs — **Visuals** (what the screensaver looks like) and **Performance** (how hard it works the GPU). Everything still applies live; there is no Apply button. The Performance tab's title updates roughly once per second to show the live FPS and GPU usage of the running preview, so the user can dial quality up or down and immediately see the cost.

**Why this priority**: This is the structural change that everything else hangs off of. Without the tab layout, none of the new controls (Glow toggle, scanlines group, custom color, live readout) have anywhere to live. It is also the most user-visible change — the dialog is the screensaver's only UI surface.

**Independent Test**: Open the settings dialog, verify two tabs labeled "Visuals" and "Performance (NN fps, NN% GPU)" exist, verify there is no Apply button, verify changes on either tab take effect in the preview live, and verify the Performance tab title refreshes within one second of FPS/GPU readings changing.

**Acceptance Scenarios**:

1. **Given** the user launches the settings dialog, **When** the dialog appears, **Then** it shows two tabs ("Visuals" and "Performance (NN fps, NN% GPU)") with no Apply button visible.
2. **Given** the dialog is open with a live preview running, **When** one second passes, **Then** the Performance tab's title text updates to reflect the current FPS and system GPU usage.
3. **Given** the user changes any control on either tab, **When** the change is made, **Then** it applies to the preview immediately without requiring a confirm/apply action.
4. **Given** the user makes changes and clicks **Cancel**, **When** the dialog closes, **Then** all changes (on both tabs) revert to the values they had when the dialog opened — including all newly added settings.
5. **Given** GPU usage data is not yet available (first second after dialog open), **When** the timer fires, **Then** the tab title shows `0% GPU` rather than stale data.

---

### User Story 2 - Explicit Glow Enable/Disable Toggle (Priority: P1)

The user wants to turn the entire bloom/glow pipeline off — for performance, for aesthetic preference, or to see the raw rain — without having to drag the Glow Intensity slider down to a special "0 = off" value. A standard checkbox on the Performance tab provides this (the Win11/WinUI pill toggle visual is deferred to a future iteration; see Deferred). When the toggle is OFF, the entire bloom pipeline is bypassed (not just zeroed out), and every glow-related control on both tabs is visually disabled with a tooltip explaining why. When ON, all glow controls return to normal and the Glow Intensity slider's minimum reverts to 1 (no more special-case "0 = off" label).

**Why this priority**: This cleans up a long-standing UX wart (the "0 = disabled" overload on the intensity slider) and gives users explicit, discoverable control over the single most expensive part of the render pipeline. It also drives the cross-tab enable/disable propagation that the new dialog framework needs to support generically.

**Independent Test**: Toggle the Glow Enabled control OFF and verify (a) all glow-related sliders on both tabs become visually disabled, (b) hovering each shows the correct tooltip, (c) the bloom pipeline is fully bypassed in the rendered output, (d) the setting persists across runs.

**Acceptance Scenarios**:

1. **Given** the Glow Enabled toggle is ON, **When** the user inspects the Visuals tab, **Then** Glow Intensity (range 1..200) and Glow Size (range 50..200) sliders are enabled, and the intensity slider's minimum is 1 with no "(disabled)" label at the bottom.
2. **Given** the Glow Enabled toggle is ON, **When** the user toggles it OFF, **Then** Glow Intensity, Glow Size, Glow Passes, Glow Resolution, Glow Smoothness, and Quality Preset sliders all become disabled (along with their prompt labels, value labels, and info buttons), and the rendered preview shows no bloom contribution.
3. **Given** the Glow Enabled toggle is OFF, **When** the user hovers a disabled glow control on the Visuals tab, **Then** a tooltip reads "Glow is disabled on the performance tab."
4. **Given** the Glow Enabled toggle is OFF, **When** the user hovers a disabled glow control on the Performance tab, **Then** a tooltip reads "Glow is disabled."
5. **Given** the user toggles Glow Enabled OFF and closes the dialog with OK, **When** the screensaver is launched again later, **Then** the toggle is still OFF and the bloom pipeline remains bypassed.
6. **Given** an installation with no prior `GlowEnabled` registry value, **When** the dialog opens for the first time, **Then** the toggle is ON (default).

---

### User Story 3 - CRT Scanlines Visual Effect (Priority: P2)

A user nostalgic for CRT monitors gets CRT-style scanlines **on by default** in v1.5, with three dialable controls in the Visuals tab: a **Scanlines Enabled** checkbox (matching the Glow Enabled checkbox on the Performance tab — both are standard checkboxes in v1.5; the Win11 pill toggle visual is deferred), a **Scanlines Intensity** slider (1..100, default 30, controls how dark the scanline gaps go), and a continuous **Scanlines Style** slider (1..100, default 50, smoothly dials line count from barely-there texture down to authentic Apple-II chunky bands via an exponential curve). The effect renders as a cheap post-process pass after the bloom composite, runs per-monitor, applies uniformly to every pixel (no source-luminance gating), and is deliberately excluded from the quality-preset system because its cost is negligible.

**Why this priority**: Self-contained visual feature with negligible perf risk and modest scope (one shader, three controls, three registry values). Doesn't block the dialog redesign or the glow toggle; can ship independently. Default-on is an intentional v1.5 visual evolution (see User Story 6).

**Independent Test**: Open the Visuals tab on a fresh install, confirm scanlines are visible by default with the three controls (Enabled toggle ON, Intensity=30, Style=50) present in that order; toggle Enabled OFF and confirm output is indistinguishable from no-scanlines; vary Style across its range and confirm line density changes smoothly from very fine to chunky; verify the effect runs on every monitor when multi-monitor is enabled.

**Acceptance Scenarios**:

1. **Given** a fresh installation, **When** the user opens the dialog, **Then** the Visuals tab shows three scanlines controls in this order — Scanlines Enabled (standard checkbox, checked), Scanlines Intensity (slider 1..100, value 30), Scanlines Style (slider 1..100, value 50) — and scanlines are visible in the preview.
2. **Given** scanlines are enabled, **When** the user toggles Scanlines Enabled OFF, **Then** the rendered output is visually identical to having no scanline pass at all, and the Intensity and Style sliders become visually disabled.
3. **Given** scanlines are enabled at Intensity=100, **When** the user reduces Intensity to 1, **Then** the scanline gaps go from pure black to barely-visible darkening (1% intensity is the dimmest meaningful setting; the toggle owns the true off state).
4. **Given** scanlines are enabled, **When** the user moves the Style slider from 1 to 100, **Then** the visible line count changes smoothly from very fine (~981 lines, barely-there texture) through typical CRT density (~387 lines at Style=50) down to chunky authentic Apple-II bands (~150 lines at Style=100), with each ~10% slider step producing a similar perceptual change.
5. **Given** scanlines are enabled and "Use all monitors" is on, **When** the screensaver runs across multiple displays, **Then** each monitor's `RenderSystem` independently renders scanlines according to the same Style/Intensity values.
6. **Given** scanlines are enabled, **When** the user opens the Performance tab, **Then** there is no scanlines-related control on that tab and the quality preset slider has no effect on scanline appearance.
7. **Given** scanlines are enabled over a frame containing both bright streak pixels and dark background pixels, **When** the frame renders, **Then** scanline darkening is applied uniformly to every pixel (no source-luminance gating — dark areas show scanlines just as bright areas do).

---

### User Story 4 - Custom Color Picker (Priority: P2)

The user is not satisfied with Green, Blue, Red, Amber, or Cycle. They pick a new entry — **Custom…** — from the Color dropdown, which opens the standard Windows color-chooser dialog. The selected RGB drives the rain streak color and is persisted along with the 16 custom-palette slots from the chooser dialog so subsequent invocations remember both the chosen color and the user's palette.

**Why this priority**: A frequently-requested customization that is genuinely small in scope: one enum variant, one combo entry, one platform call, and one registry value (plus the palette blob). Independent of dialog redesign in concept, but lives most naturally inside the new Visuals tab.

**Independent Test**: Select Color → Custom…, pick a non-default color in the system dialog, confirm the preview rain renders in that color, close and reopen the dialog/screensaver, confirm the chosen color persists.

**Acceptance Scenarios**:

1. **Given** the Color dropdown is open, **When** the user inspects the list, **Then** the entries are Green, Blue, Red, Amber, Cycle, Custom… (in that order — Custom is appended at ordinal 5; the existing v1.4 ordinals 0..4 are preserved per FR-039 / SC-007). The selected-item display text is the same `Custom…` string used in the drop-list — there is no string mutation on selection (the ellipsis stays).
2. **Given** the user picks "Custom…", **When** the selection is committed, **Then** the standard Win32 color-chooser dialog opens pre-populated with the previously saved custom color (or bright green `RGB(0, 255, 0)` on first use).
3. **Given** the user picks a color in the chooser and clicks OK, **When** the chooser closes, **Then** the preview rain immediately renders in the chosen RGB and the Color combo shows "Custom" as the active selection.
4. **Given** the user picks a color in the chooser and clicks Cancel, **When** the chooser closes, **Then** the previous color scheme remains active and the combo selection reverts.
5. **Given** the user has previously chosen a custom color, **When** the screensaver next launches, **Then** the rain renders in that same custom color.
6. **Given** the user adjusts the chooser's 16 custom-color slots, **When** they reopen the color chooser later, **Then** those 16 slots are preserved.

---

### User Story 5 - Removal of Fade-Timer Debug Overlay (Priority: P3)

The fade-timer debug overlay was already orphaned (its hotkey was removed in an earlier release; only the dialog checkbox remained as a discoverable surface). With the dialog redesign removing that checkbox anyway, the entire feature is excised top-to-bottom — application state, settings, registry value, render code, shared state, render params, controller logic, .rc resource, and unit tests.

**Why this priority**: Pure cleanup. No user-visible benefit other than a slightly tidier settings dialog (which is already accomplished by the tab redesign). Ship-ready last because it's lowest risk and lowest value.

**Independent Test**: After implementation, grep the codebase for `FadeTimer`/`fadeTimer`/`ShowFadeTimers`/`m_showFadeTimers` — there should be zero references. All other unit tests still pass.

**Acceptance Scenarios**:

1. **Given** the codebase after removal, **When** a developer searches for fade-timer symbols, **Then** no production or test references exist (case-insensitive across all source files).
2. **Given** a user has the legacy `ShowFadeTimers` DWORD set in the registry, **When** the screensaver launches, **Then** the value is silently ignored and the screensaver behaves normally.
3. **Given** the full test suite is run, **When** unit tests complete, **Then** all 410+ remaining tests pass (the deleted fade-timer tests are removed, not failing).

---

### User Story 6 - Intentional Visible Change On Upgrade (Priority: P3)

Users upgrading from v1.3 or v1.4 will see CRT scanlines appear on first launch of v1.5 because the new Scanlines Enabled toggle defaults to ON. This is a deliberate departure from the v1.4 "no visible change on upgrade" guarantee — scanlines are the headline visual evolution of v1.5, worth showing every user by default. Anyone who dislikes the effect can disable it in one click via the Visuals tab.

**Why this priority**: Not a code change in its own right, but a documented policy decision that affects user expectations and CHANGELOG copy. Low risk, low effort, but important to make explicit so reviewers and downstream packagers don't treat it as a regression.

**Independent Test**: Take a registry hive captured from a clean v1.4 installation (no `ScanlinesEnabled` value present), launch v1.5 against it, confirm scanlines render visibly on first frame. Disable scanlines via the dialog, confirm `ScanlinesEnabled=0` is written, relaunch, confirm scanlines stay off.

**Acceptance Scenarios**:

1. **Given** a registry hive from a v1.3 or v1.4 install (no scanline values present), **When** v1.5 launches for the first time against it, **Then** scanlines render visibly (Enabled defaults to ON, Intensity defaults to 30, Style defaults to 50) and this change is explicitly called out as intentional in the v1.5 CHANGELOG entry.
2. **Given** an upgraded user dislikes the default scanlines, **When** they open the Visuals tab and toggle Scanlines Enabled OFF, **Then** scanlines disappear immediately and the OFF state persists across subsequent runs.

---

### Edge Cases

- **Live FPS unavailable**: First second after dialog open, before either FPS counter or PDH GPU monitor has produced a reading. Tab title MUST show `Performance (0 fps, 0% GPU)` rather than stale data from a prior session. The integer-`0` rendering is the documented placeholder behavior (per FR-012); the unavailable-data signal lives in the lock-free `m_hasPublishedFps` sentinel, not in the rendered string.
- **Cancel after editing custom-palette swatches**: Per FR-004 and FR-035, `CustomColorPalette` is unconditionally persisted at chooser-OK time. Clicking Cancel on the outer property sheet does NOT restore the previous palette — only the active `CustomColor` rolls back.
- **Re-selecting "Custom…" while Custom is already active**: The combo handler MUST force-open `ChooseColorW` even when no `CBN_SELCHANGE` fires (because the selection didn't actually change). The chooser is pre-populated with the current `CustomColor`. See FR-029.
- **Dismissal via X button or Alt+F4**: Identical to clicking Cancel — the snapshot is rolled back and any in-progress changes are discarded. The default dialog proc routes `WM_SYSCOMMAND SC_CLOSE` through `IDCANCEL`, so no explicit `WM_CLOSE` handler is needed.
- **GPU adapter switch while dialog is open**: User changes the GPU adapter combo, render contexts recreate. The live FPS readout should resume reporting from the recreated primary monitor context within ~1 second.
- **Scanlines at non-integer DPI scaling**: The shader's per-scanline mapping is computed from the actual output render-target height and the user's Style slider value; per-monitor DPI differences are handled implicitly because each `RenderSystem` knows its own output size.
- **Scanlines Style at extremes**: At Style=1 (~981 lines on a 1080p display), individual scanlines may be sub-pixel and produce a subtle uniform darkening or faint moiré rather than visible bands — this is acceptable and matches the "barely-there texture" intent. At Style=100 (~150 lines), bands are intentionally chunky and obviously non-physical, matching the Apple-II aesthetic.
- **Scanlines on mostly-dark content**: MatrixRain's typical frame is mostly black with bright streaks. Scanlines apply uniformly to every pixel regardless of source luminance (no gating), so dark areas of the frame still show scanline structure — this is the correct CRT behavior and is what users expect from a "scanlines" toggle.
- **Glow toggle off with quality preset on Custom**: The preset slider is disabled, but the custom passes/resolution/smoothness values still persist — toggling glow back on restores them without resetting to a preset.
- **Color picker cancelled with no prior custom color**: The combo reverts to the previously-selected built-in scheme; the default `RGB(0, 255, 0)` is NOT written to the registry until the user actually commits a custom color.
- **Property sheet tab title localization**: The Performance tab title is generated at runtime with FPS/GPU numbers interpolated; the format string must be templated, not concatenated, so a future translator can re-order or pluralize.
- **Multi-monitor with primary FPS dramatically different from secondaries**: The displayed FPS is the primary monitor's only. Secondaries may run at different rates (different refresh, different load) — this is acceptable and matches the per-monitor architecture from v1.4.
- **Old registry value `ShowFadeTimers` present**: Read and discard silently; do not warn, do not delete (some users back up registry hives, so a passive ignore is safer than active cleanup).

## Requirements *(mandatory)*

### Functional Requirements

#### Dialog Structure
- **FR-001**: The settings UI MUST be a Windows property sheet (`PropertySheetW`) with exactly two pages, in this order: "Visuals" and "Performance (NN fps, NN% GPU)".
- **FR-002**: The property sheet MUST suppress the Apply button entirely (e.g., via `PSH_NOAPPLYNOW` and per-page handling such that `PSN_APPLY` does nothing distinct from live update).
- **FR-003**: Every control on either tab MUST apply its change live to the running preview without requiring an Apply or OK action.
- **FR-004**: Clicking **Cancel** MUST roll back every setting on both tabs (including all newly added settings: Glow Enabled, Scanlines Enabled, Scanlines Intensity, Scanlines Style, Custom Color) to the values they held when the dialog opened. The `CustomColorPalette` (16 chooser swatches) is explicitly **excluded** from the rollback set: palette edits are persisted unconditionally at chooser-OK time (see FR-035).
- **FR-004a**: The three dismissal paths have well-defined semantics:
  - **OK** — Commits the (already-live) settings to the registry and closes. No additional propagation step is required because every per-control change has already been pushed into `SharedState` / `ApplicationState` via the live-preview path; the render thread is already showing the latest values.
  - **Cancel** — Rolls back the snapshot via `ConfigDialogController::CancelLiveMode()`, posts a context rebuild only if a rebuild-worthy field (e.g., GPU adapter, multimon) changed, and closes.
  - **X button / Alt+F4** — Identical to **Cancel**. No explicit `WM_CLOSE` handler is needed; the default dialog proc translates `WM_SYSCOMMAND SC_CLOSE` to `WM_COMMAND IDCANCEL`, which routes through `OnCancel(hDlg)` → `CancelLiveMode()`.

#### Visuals Tab Contents (in order)
- **FR-005**: The Visuals tab MUST contain, in this top-to-bottom order: Density slider (0..100%), Speed slider (1..100%), Glow Intensity slider (1..200%), Glow Size slider (50..200%), Color combo box, Scanlines Enabled checkbox (standard `BS_AUTOCHECKBOX`, default checked), Scanlines Intensity slider (1..100%, default 30), Scanlines Style slider (1..100, default 50), Start In Fullscreen checkbox.
- **FR-006**: The Color combo box entries MUST be, in order: Green, Blue, Red, Amber, Cycle, Custom…. The first five preserve their v1.4 ordinals (0..4); `Custom` is appended at ordinal 5. Per FR-039 / SC-007 the existing five ordinals MUST remain byte-identical to the v1.4 baseline so existing registry hives reload to the same color.
- **FR-007**: The Glow Intensity slider's minimum value MUST be 1 (changed from 0); the legacy "0% (glow disabled)" special-case label MUST be removed.

#### Performance Tab Contents (in order)
- **FR-008**: The Performance tab MUST contain, in this top-to-bottom order: GPU Adapter combo, Glow Enabled checkbox (standard `BS_AUTOCHECKBOX`, default checked), Quality Preset slider (Low/Medium/High/Custom), Glow Passes slider (1..4), Glow Resolution slider (Quarter/Half/Full), Glow Smoothness slider (Low/Medium/High), Use All Monitors checkbox (renamed from "Render on all monitors"), Show Statistics checkbox (renamed from "Show debug statistics").

#### Live Performance Readout
- **FR-009**: The Performance tab's title text MUST update at most once per second via a `WM_TIMER`-driven `PSM_SETTITLE` call. The title MUST use the exact format string `Performance (%u fps, %u%% GPU)` (after `printf` substitution, e.g. `Performance (60 fps, 45% GPU)`). The mixed case (`fps` lowercase, `GPU` uppercase) is intentional and matches Windows-ecosystem conventions (Task Manager uses lowercase `fps`; `GPU` is an acronym). The format string MUST live in the .rc as a string-table entry to permit future localization; the dialog code loads it once at init and reuses it for each timer tick.
- **FR-010**: The displayed FPS MUST be the primary monitor's render-thread FPS, exposed lock-free from each `MonitorRenderContext` (e.g., via `std::atomic<float>`).
- **FR-011**: The displayed GPU% MUST come from the same PDH counter the runtime debug overlay uses.
- **FR-012**: When either FPS or GPU% data is not yet available, the corresponding integer field MUST be displayed as `0` (e.g. `Performance (0 fps, 0% GPU)` when neither has produced a reading; `Performance (60 fps, 0% GPU)` when only GPU is missing). The format string remains uniformly `%u`/`%u` — there is no separate placeholder token.
- **FR-013**: The property sheet header MUST be created with the flags required for runtime tab-title updates (`PSH_PROPTITLE` on the header, `PSP_USETITLE` on each page).

#### Glow Enabled Toggle
- **FR-014**: The Glow Enabled control MUST be implemented as a standard `BS_AUTOCHECKBOX` (comctl32 v6). Keyboard navigation, focus rendering, and MSAA/UIA accessibility are provided automatically by the common controls and require no additional code. (The Win11/WinUI pill toggle visual is deferred to a future iteration — see the Deferred section.)
- **FR-015**: When Glow Enabled is OFF, the bloom pipeline MUST be fully bypassed in `RenderSystem` (not merely run with zero intensity).
- **FR-016**: When Glow Enabled is OFF, the following Visuals-tab controls MUST be visually disabled (`EnableWindow(FALSE)`): Glow Intensity slider, Glow Size slider, and their associated prompt labels, value labels, and info buttons.
- **FR-017**: When Glow Enabled is OFF, the following Performance-tab controls MUST be visually disabled: Glow Passes, Glow Resolution, Glow Smoothness, and Quality Preset sliders, plus their associated prompt labels, value labels, and info buttons.
- **FR-018**: Hovering any disabled glow control on the Visuals tab MUST show the tooltip text: `Glow is disabled on the performance tab.`
- **FR-019**: Hovering any disabled glow control on the Performance tab MUST show the tooltip text: `Glow is disabled.`
- **FR-020**: The Glow Enabled state MUST persist across runs in registry value `GlowEnabled` (DWORD, default 1).

#### CRT Scanlines
- **FR-021**: A new post-process pass MUST be added to `RenderSystem` rendering after the bloom composite and before the final present.
- **FR-022**: The scanline shader MUST be ported from `..\Casso\Casso\Shaders\CRT\scanlines.hlsl`, preserving the upstream attribution comment (crt-pi by Davide Berra, MIT, SPDX header).
- **FR-023**: The shader's hardcoded `kNativeScanlines = 192.0` MUST be replaced with a line count uploaded per-frame from CPU via the constant buffer. The CPU MUST compute that line count from the user's Scanlines Style slider value (1..100) using the exponential mapping `lines = 1000 * pow(0.15, style / 100.0)`, yielding ~981 lines at Style=1, ~622 at Style=25, ~387 at Style=50, ~241 at Style=75, and ~150 at Style=100. The shader itself MUST remain mapless-simple: it multiplies `i.uv.y` by whatever line count CPU sent it.
- **FR-024**: Only the `g_scanlineIntensity` shader input (0..1 normalized from the 1..100 Scanlines Intensity slider) and the per-frame line count (from FR-023) MUST be wired to user-facing controls. Other constants in the shader's cbuffer (brightness, bloom, contrast, gamma, persistence, etc.) MUST be hardcoded to identity/no-op values.
- **FR-024a**: The shader's source-luminance gating from the upstream Casso port (`weight = saturate(lum * 4.0)` and the surrounding lerp) MUST be removed. Scanline darkening MUST apply uniformly to every output pixel regardless of source luminance. (Rationale: the upstream gating is tuned for Apple-II content that is mostly bright on dark backgrounds; MatrixRain's profile is the opposite, so gating would invert the expected "CRT scanlines on the whole screen" behavior.)
- **FR-025**: Each `RenderSystem` instance MUST execute its own scanline pass (per-monitor, mirroring the bloom model).
- **FR-026**: The scanlines feature MUST NOT be wired into the quality-preset system and MUST NOT add any control to the Performance tab.
- **FR-027**: Scanline state MUST persist across runs in three registry values: `ScanlinesEnabled` (DWORD, default 1), `ScanlinesIntensity` (DWORD percent 1..100, default 30), and `ScanlinesStyle` (DWORD 1..100, default 50).
- **FR-028**: A first-run installation (no prior scanline registry values) MUST have scanlines **enabled** by default (Enabled=ON, Intensity=30, Style=50) so that new users see the v1.5 CRT aesthetic immediately. The upgrade behavior from v1.3/v1.4 to v1.5 is therefore an **intentional** visible change: existing installations with no `ScanlinesEnabled` registry value will see scanlines render on first launch. This is a deliberate exception to the v1.4 "no visible change on upgrade" guarantee. The v1.5 CHANGELOG entry MUST prominently call this out and MUST mention the one-click disable path (Visuals tab → Scanlines Enabled → OFF).
- **FR-028a**: The Scanlines Enabled control MUST be implemented as a standard `BS_AUTOCHECKBOX`, matching the Glow Enabled checkbox (FR-014). (Like Glow Enabled, the Win11/WinUI pill toggle visual is deferred — see the Deferred section.)
- **FR-028b**: When Scanlines Enabled is OFF, the scanline post-process pass MUST be fully bypassed (not merely run with zero intensity), and the Scanlines Intensity and Scanlines Style sliders (with their prompt labels, value labels, and info buttons) MUST be visually disabled.
- **FR-028c**: The Scanlines Intensity slider's minimum value MUST be 1 (not 0), matching the Glow Intensity convention from FR-007. The Scanlines Enabled toggle owns the on/off semantics; a 0 value would be redundant.

#### Custom Color Picker
- **FR-029**: Selecting "Custom…" in the Color combo MUST open the standard Win32 `ChooseColorW` dialog (commdlg.h). The chooser MUST open **every time** the user picks "Custom…", including when "Custom" is already the active selection (i.e., the user re-clicks the same item). Because `CBN_SELCHANGE` does not fire when the selection does not actually change, the combo handler MUST detect same-item re-click via a **subclass-based watcher**: subclass the combo, in the subclass proc check `CB_GETDROPPEDSTATE` on `WM_LBUTTONUP` and `WM_KEYUP(VK_RETURN)`, and force-open the chooser when the listbox was dropped-down and the Custom item is now the selection.
- **FR-030**: The chooser MUST be initialized with the previously-saved custom color, or `RGB(0, 255, 0)` on first use.
- **FR-031**: On chooser OK, the selected RGB MUST be persisted to registry value `CustomColor` (DWORD, RGB-packed) and immediately applied to the live preview.
- **FR-032**: On chooser Cancel, the previous color scheme MUST remain active; `CustomColor` MUST NOT be written.
- **FR-033**: The `ColorScheme` enum MUST gain a `Custom` variant; the chosen RGB MUST flow through `SharedState` to the render thread analogously to the existing `colorScheme` field.
- **FR-034**: When the active scheme is `Custom`, rain streaks MUST be rendered using the persisted `CustomColor` RGB rather than any of the four built-in palettes.
- **FR-035**: The chooser's 16 user-editable custom-color slots MUST be persisted as a new registry value `CustomColorPalette` (REG_BINARY, 16 × DWORD = 64 bytes), and restored on subsequent chooser opens. **Palette persistence is unconditional**: edits made inside `ChooseColorW` are written through to the registry at chooser-OK time and survive an outer property-sheet Cancel. Only the active `CustomColor` participates in the snapshot/rollback set (see FR-004).

#### Fade-Timer Feature Removal
- **FR-036**: The following symbols and their definitions, declarations, callers, and tests MUST be removed entirely from the codebase:
  - `ApplicationState::ToggleDebugFadeTimes`, `SetShowDebugFadeTimes`, `GetShowDebugFadeTimes`, and the corresponding change-callback registration.
  - `ScreenSaverSettings::m_showFadeTimers` field and accessor(s).
  - `RegistrySettingsProvider::VALUE_SHOW_FADE_TIMERS` constant and its read/write code paths.
  - `RenderSystem::RenderDebugFadeTimes` and the call site that invokes it.
  - `SharedState::showDebugFadeTimes` (both the live field and the snapshot field).
  - `RenderParams::showDebugFadeTimes`.
  - `ConfigDialogController::UpdateShowFadeTimers`.
  - The "Show fade timers" checkbox in the .rc file (already removed by the tab redesign in FR-005/FR-008).
  - All fade-timer test cases in `ConfigDialogControllerTests.cpp`, `ScreenSaverSettingsTests.cpp`, and `RegistrySettingsProviderTests.cpp`.
- **FR-037**: A legacy `ShowFadeTimers` registry value present from a prior installation MUST be silently ignored on read (not deleted, not warned about).

#### Persistence / Migration
- **FR-038**: All new registry values (`GlowEnabled`, `ScanlinesEnabled`, `ScanlinesIntensity`, `ScanlinesStyle`, `CustomColor`, `CustomColorPalette`) MUST default to their specified defaults if missing on first read (`GlowEnabled`=1, `ScanlinesEnabled`=1, `ScanlinesIntensity`=30, `ScanlinesStyle`=50, `CustomColor`=`RGB(0,255,0)` on commit, `CustomColorPalette`=zeroed).
- **FR-039**: All existing v1.4 registry values MUST continue to be read and written identically — no migration, no format change, no compatibility shim required.

#### Compatibility With Existing v1.4 Infrastructure
- **FR-040**: The quality-preset system (Low/Medium/High/Custom with drift behavior) MUST be preserved unchanged; scanlines are deliberately excluded from preset coverage.
- **FR-041**: The bloom pipeline's parametric knobs (passes/resolution/taps) MUST remain unchanged.
- **FR-042**: The first-run quality-preset heuristic MUST be preserved unchanged.
- **FR-043**: The v1.4 multi-monitor, GPU-adapter selection, device-loss recovery, frame-cap, and info-tip mechanisms MUST continue to function identically.
- **FR-044**: The existing live-preview / Cancel-rollback snapshot infrastructure in `ConfigDialogController` MUST be extended to cover every new setting added by this feature.

### Key Entities

- **Property Sheet**: The new top-level dialog container. Owns two child property pages. Owns a 1 Hz `WM_TIMER` that drives the Performance tab title update. Created modeless to remain compatible with the existing live-preview model.
- **Visuals Page**: Property page hosting all "what does it look like" controls, including the new Scanlines group and the new "Custom…" Color combo entry.
- **Performance Page**: Property page hosting all "how hard does it work the hardware" controls, including the new Glow Enabled toggle and the live FPS/GPU% in its title.
- **Glow Enabled Checkbox**: A standard `BS_AUTOCHECKBOX` (v1.5). Drives the bloom-pipeline bypass and the cross-tab enable/disable propagation. The Win11/WinUI pill toggle visual is deferred to a future iteration.
- **Scanline Shader**: A short HLSL pixel shader ported from Casso, post-bloom, pre-present, per-`RenderSystem`. Two CPU-fed inputs: intensity (0..1, normalized from the Scanlines Intensity slider) and line count (computed CPU-side from the Scanlines Style slider via `lines = 1000 * pow(0.15, style/100)`). The upstream source-luminance gating is removed so darkening applies uniformly. The shader stays mapless-simple — it just multiplies `i.uv.y` by the supplied line count.
- **Scanlines Enabled Checkbox**: A standard `BS_AUTOCHECKBOX` matching the Glow Enabled checkbox (FR-014), placed on the Visuals tab. Owns the scanline pass bypass and the enable/disable propagation to the Intensity and Style sliders.
- **Custom Color**: A packed RGB DWORD plus a 16-slot palette blob. Surfaced via a new `ColorScheme::Custom` enum variant. Threaded through `SharedState` to the render thread.
- **Live FPS Publisher**: A new `std::atomic<float>` member on `MonitorRenderContext` written by the render thread per frame and read lock-free by the dialog thread for the Performance tab title.
- **Registry Schema (v1.5 additions)**: `GlowEnabled` (DWORD, default 1), `ScanlinesEnabled` (DWORD, default 1), `ScanlinesIntensity` (DWORD 1..100, default 30), `ScanlinesStyle` (DWORD 1..100, default 50), `CustomColor` (DWORD RGB), `CustomColorPalette` (REG_BINARY 64 bytes). Legacy `ShowFadeTimers` ignored on read.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: The settings dialog presents exactly two tabs ("Visuals" and "Performance (NN fps, NN% GPU)"), with no Apply button visible and zero fade-timer-related controls anywhere.
- **SC-002**: The Performance tab title reflects live FPS and GPU% changes within ≤1 second of those readings updating in the render thread / PDH counter.
- **SC-003**: With the Glow Enabled toggle OFF, every one of the six glow-related controls (Glow Intensity, Glow Size, Glow Passes, Glow Resolution, Glow Smoothness, Quality Preset) is visually disabled on its respective tab AND the rendered output shows zero bloom contribution (verifiable by frame capture or pixel sampling against a known glow source).
- **SC-004**: Hovering any disabled glow control surfaces the appropriate tab-specific tooltip text within the standard Win32 tooltip-show delay.
- **SC-005**: Selecting Color → Custom… opens the standard system color picker; on OK the rain renders in the chosen RGB on the next rendered frame; on Cancel the previous color scheme remains active and no registry write occurs.
- **SC-006**: Scanlines render visibly by default on a fresh install (Enabled=ON, Intensity=30, Style=50). With Scanlines Enabled OFF, the rendered output is bit-identical (or perceptually identical) to having no scanline pass. With Enabled ON, moving the Style slider across its 1..100 range smoothly varies line density from ~981 lines at the low end to ~150 lines at the high end (per the `lines = 1000 * pow(0.15, style/100)` curve), and scanline darkening applies uniformly to both bright streak pixels and dark background pixels with no source-luminance gating.
- **SC-007**: All v1.4 settings (Density, Speed, Glow Intensity, Glow Size, Color, Start In Fullscreen, GPU Adapter, Quality Preset, Glow Passes, Glow Resolution, Glow Smoothness, Use All Monitors, Show Statistics) persist across runs and apply live during dialog interaction with identical behavior to v1.4.
- **SC-008**: After fade-timer removal, the existing unit test suite (410+ tests excluding the deleted fade-timer tests) passes at 100%.
- **SC-009**: A registry hive containing a legacy `ShowFadeTimers` DWORD launches the v1.5 screensaver successfully with no warnings, errors, or behavior change.
- **SC-010**: All six new registry values default to their specified defaults (`GlowEnabled`=1, `ScanlinesEnabled`=1, `ScanlinesIntensity`=30, `ScanlinesStyle`=50, `CustomColor`=`RGB(0,255,0)` on commit, `CustomColorPalette`=zeroed) when missing on first read.
- **SC-011**: The dialog's OK / Cancel / X (and Alt+F4) dismissal paths have well-defined, tested semantics: **OK** commits live values to the registry; **Cancel**, **X**, and **Alt+F4** all roll back every setting on both tabs — including the five v1.5 rollback-eligible additions (Glow Enabled, Scanlines Enabled, Scanlines Intensity, Scanlines Style, Custom Color) — to its pre-dialog value with no residual side effect on the registry or live preview. `CustomColorPalette` is explicitly **outside** the rollback set (FR-004, FR-035) and is preserved across all three dismissal paths.
- **SC-012**: The scanlines feature adds no measurable cost to the quality-preset auto-selection heuristic and is not selectable via any preset (only via its dedicated controls in the Visuals tab).
- **SC-013**: An upgrade from a v1.3 or v1.4 install (registry hive with no `ScanlinesEnabled` value) launches v1.5 with scanlines visibly enabled on the first rendered frame, and the v1.5 CHANGELOG entry documents this intentional break from the v1.4 "no visible change on upgrade" guarantee along with the one-click disable path.

## Deferred (Out of Scope for v1.5)

- **Win11/WinUI-style pill toggle controls**: Both the Glow Enabled and Scanlines Enabled controls were initially specified as owner-drawn Win11-style toggles (rounded pill + sliding circle thumb). v1.5 ships them as standard `BS_AUTOCHECKBOX` controls to deliver the underlying functionality (bypass + cross-tab enable/disable propagation + persistence) without the visual polish. The pill toggle visual — including hover/focus animation, accent-color theming, and any required custom MSAA/UIA hooks — is deferred to a future iteration.

## Assumptions

- The user runs Windows 10 1809+ or Windows 11; `PropertySheetW` with runtime `PSM_SETTITLE` calls is supported reliably on these platforms (to be re-confirmed in research during the plan phase).
- The Casso peer repository (`..\Casso`) is present on the developer's machine at spec-write time, but the shader source will be **copied into** the MatrixRain repo at port time; no runtime dependency on Casso.
- The crt-pi MIT attribution in the shader source is sufficient for license compliance; LICENSE/THIRD_PARTY_NOTICES updates will be handled during plan/implementation, not in this spec.
- The 16-slot `CustomColorPalette` blob format matches `CHOOSECOLORW`'s `lpCustColors` layout (16 × COLORREF DWORDs) verbatim; no transformation needed.
- PDH GPU% counter behavior, FPS counter infrastructure, multi-monitor render context lifecycle, and live-preview / Cancel-rollback machinery from v1.4 are functioning correctly and merely need to be extended, not rewritten.
- "Primary monitor" for the FPS readout means whichever `MonitorRenderContext` corresponds to the OS primary display at the moment the timer fires; if that monitor is hot-removed, the next available context's FPS is acceptable.
- The Glow Enabled and Scanlines Enabled controls ship in v1.5 as standard `BS_AUTOCHECKBOX` controls; the Win11/WinUI pill toggle visual is deferred (see Deferred). When that future iteration lands, the toggle will be a custom-drawn control, not a real WinUI element; visual fidelity will be "looks the part" not "pixel-perfect XAML reproduction".
- No new external dependencies (NuGet packages, DLLs, SDK components) are introduced by this feature — everything is Win32 + DirectX + existing project libs.
- The `ColorScheme::Custom` enum addition is source-only and does not need to maintain numeric ordinal stability with prior versions, because the registry persists the scheme by name/index in a way that already tolerates additions (to be verified in plan).
