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

### User Story 3 - Toggle Display Mode (Priority: P3)

A user wants to switch between windowed mode for casual viewing while multitasking, and full-screen mode for an immersive experience. They can press Alt-Enter to instantly toggle between these modes.

**Why this priority**: Display mode flexibility enhances usability but the core effect works fine in either mode. This is a quality-of-life feature that builds on the complete animation experience.

**Independent Test**: Can be tested by launching the app and pressing Alt-Enter to verify the window switches between windowed and full-screen modes without disrupting the animation.

**Acceptance Scenarios**:

1. **Given** the application starts in windowed mode, **When** user presses Alt-Enter, **Then** the application switches to full-screen mode and the animation continues uninterrupted
2. **Given** the application is in full-screen mode, **When** user presses Alt-Enter, **Then** the application switches to windowed mode and the animation continues uninterrupted
3. **Given** the application switches display modes, **When** the transition completes, **Then** all character streaks maintain their positions and fade progression
4. **Given** the application is in either mode, **When** toggling multiple times rapidly, **Then** the application remains stable and responsive

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
- **FR-004**: System MUST render characters in a green color reminiscent of vintage phosphor CRT monitors. Exact color: RGB(0, 255, 100).
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
- **FR-019**: System MUST start in windowed mode by default
- **FR-020**: System MUST preserve animation state (character positions, brightness levels, density setting) when switching display modes
- **FR-021**: Users MUST be able to reposition the window in windowed mode by clicking and dragging anywhere within the window client area (borderless window drag)
- **FR-022**: System MUST center the window horizontally and vertically on the primary monitor when the application is initially created and shown
- **FR-023**: Users MUST be able to close the application by pressing the ESC key
- **FR-024**: Users MUST be able to pause the animation by pressing the Space key. Pressing Space again MUST resume the animation. Pausing freezes all character movement, fade progression, and zoom but maintains current visual state.

### Assumptions

- Default starting density will be set to a moderate level (density level 5, approximately 262 target streaks)
- Character size will scale based on window resolution to maintain readability
- Zoom speed: 2.0 units/second (50 second cycle at Z=100 wrap, noticeable growth over 60 seconds)
- The 5% character mutation chance applies per frame, assuming ~60fps (approximately 3 mutations per second per visible character)
- Window starts at default size of 1280x720 in windowed mode, centered on primary monitor
- Streak length randomized between 10 and 30 characters per streak
- Pausing freezes all animations but maintains current rendering (not a black screen)

### Key Entities

- **Character Streak**: Represents a column of falling characters, including position, depth, velocity, and a list of individual characters with their states
- **Character Instance**: Represents a single character in a streak, with properties for: character glyph, position, brightness level, time since appearance, color (white/green)
- **Viewport**: Represents the rendering surface dimensions and camera zoom level
- **Density Controller**: Tracks current density level, minimum/maximum bounds, and spawn frequency timing

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
