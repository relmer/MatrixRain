# Data Model: Matrix Rain

## Core Entities

### CharacterStreak

Represents a single column of falling characters.

**Properties**:
- `position: Vector3` - World-space position (X: horizontal screen position, Y: vertical scroll offset, Z: depth 0-100)
- `velocity: float` - Downward scroll speed in units/second (range: 50-200, varies by depth for parallax)
- `length: uint32_t` - Number of characters in streak (range: 5-30)
- `characters: std::vector<CharacterInstance>` - Ordered list of character instances (index 0 = leading white char)
- `spawnTime: float` - Time since streak creation (seconds, for fade timing)
- `fadeTimer: float` - Current fade progress for trailing characters (0-3 seconds)
- `mutationTimer: float` - Time until next character mutation (randomized 0-1 seconds)

**Invariants**:
- `length == characters.size()`
- `characters[0].color` is always white (leading character)
- `position.z` in range [0, 100]
- `velocity > 0` (always falling downward)

**Lifecycle**:
- **Spawn**: Random X position, Z depth, velocity scaled by depth, random length
- **Update**: Increment Y by velocity × deltaTime, decrement timers, mutate characters at 5% probability
- **Fade**: After 3 seconds, gradually reduce trailing character opacity (linear 1.0 → 0.0 over 3s)
- **Despawn**: When streak fully off-screen (Y > viewport height + streak length)

---

### CharacterInstance

Represents a single character within a streak.

**Properties**:
- `glyphIndex: uint16_t` - Index into character atlas (0-265, where 0-132 normal, 133-265 mirrored)
- `color: Color4` - RGBA color (white for leading char, green gradient for trailing)
- `brightness: float` - Opacity multiplier (1.0 at spawn, fades to 0.0 over 3 seconds for trailing chars)
- `scale: float` - Render scale multiplier (derived from streak depth: 0.5 at Z=100, 1.0 at Z=0)
- `positionOffset: Vector2` - Local offset from streak position (stacked vertically, spacing = glyph height)

**Invariants**:
- `glyphIndex < 266`
- `brightness` in range [0.0, 1.0]
- `scale` in range [0.5, 1.0]
- Leading character: `color = {1.0, 1.0, 1.0, 1.0}`, `brightness = 1.0`
- Trailing characters: `color = {0.0, 1.0, 0.0, 1.0}` (green), `brightness` decreases with fade

**Mutation Behavior**:
- 5% chance per character per second to re-roll `glyphIndex` (random selection from 133 glyphs, re-apply mirroring)

---

### Viewport

Represents the rendering camera and screen dimensions.

**Properties**:
- `width: uint32_t` - Screen width in pixels
- `height: uint32_t` - Screen height in pixels
- `zoomLevel: float` - Current zoom depth (unused in MVP, reserved for future camera controls)
- `projection: Matrix4x4` - Orthographic projection matrix (maps world coords to NDC)

**Invariants**:
- `width > 0 && height > 0`
- Projection matrix maps Z=0 (near) to front of screen, Z=100 (far) to back

**Update Triggers**:
- Window resize events (WM_SIZE message)
- Display mode toggle (Alt+Enter fullscreen)

---

### DensityController

Manages streak spawn rate and active streak count.

**Properties**:
- `level: int32_t` - Current density level (range: 1-10, default 5)
- `minStreaks: uint32_t` - Minimum active streaks for current level (level × 10)
- `maxStreaks: uint32_t` - Maximum active streaks for current level (level × 50)
- `spawnInterval: float` - Seconds between spawn checks (0.1s)
- `timeSinceLastSpawn: float` - Accumulator for spawn timing

**Invariants**:
- `level` in range [1, 10]
- `minStreaks = level × 10` (e.g., level 5 → 50 streaks minimum)
- `maxStreaks = level × 50` (e.g., level 5 → 250 streaks maximum)

**Spawn Logic**:
```
Every spawnInterval (0.1s):
  if activeStreakCount < maxStreaks:
    spawnProbability = (maxStreaks - activeStreakCount) / (maxStreaks - minStreaks)
    if random(0, 1) < spawnProbability:
      SpawnNewStreak()
```

**Level Adjustment**:
- `+` key: `level = min(level + 1, 10)`
- `-` key: `level = max(level - 1, 1)`

---

### CharacterSet

Manages the 133 character glyphs and their atlas metadata.

**Properties**:
- `glyphs: std::array<GlyphInfo, 266>` - Metadata for all 266 atlas entries (133 normal + 133 mirrored)
- `atlasTexture: ID3D11Texture2D*` - 2048x2048 RGBA texture with pre-rendered glyphs
- `atlasSRV: ID3D11ShaderResourceView*` - Shader resource view for texture binding

**GlyphInfo Structure**:
```cpp
struct GlyphInfo {
    Vector2 uvMin;       // Top-left UV coordinate (0-1 range)
    Vector2 uvMax;       // Bottom-right UV coordinate (0-1 range)
    uint32_t codepoint;  // Unicode codepoint (e.g., U+30A2 for katakana ア)
    bool mirrored;       // True if this is the mirrored version
};
```

**Character Groups** (indices):
- Katakana: 0-70 (normal), 71-141 (mirrored)
- Latin uppercase: 142-193 (normal), 194-245 (mirrored)
- Numerals: 246-255 (normal), 256-265 (mirrored)

**Random Selection API**:
```cpp
uint16_t GetRandomGlyph() const {
    return distribution(generator); // Random index 0-265
}
```

---

### ApplicationState

Top-level state container for the entire application.

**Properties**:
- `displayMode: enum { Windowed, Fullscreen }` - Current display mode
- `isPaused: bool` - Animation paused state (future use, not in MVP)
- `densityController: DensityController` - Density management
- `viewport: Viewport` - Rendering viewport
- `elapsedTime: float` - Total application runtime (seconds since startup)

**State Transitions**:
- Alt+Enter: Toggle `displayMode` between Windowed ↔ Fullscreen
- Window creation: Initialize in Windowed mode (800×600 default)

---

## Data Relationships

```
ApplicationState (1)
├── DensityController (1)
├── Viewport (1)
└── AnimationSystem (1)
    └── CharacterStreak (10-500)
        └── CharacterInstance (5-30 per streak)

CharacterSet (singleton)
└── GlyphInfo (266)
    └── ID3D11Texture2D (1 atlas texture)
```

**Ownership**:
- `ApplicationState` owns `DensityController` and `Viewport` (composition)
- `AnimationSystem` owns `std::vector<CharacterStreak>` (container)
- `CharacterStreak` owns `std::vector<CharacterInstance>` (container)
- `CharacterSet` is a singleton (shared read-only access)

**Lifetime**:
- `ApplicationState`: Created at startup, destroyed at shutdown
- `CharacterStreak`: Dynamically spawned/despawned during animation
- `CharacterInstance`: Lifetime tied to parent streak
- `CharacterSet`: Initialized once at startup (texture atlas creation), persistent until shutdown

---

## Performance Considerations

**Memory Budget** (per entity):
- `CharacterStreak`: ~128 bytes (includes vector overhead)
- `CharacterInstance`: ~32 bytes
- Worst-case (500 streaks × 30 chars): ~500 KB (negligible)

**Cache Efficiency**:
- `std::vector<CharacterStreak>` provides contiguous storage for iteration
- Depth sorting: Single `std::stable_sort` on 500 elements = ~0.05ms

**GPU Data**:
- Instance buffer: Packed array of per-instance attributes (position, UV, color, scale)
- Updated every frame via `ID3D11DeviceContext::UpdateSubresource` (dynamic usage)
- Buffer size: 500 streaks × 30 chars × 64 bytes/instance = ~960 KB (within D3D11 limits)

---

## Validation Rules

**Density Constraints**:
- At level 1: 10-50 streaks active
- At level 10: 100-500 streaks active
- Spawn rate self-regulates to maintain target range

**Fade Timing**:
- Trailing characters: Fade starts immediately at spawn
- Leading character: Never fades (always brightness = 1.0)
- Fade duration: Exactly 3 seconds (±50ms per success criteria SC-003)

**Viewport Bounds**:
- Streaks despawn when `position.y > viewport.height + streak.length × glyphHeight`
- Prevents memory leak from infinite streak accumulation

**Character Mutations**:
- Mutation rate: 5% probability per character per second
- Re-rolled glyph maintains same mirroring state as original

---

## Extensibility Hooks (Future)

**Pause State**:
- `ApplicationState.isPaused`: Skips animation updates, continues rendering last frame
- Input: Spacebar to toggle (not in MVP)

**Camera Zoom**:
- `Viewport.zoomLevel`: Scales projection matrix FOV
- Input: Mouse wheel to adjust (not in MVP)

**Color Schemes**:
- Replace hardcoded green with `CharacterInstance.colorScheme: enum { GreenPhosphor, Amber, Cyan }`
- Requires shader uniform update (not in MVP)

**Trail Effects**:
- `CharacterInstance.trailIntensity`: Controls motion blur strength
- Post-processing effect (not in MVP)
