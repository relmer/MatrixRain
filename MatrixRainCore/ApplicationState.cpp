#include "pch.h"

#include "ApplicationState.h"
#include "RegistrySettingsProvider.h"
#include "ScreenSaverModeContext.h"





void ApplicationState::Initialize (const ScreenSaverModeContext * pScreenSaverContext)
{
    m_pScreenSaverContext = pScreenSaverContext;
    
    // Load settings from registry (falls back to defaults if key doesn't exist)
    HRESULT hr = RegistrySettingsProvider::Load (m_settings);
    
    // hr == S_FALSE means key didn't exist, used defaults (not an error)
    // hr == S_OK means loaded from registry successfully
    // Any other HRESULT is an actual error, but we continue with defaults
    UNREFERENCED_PARAMETER (hr);
    
    // Apply settings to runtime state
    m_displayMode = m_settings.m_startFullscreen ? DisplayMode::Fullscreen : DisplayMode::Windowed;
    
    // Suppress debug overlays in preview and screensaver modes
    bool isPreviewOrScreenSaver = pScreenSaverContext &&
                                  (pScreenSaverContext->m_mode == ScreenSaverMode::ScreenSaverPreview ||
                                   pScreenSaverContext->m_mode == ScreenSaverMode::ScreenSaverFull);
    
    m_showDebugFadeTimes = isPreviewOrScreenSaver ? false : m_settings.m_showFadeTimers;
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
    SaveSettings();
}





void ApplicationState::CycleColorScheme()
{
    // Cycle to next color scheme
    m_colorScheme = GetNextColorScheme (m_colorScheme);
    
    // Update settings and save
    m_settings.m_colorSchemeKey = ColorSchemeToKey (m_colorScheme);
    
    SaveSettings();
}





void ApplicationState::ToggleDebugFadeTimes()
{
    m_showDebugFadeTimes = !m_showDebugFadeTimes;
    
    // Update settings and save
    m_settings.m_showFadeTimers = m_showDebugFadeTimes;
    SaveSettings();
}





void ApplicationState::OnDensityChanged (int densityPercent)
{
    m_settings.m_densityPercent = densityPercent;
    
    // Notify registered listener (e.g., DensityController)
    if (m_densityChangeCallback)
    {
        m_densityChangeCallback (densityPercent);
    }
    
    SaveSettings();
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
    
    SaveSettings();
}





void ApplicationState::RegisterGlowIntensityCallback (std::function<void(int)> callback)
{
    m_glowIntensityChangeCallback = callback;
}





void ApplicationState::RegisterGlowSizeCallback (std::function<void(int)> callback)
{
    m_glowSizeChangeCallback = callback;
}





void ApplicationState::SetGlowIntensity (int intensityPercent)
{
    m_settings.m_glowIntensityPercent = intensityPercent;
    
    // Notify registered listener (e.g., RenderSystem)
    if (m_glowIntensityChangeCallback)
    {
        m_glowIntensityChangeCallback (intensityPercent);
    }
    
    SaveSettings();
}





void ApplicationState::SetGlowSize (int sizePercent)
{
    m_settings.m_glowSizePercent = sizePercent;
    
    // Notify registered listener (e.g., RenderSystem)
    if (m_glowSizeChangeCallback)
    {
        m_glowSizeChangeCallback (sizePercent);
    }
    
    SaveSettings();
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
    SaveSettings();
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
    
    // Update settings and save
    m_settings.m_showFadeTimers = show;
    SaveSettings();
}





void ApplicationState::ToggleStatistics()
{
    m_showStatistics = !m_showStatistics;
    
    // Update settings and save
    m_settings.m_showDebugStats = m_showStatistics;
    SaveSettings();
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
    return RegistrySettingsProvider::Save (m_settings);
}

