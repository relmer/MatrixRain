#include "pch.h"

#include "MatrixRain/FPSCounter.h"





void FPSCounter::Update(float deltaTime)
{
    m_frameTimeAccumulator += deltaTime;
    m_frameCount++;

    // Calculate FPS when we've accumulated 1 second worth of frames
    if (m_frameTimeAccumulator >= UPDATE_INTERVAL)
    {
        m_currentFPS = static_cast<float> (m_frameCount) / m_frameTimeAccumulator;
        
        // Reset for next window
        m_frameTimeAccumulator = 0.0f;
        m_frameCount           = 0;
    }
}

