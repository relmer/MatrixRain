# Feature Specification: Matrix Rain Visual Effect

**Feature Branch**: `001-matrix-rain`  
**Created**: 2025-11-05  
**Status**: Draft  
**Input**: User description: "I'm building an app to show 'Matrix rain' in the style of The Matrix. The app should use DirectX for the best performance and should allow running both windowed and full-screen. The rain characters should be comprised of the full set of 71 half-width katakana characters (basic, dakuten, handakuten) and western Latin letters and numerals. They should be rendered backwards, flipped left-to-right, using a green color reminiscent of old green phosphor screens. The leading character of a drop of rain should render in white but turn to green as soon as the drop descends to the next character. The characters should fade to black over 3 seconds starting as soon as they first appear. The leading character of the drop should be chosen randomly from the set above, but as the drop advances down the screen, the characters it leaves behind should mostly remain unchanged. There should be a 5% chance of any still-visible (not fully faded to black) character switching to another character, but this does not alter the fade-out progression. The scene should be rendered with depth--some drops should be far away and others nearer to the camera. The scene should zoom in very slowly over time, never stopping. This will cause streaks of raining characters to appear to get bigger as the zoom increases. The characters should have a faint surrounding glow around them proportional to each of their current brightness levels. The user should be able to increase or decrease the density (number of drops/streaks) of rain by pressing + (or equals) to increase and - to decrease. There should be a minimum and maximum number of streaks visible at a time. The density is controlled by altering the frequency of new streaks being added, but their fade out rate should remain constant. The user should be able to toggle between windowed and full-screen by pressing Alt-Enter."

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
- **FR-002**: System MUST use a character set containing the full set of 71 half-width katakana characters (basic katakana, dakuten forms, and handakuten forms), Latin letters (A-Z, a-z), and numerals (0-9)
- **FR-003**: System MUST render all characters mirrored/flipped left-to-right
- **FR-004**: System MUST render characters in a green color reminiscent of vintage phosphor CRT monitors
- **FR-005**: System MUST render the leading character of each streak in white color
- **FR-006**: System MUST change the leading character to green when the streak advances to the next position
- **FR-007**: System MUST fade each character from its initial brightness to black over exactly 3 seconds
- **FR-008**: System MUST randomly select the leading character for new positions from the character set
- **FR-009**: System MUST keep trailing characters mostly unchanged, with a 5% probability per frame of any visible character switching to a different character
- **FR-010**: System MUST render character streaks at varying depths to create a 3D perspective effect
- **FR-011**: System MUST continuously zoom the camera/view inward at a slow, constant rate
- **FR-012**: System MUST render a subtle glow around each character proportional to the character's current brightness level
- **FR-013**: Users MUST be able to increase rain density by pressing the + key or = key
- **FR-014**: Users MUST be able to decrease rain density by pressing the - key
- **FR-015**: System MUST enforce a minimum number of character streaks (density floor)
- **FR-016**: System MUST enforce a maximum number of character streaks (density ceiling)
- **FR-017**: System MUST adjust density by changing the spawn frequency of new streaks, not by altering fade-out rates
- **FR-018**: Users MUST be able to toggle between windowed and full-screen display modes by pressing Alt-Enter
- **FR-019**: System MUST start in windowed mode by default
- **FR-020**: System MUST preserve animation state (character positions, brightness levels, density setting) when switching display modes

### Assumptions

- Default starting density will be set to a moderate level (approximately 50% between minimum and maximum)
- Minimum streak count will be at least 10 to ensure the effect is visible
- Maximum streak count will be constrained by performance testing (target 60fps on reference hardware)
- Character size will scale based on window resolution to maintain readability
- Zoom speed will be calibrated to be barely perceptible but create noticeable growth over 1-2 minutes
- The 5% character mutation chance applies per frame, assuming ~60fps (approximately 3 mutations per second per visible character)
- Character set includes all 71 half-width katakana characters: 46 basic (ｱ-ﾝ), 20 dakuten (ｶﾞ-ﾎﾞ), and 5 handakuten (ﾊﾟ-ﾎﾟ) forms, plus Latin letters and numerals
- Window starts at a reasonable default size (e.g., 1280x720) in windowed mode

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
