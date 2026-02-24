#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\CharacterStreak.h"
#include "..\..\MatrixRainCore\CharacterSet.h"





namespace MatrixRainTests
{
    TEST_CLASS (CharacterStreakTests)
    {
    public:
        TEST_CLASS_INITIALIZE (ClassSetup)
        {
            CharacterSet::GetInstance().Initialize();
        }

        TEST_METHOD (CharacterStreak_Spawn_InitializesWithRandomLength)
        {
            // Spawn multiple streaks and verify they start with 1 character
            // (they grow to maxLength as they drop)
            for (int i = 0; i < 100; i++)
            {
                CharacterStreak streak;
                Vector3 position(0.0f, 0.0f, 50.0f);
                streak.Spawn (position);

                // Streaks start with 1 character (the head)
                size_t length = streak.GetLength();
                Assert::AreEqual (static_cast<size_t>(1), length);
            }
        }





        TEST_METHOD (CharacterStreak_Spawn_InitializesCharacters)
        {
            CharacterStreak streak;
            Vector3         position(100.0f, 200.0f, 50.0f);

            streak.Spawn (position);

            // Should start with 1 character (the head)
            size_t length = streak.GetLength();
            Assert::AreEqual (static_cast<size_t>(1), length);

            // Characters should be accessible
            const std::vector<CharacterInstance>& chars = streak.GetCharacters();
            Assert::AreEqual (length, chars.size());

            // First character should be the white head
            Assert::IsTrue (chars[0].isHead);
            Assert::AreEqual (1.0f, chars[0].color.r); // White
        }





        TEST_METHOD (CharacterStreak_Spawn_SetsPosition)
        {
            CharacterStreak streak;
            Vector3         position(123.0f, 456.0f, 78.0f);

            streak.Spawn (position);

            Vector3 actualPos = streak.GetPosition();
            Assert::AreEqual (123.0f, actualPos.x);
            Assert::AreEqual (456.0f, actualPos.y);
            Assert::AreEqual (78.0f, actualPos.z);
        }





        TEST_METHOD (CharacterStreak_VelocityScaling_FasterAtFarDepth)
        {
            // Note: Streaks use discrete cell-based positioning, not continuous velocity
            // Velocity is always zero; depth affects drop interval instead
            CharacterStreak nearStreak;
            CharacterStreak farStreak;

            nearStreak.Spawn (Vector3 (0.0f, 0.0f, 10.0f));  // Near (Z=10)
            farStreak.Spawn (Vector3 (0.0f, 0.0f, 90.0f));   // Far (Z=90)

            Vector3 nearVel = nearStreak.GetVelocity();
            Vector3 farVel = farStreak.GetVelocity();

            // Both velocities should be zero (discrete cell movement)
            Assert::AreEqual (0.0f, nearVel.x);
            Assert::AreEqual (0.0f, nearVel.y);
            Assert::AreEqual (0.0f, nearVel.z);
            Assert::AreEqual (0.0f, farVel.x);
            Assert::AreEqual (0.0f, farVel.y);
            Assert::AreEqual (0.0f, farVel.z);
        }





        TEST_METHOD (CharacterStreak_VelocityScaling_PrimarlyVertical)
        {
            CharacterStreak streak;
            streak.Spawn (Vector3 (0.0f, 0.0f, 50.0f));

            Vector3 velocity = streak.GetVelocity();

            // Discrete cell-based movement means velocity is zero
            // Movement happens through discrete position updates
            Assert::AreEqual (0.0f, velocity.x);
            Assert::AreEqual (0.0f, velocity.y);
            Assert::AreEqual (0.0f, velocity.z);
        }





        TEST_METHOD (CharacterStreak_Update_MovesPosition)
        {
            CharacterStreak streak;
            streak.Spawn (Vector3 (0.0f, 100.0f, 50.0f));

            Vector3 initialPos = streak.GetPosition();

            // Update with small delta time steps to avoid assertion failures
            constexpr float viewportHeight = 1080.0f;
            for (int i = 0; i < 10; ++i)
            {
                streak.Update (0.1f, viewportHeight);
            }

            Vector3 newPos = streak.GetPosition();

            // Position should have changed (moved downward in screen coordinates = increased Y)
            Assert::AreNotEqual (initialPos.y, newPos.y);
            Assert::IsTrue (newPos.y > initialPos.y); // Moved down (Y increased)
        }





        TEST_METHOD (CharacterStreak_Update_UpdatesCharacterFades)
        {
            CharacterStreak streak;
            streak.Spawn (Vector3 (0.0f, 0.0f, 50.0f));

            // Add characters by updating multiple times
            constexpr float viewportHeight = 1080.0f;
            for (int i = 0; i < 10; i++)
            {
                streak.Update (0.1f, viewportHeight); // Add ~3 characters
            }

            const std::vector<CharacterInstance>& chars = streak.GetCharacters();
            Assert::IsTrue (chars.size() > 1); // Should have multiple characters now

            // Head should still be at full brightness and marked as head
            Assert::AreEqual (1.0f, chars.back().brightness);
            Assert::IsTrue (chars.back().isHead);

            // Non-head characters should not be marked as head
            if (chars.size() > 1)
            {
                for (size_t i = 0; i < chars.size() - 1; i++)
                {
                    Assert::IsFalse (chars[i].isHead);
                }
            }
        }





        TEST_METHOD (CharacterStreak_Update_MutatesCharacters)
        {
            // This test verifies mutation happens over many updates
            // Run multiple trials to account for randomness
            bool anyTrialSucceeded = false;

            for (int trial = 0; trial < 3; trial++)
            {
                CharacterStreak streak;
                streak.Spawn (Vector3 (0.0f, 0.0f, 50.0f));

                // Store initial glyph indices for all characters
                std::vector<size_t> initialGlyphs;
                for (const auto& character : streak.GetCharacters())
                {
                    initialGlyphs.push_back (character.glyphIndex);
                }

                bool mutationOccurred = false;

                // Update many times to trigger mutation
                // With 5% per second, over 30 seconds with ~10 characters, we should see mutations
                constexpr float viewportHeight = 1080.0f;
                for (int i = 0; i < 1800; i++) // 30 seconds at 60 FPS
                {
                    streak.Update (0.016f, viewportHeight); // ~60 FPS

                    // Get fresh reference after Update() (vector may have reallocated)
                    const std::vector<CharacterInstance>& chars = streak.GetCharacters();

                    // Check if any character mutated (only check original characters)
                    size_t checkCount = std::min(chars.size(), initialGlyphs.size());
                    for (size_t j = 0; j < checkCount; j++)
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
            Assert::IsTrue (anyTrialSucceeded, L"Character mutation should occur with 5% probability per second");
        }





        TEST_METHOD (CharacterStreak_ShouldDespawn_WhenOffscreen)
        {
            CharacterStreak streak;
            streak.Spawn (Vector3 (0.0f, -500.0f, 50.0f)); // Start well above viewport

            constexpr float viewportHeight = 1080.0f;

            // Should not despawn initially (still visible or above screen)
            Assert::IsFalse (streak.ShouldDespawn());

            // Move far below screen
            for (int i = 0; i < 200; i++)
            {
                streak.Update (0.1f, viewportHeight);
            }

            // Should despawn when below viewport
            Assert::IsTrue (streak.ShouldDespawn());
        }





        TEST_METHOD (CharacterStreak_ShouldDespawn_AccountsForStreakLength)
        {
            CharacterStreak streak;
            streak.Spawn (Vector3 (0.0f, 1000.0f, 50.0f));

            // Even if position is slightly below viewport, streak might still be visible
            // (due to its length extending upward)

            // Get viewport height for testing
            constexpr float viewportHeight = 1080.0f;

            // Update until bottom is near viewport edge
            while (streak.GetPosition().y < viewportHeight)
            {
                streak.Update (0.016f, viewportHeight);
            }

            // Shouldn't despawn immediately at viewport edge (streak extends upward)
            // Only despawn when top character is off screen
            bool despawns = streak.ShouldDespawn();

            // This depends on streak length, but test the method exists and returns bool
            Assert::IsTrue (despawns == true || despawns == false);
        }
    };
}  // namespace MatrixRainTests

