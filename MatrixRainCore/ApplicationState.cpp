#include "pch.h"

#include "ApplicationState.h"
#include "RegistrySettingsProvider.h"
#include "ScreenSaverModeContext.h"





void ApplicationState::Initialize (const ScreenSaverModeContext * pScreenSaverContext)
{
    UNREFERENCED_PARAMETER (pScreenSaverContext);  // Reserved for future use (e.g., suppress statistics in /s mode)
    
    // Load settings from registry (falls back to defaults if key doesn't exist)
    HRESULT hr = RegistrySettingsProvider::Load (m_settings);
    
    // hr == S_FALSE means key didn't exist, used defaults (not an error)
    // hr == S_OK means loaded from registry successfully
    // Any other HRESULT is an actual error, but we continue with defaults
    UNREFERENCED_PARAMETER (hr);
    
    // Apply settings to runtime state
    m_displayMode        = m_settings.m_startFullscreen ? DisplayMode::Fullscreen : DisplayMode::Windowed;
    m_showDebugFadeTimes = m_settings.m_showFadeTimers;
    m_showStatistics     = m_settings.m_showDebugStats;
    
    // Map color scheme key to enum
    if (m_settings.m_colorSchemeKey == L"green")
    {
        m_colorScheme = ColorScheme::Green;
    }
    else if (m_settings.m_colorSchemeKey == L"blue")
    {
        m_colorScheme = ColorScheme::Blue;
    }
    else if (m_settings.m_colorSchemeKey == L"red")
    {
        m_colorScheme = ColorScheme::Red;
    }
    else if (m_settings.m_colorSchemeKey == L"amber")
    {
        m_colorScheme = ColorScheme::Amber;
    }
    else if (m_settings.m_colorSchemeKey == L"cycle")
    {
    m_colorScheme = ColorScheme::ColorCycle;
    }
    else
    {
        // Default to green if unknown
        m_colorScheme = ColorScheme::Green;
    }
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
    switch (m_colorScheme)
    {
        case ColorScheme::Green:      m_settings.m_colorSchemeKey = L"green";  break;
        case ColorScheme::Blue:       m_settings.m_colorSchemeKey = L"blue";   break;
        case ColorScheme::Red:        m_settings.m_colorSchemeKey = L"red";    break;
        case ColorScheme::Amber:      m_settings.m_colorSchemeKey = L"amber";  break;
        case ColorScheme::ColorCycle: m_settings.m_colorSchemeKey = L"cycle";  break;
    }
    
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





void ApplicationState::SetGlowIntensity (int intensityPercent)
{
    m_settings.m_glowIntensityPercent = intensityPercent;
}





void ApplicationState::SetGlowSize (int sizePercent)
{
    m_settings.m_glowSizePercent = sizePercent;
}





void ApplicationState::ApplySettings (const ScreenSaverSettings & settings)
{
    m_settings = settings;
    
    // Update runtime state to match settings
    m_displayMode        = m_settings.m_startFullscreen ? DisplayMode::Fullscreen : DisplayMode::Windowed;
    m_showDebugFadeTimes = m_settings.m_showFadeTimers;
    m_showStatistics     = m_settings.m_showDebugStats;
    
    // Map color scheme key to enum
    if (m_settings.m_colorSchemeKey == L"green")
    {
        m_colorScheme = ColorScheme::Green;
    }
    else if (m_settings.m_colorSchemeKey == L"blue")
    {
        m_colorScheme = ColorScheme::Blue;
    }
    else if (m_settings.m_colorSchemeKey == L"red")
    {
        m_colorScheme = ColorScheme::Red;
    }
    else if (m_settings.m_colorSchemeKey == L"amber")
    {
        m_colorScheme = ColorScheme::Amber;
    }
    else if (m_settings.m_colorSchemeKey == L"cycle")
    {
        m_colorScheme = ColorScheme::ColorCycle;
    }
    else
    {
        // Default to green if unknown
        m_colorScheme = ColorScheme::Green;
    }
}






void ApplicationState::Update (float deltaTime)
{
    m_elapsedTime += deltaTime;
}





void ApplicationState::SetShowStatistics (bool show)
{
    m_showStatistics = show;
    
    // Update settings and save
    m_settings.m_showDebugStats = show;
    SaveSettings();
}





void ApplicationState::SetShowDebugFadeTimes (bool show)
{
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

