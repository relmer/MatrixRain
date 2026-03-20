#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\InputSystem.h"
#include "..\..\MatrixRainCore\DensityController.h"
#include "..\..\MatrixRainCore\ApplicationState.h"
#include "..\..\MatrixRainCore\InMemorySettingsProvider.h"
#include "..\..\MatrixRainCore\Viewport.h"





namespace MatrixRainTests
{
    TEST_CLASS (InputSystemTests)
    {
    public:

        TEST_METHOD (TestInputSystemProcessesVK_ADD)
        {
            // T097: Test InputSystem keyboard event processing for VK_ADD
            // Given: InputSystem is initialized with DensityController at 50%
            // When: ProcessKeyDown(VK_ADD) is called
            // Then: DensityController percentage should increase by 5% to 55%

            Viewport viewport;
            viewport.Resize (1920.0f, 1080.0f); // Max = 192 streaks

            DensityController densityController (viewport, 24.0f); // 50%
            InMemorySettingsProvider settingsProvider;
            ApplicationState appState (settingsProvider);
            appState.Initialize (nullptr);

            InputSystem inputSystem;
            inputSystem.Initialize (densityController, appState);

            // Verify initial state (50% of 192 = 96 streaks)
            Assert::AreEqual (50, densityController.GetPercentage (), L"Initial percentage should be 50");
            Assert::AreEqual (96, densityController.GetTargetStreakCount (), L"Initial should be 96 streaks at 50%");

            // Process VK_ADD
            bool handled = inputSystem.ProcessKeyDown (VK_ADD);

            // Verify percentage increased (55% of 192 = 105 streaks)
            Assert::IsTrue (handled, L"VK_ADD should be recognized");
            Assert::AreEqual (55, densityController.GetPercentage (), L"VK_ADD should increase to 55%");
            Assert::AreEqual (105, densityController.GetTargetStreakCount (), L"Should be 105 streaks at 55%");
        }





        TEST_METHOD (TestInputSystemProcessesVK_SUBTRACT)
        {
            // T098: Test InputSystem keyboard event processing for VK_SUBTRACT
            // Given: InputSystem is initialized with DensityController at 50%
            // When: ProcessKeyDown(VK_SUBTRACT) is called
            // Then: DensityController percentage should decrease by 5% to 45%

            Viewport viewport;
            viewport.Resize (1920.0f, 1080.0f); // Max = 192 streaks

            DensityController densityController (viewport, 24.0f); // 50%
            InMemorySettingsProvider settingsProvider;
            ApplicationState appState (settingsProvider);
            appState.Initialize (nullptr);

            InputSystem inputSystem;
            inputSystem.Initialize (densityController, appState);

            // Verify initial state (50% of 192 = 96 streaks)
            Assert::AreEqual (50, densityController.GetPercentage (), L"Initial percentage should be 50");
            Assert::AreEqual (96, densityController.GetTargetStreakCount (), L"Initial should be 96 streaks at 50%");

            // Process VK_SUBTRACT
            bool handled = inputSystem.ProcessKeyDown (VK_SUBTRACT);

            // Verify percentage decreased (45% of 192 = 86 streaks)
            Assert::IsTrue (handled, L"VK_SUBTRACT should be recognized");
            Assert::AreEqual (45, densityController.GetPercentage (), L"VK_SUBTRACT should decrease to 45%");
            Assert::AreEqual (86, densityController.GetTargetStreakCount (), L"Should be 86 streaks at 45%");
        }





        TEST_METHOD (TestInputSystemProcessesVK_OEM_PLUS)
        {
            // T099: Test InputSystem keyboard event processing for VK_OEM_PLUS
            // Given: InputSystem is initialized with DensityController at 50%
            // When: ProcessKeyDown(VK_OEM_PLUS) is called (regular '=' key, shift for '+')
            // Then: DensityController percentage should increase by 5% to 55%

            Viewport viewport;
            viewport.Resize (1920.0f, 1080.0f);

            DensityController densityController (viewport, 24.0f);
            InMemorySettingsProvider settingsProvider;
            ApplicationState appState (settingsProvider);
            appState.Initialize (nullptr);

            InputSystem inputSystem;
            inputSystem.Initialize (densityController, appState);

            // Process VK_OEM_PLUS
            bool handled = inputSystem.ProcessKeyDown (VK_OEM_PLUS);

            // Verify percentage increased
            Assert::IsTrue (handled, L"VK_OEM_PLUS should be recognized");
            Assert::AreEqual (55, densityController.GetPercentage (), L"VK_OEM_PLUS should increase to 55%");
        }





        TEST_METHOD (TestInputSystemProcessesVK_OEM_MINUS)
        {
            // T100: Test InputSystem keyboard event processing for VK_OEM_MINUS
            // Given: InputSystem is initialized with DensityController at 50%
            // When: ProcessKeyDown(VK_OEM_MINUS) is called
            // Then: DensityController percentage should decrease by 5% to 45%

            Viewport viewport;
            viewport.Resize (1920.0f, 1080.0f);

            DensityController densityController (viewport, 24.0f);
            InMemorySettingsProvider settingsProvider;
            ApplicationState appState (settingsProvider);
            appState.Initialize (nullptr);

            InputSystem inputSystem;
            inputSystem.Initialize (densityController, appState);

            // Process VK_OEM_MINUS
            bool handled = inputSystem.ProcessKeyDown (VK_OEM_MINUS);

            // Verify percentage decreased
            Assert::IsTrue (handled, L"VK_OEM_MINUS should be recognized");
            Assert::AreEqual (45, densityController.GetPercentage (), L"VK_OEM_MINUS should decrease to 45%");
        }





        // T177: Test InputSystem keyboard event processing for VK_C (color cycle)
        TEST_METHOD (TestInputSystemProcessesVK_C)
        {
            // Given: InputSystem is initialized with ApplicationState
            // When: ProcessKeyDown('C') is called
            // Then: Should return true (recognized) and cycle the color scheme

            Viewport viewport;
            viewport.Resize (1920.0f, 1080.0f);

            DensityController densityController (viewport, 24.0f);
            InMemorySettingsProvider settingsProvider;
            ApplicationState appState (settingsProvider);
            appState.Initialize (nullptr);

            InputSystem inputSystem;
            inputSystem.Initialize (densityController, appState);

            // Default color scheme is Green; cycling goes to Blue
            Assert::AreEqual (static_cast<int> (ColorScheme::Green), static_cast<int> (appState.GetColorScheme()), L"Initial scheme should be Green");

            bool handled = inputSystem.ProcessKeyDown ('C');

            Assert::IsTrue (handled, L"'C' key should be recognized");
            Assert::AreEqual (static_cast<int> (ColorScheme::Blue), static_cast<int> (appState.GetColorScheme()), L"Color scheme should cycle to Blue");
        }





        // Test InputSystem keyboard event processing for 'S' (statistics toggle)
        TEST_METHOD (TestInputSystemProcessesVK_S)
        {
            // Given: InputSystem is initialized with ApplicationState
            // When: ProcessKeyDown('S') is called
            // Then: Should return true (recognized) and toggle statistics display

            Viewport viewport;
            viewport.Resize (1920.0f, 1080.0f);

            DensityController densityController (viewport, 24.0f);
            InMemorySettingsProvider settingsProvider;
            ApplicationState appState (settingsProvider);
            appState.Initialize (nullptr);

            InputSystem inputSystem;
            inputSystem.Initialize (densityController, appState);

            // Statistics should be off initially
            Assert::IsFalse (appState.GetShowStatistics(), L"Initial statistics should be off");

            bool handled = inputSystem.ProcessKeyDown ('S');

            Assert::IsTrue (handled, L"'S' key should be recognized");
            Assert::IsTrue (appState.GetShowStatistics(), L"Statistics should be toggled on");

            // Toggle again - should turn off
            handled = inputSystem.ProcessKeyDown ('S');

            Assert::IsTrue (handled, L"'S' key should be recognized on second press");
            Assert::IsFalse (appState.GetShowStatistics(), L"Statistics should be toggled back off");
        }
    };
}  // namespace MatrixRainTests

