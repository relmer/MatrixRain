#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\OutputModeSelection.h"





namespace MatrixRainTests
{


    //  T022 (US2): every row of the SelectOutputMode table in
    //  contracts/color-math.md.
    TEST_CLASS (OutputModeSelectionTests)
    {
        public:

            TEST_METHOD (HdrOff_IsSdr_WhateverElseIsTrue)
            {
                Assert::IsTrue (OutputMode::Sdr == SelectOutputMode (false, true,  ScreenSaverMode::Normal));
                Assert::IsTrue (OutputMode::Sdr == SelectOutputMode (false, false, ScreenSaverMode::Normal));
                Assert::IsTrue (OutputMode::Sdr == SelectOutputMode (false, true,  ScreenSaverMode::ScreenSaverFull));
                Assert::IsTrue (OutputMode::Sdr == SelectOutputMode (false, true,  ScreenSaverMode::ScreenSaverPreview));
            }




            TEST_METHOD (HdrOn_ButScRgbUnsupported_IsSdr)
            {
                Assert::IsTrue (OutputMode::Sdr == SelectOutputMode (true, false, ScreenSaverMode::Normal));
                Assert::IsTrue (OutputMode::Sdr == SelectOutputMode (true, false, ScreenSaverMode::ScreenSaverFull));
            }




            TEST_METHOD (HdrOn_AndSupported_InThePreviewWindow_IsSdr)
            {
                // FR-017: the small preview inside the Windows dialog never presents in HDR.
                Assert::IsTrue (OutputMode::Sdr == SelectOutputMode (true, true, ScreenSaverMode::ScreenSaverPreview));
            }




            TEST_METHOD (HdrOn_AndSupported_InEveryOtherMode_IsHdr)
            {
                Assert::IsTrue (OutputMode::Hdr == SelectOutputMode (true, true, ScreenSaverMode::Normal));
                Assert::IsTrue (OutputMode::Hdr == SelectOutputMode (true, true, ScreenSaverMode::ScreenSaverFull));
                Assert::IsTrue (OutputMode::Hdr == SelectOutputMode (true, true, ScreenSaverMode::SettingsDialog));
                Assert::IsTrue (OutputMode::Hdr == SelectOutputMode (true, true, ScreenSaverMode::HelpRequested));
            }
    };
}
