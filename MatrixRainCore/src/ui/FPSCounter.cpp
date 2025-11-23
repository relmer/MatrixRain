#include "pch.h"
#include "matrixrain/FPSCounter.h"

namespace MatrixRain
{
    FPSCounter::FPSCounter()
        : m_currentFPS(0.0f)
        , m_frameTimeAccumulator(0.0f)
        , m_frameCount(0)
    {
    }

    void FPSCounter::Update(float deltaTime)
    {
        m_frameTimeAccumulator += deltaTime;
        m_frameCount++;

        // Calculate FPS when we've accumulated 1 second worth of frames
        if (m_frameTimeAccumulator >= UPDATE_INTERVAL)
        {
            m_currentFPS = static_cast<float>(m_frameCount) / m_frameTimeAccumulator;
            
            // Reset for next window
            m_frameTimeAccumulator = 0.0f;
            m_frameCount = 0;
        }
    }
}
