#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\HdrDisplayInventory.h"





namespace MatrixRainTests
{


    //  T043: what the settings dialog's HDR row shows for each combination of
    //  monitors and HDR mode (contracts/settings-ui.md).
    TEST_CLASS (HdrDisplayInventoryTests)
    {
        public:

            TEST_METHOD (NoHdrMonitor_GraysTheRow_AndSaysSo)
            {
                const HdrControlsState state = DescribeHdrControls (HdrDisplayInventory { 0, 0 }, HdrMode::Auto);



                Assert::IsFalse (state.rowEnabled);
                Assert::IsFalse (state.sliderEnabled);
                Assert::IsNotNull (state.disabledReason);
                Assert::AreEqual (std::wstring (L"No monitor has HDR turned on in Windows."), std::wstring (state.disabledReason));
            }




            TEST_METHOD (CapableMonitorWithHdrOff_GraysTheRow_AndSaysWhereToTurnItOn)
            {
                const HdrControlsState state = DescribeHdrControls (HdrDisplayInventory { 0, 1 }, HdrMode::Auto);



                Assert::IsFalse   (state.rowEnabled);
                Assert::IsNotNull (state.disabledReason);
                Assert::IsTrue    (std::wstring (state.disabledReason).find (L"Settings > System > Display") != std::wstring::npos,
                                   L"The tip must say where the switch is");
            }




            TEST_METHOD (HdrMonitor_EnablesTheRow_AndTheSliderInAuto)
            {
                const HdrControlsState state = DescribeHdrControls (HdrDisplayInventory { 1, 0 }, HdrMode::Auto);



                Assert::IsTrue (state.rowEnabled);
                Assert::IsTrue (state.sliderEnabled);
                Assert::IsNull (state.disabledReason);
            }




            TEST_METHOD (HdrMonitor_GraysOnlyTheSlider_WhenModeIsOff)
            {
                const HdrControlsState state = DescribeHdrControls (HdrDisplayInventory { 1, 1 }, HdrMode::Off);



                Assert::IsTrue  (state.rowEnabled,    L"The mode combo stays usable so Off can be undone");
                Assert::IsFalse (state.sliderEnabled, L"Highlight brightness does nothing with HDR mode Off");
            }
    };
}
