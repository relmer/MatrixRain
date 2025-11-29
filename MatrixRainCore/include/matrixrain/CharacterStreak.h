#pragma once





#include "MatrixRain/Math.h"
#include "MatrixRain/CharacterInstance.h"





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
    void Spawn (const Vector3 & position);

    /// <summary>
    /// Update the streak's position and character states.
    /// </summary>
    /// <param name="deltaTime">Time elapsed since last frame in seconds</param>
    /// <param name="viewportHeight">Height of the viewport in pixels</param>
    void Update (float deltaTime, float viewportHeight);

    /// <summary>
    /// Check if the streak should be removed from the scene.
    /// </summary>
    /// <param name="viewportHeight">Height of the viewport in pixels</param>
    /// <returns>True if the streak has fallen completely off screen</returns>
    bool ShouldDespawn() const;

    /// <summary>
    /// Check if the streak's head has left the viewport.
    /// Used to determine when a new streak can spawn to replace this one.
    /// </summary>
    /// <param name="viewportHeight">Height of the viewport in pixels</param>
    /// <returns>True if the head has passed the bottom of the viewport</returns>
    bool IsHeadOffscreen (float viewportHeight) const;

    /// <summary>
    /// Rescale all character positions when viewport size changes.
    /// </summary>
    /// <param name="scaleX">X scale ratio (newWidth / oldWidth)</param>
    /// <param name="scaleY">Y scale ratio (newHeight / oldHeight)</param>
    void RescalePositions (float scaleX, float scaleY);

    // Accessors
    const Vector3                        & GetPosition()       const { return m_position;          }
    const Vector3                        & GetVelocity()       const { return m_velocity;          }
    size_t                                 GetLength()         const { return m_characters.size(); }
    size_t                                 GetCharacterCount() const { return m_characters.size(); }
    const std::vector<CharacterInstance> & GetCharacters()     const { return m_characters;        }
    uint64_t                               GetID()             const { return m_id;                }

    void SetPosition (const Vector3 & position) { m_position = position; }

private:
    Vector3                        m_position         {};       // Head position of the streak (in cells)
    Vector3                        m_velocity         {};       // Velocity in pixels/second (only for drift)
    std::vector<CharacterInstance> m_characters;                // Characters in the streak
    float                          m_mutationTimer    { 0.0f }; // Timer for character mutation
    float                          m_dropTimer        { 0.0f }; // Timer for discrete cell dropping
    float                          m_dropInterval     { 0.3f }; // Cached drop interval (set at spawn, constant for streak lifetime)
    float                          m_characterSpacing { 32.0f };// Vertical spacing between characters
    size_t                         m_maxLength        { 0 };    // Maximum number of characters in this streak
    bool                           m_isInFadingPhase  { false };// True when head has reached bottom and final fade has started
    uint64_t                       m_id               { 0 };    // Unique ID for debug tracking

    // Random number generator (shared across all streaks for performance)
    static inline std::random_device s_randomDevice;
    static inline std::mt19937       s_generator { s_randomDevice() };

    // Map depth (Z: 0-100) to velocity scale (1.0 - 6.0)
    // Far streaks (Z=100) move 6x faster than near streaks (Z=0)
    static float GetVelocityScale (float depth);

    static constexpr size_t MIN_LENGTH           = 15;     // Longer minimum for better visibility
    static constexpr size_t MAX_LENGTH           = 30;
    static constexpr float  MUTATION_PROBABILITY = 0.40f;  // 40% per character per second (authentic Matrix feel)
    static constexpr float  BASE_VELOCITY        = 100.0f; // Base pixels per second
    static constexpr float  BASE_DROP_INTERVAL   = 0.3f;   // Time between drops (in seconds)
};



