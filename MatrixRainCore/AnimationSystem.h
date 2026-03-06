#pragma once





#include "CharacterStreak.h"
#include "Viewport.h"





class DensityController;





/// <summary>
/// Describes the valid coordinate ranges for streak spawning.
/// Passed to the spawn position callback so clients can choose positions
/// without needing internal knowledge of AnimationSystem spawn logic.
/// </summary>
struct SpawnRange
{
    float minX = 0.0f;
    float maxX = 0.0f;
    float minY = 0.0f;
    float maxY = 0.0f;
};





/// <summary>
/// Callback type for overriding streak spawn X position.
/// Receives the valid spawn range; returns an X position to use,
/// or nullopt to let AnimationSystem pick randomly.
/// </summary>
using SpawnPositionCallback = std::function<std::optional<float> (const SpawnRange &)>;





/// <summary>
/// A standalone character rendered through the GPU pipeline but not
/// attached to any CharacterStreak.  Used for effects like horizontal
/// tracer streaks in the help dialog.
/// </summary>
struct OverlayCharacter
{
    CharacterInstance character;       // Glyph, color, brightness, scale
    Vector3           position;        // World position (x, y, z)
};





/// <summary>
/// Manages all animated character streaks and camera zoom effects.
/// Handles spawning, updating, and despawning of streaks based on viewport bounds.
/// </summary>
class AnimationSystem
{
public:
    /// <summary>
    /// Initialize the animation system with viewport and density controller.
    /// </summary>
    /// <param name="viewport">Reference to the viewport for bounds checking</param>
    /// <param name="densityController">Reference to density controller for spawn management</param>
    void Initialize (const Viewport & viewport, DensityController & densityController);

    /// <summary>
    /// Update all active streaks and camera zoom.
    /// </summary>
    /// <param name="deltaTime">Time elapsed since last frame in seconds</param>
    void Update (float deltaTime);

    /// <summary>
    /// Spawn a new streak at a random position.
    /// X: random across viewport width
    /// Y: random above viewport (negative Y)
    /// Z: random depth between 0 and 100
    /// </summary>
    void SpawnStreak();

    /// <summary>
    /// Spawn a streak at a random position within the viewport (for filling on resize).
    /// X: random across viewport width
    /// Y: random within viewport height (0 to height)
    /// Z: random depth between 0 and 100
    /// </summary>
    void SpawnStreakInView();

    /// <summary>
    /// Remove streaks that have moved completely off screen.
    /// </summary>
    void DespawnOffscreenStreaks();

    /// <summary>
    /// Remove excess streaks when density is reduced.
    /// Removes oldest streaks (furthest from camera) first.
    /// </summary>
    /// <param name="targetCount">Target number of streaks to maintain</param>
    /// <returns>Number of streaks actually removed</returns>
    int RemoveExcessStreaks (int targetCount);

    /// <summary>
    /// Apply continuous zoom effect to all streaks.
    /// Moves camera forward (decreases Z), wraps at Z=0 boundary.
    /// </summary>
    /// <param name="deltaTime">Time elapsed since last frame in seconds</param>
    void ApplyZoom (float deltaTime);

    /// <summary>
    /// Rescale streak positions when viewport size changes.
    /// Proportionally adjusts X and Y positions to match new viewport dimensions.
    /// </summary>
    /// <param name="oldWidth">Previous viewport width</param>
    /// <param name="newWidth">New viewport width</param>
    /// <param name="oldHeight">Previous viewport height</param>
    /// <param name="newHeight">New viewport height</param>
    void RescaleStreaksForViewport (float oldWidth, float oldHeight, float newWidth, float newHeight);

    /// <summary>
    /// Clear all active streaks (used when switching display modes).
    /// </summary>
    void ClearAllStreaks();
    
    /// <summary>
    /// Update animation speed for all active streaks.
    /// </summary>
    /// <param name="speedPercent">Animation speed percentage (1-100)</param>
    void SetAnimationSpeed (int speedPercent);

    /// <summary>
    /// Override character spacing to prevent viewport-based scaling.
    /// When set, CalculateCharacterSpacing() returns this value instead
    /// of computing from viewport height.
    /// </summary>
    /// <param name="spacing">Fixed character spacing in pixels</param>
    void SetCharacterSpacingOverride (float spacing);

    /// <summary>
    /// Set a callback that can override the X position of newly spawned streaks.
    /// The callback receives the valid spawn range and returns an X position,
    /// or nullopt to use normal random placement.
    /// Pass nullptr to clear the callback.
    /// </summary>
    /// <param name="callback">Spawn position callback, or nullptr to clear</param>
    void SetSpawnPositionCallback (SpawnPositionCallback callback);

    /// <summary>
    /// Set overlay characters to be rendered alongside normal streaks.
    /// These are standalone characters not attached to any streak,
    /// useful for effects like horizontal tracer animations.
    /// </summary>
    /// <param name="overlays">Vector of overlay characters to render</param>
    void SetOverlayCharacters (std::vector<OverlayCharacter> overlays);

    // Accessors
    const std::vector<CharacterStreak>  & GetStreaks()            const { return m_streaks;            }
    const std::vector<OverlayCharacter> & GetOverlayCharacters()  const { return m_overlayCharacters;  }
    size_t                                GetActiveStreakCount()  const { return m_streaks.size();      }
    size_t                                GetActiveHeadCount()    const;
    float                                GetZoomVelocity()       const { return m_zoomVelocity;   }

    void SetZoomVelocity (float velocity) { m_zoomVelocity = velocity; }

private:
    float CalculateCharacterSpacing() const;

    std::vector<CharacterStreak>   m_streaks;                            // All active character streaks
    const Viewport               * m_viewport              = nullptr;      // Reference to viewport for bounds
    DensityController            * m_densityController     = nullptr;      // Reference to density controller (optional)
    float                          m_zoomVelocity          = DEFAULT_ZOOM_VELOCITY; // Camera zoom speed (units per second)
    float                          m_spawnTimer            = 0.0f;         // Timer for automatic spawning
    float                          m_spawnInterval         = SPAWN_INTERVAL; // Time between automatic spawns
    int                            m_previousTargetCount   = 0;            // Previous frame's target count (to detect density changes)
    int                            m_animationSpeedPercent = 100;        // Current animation speed percentage (1-100)
    std::optional<float>           m_characterSpacingOverride;            // Override for character spacing (bypasses viewport scaling)
    SpawnPositionCallback          m_spawnPositionCallback;               // Optional callback for overriding spawn X position
    std::vector<OverlayCharacter>  m_overlayCharacters;                   // Extra characters rendered alongside streaks
    
    // Random number generation
    std::random_device             m_randomDevice;
    std::mt19937                   m_generator            { m_randomDevice() }; // Seeded from random_device

    // Reusable temporary vectors for RemoveExcessStreaks (avoid per-frame heap allocations)
    std::vector<size_t>            m_activeIndices;
    std::vector<size_t>            m_inactiveIndices;
    std::vector<bool>              m_shouldRemove;

#ifdef _DEBUG
    std::vector<CharacterStreak>   m_previousFrameStreaks;               // Previous frame snapshot for diff detection
    int                            m_intentionalRemovalCount = 0;        // Number of streaks intentionally removed this frame
#endif

    static constexpr float DEFAULT_ZOOM_VELOCITY = 5.0f;   // Units per second
    static constexpr float MAX_DEPTH             = 100.0f; // Far plane
    static constexpr float SPAWN_INTERVAL        = 0.05f;  // Spawn every 50ms (fast response)
};



