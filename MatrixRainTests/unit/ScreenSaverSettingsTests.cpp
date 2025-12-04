#include "pch.h"

#include "../../MatrixRainCore/ScreenSaverSettings.h"





namespace MatrixRainTests
{
    TEST_CLASS (ScreenSaverSettingsTests)
    {
        public:
            // T007: Verify default initialization for ScreenSaverSettings
            TEST_METHOD (DefaultsAreInitialized)
            {
                ScreenSaverSettings settings;



                Assert::AreEqual (1.0f, settings.m_density, 0.001f, L"density default");
                Assert::IsTrue   (settings.m_colorSchemeKey.empty(), L"colorSchemeKey default");
                Assert::AreEqual (75, settings.m_animationSpeedPercent, L"animation speed default");
                Assert::AreEqual (100, settings.m_glowIntensityPercent, L"glow intensity default");
                Assert::AreEqual (100, settings.m_glowSizePercent, L"glow size default");
                Assert::IsTrue   (settings.m_startFullscreen, L"startFullscreen default");
                Assert::IsFalse  (settings.m_showDebugStats, L"showDebugStats default");
                Assert::IsFalse  (settings.m_showFadeTimers, L"showFadeTimers default");
                Assert::IsFalse  (settings.m_lastSavedTimestamp.has_value(), L"lastSavedTimestamp default");
            }





            // T007: Ensure Clamp() enforces bounds on all values
            TEST_METHOD (ClampEnforcesBounds)
            {
                ScreenSaverSettings settings;



                settings.m_density               = -0.5f;
                settings.m_animationSpeedPercent = 500;
                settings.m_glowIntensityPercent  = -10;
                settings.m_glowSizePercent       = 400;

                settings.Clamp();

                Assert::AreEqual (ScreenSaverSettings::MIN_DENSITY, settings.m_density, 0.001f, L"density clamped lower");
                Assert::AreEqual (ScreenSaverSettings::MAX_ANIMATION_SPEED_PERCENT, settings.m_animationSpeedPercent, L"animation speed clamped upper");
                Assert::AreEqual (ScreenSaverSettings::MIN_GLOW_INTENSITY_PERCENT, settings.m_glowIntensityPercent, L"glow intensity clamped lower");
                Assert::AreEqual (ScreenSaverSettings::MAX_GLOW_SIZE_PERCENT, settings.m_glowSizePercent, L"glow size clamped upper");
            }





            // T007: Validate helper clamp functions independently
            TEST_METHOD (ClampHelpersRespectBounds)
            {
                float clampedDensityLow  = ScreenSaverSettings::ClampDensity (-1.0f);
                float clampedDensityHigh = ScreenSaverSettings::ClampDensity (2.0f);
                float clampedDensityMid  = ScreenSaverSettings::ClampDensity (0.5f);
                int   clampedPercentLow  = ScreenSaverSettings::ClampPercent (-10, 0, 10);
                int   clampedPercentHigh = ScreenSaverSettings::ClampPercent (200, 0, 10);
                int   clampedPercentMid  = ScreenSaverSettings::ClampPercent (5, 0, 10);



                Assert::AreEqual (ScreenSaverSettings::MIN_DENSITY, clampedDensityLow, 0.001f, L"density min clamp");
                Assert::AreEqual (ScreenSaverSettings::MAX_DENSITY, clampedDensityHigh, 0.001f, L"density max clamp");
                Assert::AreEqual (0.5f, clampedDensityMid, 0.001f, L"density mid value");

                Assert::AreEqual (0, clampedPercentLow, L"percent min clamp");
                Assert::AreEqual (10, clampedPercentHigh, L"percent max clamp");
                Assert::AreEqual (5, clampedPercentMid, L"percent mid value");
            }
    };
}  // namespace MatrixRainTests
