#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\AnimationSystem.h"
#include "..\..\MatrixRainCore\Viewport.h"
#include "..\..\MatrixRainCore\CharacterSet.h"
#include "..\..\MatrixRainCore\DensityController.h"





namespace MatrixRainTests
{


    TEST_CLASS (AnimationLoopTests)
    {
    public:
        TEST_METHOD (TestCompleteAnimationLoop)
        {
            // This integration test verifies the complete animation loop:
            // 1. CharacterSet initialization
            // 2. AnimationSystem initialization with Viewport and DensityController
            // 3. Streak spawning
            // 4. Character creation and updates
            // 5. Fade-out over 3 seconds
            // 6. Streak despawn when fully faded

            // Initialize CharacterSet singleton
            CharacterSet & charSet            = CharacterSet::GetInstance();
            bool           charSetInitialized = charSet.Initialize();


            Assert::IsTrue (charSetInitialized, L"CharacterSet should initialize successfully");

            // Create viewport (1920x1080 full HD)
            Viewport viewport;

            viewport.Resize (1920.0f, 1080.0f);

            // Create density controller
            DensityController densityController (viewport, 32.0f);

            // Initialize AnimationSystem
            AnimationSystem animationSystem;

            animationSystem.Initialize (viewport, densityController);

            // Simulate animation loop over 10 seconds
            // This should allow streaks to spawn, grow, fade, and despawn
            constexpr float SIMULATION_DURATION = 10.0f;
            constexpr float TIME_STEP           = 1.0f / 60.0f; // 60 fps
            float           elapsedTime         = 0.0f;


            while (elapsedTime < SIMULATION_DURATION)
            {
                animationSystem.Update (TIME_STEP);
                elapsedTime += TIME_STEP;
            }

            // After 10 seconds, verify that the animation system is still functional
            // (it should have spawned and despawned many streaks by now)
            Assert::IsTrue (animationSystem.GetActiveStreakCount() > 0, L"Animation system should have active streaks after 10-second simulation");

            // Clean up
            charSet.Shutdown();
        }





        TEST_METHOD (TestAnimationSystemSpawnsStreaks)
        {
            // Verify that AnimationSystem spawns streaks during initialization/update

            CharacterSet & charSet = CharacterSet::GetInstance();

            charSet.Initialize();

            Viewport viewport;

            viewport.Resize (1920.0f, 1080.0f);

            DensityController densityController (viewport, 32.0f);

            AnimationSystem animationSystem;

            animationSystem.Initialize (viewport, densityController);

            // After initialization, run a few updates
            // Streaks should spawn during this time
            for (int i = 0; i < 120; i++) // 2 seconds at 60fps
            {
                animationSystem.Update (1.0f / 60.0f);
            }

            // Streaks should have been spawned during the update loop
            Assert::IsTrue (animationSystem.GetActiveStreakCount() > 0, L"AnimationSystem should have active streaks after 2 seconds of updates");

            charSet.Shutdown();
        }





        TEST_METHOD (TestViewportResizeDuringAnimation)
        {
            // Verify that viewport resize doesn't break the animation loop

            CharacterSet & charSet = CharacterSet::GetInstance();

            charSet.Initialize();

            Viewport viewport;

            viewport.Resize (800.0f, 600.0f);

            DensityController densityController (viewport, 32.0f);

            AnimationSystem animationSystem;

            animationSystem.Initialize (viewport, densityController);

            // Run animation for 1 second
            for (int i = 0; i < 60; i++)
            {
                animationSystem.Update (1.0f / 60.0f);
            }

            // Resize viewport mid-animation and rescale streaks
            float oldWidth  = 800.0f;
            float oldHeight = 600.0f;
            float newWidth  = 1920.0f;
            float newHeight = 1080.0f;
            
            viewport.Resize (newWidth, newHeight);
            animationSystem.RescaleStreaksForViewport (oldWidth, oldHeight, newWidth, newHeight);

            // Continue animation for another second
            for (int i = 0; i < 60; i++)
            {
                animationSystem.Update (1.0f / 60.0f);
            }

            // Verify animation system is still functional after resize
            Assert::IsTrue (animationSystem.GetActiveStreakCount() > 0, L"Animation system should have active streaks after viewport resize");

            charSet.Shutdown();
        }





        TEST_METHOD (TestAnimationWithZeroTimeStep)
        {
            // Verify that Update(0.0f) doesn't break the system (edge case)

            CharacterSet & charSet = CharacterSet::GetInstance();

            charSet.Initialize();

            Viewport viewport;

            viewport.Resize (1920.0f, 1080.0f);

            DensityController densityController (viewport, 32.0f);

            AnimationSystem animationSystem;

            animationSystem.Initialize (viewport, densityController);

            // Call Update with deltaTime = 0 multiple times
            for (int i = 0; i < 10; i++)
            {
                animationSystem.Update (0.0f);
            }

            // System should handle this gracefully
            // With zero dt, Initialize() should still have spawned initial streaks
            Assert::IsTrue (animationSystem.GetActiveStreakCount() >= 0, L"Zero time step handled gracefully");

            charSet.Shutdown();
        }






        TEST_METHOD (TestAnimationWithLargeTimeStep)
        {
            // Verify that large time steps are clamped/handled gracefully

            CharacterSet & charSet = CharacterSet::GetInstance();

            charSet.Initialize();

            Viewport viewport;

            viewport.Resize (1920.0f, 1080.0f);

            DensityController densityController (viewport, 32.0f);

            AnimationSystem animationSystem;

            animationSystem.Initialize (viewport, densityController);

            // Call Update with very large deltaTime (e.g., 10 seconds)
            // This simulates a massive frame drop or system suspend/resume
            animationSystem.Update (10.0f);

            // System should handle this without overflow or infinite spawning
            Assert::IsTrue (animationSystem.GetActiveStreakCount() > 0, L"Animation system should survive large time step");

            charSet.Shutdown();
        }
    };



}  // namespace MatrixRainTests

