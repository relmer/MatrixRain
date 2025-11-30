#pragma once





#include "MatrixRain/ColorScheme.h"





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
    /// <summary>
    /// Initialize application state with default values.
    /// Starts in Fullscreen display mode with Green color scheme.
    /// </summary>
    void Initialize();

    /// <summary>
    /// Toggle between Windowed and Fullscreen display modes.
    /// </summary>
    void ToggleDisplayMode();

    /// <summary>
    /// Cycle to the next color scheme.
    /// Green → Blue → Red → Amber → ColorCycle → Green
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
    void Update (float deltaTime);

    // Accessors
    DisplayMode GetDisplayMode()         const { return m_displayMode;         }
    ColorScheme GetColorScheme()         const { return m_colorScheme;         }
    bool        GetShowDebugFadeTimes()  const { return m_showDebugFadeTimes;  }
    float       GetElapsedTime()         const { return m_elapsedTime;         }
    bool        GetShowStatistics()      const { return m_showStatistics;      }

    void SetDisplayMode    (DisplayMode mode)   { m_displayMode = mode;                 }
    void SetColorScheme    (ColorScheme scheme) { m_colorScheme = scheme;               }
    void ToggleStatistics()                     { m_showStatistics = !m_showStatistics; }

private:
    DisplayMode m_displayMode        = DisplayMode::Fullscreen; // Current display mode
    ColorScheme m_colorScheme        = ColorScheme::Green;      // Current color scheme
    bool        m_showDebugFadeTimes = false;                   // Show debug fade time overlay
    bool        m_showStatistics     = false;                   // Show FPS and density statistics
    float       m_elapsedTime        = 0.0f;                    // Elapsed time for color cycling animation
};




