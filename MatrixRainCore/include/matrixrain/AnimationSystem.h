#pragma once
#include "matrixrain/CharacterStreak.h"
#include "matrixrain/Viewport.h"
#include <vector>
#include <memory>

namespace MatrixRain
{
    /// <summary>
    /// Manages all animated character streaks and camera zoom effects.
    /// Handles spawning, updating, and despawning of streaks based on viewport bounds.
    /// </summary>
    class AnimationSystem
    {
    public:
        AnimationSystem();

        /// <summary>
        /// Initialize the animation system with viewport reference.
        /// </summary>
        /// <param name="viewport">Reference to the viewport for bounds checking</param>
        void Initialize(const Viewport& viewport);

        /// <summary>
        /// Update all active streaks and camera zoom.
        /// </summary>
        /// <param name="deltaTime">Time elapsed since last frame in seconds</param>
        void Update(float deltaTime);

        /// <summary>
        /// Spawn a new streak at a random position.
        /// X: random across viewport width
        /// Y: random above viewport (negative Y)
        /// Z: random depth between 0 and 100
        /// </summary>
        void SpawnStreak();

        /// <summary>
        /// Remove streaks that have moved completely off screen.
        /// </summary>
        void DespawnOffscreenStreaks();

        /// <summary>
        /// Apply continuous zoom effect to all streaks.
        /// Moves camera forward (decreases Z), wraps at Z=0 boundary.
        /// </summary>
        /// <param name="deltaTime">Time elapsed since last frame in seconds</param>
        void ApplyZoom(float deltaTime);

        // Accessors
        const std::vector<CharacterStreak>& GetStreaks() const { return m_streaks; }
        size_t GetActiveStreakCount() const { return m_streaks.size(); }
        float GetZoomVelocity() const { return m_zoomVelocity; }
        void SetZoomVelocity(float velocity) { m_zoomVelocity = velocity; }

    private:
        std::vector<CharacterStreak> m_streaks;  // All active character streaks
        const Viewport* m_viewport;               // Reference to viewport for bounds
        float m_zoomVelocity;                     // Camera zoom speed (units per second)
        float m_spawnTimer;                       // Timer for automatic spawning
        float m_spawnInterval;                    // Time between automatic spawns

        static constexpr float DEFAULT_ZOOM_VELOCITY = 5.0f;  // Units per second
        static constexpr float MAX_DEPTH = 100.0f;            // Far plane
        static constexpr float SPAWN_INTERVAL = 0.1f;         // Spawn every 100ms
    };
}
