#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\HotkeyOverlay.h"





namespace MatrixRainTests
{




    TEST_CLASS (HotkeyOverlayTests)
    {
        public:




            ////////////////////////////////////////////////////////////
            //  T090: Initial state
            ////////////////////////////////////////////////////////////

            TEST_METHOD (InitialState_Hidden)
            {
                HotkeyOverlay overlay;



                Assert::IsTrue (overlay.GetPhase() == HotkeyOverlayPhase::Hidden,
                                L"Should start in Hidden phase");
                Assert::IsFalse (overlay.IsActive(), L"Should not be active initially");
            }




            TEST_METHOD (InitialState_OpacityZero)
            {
                HotkeyOverlay overlay;



                Assert::AreEqual (0.0f, overlay.GetOpacity(), L"Opacity should be 0 when hidden");
            }




            ////////////////////////////////////////////////////////////
            //  T091: Show transitions
            ////////////////////////////////////////////////////////////

            TEST_METHOD (Show_TransitionsToVisible)
            {
                HotkeyOverlay overlay;


                overlay.Show();



                Assert::IsTrue (overlay.GetPhase() == HotkeyOverlayPhase::Visible,
                                L"Should transition to Visible");
                Assert::IsTrue (overlay.IsActive(), L"Should be active after Show");
            }




            TEST_METHOD (Show_SetsOpacityToFull)
            {
                HotkeyOverlay overlay;


                overlay.Show();



                Assert::AreEqual (1.0f, overlay.GetOpacity(), L"Opacity should be 1.0 when visible");
            }




            TEST_METHOD (Show_BuildsHotkeyList)
            {
                HotkeyOverlay overlay;


                overlay.Show();

                const auto & hotkeys = overlay.GetHotkeys();



                Assert::IsFalse (hotkeys.empty(), L"Should have hotkey entries");

                // Verify some expected hotkeys are present
                bool foundSpace = false;
                bool foundEsc   = false;
                bool foundEnter = false;

                for (const auto & hk : hotkeys)
                {
                    if (hk.keyName == L"Space") foundSpace = true;
                    if (hk.keyName == L"Esc")   foundEsc   = true;
                    if (hk.keyName == L"Enter") foundEnter = true;
                }

                Assert::IsTrue (foundSpace, L"Should contain Space hotkey");
                Assert::IsTrue (foundEsc,   L"Should contain Esc hotkey");
                Assert::IsTrue (foundEnter, L"Should contain Enter hotkey");
            }




            TEST_METHOD (Show_WhileVisible_IsNoop)
            {
                HotkeyOverlay overlay;


                overlay.Show();
                overlay.Show();



                Assert::IsTrue (overlay.GetPhase() == HotkeyOverlayPhase::Visible,
                                L"Should stay in Visible phase");
                Assert::AreEqual (1.0f, overlay.GetOpacity(), L"Opacity should remain 1.0");
            }




            ////////////////////////////////////////////////////////////
            //  T092: Dismiss transitions
            ////////////////////////////////////////////////////////////

            TEST_METHOD (Dismiss_FromVisible_TransitionsToDissolving)
            {
                HotkeyOverlay overlay;


                overlay.Show();
                overlay.Dismiss();



                Assert::IsTrue (overlay.GetPhase() == HotkeyOverlayPhase::Dissolving,
                                L"Should transition to Dissolving from Visible");
            }




            TEST_METHOD (Dismiss_FromHidden_IsNoop)
            {
                HotkeyOverlay overlay;


                overlay.Dismiss();



                Assert::IsTrue (overlay.GetPhase() == HotkeyOverlayPhase::Hidden,
                                L"Should remain Hidden");
            }




            TEST_METHOD (Dismiss_FromDissolving_IsNoop)
            {
                HotkeyOverlay overlay;


                overlay.Show();
                overlay.Dismiss();
                overlay.Dismiss();



                Assert::IsTrue (overlay.GetPhase() == HotkeyOverlayPhase::Dissolving,
                                L"Should remain Dissolving");
            }




            ////////////////////////////////////////////////////////////
            //  T093: Update during dissolving
            ////////////////////////////////////////////////////////////

            TEST_METHOD (Update_Dissolving_OpacityDecreases)
            {
                HotkeyOverlay overlay;


                overlay.Show();
                overlay.Dismiss();

                float initialOpacity = overlay.GetOpacity();

                overlay.Update (0.1f);



                Assert::IsTrue (overlay.GetOpacity() < initialOpacity,
                                L"Opacity should decrease during dissolving");
            }




            TEST_METHOD (Update_Dissolving_TransitionsToHiddenWhenFaded)
            {
                HotkeyOverlay overlay;


                overlay.Show();
                overlay.Dismiss();

                // Update with enough time to fully dissolve
                overlay.Update (1.0f);



                Assert::IsTrue (overlay.GetPhase() == HotkeyOverlayPhase::Hidden,
                                L"Should transition to Hidden when fully faded");
                Assert::AreEqual (0.0f, overlay.GetOpacity(), L"Opacity should be 0 when hidden");
            }




            TEST_METHOD (Update_Visible_NoOpacityChange)
            {
                HotkeyOverlay overlay;


                overlay.Show();
                overlay.Update (0.5f);



                Assert::AreEqual (1.0f, overlay.GetOpacity(), L"Opacity should remain 1.0 when visible");
            }
    };




}  // namespace MatrixRainTests
