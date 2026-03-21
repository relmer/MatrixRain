#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\DensityController.h"
#include "..\..\MatrixRainCore\InMemorySettingsProvider.h"
#include "..\..\MatrixRainCore\InputSystem.h"
#include "..\..\MatrixRainCore\AnimationSystem.h"
#include "..\..\MatrixRainCore\ApplicationState.h"
#include "..\..\MatrixRainCore\Viewport.h"


namespace MatrixRainTests
{
    TEST_CLASS (DensityControlTests)
    {
    public:

        TEST_METHOD (TestDensityControlIntegration)
        {
            // T115: Integration test for density control
            // 
            // Setup: Create DensityController, InputSystem, AnimationSystem, Viewport
            // Test flow:
            //   1. Initialize systems at default 50% (96 streaks for 1920px viewport)
            //   2. Press VK_ADD to increase density to 55% (105 streaks)
            //   3. Verify ShouldSpawnStreak returns true when below target
            //   4. Press VK_SUBTRACT to decrease density to 50% (96 streaks)
            //   5. Verify density controller responds to input correctly

            // Create viewport and systems
            Viewport viewport;
            viewport.Resize (1920.0f, 1080.0f); // Max = 192 streaks

            DensityController densityController (viewport, 24.0f); // 50% of 192 = 96 streaks
            InMemorySettingsProvider settingsProvider;
            ApplicationState  appState (settingsProvider);
            appState.Initialize (nullptr);

            InputSystem inputSystem;
            inputSystem.Initialize (densityController, appState);

            AnimationSystem animationSystem;
            animationSystem.Initialize (viewport, densityController);

            // Step 1: Verify initial state (50%)
            Assert::AreEqual (50, densityController.GetPercentage (), L"Initial percentage should be 50");
            Assert::AreEqual (96, densityController.GetTargetStreakCount (), L"Initial target should be 96 streaks (50% of 192)");

            // Step 2: Increase density (VK_ADD)
            inputSystem.ProcessKeyDown (VK_ADD);
            Assert::AreEqual (55, densityController.GetPercentage (), L"After VK_ADD, percentage should be 55");
            Assert::AreEqual (105, densityController.GetTargetStreakCount (), L"After VK_ADD, target should be 105 streaks (55% of 192)");

            // Step 3: Verify spawn logic at 55%
            // With 0 active streaks, should spawn
            Assert::IsTrue (densityController.ShouldSpawnStreak (0), L"Should spawn when active count (0) < target (105)");
            // With 10 active streaks, should still spawn
            Assert::IsTrue (densityController.ShouldSpawnStreak (10), L"Should spawn when active count (10) < target (105)");
            // With 105 active streaks, should not spawn
            Assert::IsFalse (densityController.ShouldSpawnStreak (105), L"Should not spawn when active count (105) >= target (105)");
            // With 192 active streaks, should not spawn
            Assert::IsFalse (densityController.ShouldSpawnStreak (192), L"Should not spawn when active count (192) > target (105)");

            // Step 4: Decrease density (VK_SUBTRACT)
            inputSystem.ProcessKeyDown (VK_SUBTRACT);
            Assert::AreEqual (50, densityController.GetPercentage (), L"After VK_SUBTRACT, percentage should be 50");
            Assert::AreEqual (96, densityController.GetTargetStreakCount (), L"After VK_SUBTRACT, target should be 96 streaks (50% of 192)");

            // Step 5: Verify spawn logic at 50%
            Assert::IsTrue (densityController.ShouldSpawnStreak (10), L"Should spawn when active count (10) < target (96)");
            Assert::IsFalse (densityController.ShouldSpawnStreak (96), L"Should not spawn when active count (96) >= target (96)");

            // Step 6: Test other keyboard keys (VK_OEM_PLUS, VK_OEM_MINUS)
            inputSystem.ProcessKeyDown (VK_OEM_PLUS); // 50% → 55%
            Assert::AreEqual (55, densityController.GetPercentage (), L"VK_OEM_PLUS should increase to 55%");
            Assert::AreEqual (105, densityController.GetTargetStreakCount (), L"VK_OEM_PLUS should give 105 streaks (55% of 192)");

            inputSystem.ProcessKeyDown (VK_OEM_MINUS); // 55% → 50%
            Assert::AreEqual (50, densityController.GetPercentage (), L"VK_OEM_MINUS should decrease to 50%");
            Assert::AreEqual (96, densityController.GetTargetStreakCount (), L"VK_OEM_MINUS should give 96 streaks (50% of 192)");

            // Step 7: Test bounds enforcement
            // Increase to max (100%)
            for (int i = 0; i < 10; i++)
            {
                inputSystem.ProcessKeyDown (VK_ADD);
            }

            Assert::AreEqual (100, densityController.GetPercentage (), L"Max percentage should be 100");
            Assert::AreEqual (192, densityController.GetTargetStreakCount (), L"Max percentage (100%) should give 192 streaks");

            // Try to exceed max
            inputSystem.ProcessKeyDown (VK_ADD);
            Assert::AreEqual (100, densityController.GetPercentage (), L"Should remain at max percentage (100%)");
            Assert::AreEqual (192, densityController.GetTargetStreakCount (), L"Should remain at max streaks (192)");

            // Decrease to min (0%)
            for (int i = 0; i < 25; i++)
            {
                inputSystem.ProcessKeyDown (VK_SUBTRACT);
            }

            Assert::AreEqual (0, densityController.GetPercentage (), L"Min percentage should be 0");
            Assert::AreEqual (1, densityController.GetTargetStreakCount (), L"Min percentage (0%) should still give 1 streak");

            // Try to go below min
            inputSystem.ProcessKeyDown (VK_SUBTRACT);
            Assert::AreEqual (0, densityController.GetPercentage (), L"Should remain at min percentage (0%)");
            Assert::AreEqual (1, densityController.GetTargetStreakCount (), L"Should remain at min streak count (1)");
        }
    };
}  // namespace MatrixRainTests

