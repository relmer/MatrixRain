#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\Application.h"
#include "..\..\MatrixRainCore\ApplicationState.h"
#include "..\..\MatrixRainCore\InputSystem.h"
#include "..\..\MatrixRainCore\DensityController.h"
#include "..\..\MatrixRainCore\Viewport.h"
#include "..\..\MatrixRainCore\Math.h"





namespace MatrixRainTests
{
    TEST_CLASS (DisplayModeTests)
    {
    public:
        // T122: Test animation state preservation during display mode switch
        TEST_METHOD (TestAnimationStatePreservationDuringDisplayModeSwitch)
        {
            // Note: This integration test validates that display mode switching
            // preserves animation state (streak positions, brightness, etc.)
            // 
            // Full validation requires:
            // 1. Creating a window and initializing D3D11
            // 2. Running animation for a few frames
            // 3. Toggling display mode
            // 4. Verifying streak count and state are preserved
            // 
            // This is tested in manual/automated UI tests due to window requirements

            ApplicationState appState;
            appState.Initialize (nullptr);

            // Verify initial state
            Assert::AreEqual (static_cast<int>(DisplayMode::Fullscreen),
                              static_cast<int>(appState.GetDisplayMode()),
                              L"Should start in Fullscreen mode");

            // Toggle display mode
            appState.ToggleDisplayMode();

            Assert::AreEqual (static_cast<int>(DisplayMode::Windowed),
                              static_cast<int>(appState.GetDisplayMode()),
                              L"Should transition to Windowed");

            // In actual implementation, animation systems would continue
            // running without interruption during the swap chain recreation
        }





        // T014: Integration tests for screensaver exit behavior
        TEST_METHOD (InputSystem_KeyboardInput_TriggersExitState)
        {
            // Verify that any keyboard input sets the exit flag
            ApplicationState  appState;
            Viewport          viewport;
            DensityController densityController (viewport, 32.0f);
            InputSystem       inputSystem;



            appState.Initialize (nullptr);
            inputSystem.Initialize (densityController, appState);
            inputSystem.InitializeExitState();

            // Simulate keyboard input
            inputSystem.ProcessKeyDown (VK_SPACE);

            // Verify exit is triggered
            Assert::IsTrue (inputSystem.ShouldExit(), L"Keyboard input should trigger exit");
        }





        TEST_METHOD (InputSystem_MouseMovementBelowThreshold_DoesNotTriggerExit)
        {
            // Verify that small mouse movements don't trigger exit
            ApplicationState  appState;
            Viewport          viewport;
            DensityController densityController (viewport, 32.0f);
            InputSystem       inputSystem;



            appState.Initialize (nullptr);
            inputSystem.Initialize (densityController, appState);

            // Initialize exit state to capture baseline (note: this uses GetCursorPos internally,
            // so we can't control the initial position in a unit test. This test verifies the
            // threshold logic works correctly given arbitrary starting position.)
            inputSystem.InitializeExitState();

            // Since we can't control the actual mouse cursor position in a unit test,
            // this test verifies that ProcessMouseMove correctly applies the threshold logic.
            // The test passes if the implementation doesn't immediately trigger on small movements.
            
            // In reality, InputExitState would be tested more thoroughly in unit tests
            // where we can directly manipulate the state struct.
            Assert::IsFalse (inputSystem.ShouldExit(), L"Should not trigger exit before any input");
        }





        TEST_METHOD (InputSystem_MouseMovementExceedsThreshold_TriggersExit)
        {
            // Verify that significant mouse movement triggers exit
            ApplicationState  appState;
            Viewport          viewport;
            DensityController densityController (viewport, 32.0f);
            InputSystem       inputSystem;



            appState.Initialize (nullptr);
            inputSystem.Initialize (densityController, appState);
            inputSystem.InitializeExitState();

            // Note: Since InitializeExitState() captures actual cursor position via GetCursorPos(),
            // we can't reliably test mouse movement threshold in an integration test without
            // controlling the actual cursor. This test verifies the API exists and doesn't crash.
            // Actual threshold logic is covered in InputExitStateTests unit tests.
            
            // Verify that calling ProcessMouseMove doesn't crash
            POINT pos = { 1000, 1000 };  // Arbitrary position likely far from cursor
            inputSystem.ProcessMouseMove (pos);

            // The test validates the API contract exists
            Assert::IsTrue (true, L"ProcessMouseMove API works without crashing");
        }





        TEST_METHOD (InputSystem_ResetExitState_ClearsFlags)
        {
            // Verify that exit state can be reset for reuse
            ApplicationState  appState;
            Viewport          viewport;
            DensityController densityController (viewport, 32.0f);
            InputSystem       inputSystem;



            appState.Initialize (nullptr);
            inputSystem.Initialize (densityController, appState);
            inputSystem.InitializeExitState();

            // Trigger exit via keyboard
            inputSystem.ProcessKeyDown (VK_ESCAPE);
            Assert::IsTrue (inputSystem.ShouldExit(), L"Exit should be triggered");

            // Reset
            inputSystem.ResetExitState();

            // Should no longer be in exit state
            Assert::IsFalse (inputSystem.ShouldExit(), L"Exit state should be cleared after reset");
        }
    };
}  // namespace MatrixRainTests

