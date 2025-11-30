# Feature Specification: Matrix Rain Visual Effect

**Feature Branch**: `001-matrix-rain`  
**Created**: 2025-11-05  
**Status**: Draft  
**Input**: User description: "I'm building an app to show 'Matrix rain' in the style of The Matrix. The app should use DirectX for the best performance and should allow running both windowed and full-screen. The rain characters should be comprised of the full set of 71 half-width katakana characters (basic, dakuten, handakuten) and western Latin letters and numerals. They should be rendered backwards, flipped left-to-right, using a green color reminiscent of old green phosphor screens. The leading character of a drop of rain should render in white but turn to green as soon as the drop descends to the next character. The characters should fade to black over 3 seconds starting as soon as they first appear. The leading character of the drop should be chosen randomly from the set above, but as the drop advances down the screen, the characters it leaves behind should mostly remain unchanged. There should be a 5% chance of any still-visible (not fully faded to black) character switching to another character, but this does not alter the fade-out progression. The scene should be rendered with depth--some drops should be far away and others nearer to the camera. The scene should zoom in very slowly over time, never stopping. This will cause streaks of raining characters to appear to get bigger as the zoom increases. The characters should have a faint surrounding glow around them proportional to each of their current brightness levels. The user should be able to increase or decrease the density (number of drops/streaks) of rain by pressing + (or equals) to increase and - to decrease. There should be a minimum and maximum number of streaks visible at a time. The density is controlled by altering the frequency of new streaks being added, but their fade out rate should remain constant. The user should be able to toggle between windowed and full-screen by pressing Alt-Enter."

## Clarifications

### Session 2025-11-16

- Q: What is the target falling speed for character streaks? → A: Variable: Speed randomized per streak within a range
- Q: What is the exact RGB value for the green phosphor color? → A: RGB(0, 255, 100) - Classic bright Matrix green
- Q: How should the character glow effect be implemented technically? → A: Gaussian blur with radius 4-8 pixels
- Q: What is the time interval for a streak to advance one character position downward? → A: 0.3 seconds per character cell (3-4 cells/second)
- Q: What is the zoom speed (camera advancing toward scene)? → A: 2.0 units/second (moderate, 50 second cycle, more noticeable)

## User Scenarios & Testing *(mandatory)*

### User Story 1 - View Animated Matrix Rain (Priority: P1)

A user launches the application and immediately sees the iconic Matrix-style falling character animation filling their screen. The green phosphor-like characters cascade downward with realistic depth perspective, creating an immersive visual experience reminiscent of the film.

**Why this priority**: This is the core value proposition - the visual effect itself. Without this, the application has no purpose. This is the minimum viable product.

**Independent Test**: Can be fully tested by launching the application and observing the character rain animation. Success means seeing green glowing characters falling down the screen with proper fade-out and depth perspective.

**Acceptance Scenarios**:

1. **Given** the application is launched, **When** the window appears, **Then** falling character streaks are immediately visible
2. **Given** characters are falling, **When** observing for 3 seconds, **Then** characters fade from their initial brightness to black
3. **Given** multiple streaks are visible, **When** observing, **Then** some streaks appear closer (larger) and others farther away (smaller)
4. **Given** the animation is running, **When** observing the leading character of any streak, **Then** it appears white while trailing characters appear green
5. **Given** a streak is falling, **When** the leading character advances to the next position, **Then** the previous leading character turns green
6. **Given** characters are visible, **When** looking at any character, **Then** it displays a faint glow proportional to its current brightness
7. **Given** the animation is running continuously, **When** observing over 30+ seconds, **Then** the view gradually zooms in and characters appear to grow larger

---

### User Story 2 - Control Rain Density (Priority: P2)

A user wants to customize the visual intensity by adjusting how many character streaks are falling at once. They can press the plus/equals key to make the effect more dense and dramatic, or press minus to make it sparser and more subtle.

**Why this priority**: User control over density enhances the experience and allows customization for different preferences or performance needs. This builds on the core animation (P1) by adding interactivity.

**Independent Test**: Can be tested independently by launching the app and pressing +/- keys to observe changes in the number of visible character streaks.

**Acceptance Scenarios**:

1. **Given** the application is running with default density, **When** user presses + or = key, **Then** new character streaks appear more frequently
2. **Given** the application is running with default density, **When** user presses - key, **Then** new character streaks appear less frequently
3. **Given** density is at maximum, **When** user presses + or = key, **Then** density does not increase further
4. **Given** density is at minimum, **When** user presses - key, **Then** density does not decrease further
5. **Given** user has adjusted density, **When** observing character fade-out, **Then** fade-out rate remains constant at 3 seconds regardless of density setting

---

### User Story 3 - Customize Color Schemes (Priority: P3)

A user wants to personalize the visual aesthetic by choosing from different color schemes beyond the classic green. They can press the C key to cycle through various color options including blue, red, amber, or a continuously cycling rainbow effect, allowing them to match their mood or preference.

**Why this priority**: Color customization enhances personalization and visual variety without affecting core functionality. This builds on the complete animation experience by adding aesthetic options.

**Independent Test**: Can be tested by launching the app and pressing C repeatedly to verify colors change through all schemes in the correct sequence.

**Acceptance Scenarios**:

1. **Given** the application starts with green color scheme, **When** user presses C key, **Then** the color scheme changes to blue and all visible characters update immediately
2. **Given** the application is displaying any static color scheme, **When** user cycles through all schemes, **Then** the sequence is Green → Blue → Red → Amber → ColorCycle → Green
3. **Given** the ColorCycle scheme is active, **When** observing over 30+ seconds, **Then** characters smoothly transition through all color schemes continuously
4. **Given** any color scheme is active, **When** new character streaks spawn, **Then** they use the current active color scheme
5. **Given** the user changes color schemes, **When** characters are fading, **Then** fade progression continues unaffected by color changes

---

### User Story 4 - Toggle Display Mode (Priority: P4)

A user wants to switch between windowed mode for casual viewing while multitasking, and full-screen mode for an immersive experience. They can press Alt-Enter to instantly toggle between these modes.

**Why this priority**: Display mode flexibility enhances usability but the core effect works fine in either mode. This is a quality-of-life feature that builds on the complete animation experience. Lower priority than color customization as it's purely a viewing mode preference.

**Independent Test**: Can be tested by launching the app and pressing Alt-Enter to verify the window switches between windowed and full-screen modes without disrupting the animation.

**Acceptance Scenarios**:

1. **Given** the application starts in fullscreen mode, **When** user presses Alt-Enter, **Then** the application switches to windowed mode and the animation continues uninterrupted
2. **Given** the application is in windowed mode, **When** user presses Alt-Enter, **Then** the application switches to fullscreen mode and the animation continues uninterrupted
3. **Given** the application switches display modes, **When** the transition completes, **Then** all character streaks maintain their positions and fade progression
4. **Given** the application is in either mode, **When** toggling multiple times rapidly, **Then** the application remains stable and responsive

---

### User Story 5 - Monitor Performance Statistics (Priority: P5)

A user wants to understand how well the application is performing and how dense the rain effect currently is. They can press the S key to toggle an on-screen statistics overlay showing FPS, density percentage, and active streak counts.

**Why this priority**: Performance monitoring is useful for debugging and optimization but not essential for the core experience. This is primarily a diagnostic feature for power users.

**Independent Test**: Can be tested by launching the app and pressing S to verify statistics overlay appears and updates in real-time.

**Acceptance Scenarios**:

1. **Given** the application is running with statistics hidden, **When** user presses S key, **Then** the statistics overlay appears showing FPS, density %, streak count, and head count
2. **Given** the statistics overlay is visible, **When** user presses S key again, **Then** the statistics overlay is hidden
3. **Given** the statistics overlay is visible, **When** user changes density with +/- keys, **Then** the displayed density percentage and streak count update accordingly
4. **Given** the statistics overlay is visible, **When** observing during animation, **Then** the FPS counter updates continuously showing current frame rate
5. **Given** the statistics overlay is visible, **When** user changes color scheme or display mode, **Then** the statistics overlay remains visible and functional

---

### Edge Cases

- What happens when the window is resized in windowed mode? (Characters should adapt to new viewport dimensions without disrupting the animation flow)
- What happens when the system is under heavy load? (Frame rate may drop but animation should remain stable; consider frame-time-independent animation)
- What happens if the user holds down + or - key? (Density should change smoothly but respect minimum/maximum bounds)
- What happens when transitioning to/from full-screen on multi-monitor setups? (Should handle gracefully, ideally remembering windowed position)
- What happens to very old character streaks that have fully faded to black? (Should be cleaned up to prevent memory/performance issues)
- What happens to the zoom level over extended runtime (hours)? (Zoom should continue indefinitely but may need bounds to prevent extreme scaling issues)

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST render falling character streaks continuously at smooth frame rates
- **FR-002**: System MUST use a character set containing exactly 133 characters:
  - 71 half-width katakana characters (Unicode U+FF66 to U+FF9F): 46 basic katakana (ｱ-ﾝ), 20 dakuten forms (ｶﾞ-ﾎﾞ), 5 handakuten forms (ﾊﾟ-ﾎﾟ)
  - 52 Latin letters: uppercase A-Z (U+0041 to U+005A), lowercase a-z (U+0061 to U+007A)
  - 10 numerals: 0-9 (U+0030 to U+0039)
- **FR-003**: System MUST render all characters mirrored/flipped left-to-right
- **FR-004**: System MUST support multiple color schemes for rendering characters:
  - **Green** (default): RGB(0, 255, 100) - Classic bright Matrix green reminiscent of vintage phosphor CRT monitors
  - **Blue**: RGB(0, 100, 255) - Cool blue variant
  - **Red**: RGB(255, 50, 50) - Danger red variant
  - **Amber**: RGB(255, 191, 0) - Warm amber variant
  - **ColorCycle**: Continuously cycles through all color schemes smoothly based on elapsed time
- **FR-005**: System MUST render the head character (leading/frontmost character) of each streak in white color, not green
- **FR-006**: System MUST add a new white head character when the streak descends one additional character position, and simultaneously change the previous head character from white to green. The streak grows by adding new characters at the head while maintaining existing characters in their positions. Streaks advance at base interval of 0.3 seconds per character cell, with variation applied per streak based on depth.
- **FR-007**: System MUST fade each character from its initial brightness to black over exactly 3 seconds using linear interpolation. Fade begins when a character position is beyond the streak's maximum length from the head. Brightness calculation: `brightness = 1.0 - (fadeTimer / 3.0)` where fadeTimer accumulates delta time in seconds. Characters are removed when brightness reaches 0.0.
- **FR-008**: System MUST randomly select the leading character for new positions from the character set
- **FR-009**: System MUST keep trailing characters mostly unchanged, with a 5% probability per frame of any visible (brightness > 0.0) character switching to a different randomly-selected character from the character set. Mutation check performed during CharacterStreak::Update for each character. Mutation does not reset fade progression.
- **FR-010**: System MUST render character streaks at varying depths to create a 3D perspective effect. Each streak has randomized falling speed, with depth affecting both visual size and velocity.
- **FR-011**: System MUST continuously zoom the camera/view inward at a constant rate of 2.0 units/second. Zoom position wraps using modulo operation at Z=100.0 boundary to prevent numerical overflow during extended runtime (hours). Wrapping is visually imperceptible due to continuous depth distribution of streaks.
- **FR-012**: System MUST render a subtle glow around each character proportional to the character's current brightness level. Glow implemented using Gaussian blur with radius 4-8 pixels scaled by brightness.
- **FR-013**: Users MUST be able to increase rain density by pressing the + key or = key
- **FR-014**: Users MUST be able to decrease rain density by pressing the - key
- **FR-015**: System MUST enforce a minimum number of character streaks (density floor). Minimum: 10 active streaks at density level 1.
- **FR-016**: System MUST enforce a maximum number of character streaks (density ceiling). Maximum: 500 active streaks at density level 10.
- **FR-017**: System MUST adjust density by changing the spawn frequency of new streaks, not by altering fade-out rates. Density levels 1-10 map to target active streak counts using formula: `targetStreaks = 10 + (level - 1) * 54` (produces range 10 to 496, rounded to 10-500). Spawn frequency increases when current count < target.
- **FR-018**: Users MUST be able to toggle between windowed and full-screen display modes by pressing Alt-Enter
- **FR-019**: System MUST start in fullscreen mode by default
- **FR-020**: System MUST preserve animation state (character positions, brightness levels, density setting) when switching display modes
- **FR-021**: Users MUST be able to reposition the window in windowed mode by clicking and dragging anywhere within the window client area (borderless window drag)
- **FR-022**: System MUST center the window horizontally and vertically on the primary monitor when the application is initially created and shown
- **FR-023**: Users MUST be able to close the application by pressing the ESC key
- **FR-024**: Users MUST be able to pause the animation by pressing the Space key. Pressing Space again MUST resume the animation. Pausing freezes all character movement, fade progression, and zoom but maintains current visual state.
- **FR-025**: Users MUST be able to cycle through color schemes by pressing the C key. Cycling order: Green → Blue → Red → Amber → ColorCycle → Green. Color changes apply immediately to all visible characters.
- **FR-026**: Users MUST be able to toggle statistics display by pressing the S key. Statistics overlay shows: current FPS, density percentage (0-100%), active streak count, and active head character count. Statistics are hidden by default.
- **FR-027**: Users MUST be able to toggle debug fade times display by pressing the backtick/tilde key (`~). Debug overlay shows fade progression timing information for character instances. Debug display is hidden by default.

### Assumptions

- Default starting density will be set to a moderate level (density level 5, approximately 262 target streaks)
- Default color scheme is Green (classic Matrix phosphor green)
- Application starts in fullscreen mode by default
- Statistics overlay is hidden by default
- Debug fade times display is hidden by default
- Character size will scale based on window resolution to maintain readability
- Zoom speed: 2.0 units/second (50 second cycle at Z=100 wrap, noticeable growth over 60 seconds)
- The 5% character mutation chance applies per frame, assuming ~60fps (approximately 3 mutations per second per visible character)
- When toggling to windowed mode, window defaults to size of 1280x720, centered on primary monitor
- Streak length randomized between 10 and 30 characters per streak
- Pausing freezes all animations but maintains current rendering (not a black screen)
- ColorCycle mode cycles through all color schemes with a smooth transition period

### Key Entities

- **Character Streak**: Represents a column of falling characters, including position, depth, velocity, and a list of individual characters with their states
- **Character Instance**: Represents a single character in a streak, with properties for: character glyph, position, brightness level, time since appearance, color (white/green)
- **Viewport**: Represents the rendering surface dimensions and camera zoom level
- **Density Controller**: Tracks current density level, minimum/maximum bounds, and spawn frequency timing
- **Color Scheme**: Defines color palette for character rendering, supporting static colors and animated color cycling
- **FPS Counter**: Tracks and calculates frame rate using rolling average over recent frame times
- **Application State**: Manages global application settings including display mode, color scheme, and debug/statistics display flags

### Key Bindings

| Key | Action | Functional Requirement |
|-----|--------|------------------------|
| **ESC** | Close application | FR-023 |
| **Space** | Toggle pause/resume animation | FR-024 |
| **+** or **=** | Increase rain density | FR-013 |
| **-** | Decrease rain density | FR-014 |
| **Alt+Enter** | Toggle windowed/fullscreen mode | FR-018 |
| **C** | Cycle through color schemes | FR-025 |
| **S** | Toggle statistics display (FPS, density, streak count) | FR-026 |
| **`** (backtick) | Toggle debug fade times display | FR-027 |

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Application maintains 60 frames per second during animation on reference hardware (Windows 11 PC with mid-range GPU)
- **SC-002**: Character fade-out completes in exactly 3 seconds (±50ms tolerance)
- **SC-003**: Users can observe the zoom effect causing visible size increase of character streaks over 60 seconds of continuous viewing
- **SC-004**: Display mode toggle (Alt-Enter) completes within 200 milliseconds without visible animation disruption
- **SC-005**: Density changes (+ or - key press) result in visible effect within 1 second
- **SC-006**: Application runs continuously for at least 1 hour without performance degradation or memory leaks
- **SC-007**: Character glyphs render clearly and are recognizable at default window size
- **SC-008**: Depth effect is perceivable with some character streaks appearing at least 2x larger than others
- **SC-009**: Application launches to visible animation within 1 second on reference hardware
- **SC-010**: Color scheme changes (C key press) apply immediately to all visible characters without visual artifacts or performance impact
- **SC-011**: Statistics overlay (when enabled) updates in real-time with FPS counter showing values within ±5 FPS of actual frame rate
- **SC-012**: ColorCycle mode completes a full transition through all color schemes within 60 seconds (±5 seconds tolerance)
