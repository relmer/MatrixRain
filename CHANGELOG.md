# Changelog

All notable changes to MatrixRain are documented in this file.

## [1.7.0] - Unreleased

### Added

- **Native HDR output.** On a monitor with Windows HDR turned on,
  MatrixRain now presents in HDR itself instead of being converted by
  Windows. The rain sits at the brightness you set with the Windows "SDR
  content brightness" slider and follows it live, so with highlights off
  it looks the same as on an SDR monitor. Each monitor is handled on its
  own: an HDR and an SDR monitor side by side both look right, and
  turning HDR on or off in Windows takes effect within a second without
  restarting. The screen saver preview, and any failure to switch,
  fall back to SDR.
- **HDR highlights.** On HDR monitors, the head of each streak and the
  brightest glyphs just behind it now shine brighter than normal white,
  in their own color, with their glow, up to what the monitor can show.
  The rest of each trail stays at your normal brightness. Highlights
  roll off smoothly toward the monitor's peak rather than clipping. How
  much room there is depends on the monitor's peak brightness and the
  Windows SDR content brightness slider: the lower that slider, the more
  the highlights stand out.
- **HDR settings.** The Visuals tab has an **HDR highlights** setting
  (Auto or Off) and a **Brightness** slider for how bright highlights
  get. When no monitor has HDR turned on the controls gray out and the
  dialog says why; if a monitor supports HDR but has it turned off, it
  also offers a link that opens the Windows HDR settings.

### Changed

- **Light is now combined as light.** The renderer composites glyphs
  and their glow in linear light with float render targets, instead of
  in the display's gamma-encoded 8-bit values. Fading tails on black are
  smooth all the way down with no stepped bands, antialiased glyph edges
  composite as light does, and where two bright heads pass each other
  the overlap adds instead of plateauing. Everything a user can tune
  keeps its meaning: the trail fade, the glow's strength, size, hue and
  behavior where streaks overlap, the Scanlines Intensity and Style
  sliders, and every color scheme reproduce what 1.6 put on screen, to
  within a few percent of mean brightness on a fixed test scene and
  indistinguishable side by side. No settings change.
- **Shaders are compiled at build time.** A broken shader is now a build
  error carrying file, line and message, rather than a blank screen at
  run time. The D3D shader compiler is no longer loaded by the app.

### Fixed

- **Dragging the window across a monitor boundary froze the app.** In
  windowed mode, moving the window to a monitor with a different scale
  factor stalled it for five seconds, and sometimes for good, with
  Windows' translucent "not responding" ghost drawn over rain that was
  still falling. The window's resize handler was waiting on the render
  thread while the render thread was waiting on the window to finish
  resizing. The resize is now applied by the render thread itself, and
  the window answers at once. Present since windowed mode was added.
- **Reset to defaults and Cancel did not fully reach the running rain.**
  In the settings dialog, Reset moved every slider back, but density,
  speed, glow intensity and glow size kept their old values on screen
  until the slider was touched again; Cancel had the same gap.
- **Cancel did not undo density, speed or glow.** Moving those four
  sliders saved them at once, so after Cancel the rain looked right but
  the next start came up with the canceled values. The dialog now saves
  only on OK.
- **Gray squares behind the info icons.** The settings dialog painted a
  gray square behind each (i) icon on Windows 11's themed tabs; the
  icons now sit on the tab's own background.

### Upgrade note

**On an HDR monitor, streak heads are brighter than before.** HDR
highlights are on by default (HDR highlights: Auto, Brightness 80%). To
keep exactly the 1.6 look, set **HDR highlights** to **Off** on the
Visuals tab. Nothing changes on SDR monitors.

## [1.6.0] - 2026-09-21

### Changed

- **Versioning switched to MAJOR.MINOR.PATCH.** Version numbers are now
  bumped by hand when a release is cut, replacing the auto-incrementing
  build counter. This release is 1.6.0 rather than 1.5.1 so it sorts
  above 1.5.2161 and is offered as an update.

### Fixed

- **Scanline density now matches on every monitor.** The Style slider
  used to set a fixed number of scanlines across the whole screen
  height, while the rain glyphs are a fixed size. Monitors of different
  heights therefore showed different scanlines per character: a
  portrait 3840-pixel-tall panel got barely half as many as a
  2160-pixel landscape one at the same setting. Style now sets the
  number of scanlines per character cell, and that ratio is the same on
  every monitor regardless of size, rotation, or Windows display
  scaling.
- **Upper half of the Style slider was unusably coarse.** At high Style
  values the old mapping dropped to one or two scanlines per glyph,
  reading as a venetian blind rather than a CRT raster. Style now runs
  from about 10 scanlines per character cell (Style 1) down to 6
  (Style 100).
- **Scanlines aliased into a moire pattern at dense settings.** On
  1080p and 1440p panels much of the lower half of the Style range fell
  below two pixels per scanline and resolved into a crawling beat
  pattern. The scanline shader now area-averages each pixel, so dense
  settings fade smoothly instead of shimmering.
- **Scanline sliders didn't respond to Enable scanlines.** Toggling the
  checkbox on the Performance tab left the Intensity and Style sliders
  on the Visuals tab in their old state until the settings dialog was
  reopened. They now enable and disable immediately, including after
  Reset to defaults.
- The Scanline Intensity tooltip claimed 0% disables the effect, but the
  slider's minimum is 1%. It now points at **Enable scanlines** on the
  Performance tab, which turns the effect off entirely.

### Upgrade migration note

**Your saved Scanline Style value will look different after
upgrading.** The slider now controls scanlines per character instead of
scanlines per screen height, so the same number maps to a different
density. The change is largest on tall or high-resolution monitors,
which previously rendered far coarser scanlines than smaller ones. If
the new look isn't to your taste, readjust **Scanline style** on the
Visuals tab.

## [1.5.2161] - 2026-06-06

### Added

- **Two-tab configuration property sheet.** The single-page settings dialog
  is now split into **Visuals** and **Performance** tabs hosted by the Win32
  `PropertySheetW` host control. Visuals owns Density, Speed, Glow Intensity /
  Size, Color scheme, the Scanline Intensity / Style sliders, and
  Start-in-fullscreen. Performance owns Multi-monitor, Enable glow, Enable
  scanlines, GPU adapter, Quality preset + advanced sliders, and Show
  performance metrics. Each page shows the primary monitor's FPS
  and the process's GPU load in a bottom-right readout, updated once per
  second (`NN fps, NN% GPU`).
- **Cross-page "Reset to defaults" button.** Lives in the property-sheet
  footer (left of OK/Cancel) and resets every control on both tabs in one
  click; the live preview snaps back instantly.  The persisted 16-swatch
  custom-color palette is preserved across Reset (FR-035 carve-out).
- **Enable glow checkbox** on the Performance tab. Toggling it OFF
  bypasses the entire bloom pipeline (no extract / blur / composite passes)
  and greys every glow-dependent control on both tabs with an explanatory
  tooltip pointing at the toggle's location. Bloom GPU resources stay
  allocated so re-enabling is instant. Replaces the v1.4 "Glow Intensity =
  0" workaround; the slider minimum is back to 1.
- **CRT scanlines post-process** (Enable toggle on Performance; Intensity /
  Style sliders on Visuals). Enabled by default
  on fresh installs and after upgrade (see migration note below). Intensity
  slider (1-100, default 30) controls darkening strength; Style slider
  (1-100, default 50) controls line density (Style 1 ≈ 981 lines / Style
  100 ≈ 150 lines, geometric falloff). Runs independently of glow — the
  no-glow render path routes through `m_postBloomTarget` so the scanline
  PS always has a populated SRV.
- **Custom color picker** as a sixth Color combo entry (`Custom…`).
  Selecting it opens the standard Win32 `ChooseColor` dialog pre-populated
  with the prior custom RGB (default `RGB(0, 255, 0)`). The 16-swatch
  palette persists across launches (and across Reset).  A clickable
  owner-draw color swatch next to the combo previews the selected scheme;
  in Cycle mode the swatch animates at 30 Hz in sync with the rain.
  Clicking the swatch opens the chooser regardless of which scheme is
  currently active.
- **Per-page live FPS / GPU% readout** in each page's bottom-right corner.
  Stable tab title text ("Performance") avoids the per-second tab-control
  flicker that an earlier prototype had.

### Changed

- **Glow Intensity slider minimum** reverted from 0 to 1 (the dedicated
  Enable glow toggle now owns "off").
- **Label renames** in the Performance tab: "Render on all monitors" →
  "Use all monitors"; "Show debug statistics" → "Show performance
  metrics". Registry value names unchanged; existing installs round-trip
  without migration.
- **Modeless property-sheet teardown** now uses the canonical
  `PropSheet_GetCurrentPageHwnd` polling pattern in the main message
  loop instead of `DestroyWindow`-from-PSN_APPLY (which re-entered the
  frame subclass and overflowed the stack).
- **Config dialog Z-order** is re-asserted after
  `Application::RebuildContextsForCurrentMode` so toggling Start In
  Fullscreen no longer hides the dialog behind the freshly-created
  rain window.
- **Overlay text color** (Settings/Help/Exit hint, `?` hotkey list,
  `/?` usage dialog) now resolves `ColorScheme::Custom` from
  `snapshot.customColor` instead of falling through `GetColorRGB`'s
  green fallback.

### Removed

- The orphan fade-timer debug overlay (`ShowFadeTimers` registry value,
  `m_showFadeTimers` field, dialog checkbox, and the backtick (`)
  debug hotkey). Legacy `ShowFadeTimers` registry values are silently
  ignored on Load.

### Fixed

- **Reset-to-defaults wiped the saved custom-color palette.** The 16
  saved swatches now survive Reset and Reset→OK per FR-035.
- **Glow-off path could sample stale bloom.** The no-glow composite
  branch now binds a null SRV at slot 1 so the composite PS doesn't
  resample whatever bloom texture was left bound by a prior frame.
- **Window-message ID collision.** Two `WM_APP` messages (Reset-button
  reposition and custom-color-chooser open) had been defined with the
  same numeric value; separated to distinct IDs.
- **Modeless `PropertySheetW` failure path.** A `-1` return is now
  recognised as failure instead of being cast to `(HWND)-1` and treated
  as a valid sheet handle.

### Upgrade migration note

**Visible change on first launch after upgrading from v1.4:** existing
installs will see scanlines render immediately on the first v1.5 launch,
because `ScanlinesEnabled` defaults to ON. If you'd prefer the v1.4
look, open the **Performance** tab and uncheck **Enable scanlines**.
The setting persists across runs. All other v1.4 settings (density,
color scheme, glow intensity, multi-monitor, GPU adapter, quality
preset, advanced sliders) round-trip unchanged on upgrade.

### Known Issues

- ARM64 builds require serial (`/m:1`) MSBuild invocation under the
  current Visual Studio 18 ARM64 cross-compile toolchain due to a
  per-process virtual-memory limit on PCH state when the test project
  is built in parallel with the EXE. This is a build-host concern, not
  a runtime issue; the resulting binaries are identical.

## [1.4.2098] - 2026-06-06

### Added

- **User Story 1 (P1) — Runtime topology and device-loss recovery.** MatrixRain now responds to monitors being added or removed while running (WM_DISPLAYCHANGE, coalesced across windows) and to the active GPU becoming unavailable (driver reset, sleep/resume, eGPU unplugged - detected via Present HRESULT). The render context is rebuilt automatically; this fixes the previously-reported "GPU stuck at ~90% after undocking a Surface Book 3" defect where the ghost monitor's render thread continued processing forever.
- **User Story 2 (P1) — Optional multi-monitor spanning.** New "Use all monitors" checkbox in the configuration dialog (default on). Toggling it live applies within 1 second; Cancel reverts.
- **User Story 3 (P2) — GPU adapter selection.** New "GPU" dropdown in the configuration dialog listing each real adapter name (with "(default)" appended to the system default). Software/WARP adapters are excluded. Selection persists by description string; if the saved adapter is missing at startup, the application silently falls back to the system default. Live device-switch takes effect within 1 second.
- **User Story 4 (P2) — Frame cap on high-refresh monitors.** Per-monitor `FrameLimiter` engages only when the monitor's native refresh exceeds 60 Hz, capping that monitor's rendering to 60 FPS. At ≤60 Hz the existing vsync path is preserved with no measurable per-frame overhead. Substantially reduces GPU work on 144Hz / 165Hz laptop displays.
- **User Story 5 (P3) — Graphics quality preset spectrum.** New "Quality" slider (Low / Medium / High / Custom) in a "Graphics quality" group box. Three discrete tuning sliders are always visible — Glow passes (1-4), Glow resolution (Eighth / Quarter / Half / Full), Glow smoothness (Low / Medium / High). Each quality-related control has an "ⓘ" infotip; hovering or tabbing-then-pressing-Space/Enter reveals a tooltip with a description and standardized GPU-performance-impact phrase. First-run heuristic picks a starting preset based on detected GPU class and total monitor pixel count (discrete → High; integrated + modest load → Medium; integrated + heavy load → Low). Custom-drift behavior: any direct edit of an advanced control auto-flips the preset to Custom and saves the resulting values for restoration when the user later re-selects Custom.

### Changed

- Bloom pipeline is now runtime-parametric: blur passes (1-4), bloom buffer resolution (full/half/quarter/eighth), and blur kernel taps (5 / 9 / 13) are all selectable per-frame from the quality preset / advanced sliders. Three blur shader variants are compiled at startup so the tap-count switch is free at draw time.
- Default settings on a fresh install now pick a quality preset matched to detected hardware rather than always using the maximum.

## [1.3.1984] - 2026-06-03

### Added

- Multi-monitor support: in fullscreen and screensaver modes, every connected display now renders its own independent Matrix rain animation. Each monitor runs on a dedicated render thread and is paced by its own VSync, so animation stays smooth across displays.
- Per-monitor DPI awareness for multi-monitor setups: each display sizes glyphs according to its own DPI/scaling, with the color-cycle clock shared across monitors so they stay in sync.

### Changed

- Default color scheme (and the settings dialog **Reset**) is now **Color Cycle** instead of green.
- Settings dialog color names are now capitalized (Green, Blue, Red, Amber, Cycle); persisted registry values remain lowercase for compatibility.

### Fixed

- Overlay text (help hint, hotkey reference) now DPI-scales to the primary monitor on multi-monitor setups; previously the shared overlay glyph atlas could be sized to a secondary monitor's DPI, making overlays render too small on a high-DPI primary.

### Known Issues

- On setups that mix display scaling levels (e.g. a 100%-scaled monitor next to a 150%-scaled one), glyph size is derived from each monitor's Windows scaling percentage rather than true physical pixel density, so characters can appear visibly larger on the lower-scaled display. See `specs/002-add-screensaver-support/multimon-architecture.md`.

## [1.2.1983] - 2026-05-23

### Fixed

- Settings dialog changes not taking effect on the running animation in standalone mode: color scheme, "Show debug stats", and "Show fade timers" toggles updated `ApplicationState` but never propagated to the render thread's `SharedState`, so the visual change only appeared after a restart
- Copyright string corrected to "Robert Elmer"

## [1.2.1973] - 2026-03-23

### Fixed

- Render loop double-pacing: VSync (`Present(1, 0)`) and `sleep_for(16ms)` both ran per frame, capping FPS at ~32 instead of 60
- Input latency for overlay show/dismiss: moved `Present()` outside the overlay mutex so the UI thread's `OnKeyDown` no longer blocks up to 16ms waiting on VSync

## [1.2.1954] - 2026-03-23

### Fixed

- Infinite recursion in WindowsFileSystemProvider due to Win32 macro name collision
- Silent failure on uninstall errors — now shows user-facing error messages
- Use non-assert error handling for runtime file system failures in uninstall path

### Added

- VS Code "Attach to MatrixRain" debug configuration for attaching to running processes

## [1.2.1953] - 2026-03-22

### Added

- Screensaver install/uninstall via `/install` and `/uninstall` switches
- UAC elevation prompt for install/uninstall operations
- Group Policy and MDM/Intune policy detection before install
- `/force` or `--force` switch to bypass policy warnings on install
- `IFileSystemProvider` interface for testable file system operations
- `MockFileSystemProvider` for fully isolated unit tests

### Changed

- `Uninstall()` and `CleanupRegistryForUninstall()` accept `IFileSystemProvider` for testability
- Elevation check uses injectable function pointer for mock support

### Fixed

- CI test failures: screensaver installer tests no longer assume elevation state
- All unit tests fully mocked — no real filesystem, registry, or OS calls

## [1.1.1819] - 2026-03-21

### Added

- Added usage dialog (`/?` or `-?`) showing command-line options
- Help hint overlay on startup showing Settings/Help/Exit keys with auto-dismiss
- Hotkey reference overlay toggled via `?` key listing all keyboard shortcuts

### Changed

- Default rain density reduced for better visual balance

### Fixed

- Config dialog cancel not restoring keyboard input
- Overlay character cycling and initial glyph display

## [1.1.1307] - 2026-03-09

### Added

- Per-monitor DPI awareness for consistent character sizing across displays
- Multi-pass Gaussian blur for smoother, wider bloom glow
- Glow size slider now controls blur kernel radius in real time

### Changed

- Bloom compositing uses exponential soft-saturation to prevent clipping
- Lowered bloom extraction threshold for a more visible glow at normal brightness

### Fixed

- Character size and spacing now scale correctly on high-DPI displays

## [1.0.1001] - 2026-02-23

### Added

- MIT LICENSE file
- WinGet package manifest (`relmer.MatrixRain`) and automated publish step
- VERSIONINFO resource block for Windows file properties

### Changed

- Conventional commits convention adopted
- Improved thread safety: extended mutex scope in game loop, thread-local RNG
- Extracted shared color scheme parsing; removed `void*` casts
- Reuse temp vectors in `RemoveExcessStreaks` for reduced allocations
- Validate string length on registry reads
- Statically link the C runtime (eliminates vcruntime140.dll dependency)
- Explicit diagnostic when D3D debug layer is unavailable (Graphics Tools)

### Fixed

- Strengthened unit test assertions and fixed CharacterSet initialization
- WinGet workflow: use `env` context for PAT secret in step conditions

## [1.0.1000] - 2026-01-17

### Added

- ARM64 native build support (x64 + ARM64)
- VS Code C++ IntelliSense configuration for both architectures
- Test tasks for Release configurations

### Fixed

- Incorrect default in settings tests
- PCH configuration error for the profiler build flavor
- Glow effect for red and blue color schemes (luminance threshold fix)

## [0.9] - 2025-12-13

### Added

- Full screensaver mode (`/s`, `/c`, `/p` command-line switches)
- Configuration dialog with live overlay on the running animation
- Live wiring of animation speed, glow sliders, checkboxes, and color scheme to running app
- Preview mode in the Windows Screen Saver control panel
- Dual-mode `/c` handling: modal dialog or live overlay depending on context
- Registry-based settings persistence
- CLI argument parser with `/s`, `/c`, `/c:HWND`, `/p:HWND` support
- `.scr` build artifact generation (post-build copy of `.exe`)
- Input exit state tracking for screensaver mode (mouse/keyboard triggers exit)
- ScreenSaverModeContext runtime enforcement
- Error handling and unit tests for command-line parsing

### Fixed

- Reset button on settings dialog not updating color scheme of running app
- Debug overlay suppression and dialog centering issues in screensaver mode

## [0.8] - 2025-11-30

### Added

- README with project description and screenshots
- Application icon
- Robust VS Code build scripts, tasks, and launch configurations
- VS Code workspace file

### Changed

- Default to full-screen mode on launch
- `S` key toggles statistics overlay on/off
- Refactored application startup with EHM (Error Handling Macros) support

### Fixed

- Statistics text wrapping at window edge

## [0.7] - 2025-11-28

### Added

- Color cycling animation option
- Active head-based streak spawning system
- Full-screen mode (borderless window, not exclusive)
- Window drag support via `WM_NCHITTEST` handler
- API awareness manifest; profiler build configuration

### Changed

- Improved scaling behavior when switching to/from full screen
- Streak density test coverage expanded

### Fixed

- Viewport resize assertions during head-based spawning
- Streak lifetime edge cases and debug lifetime counter
- Fade timer expiration when bright time expires
- Window drag disabled in full-screen mode

## [0.6] - 2025-11-25

### Added

- Gaussian blur bloom post-processing effect
- Feathered glow effect on FPS overlay for legibility against rain streaks
- Redesigned density system: percentage-based (0–100%), reference-based viewport, doubled max streaks, 50 ms spawn interval
- FPS overlay with rain statistics
- Color cycling keyboard toggle

### Changed

- Refactored `UpdateInstanceBuffer`, `ApplyBloom`, and shader creation code

### Fixed

- Bloom rendering in full-screen mode
- Animation stopping while in full-screen mode
- Full-screen mode switched from DirectX exclusive to borderless window

## [0.5] - 2025-11-22

### Added

- Rain density control system with `+`/`-` keyboard input
- Depth sorting, zoom wrapping, and color transition
- D3D11 texture creation for character mutation
- RenderSystem wiring and integration tests
- Perspective projection

### Changed

- Drop interval no longer changes during a streak's lifetime

### Fixed

- Fade behavior; added pause/resume (`P` key)

## [0.4] - 2025-11-06

### Added

- Direct2D + DirectWrite text rendering for texture atlas (replaces placeholder glyphs)
- Discrete cell-based character dropping
- Stationary characters with fade-in-place animation
- `ESC` key to exit application

### Changed

- Adjusted rain drop speed for better visual experience
- Removed horizontal drift

### Fixed

- Despawn logic at viewport boundaries

## [0.3] - 2025-11-06

### Added

- **MVP COMPLETE**: Matrix Rain renders with placeholder glyphs
- DirectX 11 rendering pipeline (RenderSystem)
- Application class and WinMain entry point
- AnimationSystem: streak lifecycle, character mutation, fade
- CharacterSet texture atlas via Direct2D
- Viewport with TDD
- CharacterStreak and CharacterInstance with TDD

### Fixed

- Window creation error (`ERROR_INVALID_WINDOW_HANDLE`)

## [0.2] - 2025-11-05

### Added

- Math types and utility functions (Vector2, Vector3, Color)
- CharacterSet singleton and GlyphInfo struct
- Character constants for Matrix Rain glyphs (Katakana, Latin, numerals)
- High-resolution Timer utility
- Comprehensive unit tests for Phase 2 components (100% coverage)
- Precompiled headers for MatrixRain executable project

## [0.1] - 2025-11-05

### Added

- Initial project: Visual Studio solution with MatrixRainCore (static lib), MatrixRain (app), MatrixRainTests
- Matrix Rain visual effect specification
- Implementation plan and design documentation
- Task breakdown (157 tasks across 5 phases)
