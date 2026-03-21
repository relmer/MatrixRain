#include "pch.h"

#include "InputSystem.h"





////////////////////////////////////////////////////////////////////////////////
//
//  InputSystem::Initialize
//
////////////////////////////////////////////////////////////////////////////////

void InputSystem::Initialize (DensityController & densityController, ApplicationState & appState)
{
    m_densityController = &densityController;
    m_appState          = &appState;
}





////////////////////////////////////////////////////////////////////////////////
//
//  InputSystem::ProcessKeyDown
//
//  Returns true if the key was recognized and handled.
//
////////////////////////////////////////////////////////////////////////////////

bool InputSystem::ProcessKeyDown (int virtualKey)
{
    bool handled = false;


    // Any keyboard input in screensaver mode triggers exit
    if (!m_exitState.m_keyboardTriggered)
    {
        m_exitState.m_keyboardTriggered = true;
    }

    switch (virtualKey)
    {
        case 'C':
            if (m_appState)
            {
                m_appState->CycleColorScheme();
                handled = true;
            }
            break;

        case 'S':
            if (m_appState)
            {
                m_appState->ToggleStatistics();
                handled = true;
            }
            break;

        case VK_ADD:                // Numpad +
        case VK_OEM_PLUS:           // Main keyboard = (shift for +)
            OnDensityIncrease();
            handled = true;
            break;

        case VK_SUBTRACT:           // Numpad -
        case VK_OEM_MINUS:          // Main keyboard -
            OnDensityDecrease();
            handled = true;
            break;

#ifdef _DEBUG
        case VK_OEM_3:              // Backtick/tilde key (`~) — debug only
            if (m_appState)
            {
                m_appState->ToggleDebugFadeTimes();
                handled = true;
            }
            break;
#endif

        default:
            break;
    }

    return handled;
}





////////////////////////////////////////////////////////////////////////////////
//
//  InputSystem::OnDensityIncrease
//
////////////////////////////////////////////////////////////////////////////////

void InputSystem::OnDensityIncrease()
{
    if (m_densityController)
    {
        m_densityController->IncreaseLevel();
        
        // Notify app state to save new density
        if (m_appState)
        {
            m_appState->OnDensityChanged (m_densityController->GetPercentage());
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  InputSystem::OnDensityDecrease
//
////////////////////////////////////////////////////////////////////////////////

void InputSystem::OnDensityDecrease()
{
    if (m_densityController)
    {
        m_densityController->DecreaseLevel();
        
        // Notify app state to save new density
        if (m_appState)
        {
            m_appState->OnDensityChanged (m_densityController->GetPercentage());
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  InputSystem::InitializeExitState
//
//  Captures initial mouse position for screensaver exit threshold tracking
//
////////////////////////////////////////////////////////////////////////////////

void InputSystem::InitializeExitState()
{
    POINT cursorPos;

    

    // Capture current mouse position as baseline
    if (GetCursorPos (&cursorPos))
    {
        m_exitState.m_initialMousePosition = cursorPos;
    }

    // Reset trigger flags
    m_exitState.m_keyboardTriggered = false;
    m_exitState.m_mouseTriggered    = false;
}





////////////////////////////////////////////////////////////////////////////////
//
//  InputSystem::ProcessMouseMove
//
//  Detects meaningful mouse movement exceeding the exit threshold
//
////////////////////////////////////////////////////////////////////////////////

void InputSystem::ProcessMouseMove (const POINT & currentPosition)
{
    int dx = abs (currentPosition.x - m_exitState.m_initialMousePosition.x);
    int dy = abs (currentPosition.y - m_exitState.m_initialMousePosition.y);


    // Check if either axis exceeds threshold
    if (dx >= m_exitState.m_exitThresholdPixels || dy >= m_exitState.m_exitThresholdPixels)
    {
        m_exitState.m_mouseTriggered = true;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  InputSystem::ShouldExit
//
//  Returns true if screensaver exit conditions have been met
//
////////////////////////////////////////////////////////////////////////////////

bool InputSystem::ShouldExit() const
{
    return m_exitState.m_keyboardTriggered || m_exitState.m_mouseTriggered;
}





////////////////////////////////////////////////////////////////////////////////
//
//  InputSystem::ResetExitState
//
//  Clears exit state tracking (useful for pause/resume scenarios)
//
////////////////////////////////////////////////////////////////////////////////

void InputSystem::ResetExitState()
{
    m_exitState.m_keyboardTriggered = false;
    m_exitState.m_mouseTriggered    = false;

    // Recapture mouse position
    POINT cursorPos;
    if (GetCursorPos (&cursorPos))
    {
        m_exitState.m_initialMousePosition = cursorPos;
    }
}
