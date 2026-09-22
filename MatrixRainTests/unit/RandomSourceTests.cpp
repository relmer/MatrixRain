#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\AnimationSystem.h"
#include "..\..\MatrixRainCore\DensityController.h"
#include "..\..\MatrixRainCore\RandomSource.h"
#include "..\..\MatrixRainCore\Viewport.h"





namespace MatrixRainTests
{


    //  A run of the rain long enough for spawning, mutation and dropping to
    //  have all drawn from the engine, but short enough to stay a unit test.
    static constexpr uint32_t kSeed        = 42;
    static constexpr int      kStepCount   = 120;
    static constexpr float    kStepSeconds = 1.0f / 60.0f;





    ////////////////////////////////////////////////////////////////////////////
    //
    //  DrawSequence
    //
    //  A fingerprint of the engine's next count outputs.
    //
    ////////////////////////////////////////////////////////////////////////////

    static std::vector<uint32_t> DrawSequence (size_t count)
    {
        std::vector<uint32_t> draws;

        draws.reserve (count);

        for (size_t i = 0; i < count; ++i)
        {
            draws.push_back (RandomSource::Engine()());
        }

        return draws;
    }





    ////////////////////////////////////////////////////////////////////////////
    //
    //  RunAnimation
    //
    //  Reseeds, runs a fresh AnimationSystem for a fixed number of fixed-length
    //  steps, and returns a fingerprint of every streak position and glyph.
    //
    ////////////////////////////////////////////////////////////////////////////

    static std::vector<float> RunAnimation (uint32_t seed)
    {
        Viewport          viewport;
        std::vector<float> fingerprint;


        RandomSource::Reseed (seed);

        viewport.Resize (800, 600);

        DensityController densityController (viewport, 24.0f);
        AnimationSystem   animationSystem;

        animationSystem.Initialize (viewport, densityController);

        for (int step = 0; step < kStepCount; ++step)
        {
            animationSystem.Update (kStepSeconds);
        }

        for (const CharacterStreak & streak : animationSystem.GetStreaks())
        {
            fingerprint.push_back (streak.GetPosition().x);
            fingerprint.push_back (streak.GetPosition().y);
            fingerprint.push_back (streak.GetPosition().z);

            for (const CharacterInstance & character : streak.GetCharacters())
            {
                fingerprint.push_back (static_cast<float> (character.glyphIndex));
            }
        }

        return fingerprint;
    }





    TEST_CLASS (RandomSourceTests)
    {
        public:

        ////////////////////////////////////////////////////////////////////////
        // Raw engine determinism
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD (Reseed_WithSameSeed_RepeatsTheSameDraws)
        {
            RandomSource::Reseed (kSeed);
            const std::vector<uint32_t> first  = DrawSequence (32);

            RandomSource::Reseed (kSeed);
            const std::vector<uint32_t> second = DrawSequence (32);

            Assert::IsTrue (first == second,
                            L"The same seed must reproduce the same draw sequence");
        }


        TEST_METHOD (Reseed_WithDifferentSeeds_GivesDifferentDraws)
        {
            RandomSource::Reseed (kSeed);
            const std::vector<uint32_t> first  = DrawSequence (32);

            RandomSource::Reseed (kSeed + 1);
            const std::vector<uint32_t> second = DrawSequence (32);

            Assert::IsFalse (first == second,
                             L"Different seeds must not produce the same draw sequence");
        }


        TEST_METHOD (Engine_ReturnsTheSameInstanceOnAThread)
        {
            Assert::IsTrue (&RandomSource::Engine() == &RandomSource::Engine(),
                            L"A thread must draw from one engine, not a fresh one per call");
        }


        TEST_METHOD (Engine_IsPerThread)
        {
            std::mt19937 * pMainEngine  = &RandomSource::Engine();
            std::mt19937 * pOtherEngine = nullptr;

            std::thread other ([&pOtherEngine] () { pOtherEngine = &RandomSource::Engine(); });

            other.join();

            Assert::IsTrue (pMainEngine != pOtherEngine,
                            L"Each thread must own its engine so the render and UI threads never race");
        }


        TEST_METHOD (Reseed_OnOneThread_LeavesOtherThreadsAlone)
        {
            std::vector<uint32_t> otherBefore;
            std::vector<uint32_t> otherAfter;

            std::thread first ([&otherBefore] ()
                               {
                                   RandomSource::Reseed (kSeed);
                                   otherBefore = DrawSequence (8);
                               });

            first.join();

            RandomSource::Reseed (kSeed + 7);

            std::thread second ([&otherAfter] ()
                                {
                                    RandomSource::Reseed (kSeed);
                                    otherAfter = DrawSequence (8);
                                });

            second.join();

            Assert::IsTrue (otherBefore == otherAfter,
                            L"Reseeding one thread must not disturb another thread's engine");
        }


        ////////////////////////////////////////////////////////////////////////
        // The animation itself is reproducible, which is what the harness needs
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD (AnimationSystem_WithTheSameSeed_ProducesIdenticalStreaks)
        {
            const std::vector<float> first  = RunAnimation (kSeed);
            const std::vector<float> second = RunAnimation (kSeed);

            Assert::IsFalse (first.empty(),
                             L"The run must produce streaks, or it proves nothing");
            Assert::IsTrue (first == second,
                            L"Identically seeded runs must give identical positions and glyphs");
        }


        TEST_METHOD (AnimationSystem_WithDifferentSeeds_Diverges)
        {
            const std::vector<float> first  = RunAnimation (kSeed);
            const std::vector<float> second = RunAnimation (kSeed + 1);

            Assert::IsFalse (first == second,
                             L"Different seeds must give a different animation");
        }
    };


}
