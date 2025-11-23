#include "pch.h"
#include "matrixrain/InputSystem.h"

namespace MatrixRain
{
    InputSystem::InputSystem()
        : m_densityController(nullptr)
    {
    }

    void InputSystem::Initialize(DensityController& densityController)
    {
        m_densityController = &densityController;
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
}
