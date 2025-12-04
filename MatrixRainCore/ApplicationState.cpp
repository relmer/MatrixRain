#include "pch.h"

#include "ApplicationState.h"
#include "ScreenSaverModeContext.h"





void ApplicationState::Initialize (const ScreenSaverModeContext * pScreenSaverContext)
{
    UNREFERENCED_PARAMETER (pScreenSaverContext);  // Reserved for future use (e.g., suppress statistics in /s mode)
    
    // Start in fullscreen mode by default
    m_displayMode = DisplayMode::Fullscreen;
    
    // Start with green color scheme (classic Matrix)
    m_colorScheme = ColorScheme::Green;
    
    // Debug fade times off by default
    m_showDebugFadeTimes = false;
    
    // Statistics hidden by default
    m_showStatistics = false;
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
}





void ApplicationState::CycleColorScheme()
{
    // Cycle to next color scheme
    m_colorScheme = GetNextColorScheme (m_colorScheme);
}





void ApplicationState::ToggleDebugFadeTimes()
{
    m_showDebugFadeTimes = !m_showDebugFadeTimes;
}





void ApplicationState::Update (float deltaTime)
{
    m_elapsedTime += deltaTime;
}

