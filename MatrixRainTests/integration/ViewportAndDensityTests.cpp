#include "pch.h"

#include "../../MatrixRainCore/AnimationSystem.h"
#include "../../MatrixRainCore/Viewport.h"
#include "../../MatrixRainCore/DensityController.h"
#include "../../MatrixRainCore/CharacterStreak.h"
#include "../../MatrixRainCore/Math.h"


namespace MatrixRainTests
{
    TEST_CLASS (ViewportRescalingTests)
    {
    public:

        TEST_METHOD (Viewport_StreaksRescaleProportionally)
        {
            // Given: AnimationSystem with streaks
            Viewport viewport;
            viewport.Resize (1440.0f, 900.0f);

            DensityController densityController (viewport, 32.0f);

            AnimationSystem animationSystem;
            animationSystem.Initialize (viewport, densityController);

            // Spawn several streaks
            for (int i = 0; i < 10; ++i)
            {
                animationSystem.SpawnStreak ();
            }

            // Record original positions
            std::vector<Vector3> originalPositions;
            for (const auto & streak : animationSystem.GetStreaks ())
            {
                originalPositions.push_back (streak.GetPosition ());
            }

            // When: Rescale viewport (simulate fullscreen)
            float oldWidth = 1440.0f;
            float oldHeight = 900.0f;
            float newWidth = 1920.0f;
            float newHeight = 1080.0f;

            animationSystem.RescaleStreaksForViewport (oldWidth, oldHeight, newWidth, newHeight);

            // Then: Positions should be scaled proportionally
            float expectedScaleX = newWidth / oldWidth;  // 1.333...
            float expectedScaleY = newHeight / oldHeight;  // 1.2

            const auto & streaks = animationSystem.GetStreaks ();
            for (size_t i = 0; i < streaks.size (); ++i)
            {
                Vector3 newPos = streaks[i].GetPosition ();
                Vector3 oldPos = originalPositions[i];

                // X should be scaled (with some jitter tolerance)
                float expectedX = oldPos.x * expectedScaleX;
                Assert::IsTrue (abs (newPos.x - expectedX) < 20.0f, L"X position should be approximately scaled");

                // Y should be scaled exactly (no jitter)
                float expectedY = oldPos.y * expectedScaleY;
                Assert::AreEqual (expectedY, newPos.y, 0.1f, L"Y position should be scaled");

                // Z should remain unchanged
                Assert::AreEqual (oldPos.z, newPos.z, L"Z depth should not change");
            }
        }





        TEST_METHOD (Viewport_StreaksStayWithinBounds_AfterRescale)
        {
            // Given: AnimationSystem with streaks at various positions
            Viewport viewport;
            viewport.Resize (1000.0f, 800.0f);

            DensityController densityController (viewport, 32.0f);

            AnimationSystem animationSystem;
            animationSystem.Initialize (viewport, densityController);

            // Spawn streaks across the viewport
            for (int i = 0; i < 20; ++i)
            {
                animationSystem.SpawnStreakInView ();  // Spawns anywhere on screen
            }

            // When: Rescale to larger viewport
            float oldWidth = 1000.0f;
            float oldHeight = 800.0f;
            float newWidth = 2000.0f;
            float newHeight = 1600.0f;

            animationSystem.RescaleStreaksForViewport (oldWidth, oldHeight, newWidth, newHeight);

            // Then: All streaks should still be within new bounds (approximately)
            for (const auto & streak : animationSystem.GetStreaks ())
            {
                Vector3 pos = streak.GetPosition ();

                // X should be within new width (with jitter tolerance)
                Assert::IsTrue (pos.x >= -50.0f && pos.x <= newWidth + 50.0f,
                    L"X should be within new viewport bounds (with jitter)");
            }
        }





        TEST_METHOD (Viewport_RescaleDown_StreaksStillValid)
        {
            // Given: AnimationSystem with fullscreen-sized viewport
            Viewport viewport;
            viewport.Resize (1920.0f, 1080.0f);

            DensityController densityController (viewport, 32.0f);

            AnimationSystem animationSystem;
            animationSystem.Initialize (viewport, densityController);

            for (int i = 0; i < 15; ++i)
            {
                animationSystem.SpawnStreak ();
            }

            // When: Rescale down to windowed mode
            float oldWidth = 1920.0f;
            float oldHeight = 1080.0f;
            float newWidth = 1440.0f;
            float newHeight = 900.0f;

            animationSystem.RescaleStreaksForViewport (oldWidth, oldHeight, newWidth, newHeight);

            // Then: Streaks should be scaled proportionally smaller
            for (const auto & streak : animationSystem.GetStreaks ())
            {
                Vector3 pos = streak.GetPosition ();

                // Positions should be within scaled-down bounds (with jitter)
                Assert::IsTrue (pos.x >= -50.0f && pos.x <= newWidth + 50.0f,
                    L"Scaled-down X should fit in smaller viewport");
            }
        }





        TEST_METHOD (Viewport_CharacterPositions_ScaleWithStreak)
        {
            // Given: A streak with multiple characters
            CharacterStreak streak;
            Vector3         startPos (500.0f, 200.0f, 50.0f);

            streak.Spawn (startPos);

            // Let it grow
            for (int i = 0; i < 10; ++i)
            {
                streak.Update (0.3f, 1000.0f);
            }

            // Record original streak head position
            Vector3 originalHeadPos = streak.GetPosition ();

            // When: Rescale
            float scaleX = 1.5f;
            float scaleY = 1.2f;
            streak.RescalePositions (scaleX, scaleY);

            // Then: Character Y positions should be recalculated based on scaled head position
            // RescalePositions recalculates character positions using: newHeadY - (distanceFromHead * spacing)
            Vector3 newHeadPos = streak.GetPosition ();
            Assert::AreEqual (originalHeadPos.y * scaleY, newHeadPos.y, 0.01f,
                L"Streak head Y position should be scaled");

            const auto & characters = streak.GetCharacters ();
            const float characterSpacing = 32.0f;

            for (size_t i = 0; i < characters.size (); ++i)
            {
                // Characters are stored tail-first, head-last
                size_t distanceFromHead = characters.size () - 1 - i;
                float expectedY = newHeadPos.y - (distanceFromHead * characterSpacing);

                Assert::AreEqual (expectedY, characters[i].positionOffset.y, 0.01f,
                    L"Character Y position should be recalculated from scaled head position");
            }
        }
    };
}  // namespace MatrixRainTests





namespace MatrixRainTests
{
    TEST_CLASS (DensityChangeTests)
    {
    public:

        TEST_METHOD (Density_Increase_SpawnsNewStreaks)
        {
            // Given: AnimationSystem at low density
            Viewport viewport;
            viewport.Resize (1920.0f, 1080.0f);

            DensityController densityController (viewport, 32.0f);
            densityController.SetPercentage (20);  // 20% = 24 streaks

            AnimationSystem animationSystem;
            animationSystem.Initialize (viewport, densityController);

            // Let it stabilize at 20%
            for (int i = 0; i < 100; ++i)
            {
                animationSystem.Update (0.016f);
            }

            size_t countAtLowDensity = animationSystem.GetActiveStreakCount ();

            // When: Increase density to 80%
            densityController.SetPercentage (80);  // 80% = 96 streaks

            // Update for a few frames to allow spawning
            for (int i = 0; i < 10; ++i)
            {
                animationSystem.Update (0.016f);
            }

            // Then: Should have more streaks
            size_t countAtHighDensity = animationSystem.GetActiveStreakCount ();
            Assert::IsTrue (countAtHighDensity > countAtLowDensity,
                L"Increasing density should add streaks");
        }





        TEST_METHOD (Density_Decrease_RemovesStreaks)
        {
            // Given: AnimationSystem at high density
            Viewport viewport;
            viewport.Resize (1920.0f, 1080.0f);

            DensityController densityController (viewport, 32.0f);
            densityController.SetPercentage (80);  // 80%

            AnimationSystem animationSystem;
            animationSystem.Initialize (viewport, densityController);

            // Let it stabilize
            for (int i = 0; i < 100; ++i)
            {
                animationSystem.Update (0.016f);
            }

            size_t countAtHighDensity = animationSystem.GetActiveStreakCount ();

            // When: Decrease density
            densityController.SetPercentage (20);  // 20%

            // Update immediately
            animationSystem.Update (0.016f);

            // Then: Should have fewer streaks
            size_t countAtLowDensity = animationSystem.GetActiveStreakCount ();
            Assert::IsTrue (countAtLowDensity < countAtHighDensity,
                L"Decreasing density should remove streaks immediately");
        }





        TEST_METHOD (Density_SpawnsBurstOnIncrease_NotGradual)
        {
            // Given: AnimationSystem at medium density
            Viewport viewport;
            viewport.Resize (1920.0f, 1080.0f);

            DensityController densityController (viewport, 32.0f);
            densityController.SetPercentage (40);

            AnimationSystem animationSystem;
            animationSystem.Initialize (viewport, densityController);

            // Stabilize
            for (int i = 0; i < 100; ++i)
            {
                animationSystem.Update (0.016f);
            }

            size_t beforeCount = animationSystem.GetActiveStreakCount ();

            // When: Large density increase
            densityController.SetPercentage (90);

            // Update ONCE
            animationSystem.Update (0.016f);
            size_t afterOneFrame = animationSystem.GetActiveStreakCount ();

            // Then: Should spawn most/all needed streaks in one frame (burst)
            size_t increase = afterOneFrame - beforeCount;
            Assert::IsTrue (increase > 30, L"Should burst spawn many streaks in single frame");
        }





        TEST_METHOD (Density_NoStreakDuplication)
        {
            // Given: AnimationSystem
            Viewport viewport;
            viewport.Resize (1920.0f, 1080.0f);

            DensityController densityController (viewport, 32.0f);
            AnimationSystem animationSystem;
            animationSystem.Initialize (viewport, densityController);

            // When: Rapidly change density up and down
            for (int cycle = 0; cycle < 10; ++cycle)
            {
                densityController.SetPercentage (80);
                animationSystem.Update (0.016f);

                densityController.SetPercentage (20);
                animationSystem.Update (0.016f);
            }

            // Then: Streak count should be reasonable (not duplicated/accumulated)
            size_t finalCount = animationSystem.GetActiveStreakCount ();
            int targetCount = densityController.GetTargetStreakCount ();

            // Should be close to target (within reasonable tolerance)
            int diff = abs (static_cast<int>(finalCount) - targetCount);
            Assert::IsTrue (diff < 50, L"Streak count should match target after density changes");
        }
    };
}  // namespace MatrixRainTests





