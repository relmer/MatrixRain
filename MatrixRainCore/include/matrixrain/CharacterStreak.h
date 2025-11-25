#pragma once
#include "MatrixRain/Math.h"
#include "MatrixRain/CharacterInstance.h"

namespace MatrixRain
{
    /// <summary>
    /// Represents a vertical streak of Matrix characters
    /// falling through 3D space with velocity-based animation.
    /// </summary>
    class CharacterStreak
    {
    public:
        CharacterStreak() = default;

        /// <summary>
        /// Initialize the streak at a given position with random length and velocity.
        /// </summary>
        /// <param name="position">Starting position in 3D space (x, y, z)</param>
        void Spawn(const Vector3& position);

        /// <summary>
        /// Update the streak's position and character states.
        /// </summary>
        /// <param name="deltaTime">Time elapsed since last frame in seconds</param>
        /// <param name="viewportHeight">Height of the viewport in pixels</param>
        void Update(float deltaTime, float viewportHeight);

        /// <summary>
        /// Check if the streak should be removed from the scene.
        /// </summary>
        /// <param name="viewportHeight">Height of the viewport in pixels</param>
        /// <returns>True if the streak has fallen completely off screen</returns>
        bool ShouldDespawn(float viewportHeight) const;

        // Accessors
        const Vector3& GetPosition() const { return m_position; }
        void SetPosition(const Vector3& position) { m_position = position; }
        const Vector3& GetVelocity() const { return m_velocity; }
        size_t GetLength() const { return m_characters.size(); }
        const std::vector<CharacterInstance>& GetCharacters() const { return m_characters; }

    private:
        Vector3 m_position{};               // Head position of the streak (in cells)
        Vector3 m_velocity{};               // Velocity in pixels/second (only for drift)
        std::vector<CharacterInstance> m_characters; // Characters in the streak
        float m_mutationTimer{ 0.0f };      // Timer for character mutation
        float m_dropTimer{ 0.0f };          // Timer for discrete cell dropping
        float m_dropInterval{ 0.3f };       // Cached drop interval (set at spawn, constant for streak lifetime)
        float m_characterSpacing{ 32.0f };  // Vertical spacing between characters
        size_t m_maxLength{ 0 };            // Maximum number of characters in this streak
        bool m_isInFadingPhase{ false };    // True when head has reached bottom and final fade has started

        static constexpr size_t MIN_LENGTH = 20; // Longer minimum for better visibility
        static constexpr size_t MAX_LENGTH = 30;
        static constexpr float MUTATION_PROBABILITY = 0.40f; // 40% per character per second (authentic Matrix feel)
        static constexpr float BASE_VELOCITY = 100.0f; // Base pixels per second
        static constexpr float BASE_DROP_INTERVAL = 0.3f; // Time between drops (in seconds)
    };
}
