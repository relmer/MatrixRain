#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\MultiMonitorGate.h"




namespace MatrixRainTests
{


    TEST_CLASS (MultiMonitorGateTests)
    {
        public:

            TEST_METHOD (Preview_AlwaysFalse_RegardlessOfOtherInputs)
            {
                Assert::IsFalse (ShouldSpanAllMonitors (true,  DisplayMode::Fullscreen, ScreenSaverMode::ScreenSaverPreview));
                Assert::IsFalse (ShouldSpanAllMonitors (true,  DisplayMode::Windowed,   ScreenSaverMode::ScreenSaverPreview));
                Assert::IsFalse (ShouldSpanAllMonitors (false, DisplayMode::Fullscreen, ScreenSaverMode::ScreenSaverPreview));
                Assert::IsFalse (ShouldSpanAllMonitors (false, DisplayMode::Windowed,   ScreenSaverMode::ScreenSaverPreview));
            }




            TEST_METHOD (HelpRequested_AlwaysFalse_RegardlessOfOtherInputs)
            {
                Assert::IsFalse (ShouldSpanAllMonitors (true,  DisplayMode::Fullscreen, ScreenSaverMode::HelpRequested));
                Assert::IsFalse (ShouldSpanAllMonitors (true,  DisplayMode::Windowed,   ScreenSaverMode::HelpRequested));
                Assert::IsFalse (ShouldSpanAllMonitors (false, DisplayMode::Fullscreen, ScreenSaverMode::HelpRequested));
                Assert::IsFalse (ShouldSpanAllMonitors (false, DisplayMode::Windowed,   ScreenSaverMode::HelpRequested));
            }




            TEST_METHOD (MultiMonEnabled_PlusFullscreen_PlusNormal_ReturnsTrue)
            {
                Assert::IsTrue (ShouldSpanAllMonitors (true, DisplayMode::Fullscreen, ScreenSaverMode::Normal));
                Assert::IsTrue (ShouldSpanAllMonitors (true, DisplayMode::Fullscreen, ScreenSaverMode::ScreenSaverFull));
                Assert::IsTrue (ShouldSpanAllMonitors (true, DisplayMode::Fullscreen, std::nullopt));
            }




            TEST_METHOD (MultiMonDisabled_AlwaysFalse_OutsidePreviewHelp)
            {
                Assert::IsFalse (ShouldSpanAllMonitors (false, DisplayMode::Fullscreen, ScreenSaverMode::Normal));
                Assert::IsFalse (ShouldSpanAllMonitors (false, DisplayMode::Fullscreen, ScreenSaverMode::ScreenSaverFull));
                Assert::IsFalse (ShouldSpanAllMonitors (false, DisplayMode::Fullscreen, std::nullopt));
            }




            TEST_METHOD (Windowed_AlwaysFalse_OutsidePreviewHelp)
            {
                Assert::IsFalse (ShouldSpanAllMonitors (true,  DisplayMode::Windowed, ScreenSaverMode::Normal));
                Assert::IsFalse (ShouldSpanAllMonitors (false, DisplayMode::Windowed, ScreenSaverMode::Normal));
                Assert::IsFalse (ShouldSpanAllMonitors (true,  DisplayMode::Windowed, std::nullopt));
            }
    };


}
