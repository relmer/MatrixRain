#include "pch.h"
#include "matrixrain/DensityController.h"
#include <algorithm>

namespace MatrixRain
{
    DensityController::DensityController()
        : m_level(DEFAULT_LEVEL)
    {
    }

    void DensityController::Initialize()
    {
        m_level = DEFAULT_LEVEL;
    }

    void DensityController::IncreaseLevel()
    {
        m_level = std::min(m_level + 1, MAX_LEVEL);
    }

    void DensityController::DecreaseLevel()
    {
        m_level = std::max(m_level - 1, MIN_LEVEL);
    }

    int DensityController::GetTargetStreakCount() const
    {
        // Formula: targetStreaks = 10 + (level-1)*54
        return BASE_STREAK_COUNT + (m_level - 1) * STREAK_INCREMENT_PER_LEVEL;
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
