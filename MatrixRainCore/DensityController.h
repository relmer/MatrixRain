#pragma once





class Viewport;

/// <summary>
/// Controls the density of falling character streaks via percentage-based system (0-100%).
/// Min streaks: 1 (always at least one, even at 0%)
/// Max streaks: viewportWidth / characterWidth / 2 (allows for overlap)
/// Formula: targetStreaks = max(1, (maxPossibleStreaks * percentage / 100))
/// </summary>
class DensityController
{
public:
    DensityController (const Viewport & viewport, float characterWidth);

    /// <summary>
    /// Increase density percentage by step amount (max 100%).
    /// </summary>
    void IncreaseLevel();

    /// <summary>
    /// Decrease density percentage by step amount (min 0%).
    /// </summary>
    void DecreaseLevel();

    /// <summary>
    /// Set density percentage directly (for testing). Clamps to [0, 100].
    /// </summary>
    /// <param name="percentage">Desired percentage (0-100)</param>
    void SetPercentage (int percentage);

    /// <summary>
    /// Get target number of streaks based on current percentage and viewport width.
    /// Formula: max(1, (viewportWidth / characterWidth / 2) * percentage / 100)
    /// </summary>
    /// <returns>Target streak count for current percentage</returns>
    int  GetTargetStreakCount() const;

    /// <summary>
    /// Determine if a new streak should spawn based on current active count.
    /// Returns true more often when below target, false when at/above target.
    /// </summary>
    /// <param name="currentActiveCount">Current number of active streaks</param>
    /// <returns>True if should spawn, false otherwise</returns>
    bool ShouldSpawnStreak (int currentActiveCount) const;

    // Accessors
    int GetPercentage()         const  { return m_percentage; }
    int GetMaxPossibleStreaks() const;

private:
    int              m_percentage      { DEFAULT_PERCENTAGE};    // Current density percentage (0-100)
    const Viewport & m_viewport;                                 // Reference to viewport for width calculations
    float            m_characterWidth;                           // Width of one character in pixels (horizontal spacing)
    
    static constexpr int MIN_PERCENTAGE     = 0;
    static constexpr int MAX_PERCENTAGE     = 100;
    static constexpr int DEFAULT_PERCENTAGE = 80; // Start at 80% density
    static constexpr int PERCENTAGE_STEP    = 5;  // Change by 5% per +/- press
    static constexpr int MIN_STREAKS        = 1;  // Always show at least one streak
};





