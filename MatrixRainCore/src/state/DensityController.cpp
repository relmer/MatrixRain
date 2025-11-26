#include "pch.h"

#include "MatrixRain/DensityController.h"
#include "MatrixRain/Viewport.h"

namespace MatrixRain
{




    DensityController::DensityController(const Viewport& viewport, float characterWidth)
        : m_percentage(DEFAULT_PERCENTAGE)
        , m_viewport(viewport)
        , m_characterWidth(characterWidth)
    {
    }




    void DensityController::IncreaseLevel()
    {
        m_percentage = std::min(m_percentage + PERCENTAGE_STEP, MAX_PERCENTAGE);
    }

    void DensityController::DecreaseLevel()
    {
        m_percentage = std::max(m_percentage - PERCENTAGE_STEP, MIN_PERCENTAGE);
    }

    int DensityController::GetMaxPossibleStreaks() const
    {
        // Max streaks = viewport width / (character horizontal spacing / 2)
        // At 100%, streaks fill the screen with double density (overlapping allowed)
        float viewportWidth = m_viewport.GetWidth();
        int maxStreaks = static_cast<int>(viewportWidth / m_characterWidth * 8.0f);
        
        // Ensure at least minimum
        return std::max(maxStreaks, MIN_STREAKS);
    }




    int DensityController::GetTargetStreakCount() const
    {
        // Calculate max possible streaks based on viewport width
        int maxPossible = GetMaxPossibleStreaks();

        // Apply percentage: targetStreaks = maxPossible * (percentage / 100)
        // But always show at least MIN_STREAKS even at 0%
        int target = static_cast<int>((maxPossible * m_percentage) / 100.0f);
        return std::max(target, MIN_STREAKS);
    }

    bool DensityController::ShouldSpawnStreak(int currentActiveCount) const
    {
        int target = GetTargetStreakCount();
        
        // Always spawn if below target
        if (currentActiveCount < target)
        {
            return true;
        }
        
        // Never spawn if at or above target
        return false;
    }
}
