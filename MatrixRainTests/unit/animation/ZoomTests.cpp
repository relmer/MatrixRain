#include "pch.h"
#include "matrixrain/AnimationSystem.h"
#include "matrixrain/Viewport.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace MatrixRain;

namespace MatrixRainTests
{
	TEST_CLASS(ZoomTests)
	{
	public:
		
		TEST_METHOD(TestZoomWrappingAtZ100Boundary)
		{
			// T046: Test zoom wrapping at Z=100 boundary
			// 
			// Given: Streaks approaching Z=100 (far plane boundary)
			// When: AnimationSystem::ApplyZoom is called with deltaTime
			// Then: Z positions wrap using modulo to prevent overflow
			//
			// This ensures continuous zoom can run for hours without numerical issues
			
			// Create animation system with viewport
			Viewport viewport;
			viewport.Resize(800, 600);
			AnimationSystem animationSystem;
			animationSystem.Initialize(viewport);
			
			// Spawn a streak and manually set its position near Z=0
			animationSystem.SpawnStreak();
			const auto& streaks = animationSystem.GetStreaks();
			if (!streaks.empty())
			{
				// Manually modify the streak position for testing
				CharacterStreak* streak = const_cast<CharacterStreak*>(&streaks[0]);
				streak->SetPosition(Vector3(50.0f, -20.0f, 0.5f));
				
				Assert::AreEqual(0.5f, streaks[0].GetPosition().z, 0.01f, L"Initial Z should be 0.5");
				
				// Apply zoom that would push Z below 0
				// Default zoom velocity is 5.0 units/sec, so 0.15 seconds = 0.75 unit decrease
				animationSystem.Update(0.15f);
				
				// After zooming 0.75 unit from Z=0.5, should wrap to approximately Z=99.75
				// (0.5 - 0.75 = -0.25, wraps to 100 + (-0.25) = 99.75)
				float finalZ = animationSystem.GetStreaks()[0].GetPosition().z;
				Assert::IsTrue(finalZ > 99.0f && finalZ < 100.0f, L"Z should wrap to ~99.75 after crossing Z=0");
			}
		}

		TEST_METHOD(TestZoomContinuousMovement)
		{
			// Verify zoom continuously decreases Z position at 5.0 units/second (default)
			
			// Create animation system with viewport
			Viewport viewport;
			viewport.Resize(800, 600);
			AnimationSystem animationSystem;
			animationSystem.Initialize(viewport);
			
			// Spawn streak and set known position
			animationSystem.SpawnStreak();
			const auto& streaks = animationSystem.GetStreaks();
			if (!streaks.empty())
			{
				CharacterStreak* streak = const_cast<CharacterStreak*>(&streaks[0]);
				streak->SetPosition(Vector3(50.0f, -20.0f, 50.0f));
				
				float initialZ = streaks[0].GetPosition().z;
				Assert::AreEqual(50.0f, initialZ, 0.01f, L"Initial Z should be 50.0");
				
				// Apply zoom for 0.2 seconds (should decrease by 1.0 unit at 5.0 units/sec)
				animationSystem.Update(0.2f);
				
				// Verify Z decreased by approximately 1.0 unit
				float finalZ = animationSystem.GetStreaks()[0].GetPosition().z;
				Assert::AreEqual(49.0f, finalZ, 0.01f, L"Z should decrease by 1.0 unit (5.0 * 0.2s)");
			}
		}

		TEST_METHOD(TestZoomDoesNotAffectXY)
		{
			// Zoom should only modify Z, not X positions (Y changes due to falling)
			
			// Create animation system with viewport
			Viewport viewport;
			viewport.Resize(800, 600);
			AnimationSystem animationSystem;
			animationSystem.Initialize(viewport);
			
			// Spawn streak and set known position
			animationSystem.SpawnStreak();
			const auto& streaks = animationSystem.GetStreaks();
			if (!streaks.empty())
			{
				CharacterStreak* streak = const_cast<CharacterStreak*>(&streaks[0]);
				streak->SetPosition(Vector3(75.0f, 100.0f, 50.0f));
				
				// Record initial X position
				Vector3 initialPos = streaks[0].GetPosition();
				float initialX = initialPos.x;
				
				// Apply zoom
				animationSystem.Update(1.0f);
				
				// Verify X unchanged (Y changes due to streak velocity, Z changes due to zoom)
				Vector3 finalPos = animationSystem.GetStreaks()[0].GetPosition();
				Assert::AreEqual(initialX, finalPos.x, 0.01f, L"X position should not be affected by zoom");
				// Note: Y position changes due to streak falling velocity
			}
		}

		TEST_METHOD(TestMultipleZoomCycles)
		{
			// Test zoom over extended runtime (simulating hours)
			
			// Create animation system with viewport
			Viewport viewport;
			viewport.Resize(800, 600);
			AnimationSystem animationSystem;
			animationSystem.Initialize(viewport);
			
			// Spawn streak and set position
			animationSystem.SpawnStreak();
			const auto& streaks = animationSystem.GetStreaks();
			if (!streaks.empty())
			{
				CharacterStreak* streak = const_cast<CharacterStreak*>(&streaks[0]);
				streak->SetPosition(Vector3(50.0f, -20.0f, 50.0f));
				
				// Simulate 50 complete zoom cycles
				// At 5.0 units/sec, each cycle through 100 units takes 20 seconds
				// 50 cycles = 1000 seconds total
				for (int i = 0; i < 50; ++i)
				{
					// Update in 1-second increments for 20 seconds per cycle
					for (int j = 0; j < 20; ++j)
					{
						animationSystem.Update(1.0f);
						
						if (!animationSystem.GetStreaks().empty())
						{
							// Verify positions remain valid (no overflow, no NaN)
							Vector3 pos = animationSystem.GetStreaks()[0].GetPosition();
							Assert::IsFalse(std::isnan(pos.x), L"X should not be NaN");
							Assert::IsFalse(std::isnan(pos.y), L"Y should not be NaN");
							Assert::IsFalse(std::isnan(pos.z), L"Z should not be NaN");
							Assert::IsTrue(pos.z >= 0.0f && pos.z < 100.0f, L"Z should remain in valid range [0, 100)");
						}
					}
				}
			}
		}
	};
}
