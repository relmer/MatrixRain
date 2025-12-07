#pragma once

#include "ColorScheme.h"
#include "ScreenSaverSettings.h"





struct ScreenSaverModeContext;





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
    /// If screensaver context is provided, may suppress statistics and other runtime features.
    /// </summary>
    /// <param name="pScreenSaverContext">Screensaver mode context for runtime behavior (nullptr for normal mode)</param>
    void Initialize (const ScreenSaverModeContext * pScreenSaverContext);

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
    /// Called when density changes to update and save settings.
    /// </summary>
    /// <param name="densityPercent">New density percentage (0-100)</param>
    void OnDensityChanged (int densityPercent);
    
    /// <summary>
    /// Register a callback to be notified when density changes.
    /// </summary>
    /// <param name="callback">Function to call with new density percentage</param>
    void RegisterDensityChangeCallback (std::function<void(int)> callback);

    /// <summary>
    /// Update animation speed setting.
    /// </summary>
    /// <param name="speedPercent">Animation speed percentage (1-100)</param>
    void SetAnimationSpeed (int speedPercent);

    /// <summary>
    /// Update glow intensity setting.
    /// </summary>
    /// <param name="intensityPercent">Glow intensity percentage (0-200)</param>
    void SetGlowIntensity (int intensityPercent);

    /// <summary>
    /// Update glow size setting.
    /// </summary>
    /// <param name="sizePercent">Glow size percentage (50-200)</param>
    void SetGlowSize (int sizePercent);

    /// <summary>
    /// Apply full settings to application state (for live dialog revert).
    /// </summary>
    /// <param name="settings">Settings to apply</param>
    void ApplySettings (const ScreenSaverSettings & settings);

    /// <summary>
    /// Update application state with elapsed time.
    /// Used for color cycling animation.
    /// </summary>
    /// <param name="deltaTime">Time elapsed since last update in seconds</param>
    void Update (float deltaTime);

    // Accessors
    DisplayMode               GetDisplayMode()         const { return m_displayMode;         }
    ColorScheme               GetColorScheme()         const { return m_colorScheme;         }
    bool                      GetShowDebugFadeTimes()  const { return m_showDebugFadeTimes;  }
    float                     GetElapsedTime()         const { return m_elapsedTime;         }
    bool                      GetShowStatistics()      const { return m_showStatistics;      }
    const ScreenSaverSettings GetSettings()            const { return m_settings;            }

    void SetDisplayMode       (DisplayMode mode)   { m_displayMode = mode;                 }
    void SetColorScheme       (ColorScheme scheme) { m_colorScheme = scheme;               }
    void ToggleStatistics();
    
    /// <summary>
    /// Save current settings to registry.
    /// </summary>
    /// <returns>S_OK on success, error HRESULT otherwise</returns>
    HRESULT SaveSettings();

private:
    DisplayMode                  m_displayMode           = DisplayMode::Fullscreen; // Current display mode
    ColorScheme                  m_colorScheme           = ColorScheme::Green;      // Current color scheme
    bool                         m_showDebugFadeTimes    = false;                   // Show debug fade time overlay
    bool                         m_showStatistics        = false;                   // Show FPS and density statistics
    float                        m_elapsedTime           = 0.0f;                    // Elapsed time for color cycling animation
    ScreenSaverSettings          m_settings;                                        // User-configurable settings
    std::function<void(int)>     m_densityChangeCallback = nullptr;                // Callback for density changes
};




