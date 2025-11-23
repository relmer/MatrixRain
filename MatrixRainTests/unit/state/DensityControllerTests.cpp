#include "pch.h"


#include "MatrixRain/DensityController.h"


using namespace MatrixRain;
namespace MatrixRainTests
{
	TEST_CLASS(DensityControllerTests)
	{
	public:
		
		TEST_METHOD(TestDensityControllerInitializationWithLevel5Default)
		{
			// T092: Test DensityController initialization with level 5 default
			// Formula: targetStreaks = 10 + (level-1)*54
			// Level 5: 10 + (5-1)*54 = 10 + 4*54 = 10 + 216 = 226
			
			DensityController controller;
			controller.Initialize();
			
			Assert::AreEqual(5, controller.GetLevel(), L"Default level should be 5");
			Assert::AreEqual(226, controller.GetTargetStreakCount(), L"Target streak count for level 5 should be 226");
		}

		TEST_METHOD(TestDensityControllerIncreasesToMax10)
		{
			// T093: Test DensityController level increase to max 10
			// Formula: targetStreaks = 10 + (level-1)*54
			// Level 10: 10 + (10-1)*54 = 10 + 9*54 = 10 + 486 = 496
			
			DensityController controller;
			controller.Initialize();
			
			// Increase from level 5 to 10 (5 increments)
			for (int i = 0; i < 5; ++i)
			{
				controller.IncreaseLevel();
			}
			
			Assert::AreEqual(10, controller.GetLevel(), L"Level should be at max 10");
			Assert::AreEqual(496, controller.GetTargetStreakCount(), L"Target streak count for level 10 should be 496");
			
			// Try to increase beyond max
			controller.IncreaseLevel();
			controller.IncreaseLevel();
			
			Assert::AreEqual(10, controller.GetLevel(), L"Level should remain at max 10");
			Assert::AreEqual(496, controller.GetTargetStreakCount(), L"Target streak count should remain 496");
		}

		TEST_METHOD(TestDensityControllerDecreasesToMin1)
		{
			// T094: Test DensityController level decrease to min 1
			// Formula: targetStreaks = 10 + (level-1)*54
			// Level 1: 10 + (1-1)*54 = 10 + 0 = 10
			
			DensityController controller;
			controller.Initialize();
			
			// Decrease from level 5 to 1 (4 decrements)
			for (int i = 0; i < 4; ++i)
			{
				controller.DecreaseLevel();
			}
			
			Assert::AreEqual(1, controller.GetLevel(), L"Level should be at min 1");
			Assert::AreEqual(10, controller.GetTargetStreakCount(), L"Target streak count for level 1 should be 10");
			
			// Try to decrease beyond min
			controller.DecreaseLevel();
			controller.DecreaseLevel();
			
			Assert::AreEqual(1, controller.GetLevel(), L"Level should remain at min 1");
			Assert::AreEqual(10, controller.GetTargetStreakCount(), L"Target streak count should remain 10");
		}

		TEST_METHOD(TestDensityControllerSpawnProbability)
		{
			// T095: Test DensityController spawn probability calculation
			
			DensityController controller;
			controller.Initialize();
			
			int target = controller.GetTargetStreakCount(); // 226 for level 5
			
			// Below target: should spawn
			Assert::IsTrue(controller.ShouldSpawnStreak(0), L"Should spawn when count is 0");
			Assert::IsTrue(controller.ShouldSpawnStreak(100), L"Should spawn when below target");
			Assert::IsTrue(controller.ShouldSpawnStreak(target - 1), L"Should spawn when just below target");
			
			// At or above target: should not spawn
			Assert::IsFalse(controller.ShouldSpawnStreak(target), L"Should not spawn when at target");
			Assert::IsFalse(controller.ShouldSpawnStreak(target + 1), L"Should not spawn when above target");
			Assert::IsFalse(controller.ShouldSpawnStreak(500), L"Should not spawn when well above target");
		}

		TEST_METHOD(TestDensityControllerBoundsEnforcement)
		{
			// T096: Test DensityController bounds enforcement
			
			DensityController controller;
			controller.Initialize();
			
			// Test max bound enforcement
			for (int i = 0; i < 20; ++i) // Try to go way beyond max
			{
				controller.IncreaseLevel();
			}
			Assert::AreEqual(10, controller.GetLevel(), L"Level should be clamped at max 10");
			Assert::AreEqual(496, controller.GetTargetStreakCount(), L"Target count should match max level");
			
			// Test min bound enforcement
			for (int i = 0; i < 20; ++i) // Try to go way beyond min
			{
				controller.DecreaseLevel();
			}
			Assert::AreEqual(1, controller.GetLevel(), L"Level should be clamped at min 1");
			Assert::AreEqual(10, controller.GetTargetStreakCount(), L"Target count should match min level");
		}
	};
}
