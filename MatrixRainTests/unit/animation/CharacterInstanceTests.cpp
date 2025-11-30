#include "pch.h"


#include "MatrixRain/CharacterInstance.h"


namespace MatrixRainTests
{
    TEST_CLASS (CharacterInstanceTests)
    {
    public:
        TEST_METHOD (CharacterInstance_DefaultConstructor_InitializesFields)
        {
            CharacterInstance instance;

            // Should initialize with default values
            Assert::AreEqual (static_cast<size_t>(0), instance.glyphIndex);
            Assert::AreEqual (1.0f, instance.brightness);
            Assert::AreEqual (1.0f, instance.scale);
            Assert::AreEqual (0.0f, instance.positionOffset.x);
            Assert::AreEqual (0.0f, instance.positionOffset.y);
        }





        TEST_METHOD (CharacterInstance_ParameterizedConstructor_SetsAllFields)
        {
            size_t  glyphIndex = 42;
            Color4  color (0.0f, 1.0f, 0.0f, 1.0f); // Green
            float   brightness = 0.8f;
            float   scale      = 1.5f;
            Vector2 offset (10.0f, 20.0f);

            CharacterInstance instance(glyphIndex, color, brightness, scale, offset);

            Assert::AreEqual (glyphIndex, instance.glyphIndex);
            Assert::AreEqual (0.0f, instance.color.r);
            Assert::AreEqual (1.0f, instance.color.g);
            Assert::AreEqual (0.0f, instance.color.b);
            Assert::AreEqual (brightness, instance.brightness);
            Assert::AreEqual (scale, instance.scale);
            Assert::AreEqual (10.0f, instance.positionOffset.x);
            Assert::AreEqual (20.0f, instance.positionOffset.y);
        }





        TEST_METHOD (CharacterInstance_Update_FadesBrightnessOverTime)
        {
            CharacterInstance instance;
            instance.brightness = 1.0f;
            instance.lifetime = 3.0f;  // In fade phase (lifetime <= fadeTime)
            instance.fadeTime = 3.0f;

            // Update with 1.5 seconds elapsed (5 steps of 0.3s)
            for (int i = 0; i < 5; ++i)
            {
                instance.Update (0.3f);
            }

            // Brightness should be approximately 0.5 (1.5 remaining / 3.0 duration)
            Assert::IsTrue (instance.brightness >= 0.4f && instance.brightness <= 0.6f);
            Assert::AreEqual (1.5f, instance.lifetime, 0.01f);
        }





        TEST_METHOD (CharacterInstance_Update_CompleteFade_ClampsBrightnessToZero)
        {
            CharacterInstance instance;
            instance.brightness = 1.0f;
            instance.lifetime = 3.0f;  // In fade phase (lifetime <= fadeTime)
            instance.fadeTime = 3.0f;

            // Update with 3.6 seconds elapsed (12 steps of 0.3s)
            for (int i = 0; i < 12; ++i)
            {
                instance.Update (0.3f);
            }

            // Brightness should be clamped to 0.0
            Assert::AreEqual (0.0f, instance.brightness);
            Assert::IsTrue (instance.lifetime <= 0.0f); // Expired
        }





        TEST_METHOD (CharacterInstance_Update_AccumulatesDeltaTime)
        {
            CharacterInstance instance;
            instance.lifetime = 3.0f;  // In fade phase (lifetime <= fadeTime)
            instance.fadeTime = 3.0f;

            instance.Update (0.3f);
            Assert::AreEqual (2.7f, instance.lifetime, 0.01f);

            instance.Update (0.3f);
            Assert::AreEqual (2.4f, instance.lifetime, 0.01f);

            instance.Update (0.3f);
            Assert::AreEqual (2.1f, instance.lifetime, 0.01f);
        }





        TEST_METHOD (CharacterInstance_Update_LinearFadeCurve)
        {
            CharacterInstance instance;
            instance.brightness = 1.0f;
            instance.lifetime = 3.0f;  // In fade phase (lifetime <= fadeTime)
            instance.fadeTime = 3.0f;

            // Test linear fade at different points
            instance.Update (0.0f);
            Assert::AreEqual (1.0f, instance.brightness, 0.01f); // t=0: lifetime=3.0, brightness=1.0

            instance.Update (0.3f);
            instance.Update (0.3f);
            instance.Update (0.15f);
            Assert::AreEqual (0.75f, instance.brightness, 0.01f); // t=0.75: lifetime=2.25, brightness=0.75

            instance.Update (0.3f);
            instance.Update (0.3f);
            instance.Update (0.15f);
            Assert::AreEqual (0.5f, instance.brightness, 0.01f); // t=1.5: lifetime=1.5, brightness=0.5

            // Complete remaining fade (1.5s = 5x 0.3s)
            for (int i = 0; i < 5; ++i)
            {
                instance.Update (0.3f);
            }

            Assert::AreEqual (0.0f, instance.brightness, 0.01f); // t=3.0: lifetime=0.0, brightness=0.0
        }





        TEST_METHOD (CharacterInstance_FadesDuration_IsThreeSeconds)
        {
            CharacterInstance instance;

            instance.brightness = 1.0f;
            instance.lifetime = 3.0f;  // In fade phase (lifetime <= fadeTime)
            instance.fadeTime = 3.0f;

            // Fade should complete at exactly 3 seconds
            // Approach 2.99s with multiple smaller steps (9x 0.3s + 0.29s = 2.99s)
            for (int i = 0; i < 9; ++i)
            {
                instance.Update (0.3f);
            }

            instance.Update (0.29f);
            Assert::IsTrue (instance.brightness > 0.0f); // Still fading

            instance.Update (0.02f); // Total: 3.01 seconds
            Assert::AreEqual (0.0f, instance.brightness, 0.01f); // Fully faded
        }
    };
}  // namespace MatrixRainTests

