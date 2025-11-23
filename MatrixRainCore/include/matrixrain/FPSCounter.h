#pragma once

namespace MatrixRain
{
    /// <summary>
    /// FPS counter with rolling average calculation over 1-second window.
    /// </summary>
    class FPSCounter
    {
    public:
        FPSCounter();

        /// <summary>
        /// Update the FPS counter with the current frame's delta time.
        /// </summary>
        /// <param name="deltaTime">Time elapsed since last frame in seconds</param>
        void Update(float deltaTime);

        /// <summary>
        /// Get the current FPS value (rolling average over 1 second).
        /// </summary>
        /// <returns>Current FPS value</returns>
        float GetFPS() const { return m_currentFPS; }

    private:
        float m_currentFPS;        // Current calculated FPS
        float m_frameTimeAccumulator;  // Accumulated frame time
        int m_frameCount;          // Frame count in current window
        
        static constexpr float UPDATE_INTERVAL = 1.0f;  // Update FPS every second
    };
}
