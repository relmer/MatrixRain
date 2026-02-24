#pragma once





#include "CharacterStreak.h"
#include "Viewport.h"





class DensityController;

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

    // Accessors
    const std::vector<CharacterStreak> & GetStreaks()            const { return m_streaks;        }
    size_t                               GetActiveStreakCount()  const { return m_streaks.size(); }
    size_t                               GetActiveHeadCount()    const;
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



