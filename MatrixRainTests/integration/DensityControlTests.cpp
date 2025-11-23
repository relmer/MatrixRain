#include "pch.h"

#include "MatrixRain/DensityController.h"
#include "MatrixRain/InputSystem.h"
#include "MatrixRain/AnimationSystem.h"
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
			//   1. Initialize systems at default level 5 (226 streaks target)
			//   2. Press VK_ADD to increase density to level 6 (280 streaks)
			//   3. Verify ShouldSpawnStreak returns true when below target
			//   4. Press VK_SUBTRACT to decrease density to level 5 (226 streaks)
			//   5. Verify density controller responds to input correctly
			
			// Create systems
			DensityController densityController;
			densityController.Initialize(); // Level 5 → 226 streaks
			
			InputSystem inputSystem;
			inputSystem.Initialize(densityController);
			
			Viewport viewport;
			viewport.Resize(1280.0f, 720.0f);
			
			AnimationSystem animationSystem;
			animationSystem.Initialize(viewport, densityController);
			
			// Step 1: Verify initial state (level 5)
			Assert::AreEqual(226, densityController.GetTargetStreakCount(), L"Initial target should be 226 streaks (level 5)");
			
			// Step 2: Increase density (VK_ADD)
			inputSystem.ProcessKeyDown(VK_ADD);
			Assert::AreEqual(280, densityController.GetTargetStreakCount(), L"After VK_ADD, target should be 280 streaks (level 6)");
			
			// Step 3: Verify spawn logic at level 6
			// With 0 active streaks, should spawn
			Assert::IsTrue(densityController.ShouldSpawnStreak(0), L"Should spawn when active count (0) < target (280)");
			// With 100 active streaks, should still spawn
			Assert::IsTrue(densityController.ShouldSpawnStreak(100), L"Should spawn when active count (100) < target (280)");
			// With 280 active streaks, should not spawn
			Assert::IsFalse(densityController.ShouldSpawnStreak(280), L"Should not spawn when active count (280) >= target (280)");
			// With 300 active streaks, should not spawn
			Assert::IsFalse(densityController.ShouldSpawnStreak(300), L"Should not spawn when active count (300) > target (280)");
			
			// Step 4: Decrease density (VK_SUBTRACT)
			inputSystem.ProcessKeyDown(VK_SUBTRACT);
			Assert::AreEqual(226, densityController.GetTargetStreakCount(), L"After VK_SUBTRACT, target should be 226 streaks (level 5)");
			
			// Step 5: Verify spawn logic at level 5
			Assert::IsTrue(densityController.ShouldSpawnStreak(100), L"Should spawn when active count (100) < target (226)");
			Assert::IsFalse(densityController.ShouldSpawnStreak(226), L"Should not spawn when active count (226) >= target (226)");
			
			// Step 6: Test other keyboard keys (VK_OEM_PLUS, VK_OEM_MINUS)
			inputSystem.ProcessKeyDown(VK_OEM_PLUS); // Level 5 → 6
			Assert::AreEqual(280, densityController.GetTargetStreakCount(), L"VK_OEM_PLUS should increase to level 6 (280 streaks)");
			
			inputSystem.ProcessKeyDown(VK_OEM_MINUS); // Level 6 → 5
			Assert::AreEqual(226, densityController.GetTargetStreakCount(), L"VK_OEM_MINUS should decrease to level 5 (226 streaks)");
			
			// Step 7: Test bounds enforcement
			// Increase to max (level 10)
			for (int i = 0; i < 10; i++)
			{
				inputSystem.ProcessKeyDown(VK_ADD);
			}
			Assert::AreEqual(496, densityController.GetTargetStreakCount(), L"Max level 10 should give 496 streaks");
			
			// Try to exceed max
			inputSystem.ProcessKeyDown(VK_ADD);
			Assert::AreEqual(496, densityController.GetTargetStreakCount(), L"Should remain at max level 10 (496 streaks)");
			
			// Decrease to min (level 1)
			for (int i = 0; i < 15; i++)
			{
				inputSystem.ProcessKeyDown(VK_SUBTRACT);
			}
			Assert::AreEqual(10, densityController.GetTargetStreakCount(), L"Min level 1 should give 10 streaks");
			
			// Try to go below min
			inputSystem.ProcessKeyDown(VK_SUBTRACT);
			Assert::AreEqual(10, densityController.GetTargetStreakCount(), L"Should remain at min level 1 (10 streaks)");
		}
	};
}
