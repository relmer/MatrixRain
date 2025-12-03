#include "pch.h"

#include "MatrixRain/InputSystem.h"





void InputSystem::Initialize (DensityController & densityController, ApplicationState & appState)
{
    m_densityController = &densityController;
    m_appState          = &appState;
}





void InputSystem::ProcessKeyDown (int virtualKey)
{
    // Any keyboard input in screensaver mode triggers exit
    if (!m_exitState.m_keyboardTriggered)
    {
        m_exitState.m_keyboardTriggered = true;
    }

    switch (virtualKey)
    {
        case VK_ADD:                // Numpad +
        case VK_OEM_PLUS:           // Main keyboard = (shift for +)
            OnDensityIncrease();
            break;

        case VK_SUBTRACT:           // Numpad -
        case VK_OEM_MINUS:          // Main keyboard -
            OnDensityDecrease();
            break;

        case VK_OEM_3:              // Backtick/tilde key (`~)
            if (m_appState)
            {
                m_appState->ToggleDebugFadeTimes();
            }
            break;

        default:
            break;
    }
}





void InputSystem::OnDensityIncrease()
{
    if (m_densityController)
    {
        m_densityController->IncreaseLevel();
    }
}





void InputSystem::OnDensityDecrease()
{
    if (m_densityController)
    {
        m_densityController->DecreaseLevel();
    }
}


    


bool InputSystem::IsAltEnterPressed (int virtualKey) const
{
    // Check if Enter key is pressed
    if (virtualKey != VK_RETURN)
    {
        return false;
    }

    // Check if Alt key is currently held down
    // GetAsyncKeyState returns high-order bit set if key is down
    return (GetAsyncKeyState (VK_MENU) & 0x8000) != 0;
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
