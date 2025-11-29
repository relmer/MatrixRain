#include "pch.h"


#include "MatrixRain/ApplicationState.h"


namespace MatrixRainTests
{


    TEST_CLASS (ApplicationStateTests)
    {
        public:
            // T116: Test ApplicationState display mode initialization to Windowed
            TEST_METHOD (TestApplicationStateInitializesToWindowed)
            {
                ApplicationState appState;
                appState.Initialize();

                DisplayMode mode = appState.GetDisplayMode();
                Assert::AreEqual (static_cast<int>(DisplayMode::Windowed), static_cast<int>(mode),
                    L"ApplicationState should initialize to Windowed display mode");
            }

            // T117: Test ApplicationState ToggleDisplayMode transition Windowed→Fullscreen
            TEST_METHOD (TestToggleDisplayModeWindowedToFullscreen)
            {
                ApplicationState appState;
                appState.Initialize();

                // Verify starts in windowed
                Assert::AreEqual (static_cast<int>(DisplayMode::Windowed), 
                    static_cast<int>(appState.GetDisplayMode()),
                    L"Should start in Windowed mode");

                // Toggle to fullscreen
                appState.ToggleDisplayMode();

                Assert::AreEqual (static_cast<int>(DisplayMode::Fullscreen),
                    static_cast<int>(appState.GetDisplayMode()),
                    L"Should transition to Fullscreen mode after toggle");
            }

            // T118: Test ApplicationState ToggleDisplayMode transition Fullscreen→Windowed
            TEST_METHOD (TestToggleDisplayModeFullscreenToWindowed)
            {
                ApplicationState appState;
                appState.Initialize();

                // Toggle to fullscreen
                appState.ToggleDisplayMode();
                Assert::AreEqual (static_cast<int>(DisplayMode::Fullscreen),
                    static_cast<int>(appState.GetDisplayMode()),
                    L"Should be in Fullscreen after first toggle");

                // Toggle back to windowed
                appState.ToggleDisplayMode();
                Assert::AreEqual (static_cast<int>(DisplayMode::Windowed),
                    static_cast<int>(appState.GetDisplayMode()),
                    L"Should transition back to Windowed mode after second toggle");
            }
    };



}  // namespace MatrixRainTests

