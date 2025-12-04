#pragma once





#include "DensityController.h"
#include "ApplicationState.h"
#include "InputExitState.h"





/// <summary>
/// Handles keyboard input events and translates them to application actions.
/// Supports density control via +/- keys (both main keyboard and numpad).
/// </summary>
class InputSystem
{
public:
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

    /// <summary>
    /// Initialize exit state tracking for screensaver mode.
    /// Captures the initial mouse position to establish threshold baseline.
    /// </summary>
    void InitializeExitState();

    /// <summary>
    /// Process mouse movement to determine if exit threshold has been exceeded.
    /// </summary>
    /// <param name="currentPosition">Current mouse position</param>
    void ProcessMouseMove (const POINT & currentPosition);

    /// <summary>
    /// Check if screensaver exit conditions have been met.
    /// </summary>
    /// <returns>True if keyboard input or significant mouse movement detected</returns>
    bool ShouldExit() const;

    /// <summary>
    /// Reset exit state tracking (e.g., when resuming after pause).
    /// </summary>
    void ResetExitState();

    
private:
    void OnDensityIncrease();
    void OnDensityDecrease();

    DensityController * m_densityController { nullptr };
    ApplicationState  * m_appState          { nullptr };
    InputExitState      m_exitState;
};





