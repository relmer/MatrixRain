#include "pch.h"

#include "MatrixRain/CharacterStreak.h"
#include "MatrixRain/AnimationSystem.h"
#include "MatrixRain/Viewport.h"
#include "MatrixRain/DensityController.h"


using namespace MatrixRain;
namespace MatrixRainTests
{
    TEST_CLASS(StreakLifecycleTests)
    {
    public:
        
        TEST_METHOD(Streak_CharactersFadeInOrder_TailFirst)
        {
            // Given: A streak with multiple characters that has reached bottom
            CharacterStreak streak;
            Vector3 startPos(100.0f, 0.0f, 50.0f);
            streak.Spawn(startPos);
            
            // Simulate streak growing by updating until it has several characters
            float viewportHeight = 1000.0f;
            for (int i = 0; i < 10; ++i)
            {
                streak.Update(0.3f, viewportHeight);  // Drop interval
            }
            
            size_t initialCount = streak.GetCharacters().size();
            Assert::IsTrue(initialCount >= 5, L"Streak should have multiple characters");
            
            // Force streak to bottom so fading begins
            Vector3 bottomPos(100.0f, viewportHeight + 100.0f, 50.0f);
            streak.SetPosition(bottomPos);
            streak.Update(0.3f, viewportHeight);  // Trigger fading phase
            
            // Record initial lifetimes
            const auto& chars = streak.GetCharacters();
            std::vector<float> initialLifetimes;
            for (const auto& ch : chars)
            {
                initialLifetimes.push_back(ch.lifetime);
            }
            
            // Verify tail (index 0) has LESS lifetime than characters further back
            for (size_t i = 1; i < initialLifetimes.size(); ++i)
            {
                Assert::IsTrue(initialLifetimes[0] < initialLifetimes[i], 
                    L"Tail character should have less lifetime than characters further back");
            }
            
            // When: Update several times
            for (int i = 0; i < 20; ++i)
            {
                streak.Update(0.2f, viewportHeight);
            }
            
            // Then: Streak should have fewer characters (tail faded)
            size_t finalCount = streak.GetCharacters().size();
            Assert::IsTrue(finalCount < initialCount, L"Some characters should have faded from tail");
        }

        TEST_METHOD(Streak_NoSynchronousFading_AtHighDensity)
        {
            // Given: Multiple streaks at various stages
            std::vector<CharacterStreak> streaks;
            for (int i = 0; i < 50; ++i)
            {
                CharacterStreak streak;
                Vector3 pos(static_cast<float>(i * 30), static_cast<float>(i * 10), 50.0f);
                streak.Spawn(pos);
                streaks.push_back(streak);
            }
            
            float viewportHeight = 1080.0f;
            
            // Let them all grow
            for (int frame = 0; frame < 30; ++frame)
            {
                for (auto& streak : streaks)
                {
                    streak.Update(0.016f, viewportHeight);
                }
            }
            
            // Force some to bottom
            for (size_t i = 0; i < streaks.size(); i += 2)
            {
                Vector3 pos = streaks[i].GetPosition();
                pos.y = viewportHeight + 50.0f;
                streaks[i].SetPosition(pos);
            }
            
            // Record character counts
            std::vector<size_t> countsBeforeFade;
            for (const auto& streak : streaks)
            {
                countsBeforeFade.push_back(streak.GetCharacters().size());
            }
            
            // When: Update for fading period
            for (int frame = 0; frame < 200; ++frame)  // ~3.2 seconds at 60fps
            {
                for (auto& streak : streaks)
                {
                    streak.Update(0.016f, viewportHeight);
                }
            }
            
            // Then: Streaks should NOT all have the same character count
            // (if they're fading synchronously, they'd all drop to 0 at once)
            std::set<size_t> uniqueCounts;
            for (const auto& streak : streaks)
            {
                uniqueCounts.insert(streak.GetCharacters().size());
            }
            
            // Should have variety in character counts (not all the same)
            Assert::IsTrue(uniqueCounts.size() > 5, L"Character counts should vary (not synchronous fading)");
        }

        TEST_METHOD(Streak_FullLifecycle_SpawnToComplete)
        {
            // Given: A new streak
            CharacterStreak streak;
            Vector3 startPos(500.0f, -100.0f, 50.0f);
            streak.Spawn(startPos);
            
            float viewportHeight = 1080.0f;
            int updateCount = 0;
            
            // Phase 1: Growing (should add characters)
            while (streak.GetPosition().y < viewportHeight && updateCount < 500)
            {
                size_t beforeCount = streak.GetCharacters().size();
                streak.Update(0.3f, viewportHeight);
                size_t afterCount = streak.GetCharacters().size();
                
                // Should be adding characters
                Assert::IsTrue(afterCount >= beforeCount, L"Should be growing or stable, not shrinking");
                updateCount++;
            }
            
            Assert::IsTrue(streak.GetCharacters().size() > 0, L"Should have characters after growing");
            
            // Phase 2: Fading (should remove characters from tail)
            size_t peakCount = streak.GetCharacters().size();
            updateCount = 0;
            
            while (!streak.GetCharacters().empty() && updateCount < 500)
            {
                streak.Update(0.2f, viewportHeight);
                updateCount++;
            }
            
            // Then: Should eventually despawn
            Assert::IsTrue(streak.ShouldDespawn(viewportHeight), L"Streak should despawn when all characters fade");
            Assert::AreEqual(size_t(0), streak.GetCharacters().size(), L"Should have no characters left");
        }

        TEST_METHOD(AnimationSystem_StreaksDespawnAndRespawn)
        {
            // Given: AnimationSystem with density controller
            Viewport viewport;
            viewport.Resize(1920.0f, 1080.0f);
            
            DensityController densityController(viewport, 32.0f);
            densityController.SetPercentage(20);  // Low density for testing
            
            AnimationSystem animationSystem;
            animationSystem.Initialize(viewport, densityController);
            
            // When: Run for enough time for streaks to complete lifecycle
            for (int i = 0; i < 2000; ++i)  // ~33 seconds at 60fps
            {
                animationSystem.Update(0.016f);
            }
            
            // Then: Should still have streaks (they should respawn)
            Assert::IsTrue(animationSystem.GetActiveStreakCount() > 0, L"Should maintain streak count through despawn/respawn");
        }

        TEST_METHOD(Streak_LengthWithinExpectedRange)
        {
            // Given: Many streaks
            std::vector<CharacterStreak> streaks;
            for (int i = 0; i < 100; ++i)
            {
                CharacterStreak streak;
                Vector3 pos(static_cast<float>(i * 10), 0.0f, 50.0f);
                streak.Spawn(pos);
                streaks.push_back(streak);
            }
            
            // Let them grow fully
            float viewportHeight = 2000.0f;  // Large viewport
            for (int frame = 0; frame < 200; ++frame)
            {
                for (auto& streak : streaks)
                {
                    streak.Update(0.016f, viewportHeight);
                }
            }
            
            // Then: All streaks should be within min/max length
            // Based on CharacterStreak::MIN_LENGTH and MAX_LENGTH
            for (const auto& streak : streaks)
            {
                size_t len = streak.GetCharacters().size();
                Assert::IsTrue(len >= 20 && len <= 40, L"Streak length should be within MIN/MAX bounds");
            }
        }

        TEST_METHOD(Streak_CharactersNeverNegativeBrightness)
        {
            // Given: A streak that we'll run through its full lifecycle
            CharacterStreak streak;
            Vector3 startPos(500.0f, 0.0f, 50.0f);
            streak.Spawn(startPos);
            
            float viewportHeight = 1080.0f;
            
            // When: Update through entire lifecycle
            for (int i = 0; i < 1000; ++i)
            {
                streak.Update(0.016f, viewportHeight);
                
                // Then: All characters should have valid brightness at all times
                for (const auto& character : streak.GetCharacters())
                {
                    Assert::IsTrue(character.brightness >= 0.0f && character.brightness <= 1.0f,
                        L"Character brightness must be in [0, 1] range");
                }
            }
        }
    };
}




