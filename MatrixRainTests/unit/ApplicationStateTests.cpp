#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\ApplicationState.h"
#include "..\..\MatrixRainCore\InMemorySettingsProvider.h"





namespace MatrixRainTests
{
    TEST_CLASS (ApplicationStateTests)
    {
    private:
        InMemorySettingsProvider m_settingsProvider;

    public:
        TEST_METHOD_INITIALIZE (MethodSetup)
        {
            m_settingsProvider.Clear();
        }

        // T116: Test ApplicationState display mode initialization to Fullscreen
        TEST_METHOD (TestApplicationStateInitializesToFullscreen)
        {
            ApplicationState appState (m_settingsProvider);
            appState.Initialize (nullptr);

            DisplayMode mode = appState.GetDisplayMode ();
            Assert::AreEqual (static_cast<int>(DisplayMode::Fullscreen), 
                              static_cast<int>(mode),
                              L"ApplicationState should initialize to Fullscreen display mode");
        }





        // T117: Test ApplicationState ToggleDisplayMode transition Fullscreen→Windowed
        TEST_METHOD (TestToggleDisplayModeFullscreenToWindowed)
        {
            ApplicationState appState (m_settingsProvider);
            appState.Initialize (nullptr);

            // Verify starts in fullscreen
            Assert::AreEqual (static_cast<int>(DisplayMode::Fullscreen),
                              static_cast<int>(appState.GetDisplayMode ()),
                              L"Should start in Fullscreen mode");

            // Toggle to windowed
            appState.ToggleDisplayMode ();

            Assert::AreEqual (static_cast<int>(DisplayMode::Windowed),
                              static_cast<int>(appState.GetDisplayMode ()),
                              L"Should transition to Windowed mode after toggle");
        }





        // T118: Test ApplicationState ToggleDisplayMode transition Windowed→Fullscreen
        TEST_METHOD (TestToggleDisplayModeWindowedToFullscreen)
        {
            ApplicationState appState (m_settingsProvider);
            appState.Initialize (nullptr);

            // Toggle to windowed
            appState.ToggleDisplayMode ();
            Assert::AreEqual (static_cast<int>(DisplayMode::Windowed),
                              static_cast<int>(appState.GetDisplayMode ()),
                              L"Should be in Windowed after first toggle");

            // Toggle back to fullscreen
            appState.ToggleDisplayMode ();
            Assert::AreEqual (static_cast<int>(DisplayMode::Fullscreen),
                              static_cast<int>(appState.GetDisplayMode ()),
                              L"Should transition back to Fullscreen mode after second toggle");
        }





        // ApplySettings is how the settings dialog's Reset to defaults and
        // Cancel reach the running rain, so it must announce every setting
        // the renderer follows live. It used to skip density, speed, glow
        // intensity, glow size and the advanced graphics values: Reset moved
        // those sliders back while the rain kept the old values.
        TEST_METHOD (ApplySettings_NotifiesEveryLiveSetting)
        {
            ApplicationState       appState (m_settingsProvider);
            ScreenSaverSettings    settings;
            int                    density      = -1;
            int                    speed        = -1;
            int                    glowIntensity = -1;
            int                    glowSize     = -1;
            AdvancedGraphicsValues advanced     = {};
            bool                   gotAdvanced  = false;



            appState.Initialize (nullptr);

            appState.RegisterDensityChangeCallback    ([&] (int value) { density       = value; });
            appState.RegisterAnimationSpeedCallback   ([&] (int value) { speed         = value; });
            appState.RegisterGlowIntensityCallback    ([&] (int value) { glowIntensity = value; });
            appState.RegisterGlowSizeCallback         ([&] (int value) { glowSize      = value; });
            appState.RegisterAdvancedGraphicsCallback ([&] (const AdvancedGraphicsValues & value) { advanced = value; gotAdvanced = true; });

            settings.m_densityPercent                = 37;
            settings.m_animationSpeedPercent         = 41;
            settings.m_glowIntensityPercent          = 150;
            settings.m_glowSizePercent               = 175;
            settings.m_advancedValues.m_blurPasses   = 2;
            settings.m_advancedValues.m_glowIntensityPercent = 150;

            appState.ApplySettings (settings);

            Assert::AreEqual (37,  density,       L"Density must reach the renderer");
            Assert::AreEqual (41,  speed,         L"Animation speed must reach the renderer");
            Assert::AreEqual (150, glowIntensity, L"Glow intensity must reach the renderer");
            Assert::AreEqual (175, glowSize,      L"Glow size must reach the renderer");
            Assert::IsTrue   (gotAdvanced,        L"Advanced graphics values must reach the renderer");
            Assert::AreEqual (2,   advanced.m_blurPasses);
        }
    };
}  // namespace MatrixRainTests

