#include "pch.h"


#include "MatrixRain/CharacterInstance.h"


namespace MatrixRainTests
{


    TEST_CLASS (CharacterLifecycleTests)
    {
        public:

            TEST_METHOD (CharacterInstance_NeverExceedsLifetime)
            {
                // Given: Character with specific lifetime
                CharacterInstance character;
                character.lifetime = 5.0f;
                character.fadeTime = 3.0f;
                character.brightness = 1.0f;

                // When: Update for exactly lifetime duration
                character.Update (5.0f);

                // Then: Brightness should be 0, lifetime should be 0 (not negative)
                Assert::AreEqual (0.0f, character.brightness, 0.001f, L"Brightness should be exactly 0 after lifetime expires");
                Assert::AreEqual (0.0f, character.lifetime, 0.001f, L"Lifetime should be clamped at 0, not negative");

                // When: Update beyond lifetime
                character.lifetime = 5.0f;
                character.Update (10.0f);

                // Then: Should still be clamped at 0
                Assert::AreEqual (0.0f, character.brightness, 0.001f, L"Brightness should remain 0");
                Assert::AreEqual (0.0f, character.lifetime, 0.001f, L"Lifetime should remain 0");
            }






            TEST_METHOD (CharacterInstance_BrightPhaseCorrectDuration)
            {
                // Given: Character with brightTime = 2.0s, fadeTime = 3.0s
                CharacterInstance character;
                character.fadeTime = 3.0f;
                character.lifetime = 5.0f;  // 2.0s bright + 3.0s fade

                // When: Update for 1.0s (still in bright phase)
                character.Update (1.0f);

                // Then: Should be at full brightness
                Assert::AreEqual (1.0f, character.brightness, 0.001f, L"Should be full brightness during bright phase");
                Assert::AreEqual (4.0f, character.lifetime, 0.001f, L"Lifetime should decrease");

                // When: Update for another 1.0s (still bright, at edge)
                character.Update (1.0f);

                // Then: Should still be full brightness (lifetime = fadeTime exactly)
                Assert::AreEqual (1.0f, character.brightness, 0.001f, L"Should be full brightness when lifetime == fadeTime");
                Assert::AreEqual (3.0f, character.lifetime, 0.001f);
            }






            TEST_METHOD (CharacterInstance_FadePhaseLinearDecay)
            {
                // Given: Character in fade phase (lifetime <= fadeTime)
                CharacterInstance character;
                character.fadeTime = 3.0f;
                character.lifetime = 3.0f;  // Exactly at start of fade

                // When: Update for 0.75s (25% into fade)
                character.Update (0.75f);

                // Then: Brightness should be 0.75 (2.25 / 3.0)
                float expectedBrightness = 2.25f / 3.0f;
                Assert::AreEqual (expectedBrightness, character.brightness, 0.001f, L"Brightness should decay linearly");
                Assert::AreEqual (2.25f, character.lifetime, 0.001f);

                // When: Update for another 1.5s (75% into fade total)
                character.Update (1.5f);

                // Then: Brightness should be 0.25 (0.75 / 3.0)
                expectedBrightness = 0.75f / 3.0f;
                Assert::AreEqual (expectedBrightness, character.brightness, 0.001f);
                Assert::AreEqual (0.75f, character.lifetime, 0.001f);

                // When: Update to completion
                character.Update (0.75f);

                // Then: Should be fully faded
                Assert::AreEqual (0.0f, character.brightness, 0.001f);
                Assert::AreEqual (0.0f, character.lifetime, 0.001f);
            }






            TEST_METHOD (CharacterInstance_BrightnessNeverNegative)
            {
                // Given: Character at various lifecycle stages
                CharacterInstance character;
                character.fadeTime = 3.0f;

                // Test 1: Bright phase
                character.lifetime = 10.0f;
                character.Update (0.1f);
                Assert::IsTrue (character.brightness >= 0.0f && character.brightness <= 1.0f, L"Brightness in valid range during bright phase");

                // Test 2: Fade phase
                character.lifetime = 1.5f;
                character.Update (0.1f);
                Assert::IsTrue (character.brightness >= 0.0f && character.brightness <= 1.0f, L"Brightness in valid range during fade phase");

                // Test 3: After expiration
                character.lifetime = 0.1f;
                character.Update (1.0f);
                Assert::IsTrue (character.brightness >= 0.0f && character.brightness <= 1.0f, L"Brightness in valid range after expiration");
            }






            TEST_METHOD (CharacterInstance_AccumulatedUpdatesMatchSingleUpdate)
            {
                // Given: Two identical characters
                CharacterInstance char1, char2;
                char1.lifetime = char2.lifetime = 5.0f;
                char1.fadeTime = char2.fadeTime = 3.0f;

                // When: Update one with accumulated small steps, other with single step
                float totalTime = 2.5f;
                char1.Update (0.5f);
                char1.Update (0.5f);
                char1.Update (0.5f);
                char1.Update (0.5f);
                char1.Update (0.5f);

                char2.Update (totalTime);

                // Then: Should have same brightness and lifetime
                Assert::AreEqual (char2.brightness, char1.brightness, 0.001f, L"Accumulated updates should match single update");
                Assert::AreEqual (char2.lifetime, char1.lifetime, 0.001f, L"Lifetime should be consistent");
            }






            TEST_METHOD (CharacterInstance_ZeroLifetimeAlwaysDead)
            {
                // Given: Character with zero lifetime
                CharacterInstance character;
                character.lifetime = 0.0f;
                character.fadeTime = 3.0f;

                // When: Update
                character.Update (0.0f);

                // Then: Should be dead
                Assert::AreEqual (0.0f, character.brightness, L"Zero lifetime should mean dead");

                // When: Try to update further
                character.Update (1.0f);

                // Then: Should remain dead
                Assert::AreEqual (0.0f, character.brightness, L"Should stay dead");
            }
    };



}  // namespace MatrixRainTests

