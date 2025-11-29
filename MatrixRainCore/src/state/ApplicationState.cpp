#include "pch.h"
#include "MatrixRain/ApplicationState.h"

namespace MatrixRain
{
    ApplicationState::ApplicationState()
        : m_displayMode(DisplayMode::Windowed)
        , m_colorScheme(ColorScheme::Green)
        , m_showDebugFadeTimes(false)
        , m_elapsedTime(0.0f)
    {
    }

    void ApplicationState::Initialize()
    {
        // Start in windowed mode (FR-019)
        m_displayMode = DisplayMode::Windowed;
        
        // Start with green color scheme (classic Matrix)
        m_colorScheme = ColorScheme::Green;
        
        // Debug fade times off by default
        m_showDebugFadeTimes = false;
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
        m_colorScheme = GetNextColorScheme(m_colorScheme);
    }





    void ApplicationState::ToggleDebugFadeTimes()
    {
        m_showDebugFadeTimes = !m_showDebugFadeTimes;
    }

    void ApplicationState::Update(float deltaTime)
    {
        m_elapsedTime += deltaTime;
    }
}

