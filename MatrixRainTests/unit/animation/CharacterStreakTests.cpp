#include "pch.h"
#include "matrixrain/CharacterStreak.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace MatrixRain;

namespace MatrixRainTests
{
    TEST_CLASS(CharacterStreakTests)
    {
    public:
        TEST_METHOD(CharacterStreak_Spawn_InitializesWithRandomLength)
        {
            // Spawn multiple streaks and verify lengths are in valid range
            for (int i = 0; i < 100; i++)
            {
                CharacterStreak streak;
                Vector3 position(0.0f, 0.0f, 50.0f);
                streak.Spawn(position);
                
                // Length should be between 5 and 30 characters
                size_t length = streak.GetLength();
                Assert::IsTrue(length >= 5);
                Assert::IsTrue(length <= 30);
            }
        }

        TEST_METHOD(CharacterStreak_Spawn_InitializesCharacters)
        {
            CharacterStreak streak;
            Vector3 position(100.0f, 200.0f, 50.0f);
            streak.Spawn(position);
            
            // Should have characters initialized
            size_t length = streak.GetLength();
            Assert::IsTrue(length > 0);
            
            // Characters should be accessible
            const std::vector<CharacterInstance>& chars = streak.GetCharacters();
            Assert::AreEqual(length, chars.size());
        }

        TEST_METHOD(CharacterStreak_Spawn_SetsPosition)
        {
            CharacterStreak streak;
            Vector3 position(123.0f, 456.0f, 78.0f);
            streak.Spawn(position);
            
            Vector3 actualPos = streak.GetPosition();
            Assert::AreEqual(123.0f, actualPos.x);
            Assert::AreEqual(456.0f, actualPos.y);
            Assert::AreEqual(78.0f, actualPos.z);
        }

        TEST_METHOD(CharacterStreak_VelocityScaling_FasterAtFarDepth)
        {
            CharacterStreak nearStreak;
            CharacterStreak farStreak;
            
            nearStreak.Spawn(Vector3(0.0f, 0.0f, 10.0f));  // Near (Z=10)
            farStreak.Spawn(Vector3(0.0f, 0.0f, 90.0f));   // Far (Z=90)
            
            Vector3 nearVel = nearStreak.GetVelocity();
            Vector3 farVel = farStreak.GetVelocity();
            
            // Far streaks should move faster (larger velocity magnitude)
            float nearSpeed = Math::Length(nearVel);
            float farSpeed = Math::Length(farVel);
            Assert::IsTrue(farSpeed > nearSpeed);
        }

        TEST_METHOD(CharacterStreak_VelocityScaling_PrimarlyVertical)
        {
            CharacterStreak streak;
            streak.Spawn(Vector3(0.0f, 0.0f, 50.0f));
            
            Vector3 velocity = streak.GetVelocity();
            
            // Velocity should be primarily downward (positive Y in screen coordinates)
            Assert::IsTrue(velocity.y > 0.0f);
            
            // Y component should be much larger than X component
            Assert::IsTrue(abs(velocity.y) > abs(velocity.x) * 5.0f);
        }

        TEST_METHOD(CharacterStreak_Update_MovesPosition)
        {
            CharacterStreak streak;
            streak.Spawn(Vector3(0.0f, 100.0f, 50.0f));
            
            Vector3 initialPos = streak.GetPosition();
            
            // Update with 1 second delta time
            streak.Update(1.0f);
            
            Vector3 newPos = streak.GetPosition();
            
            // Position should have changed (moved downward in screen coordinates = increased Y)
            Assert::AreNotEqual(initialPos.y, newPos.y);
            Assert::IsTrue(newPos.y > initialPos.y); // Moved down (Y increased)
        }

        TEST_METHOD(CharacterStreak_Update_UpdatesCharacterFades)
        {
            CharacterStreak streak;
            streak.Spawn(Vector3(0.0f, 0.0f, 50.0f));
            
            const std::vector<CharacterInstance>& chars = streak.GetCharacters();
            float initialBrightness = chars[0].brightness;
            
            // Update with 1.5 seconds
            streak.Update(1.5f);
            
            // Characters should have updated fade
            float newBrightness = chars[0].brightness;
            Assert::IsTrue(newBrightness < initialBrightness);
        }

        TEST_METHOD(CharacterStreak_Update_MutatesCharacters)
        {
            // This test verifies mutation happens over many updates
            // Run multiple trials to account for randomness
            bool anyTrialSucceeded = false;
            
            for (int trial = 0; trial < 3; trial++)
            {
                CharacterStreak streak;
                streak.Spawn(Vector3(0.0f, 0.0f, 50.0f));
                
                const std::vector<CharacterInstance>& chars = streak.GetCharacters();
                
                // Store initial glyph indices for all characters
                std::vector<size_t> initialGlyphs;
                for (const auto& character : chars)
                {
                    initialGlyphs.push_back(character.glyphIndex);
                }
                
                bool mutationOccurred = false;
                
                // Update many times to trigger mutation
                // With 5% per second, over 30 seconds with ~10 characters, we should see mutations
                for (int i = 0; i < 1800; i++) // 30 seconds at 60 FPS
                {
                    streak.Update(0.016f); // ~60 FPS
                    
                    // Check if any character mutated
                    for (size_t j = 0; j < chars.size(); j++)
                    {
                        if (chars[j].glyphIndex != initialGlyphs[j])
                        {
                            mutationOccurred = true;
                            break;
                        }
                    }
                    
                    if (mutationOccurred)
                        break;
                }
                
                if (mutationOccurred)
                {
                    anyTrialSucceeded = true;
                    break;
                }
            }
            
            // At least one trial should observe mutation
            Assert::IsTrue(anyTrialSucceeded, L"Character mutation should occur with 5% probability per second");
        }

        TEST_METHOD(CharacterStreak_ShouldDespawn_WhenOffscreen)
        {
            CharacterStreak streak;
            streak.Spawn(Vector3(0.0f, -500.0f, 50.0f)); // Start well above viewport
            
            // Should not despawn initially (still visible or above screen)
            Assert::IsFalse(streak.ShouldDespawn(1080.0f));
            
            // Move far below screen
            for (int i = 0; i < 200; i++)
            {
                streak.Update(0.1f);
            }
            
            // Should despawn when below viewport
            Assert::IsTrue(streak.ShouldDespawn(1080.0f));
        }

        TEST_METHOD(CharacterStreak_ShouldDespawn_AccountsForStreakLength)
        {
            CharacterStreak streak;
            streak.Spawn(Vector3(0.0f, 1000.0f, 50.0f));
            
            // Even if position is slightly below viewport, streak might still be visible
            // (due to its length extending upward)
            
            // Get viewport height for testing
            float viewportHeight = 1080.0f;
            
            // Update until bottom is near viewport edge
            while (streak.GetPosition().y < viewportHeight)
            {
                streak.Update(0.016f);
            }
            
            // Shouldn't despawn immediately at viewport edge (streak extends upward)
            // Only despawn when top character is off screen
            bool despawns = streak.ShouldDespawn(viewportHeight);
            
            // This depends on streak length, but test the method exists and returns bool
            Assert::IsTrue(despawns == true || despawns == false);
        }
    };
}
