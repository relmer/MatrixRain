#pragma once

namespace MatrixRain
{
    /// <summary>
    /// Controls the density of falling character streaks via a 10-level system.
    /// Implements formula: targetStreaks = 10 + (level-1)*54
    /// Level 1 (min): 10 streaks
    /// Level 5 (default): 226 streaks
    /// Level 10 (max): 496 streaks
    /// </summary>
    class DensityController
    {
    public:
        DensityController();

        /// <summary>
        /// Initialize with default level 5 (226 target streaks).
        /// </summary>
        void Initialize();

        /// <summary>
        /// Increase density level (max 10).
        /// </summary>
        void IncreaseLevel();

        /// <summary>
        /// Decrease density level (min 1).
        /// </summary>
        void DecreaseLevel();

        /// <summary>
        /// Get target number of streaks for current level.
        /// Formula: targetStreaks = 10 + (level-1)*54
        /// </summary>
        /// <returns>Target streak count for current level</returns>
        int GetTargetStreakCount() const;

        /// <summary>
        /// Determine if a new streak should spawn based on current active count.
        /// Returns true more often when below target, false when at/above target.
        /// </summary>
        /// <param name="currentActiveCount">Current number of active streaks</param>
        /// <returns>True if should spawn, false otherwise</returns>
        bool ShouldSpawnStreak(int currentActiveCount) const;

        // Accessors
        int GetLevel() const { return m_level; }

    private:
        int m_level;                    // Current density level (1-10)
        
        static constexpr int MIN_LEVEL = 1;
        static constexpr int MAX_LEVEL = 10;
        static constexpr int DEFAULT_LEVEL = 5;
        static constexpr int BASE_STREAK_COUNT = 10;
        static constexpr int STREAK_INCREMENT_PER_LEVEL = 54;
    };
}
