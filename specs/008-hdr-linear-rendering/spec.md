# Feature Specification: HDR Output and Linear-Light Rendering

**Feature Branch**: `008-hdr-linear-rendering`

**Created**: 2026-09-21

**Status**: Draft

**Last Updated**: 2026-09-21 (planning: "glyph compositing" replaces "additive glyph stacking", since glyphs are alpha-composited; FR-011, US2 scenario 3 and SC-004 aligned with the existing all-monitor rebuild on display-topology changes; SC-005 conditioned on SDR brightness, since headroom depends on it; analysis: FR-013 wording, SC-003 scoped to Phase 2 / HDR mode Off, key entity renamed to match the data model)

**Base Branch**: `master` (v1.6.0)

**Input**: User description: "HDR output and linear-light rendering for MatrixRain. Today the whole render pipeline is 8-bit SDR ... no color space is declared, so Windows treats the output as sRGB. On an HDR display, Windows maps it at the per-display SDR content brightness and nothing can exceed SDR white, so the bright streak heads never use the panel's highlight headroom. All blending (additive glyph stacking, bloom, scanline darkening) is also done on gamma-encoded values as if they were linear light, so glow falls off wrongly and bright overlaps flatten early -- this affects SDR users too. Deliver in three independently shippable phases: (1) linear-light rendering with SDR output, (2) native HDR output on HDR-enabled monitors with highlights still capped at SDR white, (3) highlight headroom above SDR white with tone mapping, an HDR mode setting and a highlight brightness control."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Physically Correct Light Blending on Every Display, Same Look as v1.6 (Priority: P1)

A user on any monitor, SDR or HDR, watches the rain. Glow spreads and fades the way light does: halos around bright heads fall off smoothly instead of ending in a hard edge, overlapping halos add up to something brighter rather than flattening into a dull plateau, and glyph edges blend cleanly where streaks cross, and the long fading tails on the black background show no visible banding. Scanline darkening reads as a raster laid over glowing phosphor rather than as a gray veil. At default settings the effect looks like the same MatrixRain the user already knows, just cleaner.

**Why this priority**: This fixes a correctness problem that affects every user, not just HDR owners, and it is the foundation the HDR phases build on: HDR output is only meaningful once the image is computed in light-linear terms. It ships on its own without any HDR hardware.

**Independent Test**: On an SDR monitor, compare v1.6 and this build side by side at default settings. Verify the overall brightness, glow size and color look essentially unchanged; that glow halos fade smoothly without a visible edge; that fading tails show no banding steps; and that where two bright halos overlap the result is brighter than either alone.

**Acceptance Scenarios**:

1. **Given** default settings on an SDR monitor, **When** the user compares this build to v1.6, **Then** overall brightness, glow extent and rain color at a normal viewing distance are perceptually the same, differing only in the smoothness of gradients and overlaps.
2. **Given** a slowly fading tail on a black background, **When** the user examines it closely, **Then** no stepped bands are visible across the fade.
3. **Given** two bright streak heads whose glow halos overlap, **When** they overlap, **Then** the overlap is brighter than either halo alone, without a flat, clipped plateau.
4. **Given** scanlines enabled at any intensity, **When** the user compares a bright glyph with and without scanlines, **Then** the scanline gaps darken it in proportion to the Intensity setting, with the same density per character as v1.6.
5. **Given** each built-in color scheme and a custom color, **When** rendered, **Then** the color matches the selected scheme or picked color as closely as it did in v1.6.
6. **Given** each quality preset, **When** the user selects it, **Then** the frame rate stays at or near the display refresh rate on hardware that held it in v1.6.

---

### User Story 2 - Native HDR Output That Matches the User's SDR Brightness (Priority: P2)

A user with Windows HDR turned on for one or more monitors runs MatrixRain. On each HDR monitor the app now drives the display natively as HDR instead of relying on Windows to convert its SDR output. In this phase nothing looks different: the rain appears at the same brightness the user has chosen for SDR content on that display, and help, hotkey, usage and statistics overlays are crisp and correctly bright. A user with one HDR monitor and one SDR monitor sees both monitors behave correctly at the same time. If the user switches HDR on or off in Windows Settings while MatrixRain is running, the affected monitor adapts within moments without the app restarting or crashing.

**Why this priority**: This establishes correct, native HDR presentation, which highlight headroom (User Story 3) depends on, while deliberately keeping the look unchanged, so any regressions can be isolated to the output path rather than to a creative change.

**Independent Test**: With Windows HDR on for a monitor, run this build and v1.6 in turn at the same settings. Verify the brightness and color match; open every overlay and verify it is legible and at normal brightness; toggle HDR off and on in Windows Settings while running and verify the monitor keeps rendering correctly each time.

**Acceptance Scenarios**:

1. **Given** Windows HDR is on for a monitor, **When** MatrixRain runs, **Then** that monitor is driven with native HDR output and the rain's brightness matches the SDR content brightness the user set for that display in Windows.
2. **Given** one HDR monitor and one SDR monitor, **When** MatrixRain runs on both, **Then** each renders correctly in its own mode and the rain's character appearance is the same on both.
3. **Given** MatrixRain is running, **When** the user turns Windows HDR on or off for a monitor, **Then** that monitor switches to the matching output mode within 2 seconds, without restarting the app.
4. **Given** an HDR monitor, **When** the user opens the help, hotkey reference, usage or statistics overlay, **Then** the overlay text is sharp, correctly colored and no brighter or dimmer than it appears on an SDR monitor at the user's SDR brightness.
5. **Given** an HDR monitor, **When** the user changes the SDR content brightness slider in Windows while MatrixRain runs, **Then** the rain's brightness follows the new setting.
6. **Given** a display, driver or remote session that cannot present HDR, **When** MatrixRain runs there, **Then** it falls back to SDR output exactly as in User Story 1, with no error shown.

---

### User Story 3 - Glowing Highlights Beyond SDR White (Priority: P3)

A user with an HDR monitor wants the rain to look like it is made of light. The white leading glyphs of each streak, and the brightest parts of their glow, now shine brighter than anything an SDR display can show, up to what that monitor can physically produce, while the green trails stay at the user's normal SDR brightness. Nothing clips harshly: the brightest highlights roll off smoothly as they approach the display's limit. The user can choose how bright the highlights get, or turn the HDR treatment off entirely and keep the SDR look on an HDR monitor.

**Why this priority**: This is the visible payoff of the feature, but it is purely additive: Stories 1 and 2 are complete and valuable without it, and it is the part most dependent on subjective tuning on real hardware.

**Independent Test**: On an HDR monitor, with HDR mode set to Auto, verify streak heads are visibly brighter than SDR white while trails stay at normal brightness; raise and lower the highlight brightness control and verify highlights follow; set HDR mode to Off and verify the look returns to that of User Story 2.

**Acceptance Scenarios**:

1. **Given** an HDR monitor and HDR mode set to Auto, **When** streaks fall, **Then** streak heads and the core of their glow appear brighter than SDR white, while trails remain at the user's SDR content brightness.
2. **Given** highlights approaching the display's peak brightness, **When** they are rendered, **Then** their brightness rolls off smoothly with no visible hard clip, color shift or flat-topped plateau.
3. **Given** the user moves the highlight brightness control, **When** the change is made, **Then** highlight brightness changes immediately in the live preview, and the setting persists across runs.
4. **Given** HDR mode set to Off, **When** MatrixRain runs on an HDR monitor, **Then** no content exceeds SDR white and the result matches User Story 2.
5. **Given** two HDR monitors with different peak brightness, **When** MatrixRain runs on both, **Then** trail brightness, glyph shape, glow extent and scanline density match; only the height of the highlights differs, according to what each display supports.
6. **Given** an SDR monitor, **When** the user views the settings dialog, **Then** the HDR controls either do not apply or clearly indicate they affect HDR displays only, and changing them has no effect on that monitor.
7. **Given** the user presses Cancel in the settings dialog after changing HDR settings, **When** the dialog closes, **Then** the HDR settings revert along with every other setting.

---

### Edge Cases

- **HDR display with little headroom** (for example, a panel whose peak brightness is at or barely above the user's SDR white level): highlights gain little or no extra brightness and must still roll off smoothly rather than clip or darken the rest of the image.
- **SDR white level set above the display's reported peak**: normal content must not be pushed past what the display can show; the app treats the effective ceiling as the lower of the two.
- **Display reports no or implausible peak brightness**: a conservative default ceiling is used rather than an extreme or zero value.
- **HDR toggled, monitor hot-plugged, or resolution changed while running**: only the affected monitor rebuilds its output; others keep rendering without interruption.
- **Graphics device lost or reset while in HDR mode**: recovery restores the same output mode the monitor was in before the loss.
- **Settings dialog open when HDR is toggled**: the live preview and dialog remain usable; the HDR controls' availability updates to reflect the new state.
- **Screensaver preview window** (the small preview in Windows' screensaver settings): stays SDR and looks as it does in v1.6.
- **Very saturated custom colors** (for example, pure blue): remain the picked hue in every phase and on every display, without hue shifts introduced by brightness scaling or roll-off.
- **Windows Auto HDR or other system-level SDR-to-HDR processing**: must not double-process MatrixRain's output once it presents natively as HDR.
- **Screenshots and screen recordings of an HDR monitor**: may look different from the live display in tools that do not understand HDR; this is not treated as a defect.
- **Quality presets on weaker GPUs**: the added precision must not push the Low preset below the frame rate it held in v1.6 on the same hardware.

## Requirements *(mandatory)*

### Functional Requirements

#### Phase 1 - Linear-light rendering (User Story 1)

- **FR-001**: The system MUST perform all light *combination* — compositing glyphs and overlays into the scene, blurring, and adding glow — on values proportional to emitted light (linear light), not on gamma-encoded values. The *shape and strength* of each effect a user can tune (the trail fade curve, the halo's falloff, the weight of antialiased glyph edges, and the darkening the Scanlines Intensity slider applies) MUST reproduce v1.6's on-screen result, which means those terms are computed in gamma space where they were authored and converted to linear light afterward. Principle: light combines in linear light; everything a user can tune keeps its v1.6 meaning. (Revised during Phase 1 calibration: the original wording put scanline darkening in linear light, which made every saved Intensity value about 50% weaker than the user had set it.)
- **FR-002**: The system MUST convert all color inputs (built-in schemes, cycling scheme, custom colors, overlay colors) from their authored display encoding to linear light before they are blended.
- **FR-003**: Intermediate results MUST carry enough precision that smooth fades on a black background show no visible banding, and bright overlaps are not clipped before the final output step.
- **FR-004**: The system MUST encode the final image for the display only once, at output.
- **FR-005**: At default settings, the overall brightness, glow extent and color of the rain on an SDR display MUST remain perceptually equivalent to v1.6; default glow and brightness parameters MUST be recalibrated as needed so that the correctness fix does not read as a change of style. Every user-facing slider (Glow Intensity, Glow Size, Scanlines Intensity and Style) MUST produce at each position the result it produced in v1.6, so that saved settings keep their meaning; where linear-light combination changes a slider's effect, the slider's mapping is corrected, not the user's value.
- **FR-006**: Existing saved settings (glow intensity, glow size, scanline intensity and style, color, quality) MUST keep producing a look close to what they produced in v1.6, so users who tuned their settings do not need to retune them.
- **FR-007**: Scanline density per character MUST remain as defined in v1.6 (identical on every monitor regardless of size, rotation or display scaling).
- **FR-008**: Each quality preset MUST keep the frame rate it achieved in v1.6 on the same hardware, or the presets MUST be re-tuned so that it does.

#### Phase 2 - Native HDR output (User Story 2)

- **FR-009**: The system MUST determine, per monitor, whether Windows HDR is enabled, and MUST present natively in HDR on HDR-enabled monitors and in SDR on all others.
- **FR-010**: The system MUST support monitors in different modes at the same time (mixed HDR and SDR setups).
- **FR-011**: When a monitor's HDR state, the set of connected monitors, or a monitor's resolution changes while running, the system MUST reconfigure the affected monitor(s) within 2 seconds without restarting. Where the change is detected per monitor, other monitors MUST NOT be disturbed; where Windows reports it as a display-topology change, other monitors MAY briefly re-initialize, as they do for any display change today.
- **FR-012**: On HDR monitors, content at SDR white MUST be displayed at the SDR content brightness the user has set for that display in Windows, and MUST follow changes to that setting while running.
- **FR-013**: In this phase, rendered content MUST NOT exceed SDR white on any monitor.
- **FR-014**: The help, hotkey reference, usage and statistics overlays MUST render correctly on HDR monitors: sharp, correctly colored, and at the same apparent brightness as on SDR monitors.
- **FR-015**: Where HDR presentation is unavailable or fails to initialize (display, driver, remote session or insufficient hardware capability), the system MUST fall back silently to the Phase 1 SDR output for that monitor.
- **FR-016**: Recovery from a lost or reset graphics device MUST restore each monitor to the output mode it was in before the loss.
- **FR-017**: The screensaver preview window MUST continue to render in SDR.

#### Phase 3 - Highlight headroom (User Story 3)

- **FR-018**: On HDR monitors with HDR mode set to Auto, the brightest content — streak heads and the core of their glow — MUST be allowed to exceed SDR white, up to the display's peak brightness.
- **FR-019**: Content approaching the display's brightness ceiling MUST roll off smoothly toward it, with no hard clip, and the roll-off MUST preserve hue.
- **FR-020**: Trails, background and overlays MUST remain at the user's SDR content brightness; only highlights gain headroom.
- **FR-021**: The system MUST provide an **HDR mode** setting with the values **Auto** (use highlight headroom on monitors where Windows HDR is on) and **Off** (never exceed SDR white). Default: **Auto**.
- **FR-022**: The system MUST provide a **highlight brightness** control that sets how far above SDR white highlights may go, bounded by each display's peak. It MUST apply live and persist like other settings.
- **FR-023**: The HDR mode and highlight brightness settings MUST participate in the settings dialog's live preview, Cancel rollback and Reset to defaults behavior like every existing setting.
- **FR-024**: The settings dialog MUST make clear that the HDR settings affect only HDR-enabled displays.
- **FR-025**: Apart from highlight brightness, which depends on each display's physical capability, character appearance — trail brightness relative to SDR white, glyph shape, glow extent, color and scanline density — MUST be identical across monitors.
- **FR-026**: When a display's reported peak brightness is missing or implausible, the system MUST assume a conservative ceiling; when the SDR white level exceeds the reported peak, the effective ceiling MUST be the lower of the two.

#### Quality (all phases)

- **FR-027**: The pure computations underlying this feature — conversion between display encoding and linear light, SDR white-level scaling, highlight roll-off, and the per-monitor choice of output mode — MUST be covered by automated unit tests.
- **FR-028**: Visual correctness on HDR hardware (brightness matching, overlay legibility, highlight roll-off, mixed-mode setups, live HDR toggling) MUST be verified manually on real HDR displays before each phase ships.

### Key Entities

- **Display output mode**: Per monitor, whether it is presented as SDR or HDR, derived from Windows' HDR state for that monitor and the user's HDR mode setting.
- **Display luminance** (`DisplayLuminance`): Per monitor, the user's SDR white level and the display's reported peak brightness, which together define where normal content sits and how much headroom highlights have.
- **HDR settings**: The persisted, user-facing HDR mode (Auto / Off) and highlight brightness, stored and rolled back with the other screensaver settings.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: At default settings on an SDR monitor, viewers comparing v1.6 and this build side by side cannot reliably tell which is which except by looking for gradient smoothness and bright overlaps.
- **SC-002**: Fading tails on a black background show zero visible banding steps at normal and close viewing distances, on both SDR and HDR monitors.
- **SC-003**: On an HDR monitor, with no content above SDR white (Phase 2, or HDR mode Off in Phase 3), the rain's measured brightness at SDR white is within 5% of the display's configured SDR content brightness.
- **SC-004**: Toggling Windows HDR for a monitor while running results in correct output on that monitor within 2 seconds, in 10 out of 10 attempts, with no crash or freeze on any monitor.
- **SC-005**: With HDR mode Auto on a display capable of at least 600 nits peak and an SDR content brightness of at most 250 nits, streak heads measure at least 2x the SDR white level at the default highlight brightness.
- **SC-006**: No monitor, SDR or HDR, drops more than 5% below the frame rate v1.6 achieved at the same quality preset on the same hardware.
- **SC-007**: On a mixed HDR/SDR setup, trail brightness, glyph shape, glow extent and scanline density are indistinguishable between monitors when compared side by side.
- **SC-008**: All overlays are fully legible on HDR monitors in every phase, with no text rendered noticeably brighter or dimmer than on an SDR monitor at the same SDR brightness.

## Assumptions

- **Phasing**: The three phases ship as separate releases in order; each is complete and valuable without the ones after it. Phase 2 has no visible change by design.
- **Calibration target for Phase 1**: "Must not regress the current look" means default settings are recalibrated so that brightness and glow size stay perceptually the same; the smoother falloff, stronger overlaps and absence of banding are the intended visible differences.
- **Color gamut**: Colors stay within the standard display color range (the same primaries as today). Wide-gamut, more saturated colors on wide-gamut panels are out of scope.
- **Which content gets headroom**: Only streak heads and the brightest part of their glow exceed SDR white. Trails, background, scanline gaps and all overlays stay at or below SDR white.
- **Default highlight brightness**: Moderate by default — clearly brighter than SDR white on a typical 600–1000-nit HDR monitor without being uncomfortable in a dim room — with the maximum bounded by each display's peak.
- **HDR mode default**: Auto, so HDR owners get the new look after upgrading to Phase 3; it is called out in the release notes with how to switch it Off.
- **Windows is the source of truth for HDR state**: The app follows Windows' per-monitor HDR on/off switch and SDR brightness setting; it does not turn HDR on or off itself.
- **Platform**: Windows 11 with a GPU and driver that support HDR presentation; anything less falls back to SDR (FR-015).
- **Out of scope**: HDR in the screensaver preview window, HDR screenshots/recordings, per-monitor HDR settings, and HDR10 metadata or tone-mapping hints to the display.
