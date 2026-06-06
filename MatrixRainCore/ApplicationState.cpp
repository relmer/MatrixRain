#include "pch.h"

#include "ApplicationState.h"
#include "ScreenSaverModeContext.h"





ApplicationState::ApplicationState (ISettingsProvider & settingsProvider) :
    m_settingsProvider (settingsProvider)
{
}





void ApplicationState::Initialize (const ScreenSaverModeContext * pScreenSaverContext)
{
    m_pScreenSaverContext = pScreenSaverContext;
    
    // Load settings from provider (falls back to defaults if no data exists)
    HRESULT hr = m_settingsProvider.Load (m_settings);
    
    // hr == S_FALSE means key didn't exist, used defaults (not an error).
    // Capture this for first-run heuristics (e.g., FR-037 quality preset).
    m_isFirstRun = (hr == S_FALSE);
    UNREFERENCED_PARAMETER (hr);
    
    // Apply settings to runtime state
    m_displayMode = m_settings.m_startFullscreen ? DisplayMode::Fullscreen : DisplayMode::Windowed;

    // Screensaver mode always forces fullscreen regardless of registry setting
    if (pScreenSaverContext && pScreenSaverContext->m_mode == ScreenSaverMode::ScreenSaverFull)
    {
        m_displayMode = DisplayMode::Fullscreen;
    }
    
    // Suppress debug overlays in preview and screensaver modes
    bool isPreviewOrScreenSaver = pScreenSaverContext &&
                                  (pScreenSaverContext->m_mode == ScreenSaverMode::ScreenSaverPreview ||
                                   pScreenSaverContext->m_mode == ScreenSaverMode::ScreenSaverFull);
    
    m_showDebugFadeTimes = false;
    m_showStatistics     = isPreviewOrScreenSaver ? false : m_settings.m_showDebugStats;
    
    // Map color scheme key to enum
    m_colorScheme = ParseColorSchemeKey (m_settings.m_colorSchemeKey);
}





void ApplicationState::ToggleDisplayMode()
{
    // Toggle between windowed and fullscreen
    if (m_displayMode == DisplayMode::Windowed)
    {
        m_displayMode = DisplayMode::Fullscreen;
    }
    else
    {
        m_displayMode = DisplayMode::Windowed;
    }
    
    // Update settings and save
    m_settings.m_startFullscreen = (m_displayMode == DisplayMode::Fullscreen);

    HRESULT hr = SaveSettings();
    IGNORE_RETURN_VALUE (hr, S_OK);
}





void ApplicationState::CycleColorScheme()
{
    // Cycle to next color scheme
    m_colorScheme = GetNextColorScheme (m_colorScheme);
    
    // Update settings and save
    m_settings.m_colorSchemeKey = ColorSchemeToKey (m_colorScheme);

    // Notify registered listener (e.g., Application's SharedState sync)
    if (m_colorSchemeChangeCallback)
    {
        m_colorSchemeChangeCallback (m_colorScheme);
    }

    HRESULT hr = SaveSettings();
    IGNORE_RETURN_VALUE (hr, S_OK);
}





void ApplicationState::ToggleDebugFadeTimes()
{
    m_showDebugFadeTimes = !m_showDebugFadeTimes;

    // Notify registered listener (e.g., Application's SharedState sync)
    if (m_showDebugFadeTimesChangeCallback)
    {
        m_showDebugFadeTimesChangeCallback (m_showDebugFadeTimes);
    }
}





void ApplicationState::OnDensityChanged (int densityPercent)
{
    m_settings.m_densityPercent = densityPercent;
    
    // Notify registered listener (e.g., DensityController)
    if (m_densityChangeCallback)
    {
        m_densityChangeCallback (densityPercent);
    }

    HRESULT hr = SaveSettings();
    IGNORE_RETURN_VALUE (hr, S_OK);
}





void ApplicationState::RegisterDensityChangeCallback (std::function<void(int)> callback)
{
    m_densityChangeCallback = callback;
}





void ApplicationState::RegisterAnimationSpeedCallback (std::function<void(int)> callback)
{
    m_animationSpeedChangeCallback = callback;
}





void ApplicationState::SetAnimationSpeed (int speedPercent)
{
    m_settings.m_animationSpeedPercent = speedPercent;
    
    // Notify registered listener (e.g., AnimationSystem)
    if (m_animationSpeedChangeCallback)
    {
        m_animationSpeedChangeCallback (speedPercent);
    }

    HRESULT hr = SaveSettings();
    IGNORE_RETURN_VALUE (hr, S_OK);
}





void ApplicationState::RegisterGlowIntensityCallback (std::function<void(int)> callback)
{
    m_glowIntensityChangeCallback = callback;
}





void ApplicationState::RegisterGlowSizeCallback (std::function<void(int)> callback)
{
    m_glowSizeChangeCallback = callback;
}





void ApplicationState::RegisterAdvancedGraphicsCallback (std::function<void(const AdvancedGraphicsValues &)> callback)
{
    m_advancedGraphicsChangeCallback = callback;
}





void ApplicationState::RegisterColorSchemeCallback (std::function<void(ColorScheme)> callback)
{
    m_colorSchemeChangeCallback = callback;
}





void ApplicationState::SetColorScheme (ColorScheme scheme)
{
    m_colorScheme               = scheme;
    m_settings.m_colorSchemeKey = ColorSchemeToKey (scheme);

    // Notify registered listener (e.g., Application's SharedState sync)
    if (m_colorSchemeChangeCallback)
    {
        m_colorSchemeChangeCallback (scheme);
    }
}





void ApplicationState::RegisterShowStatisticsCallback (std::function<void(bool)> callback)
{
    m_showStatisticsChangeCallback = callback;
}





void ApplicationState::RegisterShowDebugFadeTimesCallback (std::function<void(bool)> callback)
{
    m_showDebugFadeTimesChangeCallback = callback;
}





void ApplicationState::SetGlowIntensity (int intensityPercent)
{
    m_settings.m_glowIntensityPercent = intensityPercent;
    
    // Notify registered listener (e.g., RenderSystem)
    if (m_glowIntensityChangeCallback)
    {
        m_glowIntensityChangeCallback (intensityPercent);
    }

    HRESULT hr = SaveSettings();
    IGNORE_RETURN_VALUE (hr, S_OK);
}





void ApplicationState::SetGlowSize (int sizePercent)
{
    m_settings.m_glowSizePercent = sizePercent;
    
    // Notify registered listener (e.g., RenderSystem)
    if (m_glowSizeChangeCallback)
    {
        m_glowSizeChangeCallback (sizePercent);
    }

    HRESULT hr = SaveSettings();
    IGNORE_RETURN_VALUE (hr, S_OK);
}





void ApplicationState::SetAdvancedGraphics (const AdvancedGraphicsValues & values)
{
    m_settings.m_advancedValues = values;

    if (m_advancedGraphicsChangeCallback)
    {
        m_advancedGraphicsChangeCallback (values);
    }

    // Note: we DON'T SaveSettings() here - the controller already owns
    // the persistence path on dialog OK / live cancel.  This setter is
    // purely the live-preview pump.
}





void ApplicationState::ApplySettings (const ScreenSaverSettings & settings)
{
    m_settings = settings;
    
    // Update runtime state to match settings
    m_displayMode        = m_settings.m_startFullscreen ? DisplayMode::Fullscreen : DisplayMode::Windowed;
    m_showDebugFadeTimes = m_settings.m_showFadeTimers;
    m_showStatistics     = m_settings.m_showDebugStats;
    
    // Map color scheme key to enum
    m_colorScheme = ParseColorSchemeKey (m_settings.m_colorSchemeKey);

    // Notify registered listeners so renderer-side state (e.g., SharedState) stays in sync
    if (m_colorSchemeChangeCallback)
    {
        m_colorSchemeChangeCallback (m_colorScheme);
    }

    if (m_showStatisticsChangeCallback)
    {
        m_showStatisticsChangeCallback (m_showStatistics);
    }

    if (m_showDebugFadeTimesChangeCallback)
    {
        m_showDebugFadeTimesChangeCallback (m_showDebugFadeTimes);
    }
}






void ApplicationState::Update (float deltaTime)
{
    m_elapsedTime += deltaTime;
}





void ApplicationState::SetShowStatistics (bool show)
{
    // Prevent enabling debug statistics in preview or screensaver modes
    bool isPreviewOrScreenSaver = m_pScreenSaverContext &&
                                  (m_pScreenSaverContext->m_mode == ScreenSaverMode::ScreenSaverPreview ||
                                   m_pScreenSaverContext->m_mode == ScreenSaverMode::ScreenSaverFull);
    
    if (isPreviewOrScreenSaver && show)
    {
        return;  // Ignore request to enable in preview/screensaver modes
    }
    
    m_showStatistics = show;
    
    // Update settings and save
    m_settings.m_showDebugStats = show;

    // Notify registered listener (e.g., Application's SharedState sync)
    if (m_showStatisticsChangeCallback)
    {
        m_showStatisticsChangeCallback (show);
    }

    HRESULT hr = SaveSettings();
    IGNORE_RETURN_VALUE (hr, S_OK);
}





void ApplicationState::SetShowDebugFadeTimes (bool show)
{
    // Prevent enabling fade timers in preview or screensaver modes
    bool isPreviewOrScreenSaver = m_pScreenSaverContext &&
                                  (m_pScreenSaverContext->m_mode == ScreenSaverMode::ScreenSaverPreview ||
                                   m_pScreenSaverContext->m_mode == ScreenSaverMode::ScreenSaverFull);
    
    if (isPreviewOrScreenSaver && show)
    {
        return;  // Ignore request to enable in preview/screensaver modes
    }
    
    m_showDebugFadeTimes = show;

    // Notify registered listener (e.g., Application's SharedState sync)
    if (m_showDebugFadeTimesChangeCallback)
    {
        m_showDebugFadeTimesChangeCallback (show);
    }
}





void ApplicationState::ToggleStatistics()
{
    m_showStatistics = !m_showStatistics;
    
    // Update settings and save
    m_settings.m_showDebugStats = m_showStatistics;

    // Notify registered listener (e.g., Application's SharedState sync)
    if (m_showStatisticsChangeCallback)
    {
        m_showStatisticsChangeCallback (m_showStatistics);
    }

    HRESULT hr = SaveSettings();
    IGNORE_RETURN_VALUE (hr, S_OK);
}






////////////////////////////////////////////////////////////////////////////////
//
//  ApplicationState::SaveSettings
//
//  Persist current settings to registry
//
////////////////////////////////////////////////////////////////////////////////

HRESULT
ApplicationState::SaveSettings()
{
    return m_settingsProvider.Save (m_settings);
}




////////////////////////////////////////////////////////////////////////////////
//
//  ApplicationState::ApplyFirstRunQualityPreset
//
////////////////////////////////////////////////////////////////////////////////

HRESULT ApplicationState::ApplyFirstRunQualityPreset (QualityPreset preset)
{
    if (preset != QualityPreset::Custom)
    {
        m_settings.m_qualityPreset  = preset;
        m_settings.m_advancedValues = LookupPresetValues (preset);

        // Keep the legacy top-level glow-intensity field in sync with the
        // advanced cluster's copy.  ScreenSaverSettings carries glow
        // intensity in both m_glowIntensityPercent (read by Application's
        // unconditional SharedState seed) and m_advancedValues.m_glow-
        // IntensityPercent (read by the live-mode advanced-graphics
        // callback).  RegistrySettingsProvider::Save persists the top-
        // level field — without this mirror the preset's intensity (e.g.
        // 75 for Low) is silently overwritten by the unchanged default
        // (100), and the next launch starts at the wrong value.
        m_settings.m_glowIntensityPercent = m_settings.m_advancedValues.m_glowIntensityPercent;
    }

    return SaveSettings();
}

