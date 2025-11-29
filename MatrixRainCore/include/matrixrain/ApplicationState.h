#pragma once
#include "MatrixRain/ColorScheme.h"

namespace MatrixRain
{
    /// <summary>
    /// Display mode enumeration for windowed vs fullscreen
    /// </summary>
    enum class DisplayMode
    {
        Windowed,
        Fullscreen
    };

    /// <summary>
    /// Manages application-wide state including display mode and color scheme.
    /// Coordinates between different systems to maintain consistent state.
    /// </summary>
    class ApplicationState
    {
    public:
        ApplicationState();

        /// <summary>
        /// Initialize application state with default values.
        /// Starts in Windowed display mode with Green color scheme.
        /// </summary>
        void Initialize();

        /// <summary>
        /// Toggle between Windowed and Fullscreen display modes.
        /// </summary>
        void ToggleDisplayMode();

        /// <summary>
        /// Cycle to the next color scheme.
        /// Green → Blue → Red → Amber → Green
        /// </summary>
        void CycleColorScheme();

        /// <summary>
        /// Toggle debug fade times display.
        /// </summary>
        void ToggleDebugFadeTimes();

        /// <summary>
        /// Update application state with elapsed time.
        /// Used for color cycling animation.
        /// </summary>
        /// <param name="deltaTime">Time elapsed since last update in seconds</param>
        void Update(float deltaTime);

        // Accessors
        DisplayMode GetDisplayMode() const { return m_displayMode; }
        void SetDisplayMode(DisplayMode mode) { m_displayMode = mode; }
        ColorScheme GetColorScheme() const { return m_colorScheme; }
        void SetColorScheme(ColorScheme scheme) { m_colorScheme = scheme; }
        bool GetShowDebugFadeTimes() const { return m_showDebugFadeTimes; }
        float GetElapsedTime() const { return m_elapsedTime; }

    private:
        DisplayMode m_displayMode;       // Current display mode
        ColorScheme m_colorScheme;       // Current color scheme
        bool        m_showDebugFadeTimes; // Show debug fade time overlay
        float       m_elapsedTime;       // Elapsed time for color cycling animation
    };
}

