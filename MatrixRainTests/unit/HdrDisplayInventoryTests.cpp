#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\HdrDisplayInventory.h"





namespace MatrixRainTests
{


    //  T043: what the settings dialog's HDR row shows for each combination of
    //  monitors and HDR mode (contracts/settings-ui.md).
    TEST_CLASS (HdrDisplayInventoryTests)
    {
        public:

            TEST_METHOD (NoHdrMonitor_GraysTheControls_AndSaysSo)
            {
                const HdrControlsState state = DescribeHdrControls (HdrDisplayInventory { 0, 0 }, HdrMode::Auto);



                Assert::IsFalse   (state.modeEnabled);
                Assert::IsFalse   (state.sliderEnabled);
                Assert::IsNotNull (state.statusText);
                Assert::IsFalse   (state.showSettingsLink, L"Nothing to turn on");
            }




            TEST_METHOD (CapableMonitorWithHdrOff_OffersTheSettingsLink)
            {
                const HdrControlsState state = DescribeHdrControls (HdrDisplayInventory { 0, 1 }, HdrMode::Auto);



                Assert::IsFalse   (state.modeEnabled);
                Assert::IsNotNull (state.statusText);
                Assert::IsTrue    (state.showSettingsLink, L"HDR can be turned on, so offer the way there");
            }




            TEST_METHOD (HdrMonitor_EnablesEverything_AndShowsNoStatus)
            {
                const HdrControlsState state = DescribeHdrControls (HdrDisplayInventory { 1, 0 }, HdrMode::Auto);



                Assert::IsTrue  (state.modeEnabled);
                Assert::IsTrue  (state.sliderEnabled);
                Assert::IsNull  (state.statusText);
                Assert::IsFalse (state.showSettingsLink);
            }




            TEST_METHOD (HdrMonitor_GraysOnlyTheSlider_WhenModeIsOff)
            {
                const HdrControlsState state = DescribeHdrControls (HdrDisplayInventory { 1, 1 }, HdrMode::Off);



                Assert::IsTrue  (state.modeEnabled,   L"The mode combo stays usable so Off can be undone");
                Assert::IsFalse (state.sliderEnabled, L"Highlight brightness does nothing with HDR mode Off");
                Assert::IsNull  (state.statusText,    L"One monitor already has HDR on; nothing to report");
            }
    };
}
