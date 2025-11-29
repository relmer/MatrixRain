#pragma once





#include "MatrixRain/DensityController.h"
#include "MatrixRain/ApplicationState.h"





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
        /// Initialize the input system with required component references.
        /// </summary>
        /// <param name="densityController">Reference to density controller for +/- keys</param>
        /// <param name="appState">Reference to application state for debug toggle</param>
        void Initialize (DensityController & densityController, ApplicationState & appState);

        /// <summary>
        /// Process a keyboard key-down event.
        /// </summary>
        /// <param name="virtualKey">Virtual key code (VK_*)</param>
        void ProcessKeyDown (int virtualKey);

        /// <summary>
        /// Check if Alt+Enter keys are currently pressed.
        /// Used for display mode toggle detection.
        /// </summary>
        /// <param name="virtualKey">The virtual key code to check (should be VK_RETURN)</param>
        /// <returns>True if Enter is pressed while Alt is held down</returns>
        bool IsAltEnterPressed (int virtualKey) const;

    private:
        void OnDensityIncrease();
        void OnDensityDecrease();

        DensityController * m_densityController;
        ApplicationState  * m_appState;
    };
}





