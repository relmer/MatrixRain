#include "pch.h"

#include "MatrixRain/InputSystem.h"

namespace MatrixRain
{
    InputSystem::InputSystem()
        : m_densityController(nullptr)
        , m_appState(nullptr)
    {
    }

    void InputSystem::Initialize(DensityController& densityController, ApplicationState& appState)
    {
        m_densityController = &densityController;
        m_appState = &appState;
    }

    void InputSystem::ProcessKeyDown(int virtualKey)
    {
        switch (virtualKey)
        {
        case VK_ADD:        // Numpad +
        case VK_OEM_PLUS:   // Main keyboard = (shift for +)
            OnDensityIncrease();
            break;

        case VK_SUBTRACT:   // Numpad -
        case VK_OEM_MINUS:  // Main keyboard -
            OnDensityDecrease();
            break;

        case VK_OEM_3:      // Backtick/tilde key (`~)
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




    bool InputSystem::IsAltEnterPressed(int virtualKey) const
    {
        // Check if Enter key is pressed
        if (virtualKey != VK_RETURN)
        {
            return false;
        }

        // Check if Alt key is currently held down
        // GetAsyncKeyState returns high-order bit set if key is down
        return (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    }
}
