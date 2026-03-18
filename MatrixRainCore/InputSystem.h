#pragma once





#include "DensityController.h"
#include "ApplicationState.h"
#include "InputExitState.h"





class InputSystem
{
public:
    void Initialize          (DensityController & densityController, ApplicationState & appState);
    bool ProcessKeyDown      (int virtualKey);
    void InitializeExitState ();
    void ProcessMouseMove    (const POINT & currentPosition);
    bool ShouldExit          () const;
    void ResetExitState      ();



private:
    void OnDensityIncrease ();
    void OnDensityDecrease ();

    DensityController * m_densityController { nullptr };
    ApplicationState  * m_appState          { nullptr };
    InputExitState      m_exitState;
};





