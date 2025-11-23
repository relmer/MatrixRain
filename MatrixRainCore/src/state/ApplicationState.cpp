#include "pch.h"
#include "matrixrain/ApplicationState.h"

namespace MatrixRain
{
    ApplicationState::ApplicationState()
        : m_displayMode(DisplayMode::Windowed)
    {
    }

    void ApplicationState::Initialize()
    {
        // Start in windowed mode (FR-019)
        m_displayMode = DisplayMode::Windowed;
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
}
