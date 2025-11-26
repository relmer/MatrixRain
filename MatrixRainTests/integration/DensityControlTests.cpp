#include "pch.h"

#include "MatrixRain/DensityController.h"
#include "MatrixRain/InputSystem.h"
#include "MatrixRain/AnimationSystem.h"
#include "MatrixRain/ApplicationState.h"
#include "MatrixRain/Viewport.h"


using namespace MatrixRain;
namespace MatrixRainTests
{
	TEST_CLASS(DensityControlTests)
	{
	public:
		
		TEST_METHOD(TestDensityControlIntegration)
		{
			// T115: Integration test for density control
			// 
			// Setup: Create DensityController, InputSystem, AnimationSystem, Viewport
			// Test flow:
			//   1. Initialize systems at default 80% (48 streaks for 1920px viewport)
			//   2. Press VK_ADD to increase density to 85% (51 streaks)
			//   3. Verify ShouldSpawnStreak returns true when below target
			//   4. Press VK_SUBTRACT to decrease density to 80% (48 streaks)
			//   5. Verify density controller responds to input correctly
			
			// Create viewport and systems
			Viewport viewport;
			viewport.Resize(1920.0f, 1080.0f); // Max = 120 streaks
			
			DensityController densityController(viewport, 32.0f); // 80% of 120 = 96 streaks
			ApplicationState appState;
			
			InputSystem inputSystem;
			inputSystem.Initialize(densityController, appState);
			
			AnimationSystem animationSystem;
			animationSystem.Initialize(viewport, densityController);
			
			// Step 1: Verify initial state (80%)
			Assert::AreEqual(80, densityController.GetPercentage(), L"Initial percentage should be 80");
			Assert::AreEqual(96, densityController.GetTargetStreakCount(), L"Initial target should be 96 streaks (80% of 120)");
			
			// Step 2: Increase density (VK_ADD)
			inputSystem.ProcessKeyDown(VK_ADD);
			Assert::AreEqual(85, densityController.GetPercentage(), L"After VK_ADD, percentage should be 85");
			Assert::AreEqual(102, densityController.GetTargetStreakCount(), L"After VK_ADD, target should be 102 streaks (85% of 120)");
			
			// Step 3: Verify spawn logic at 85%
			// With 0 active streaks, should spawn
			Assert::IsTrue(densityController.ShouldSpawnStreak(0), L"Should spawn when active count (0) < target (102)");
			// With 10 active streaks, should still spawn
			Assert::IsTrue(densityController.ShouldSpawnStreak(10), L"Should spawn when active count (10) < target (102)");
			// With 102 active streaks, should not spawn
			Assert::IsFalse(densityController.ShouldSpawnStreak(102), L"Should not spawn when active count (102) >= target (102)");
			// With 120 active streaks, should not spawn
			Assert::IsFalse(densityController.ShouldSpawnStreak(120), L"Should not spawn when active count (120) > target (102)");
			
			// Step 4: Decrease density (VK_SUBTRACT)
			inputSystem.ProcessKeyDown(VK_SUBTRACT);
			Assert::AreEqual(80, densityController.GetPercentage(), L"After VK_SUBTRACT, percentage should be 80");
			Assert::AreEqual(96, densityController.GetTargetStreakCount(), L"After VK_SUBTRACT, target should be 96 streaks (80% of 120)");
			
			// Step 5: Verify spawn logic at 80%
			Assert::IsTrue(densityController.ShouldSpawnStreak(10), L"Should spawn when active count (10) < target (96)");
			Assert::IsFalse(densityController.ShouldSpawnStreak(96), L"Should not spawn when active count (96) >= target (96)");
			
			// Step 6: Test other keyboard keys (VK_OEM_PLUS, VK_OEM_MINUS)
			inputSystem.ProcessKeyDown(VK_OEM_PLUS); // 80% → 85%
			Assert::AreEqual(85, densityController.GetPercentage(), L"VK_OEM_PLUS should increase to 85%");
			Assert::AreEqual(102, densityController.GetTargetStreakCount(), L"VK_OEM_PLUS should give 102 streaks (85% of 120)");
			
			inputSystem.ProcessKeyDown(VK_OEM_MINUS); // 85% → 80%
			Assert::AreEqual(80, densityController.GetPercentage(), L"VK_OEM_MINUS should decrease to 80%");
			Assert::AreEqual(96, densityController.GetTargetStreakCount(), L"VK_OEM_MINUS should give 96 streaks (80% of 120)");
			
			// Step 7: Test bounds enforcement
			// Increase to max (100%)
			for (int i = 0; i < 10; i++)
			{
				inputSystem.ProcessKeyDown(VK_ADD);
			}
			Assert::AreEqual(100, densityController.GetPercentage(), L"Max percentage should be 100");
			Assert::AreEqual(120, densityController.GetTargetStreakCount(), L"Max percentage (100%) should give 120 streaks");
			
			// Try to exceed max
			inputSystem.ProcessKeyDown(VK_ADD);
			Assert::AreEqual(100, densityController.GetPercentage(), L"Should remain at max percentage (100%)");
			Assert::AreEqual(120, densityController.GetTargetStreakCount(), L"Should remain at max streaks (120)");
			
			// Decrease to min (0%)
			for (int i = 0; i < 25; i++)
			{
				inputSystem.ProcessKeyDown(VK_SUBTRACT);
			}
			Assert::AreEqual(0, densityController.GetPercentage(), L"Min percentage should be 0");
			Assert::AreEqual(1, densityController.GetTargetStreakCount(), L"Min percentage (0%) should still give 1 streak");
			
			// Try to go below min
			inputSystem.ProcessKeyDown(VK_SUBTRACT);
			Assert::AreEqual(0, densityController.GetPercentage(), L"Should remain at min percentage (0%)");
			Assert::AreEqual(1, densityController.GetTargetStreakCount(), L"Should remain at min streak count (1)");
		}
	};
}

