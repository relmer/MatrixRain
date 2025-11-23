#pragma once
#include "matrixrain/DensityController.h"

namespace MatrixRain
{
    /// <summary>
    /// Handles keyboard input events and translates them to application actions.
    /// Supports density control via +/- keys (both main keyboard and numpad).
    /// </summary>
    class InputSystem
    {
    public:
        InputSystem();

        /// <summary>
        /// Initialize the input system with a density controller reference.
        /// </summary>
        /// <param name="densityController">Reference to density controller for +/- keys</param>
        void Initialize(DensityController& densityController);

        /// <summary>
        /// Process a keyboard key-down event.
        /// </summary>
        /// <param name="virtualKey">Virtual key code (VK_*)</param>
        void ProcessKeyDown(int virtualKey);

    private:
        void OnDensityIncrease();
        void OnDensityDecrease();

        DensityController* m_densityController;
    };
}
