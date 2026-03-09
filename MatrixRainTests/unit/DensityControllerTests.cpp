#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\DensityController.h"
#include "..\..\MatrixRainCore\Viewport.h"





namespace MatrixRainTests
{
    TEST_CLASS (DensityControllerTests)
    {
    public:

		TEST_METHOD (TestDensityControllerInitializationWithDefaultPercentage)
		{
			// Test DensityController initialization with 80% default
			// For a 1920px viewport: max = 1920 / 24 * 4 = 320 streaks
			// At 80%: target = 320 * 0.80 = 256 streaks

			Viewport viewport;
			viewport.Resize (1920.0f, 1080.0f);

			DensityController controller(viewport, 24.0f);

			Assert::AreEqual (80, controller.GetPercentage(), L"Default percentage should be 80");
			Assert::AreEqual (320, controller.GetMaxPossibleStreaks(), L"Max possible streaks for 1920px should be 320");
			Assert::AreEqual (256, controller.GetTargetStreakCount(), L"Target streak count at 80% should be 256");
		}





		TEST_METHOD (TestDensityControllerIncreasesToMax100Percent)
		{
			// Test DensityController percentage increase to max 100%
			// Step size is 5%, so 80% → 85% → 90% → 95% → 100%

			Viewport viewport;
			viewport.Resize (1920.0f, 1080.0f);

			DensityController controller(viewport, 24.0f);

			// Increase from 80% to 100% (4 steps of 5%)
			controller.IncreaseLevel(); // 85%
			controller.IncreaseLevel(); // 90%
			controller.IncreaseLevel(); // 95%
			controller.IncreaseLevel(); // 100%

			Assert::AreEqual (100, controller.GetPercentage(), L"Percentage should be at max 100");
			Assert::AreEqual (320, controller.GetTargetStreakCount(), L"Target at 100% should equal max possible (320)");

			// Try to increase beyond max
			controller.IncreaseLevel();
			controller.IncreaseLevel();

			Assert::AreEqual (100, controller.GetPercentage(), L"Percentage should remain at max 100");
			Assert::AreEqual (320, controller.GetTargetStreakCount(), L"Target should remain at max (320)");
		}





    	TEST_METHOD (TestDensityControllerDecreasesToMin0PercentButKeepsMinStreak)
    	{
    		// Test DensityController percentage decrease to min 0%
    		// Even at 0%, should maintain minimum of 1 streak

    		Viewport viewport;
    		viewport.Resize (1920.0f, 1080.0f);

    		DensityController controller(viewport, 24.0f);

    		// Decrease from 80% to 0% (16 steps of 5%)
    		for (int i = 0; i < 16; ++i)
    		{
				controller.DecreaseLevel();
    		}

    		Assert::AreEqual (0, controller.GetPercentage(), L"Percentage should be at min 0");
    		Assert::AreEqual (1, controller.GetTargetStreakCount(), L"Target at 0% should be minimum 1 streak");

    		// Try to decrease beyond min
    		controller.DecreaseLevel();
    		controller.DecreaseLevel();

    		Assert::AreEqual (0, controller.GetPercentage(), L"Percentage should remain at min 0");
    		Assert::AreEqual (1, controller.GetTargetStreakCount(), L"Target should remain at minimum 1");
    	}






    	TEST_METHOD (TestDensityControllerSpawnProbability)
    	{
    		// Test DensityController spawn probability calculation

    		Viewport viewport;
    		viewport.Resize (1920.0f, 1080.0f);

    		DensityController controller(viewport, 24.0f);

    		int target = controller.GetTargetStreakCount(); // 256 for 80% at 1920px

    		// Below target: should spawn
    		Assert::IsTrue (controller.ShouldSpawnStreak (0), L"Should spawn when count is 0");
    		Assert::IsTrue (controller.ShouldSpawnStreak (10), L"Should spawn when below target");
    		Assert::IsTrue (controller.ShouldSpawnStreak (target - 1), L"Should spawn when just below target");

    		// At or above target: should not spawn
    		Assert::IsFalse (controller.ShouldSpawnStreak (target), L"Should not spawn when at target");
    		Assert::IsFalse (controller.ShouldSpawnStreak (target + 1), L"Should not spawn when above target");
    		Assert::IsFalse (controller.ShouldSpawnStreak (target + 100), L"Should not spawn when well above target");
    	}






		TEST_METHOD (TestDensityControllerBoundsEnforcement)
		{
			// Test DensityController bounds enforcement

			Viewport viewport;
			viewport.Resize (1920.0f, 1080.0f);

			DensityController controller(viewport, 24.0f);

			// Test max bound enforcement
			for (int i = 0; i < 30; ++i) // Try to go way beyond max
			{
				controller.IncreaseLevel();
			}

			Assert::AreEqual (100, controller.GetPercentage(), L"Percentage should be clamped at max 100");
			Assert::AreEqual (320, controller.GetTargetStreakCount(), L"Target count should match 100% of max");

    		// Test min bound enforcement
    		for (int i = 0; i < 30; ++i) // Try to go way beyond min
    		{
				controller.DecreaseLevel();
    		}

    		Assert::AreEqual (0, controller.GetPercentage(), L"Percentage should be clamped at min 0");
    		Assert::AreEqual (1, controller.GetTargetStreakCount(), L"Target count should be minimum 1 streak");
    	}





		TEST_METHOD (TestDensityControllerScalesWithViewportWidth)
		{
			// Test that max streaks scales with viewport width

			// Small viewport: 640px
			Viewport smallViewport;
			smallViewport.Resize (640.0f, 480.0f);
			DensityController smallController(smallViewport, 24.0f);

			// Max = 640 / 24 * 4 = 106 streaks (truncated from 106.667)
			Assert::AreEqual (106, smallController.GetMaxPossibleStreaks(), L"Max for 640px should be 106");

			// Medium viewport: 1920px
			Viewport mediumViewport;
			mediumViewport.Resize (1920.0f, 1080.0f);
			DensityController mediumController(mediumViewport, 24.0f);

			// Max = 1920 / 24 * 4 = 320 streaks
			Assert::AreEqual (320, mediumController.GetMaxPossibleStreaks(), L"Max for 1920px should be 320");

			// Large viewport: 3840px (4K)
			Viewport largeViewport;
			largeViewport.Resize (3840.0f, 2160.0f);
			DensityController largeController(largeViewport, 24.0f);

			// Max = 3840 / 24 * 4 = 640 streaks
			Assert::AreEqual (640, largeController.GetMaxPossibleStreaks(), L"Max for 3840px should be 640");
		}
    };
}  // namespace MatrixRainTests

