#pragma once

#include "ColorScheme.h"
#include "ISettingsProvider.h"
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
    explicit ApplicationState (ISettingsProvider & settingsProvider);

    /// <summary>
    /// Initialize application state with default values.
    /// Starts in Fullscreen display mode with Green color scheme.
    /// If screensaver context is provided, may suppress statistics and other runtime features.
    /// </summary>
    /// <param name="pScreenSaverContext">Screensaver mode context for runtime behavior (nullptr for normal mode)</param>
    void Initialize (const ScreenSaverModeContext * pScreenSaverContext);

    /// Was the most recent settings Load() a brand-new install (no
    /// registry key existed)?  Used by Application to decide whether to
    /// run the first-run quality-preset heuristic (FR-037).
    bool IsFirstRun () const { return m_isFirstRun; }

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
    /// Register a callback to be notified when animation speed changes.
    /// </summary>
    /// <param name="callback">Function to call with new animation speed percentage</param>
    void RegisterAnimationSpeedCallback (std::function<void(int)> callback);

    /// <summary>
    /// Register a callback to be notified when glow intensity changes.
    /// </summary>
    /// <param name="callback">Function to call with new glow intensity percentage</param>
    void RegisterGlowIntensityCallback (std::function<void(int)> callback);

    /// <summary>
    /// Register a callback to be notified when glow size changes.
    /// </summary>
    /// <param name="callback">Function to call with new glow size percentage</param>
    void RegisterGlowSizeCallback (std::function<void(int)> callback);

    /// <summary>
    /// Register a callback to be notified when the advanced graphics
    /// values (preset-driven or custom) change.  Used by Application to
    /// push the new values into SharedState so the render thread picks
    /// them up via the snapshot path on the next frame (US5 live preview).
    /// </summary>
    void RegisterAdvancedGraphicsCallback (std::function<void(const AdvancedGraphicsValues &)> callback);

    /// <summary>
    /// Register a callback to be notified when the color scheme changes
    /// (via dialog, hotkey, or full settings apply).
    /// </summary>
    /// <param name="callback">Function to call with the new color scheme</param>
    void RegisterColorSchemeCallback (std::function<void(ColorScheme)> callback);

    /// <summary>
    /// Register a callback to be notified when the show-statistics flag
    /// changes (via dialog, hotkey, or full settings apply).
    /// </summary>
    /// <param name="callback">Function to call with the new visibility flag</param>
    void RegisterShowStatisticsCallback (std::function<void(bool)> callback);

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
    /// Update the advanced graphics values (passes / resolution / smoothness
    /// + glow intensity as a unit).  Fires the advanced-graphics callback
    /// so SharedState picks up the new values for the render thread.
    /// </summary>
    void SetAdvancedGraphics (const AdvancedGraphicsValues & values);

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
    float                     GetElapsedTime()         const { return m_elapsedTime;         }
    bool                      GetShowStatistics()      const { return m_showStatistics;      }
    const ScreenSaverSettings GetSettings()            const { return m_settings;            }

    /// First-run convenience: apply a heuristically-chosen quality preset
    /// to the in-memory settings, populate the advanced values from its
    /// lookup row, and persist.  Called once at startup by Application
    /// when IsFirstRun() is true (FR-037).
    HRESULT ApplyFirstRunQualityPreset (QualityPreset preset);

    void    SetDisplayMode        (DisplayMode mode)   { m_displayMode = mode;                 }
    void    SetColorScheme        (ColorScheme scheme);
    void    SetShowStatistics     (bool show);
    void    ToggleStatistics();
    HRESULT SaveSettings();

private:
    ISettingsProvider              & m_settingsProvider;                                           // Settings provider (not owned)
    DisplayMode                      m_displayMode                      = DisplayMode::Fullscreen; // Current display mode
    ColorScheme                      m_colorScheme                      = ColorScheme::Green;      // Current color scheme
    bool                             m_showStatistics                   = false;                   // Show FPS and density statistics
    float                            m_elapsedTime                      = 0.0f;                    // Elapsed time for color cycling animation
    ScreenSaverSettings              m_settings;                                                   // User-configurable settings
    bool                             m_isFirstRun                       = false;                   // True iff Load() found no registry key
    const ScreenSaverModeContext   * m_pScreenSaverContext              = nullptr;                 // Screensaver mode context (nullptr = normal mode)
    std::function<void(int)>         m_densityChangeCallback            = nullptr;                 // Callback for density changes
    std::function<void(int)>         m_animationSpeedChangeCallback     = nullptr;                 // Callback for animation speed changes
    std::function<void(int)>         m_glowIntensityChangeCallback      = nullptr;                 // Callback for glow intensity changes
    std::function<void(int)>         m_glowSizeChangeCallback           = nullptr;                 // Callback for glow size changes
    std::function<void(ColorScheme)> m_colorSchemeChangeCallback        = nullptr;                 // Callback for color scheme changes
    std::function<void(bool)>        m_showStatisticsChangeCallback     = nullptr;                 // Callback for show-statistics changes
    std::function<void(const AdvancedGraphicsValues &)> m_advancedGraphicsChangeCallback = nullptr; // Callback for US5 advanced graphics changes
};




