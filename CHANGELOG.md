# Changelog

All notable changes to MatrixRain are documented in this file.

## [Unreleased]

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
