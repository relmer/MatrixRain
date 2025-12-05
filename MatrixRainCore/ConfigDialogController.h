#pragma once

#include "ScreenSaverSettings.h"





/// <summary>
/// Presenter/controller for the screensaver configuration dialog.
/// Implements MVP pattern: manages settings validation, persistence coordination,
/// and provides interface between dialog UI and RegistrySettingsProvider.
/// </summary>
class ConfigDialogController
{
public:
    /// <summary>
    /// Initialize controller and load current settings from registry.
    /// Falls back to defaults if registry key doesn't exist.
    /// </summary>
    /// <returns>S_OK on success, error HRESULT otherwise</returns>
    HRESULT Initialize();

    /// <summary>
    /// Update density percentage with validation and clamping.
    /// </summary>
    /// <param name="densityPercent">Density value (clamped to 0-100)</param>
    void UpdateDensity (int densityPercent);

    /// <summary>
    /// Update color scheme key. Rejects invalid schemes.
    /// </summary>
    /// <param name="colorSchemeKey">Color scheme identifier (green, blue, red, amber, cycle)</param>
    void UpdateColorScheme (const std::wstring & colorSchemeKey);

    /// <summary>
    /// Update animation speed percentage with validation and clamping.
    /// </summary>
    /// <param name="animationSpeedPercent">Animation speed value (clamped to 1-100)</param>
    void UpdateAnimationSpeed (int animationSpeedPercent);

    /// <summary>
    /// Update glow intensity percentage with validation and clamping.
    /// </summary>
    /// <param name="glowIntensityPercent">Glow intensity value (clamped to 0-200)</param>
    void UpdateGlowIntensity (int glowIntensityPercent);

    /// <summary>
    /// Update glow size percentage with validation and clamping.
    /// </summary>
    /// <param name="glowSizePercent">Glow size value (clamped to 50-200)</param>
    void UpdateGlowSize (int glowSizePercent);

    /// <summary>
    /// Update start fullscreen flag.
    /// </summary>
    /// <param name="startFullscreen">True to start in fullscreen mode</param>
    void UpdateStartFullscreen (bool startFullscreen);

    /// <summary>
    /// Update show debug stats flag.
    /// </summary>
    /// <param name="showDebugStats">True to show debug statistics</param>
    void UpdateShowDebugStats (bool showDebugStats);

    /// <summary>
    /// Update show fade timers flag.
    /// </summary>
    /// <param name="showFadeTimers">True to show fade timer overlay</param>
    void UpdateShowFadeTimers (bool showFadeTimers);

    /// <summary>
    /// Apply and persist all changes to registry.
    /// </summary>
    /// <returns>S_OK on success, error HRESULT otherwise</returns>
    HRESULT ApplyChanges();

    /// <summary>
    /// Discard pending changes (does not modify registry).
    /// </summary>
    void CancelChanges();

    /// <summary>
    /// Reset all settings to factory defaults.
    /// </summary>
    void ResetToDefaults();

    /// <summary>
    /// Get current settings (with pending modifications).
    /// </summary>
    /// <returns>Current settings structure</returns>
    const ScreenSaverSettings & GetSettings() const { return m_settings; }

private:
    ScreenSaverSettings m_settings;                    // Current settings (may include pending changes)
    ScreenSaverSettings m_originalSettings;            // Original settings loaded from registry
    bool                m_isValidColorScheme (const std::wstring & key) const;
};


