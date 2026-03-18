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



                Assert::IsTrue (overlay.GetPhase() == OverlayPhase::Hidden,
                                L"Should start in Hidden phase");
                Assert::IsFalse (overlay.IsActive(), L"Should not be active initially");
            }




            TEST_METHOD (InitialState_CharactersInitialized)
            {
                HotkeyOverlay overlay;

                auto chars = overlay.GetCharacters();



                Assert::IsFalse (chars.empty(), L"Should have character data after construction");

                // All characters should start Hidden
                for (const auto & ch : chars)
                {
                    Assert::IsTrue (ch.phase == CharPhase::Hidden,
                                    L"All characters should start in Hidden phase");
                }
            }




            ////////////////////////////////////////////////////////////
            //  T091: Show transitions
            ////////////////////////////////////////////////////////////

            TEST_METHOD (Show_TransitionsToRevealing)
            {
                HotkeyOverlay overlay;


                overlay.Show();



                Assert::IsTrue (overlay.GetPhase() == OverlayPhase::Revealing,
                                L"Should transition to Revealing");
                Assert::IsTrue (overlay.IsActive(), L"Should be active after Show");
            }




            TEST_METHOD (Show_CharactersInitializedToHiddenForReveal)
            {
                HotkeyOverlay overlay;


                overlay.Show();

                auto chars = overlay.GetCharacters();
                bool allHidden = true;



                for (const auto & ch : chars)
                {
                    if (!ch.isSpace && ch.phase != CharPhase::Hidden)
                    {
                        allHidden = false;
                        break;
                    }
                }

                Assert::IsTrue (allHidden, L"Non-space characters should be Hidden (awaiting reveal) after Show");
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




            TEST_METHOD (Show_WhileRevealing_ResetsToRevealing)
            {
                HotkeyOverlay overlay;


                overlay.Show();
                overlay.Show();



                Assert::IsTrue (overlay.GetPhase() == OverlayPhase::Revealing,
                                L"Should restart in Revealing phase");
            }




            ////////////////////////////////////////////////////////////
            //  T092: Reveal completes to Holding
            ////////////////////////////////////////////////////////////

            TEST_METHOD (RevealComplete_TransitionsToHolding)
            {
                HotkeyOverlay overlay;


                overlay.Show();

                // Advance enough time for all characters to resolve
                overlay.Update (5.0f);



                Assert::IsTrue (overlay.GetPhase() == OverlayPhase::Holding,
                                L"Should transition to Holding after reveal completes");
            }




            ////////////////////////////////////////////////////////////
            //  T093: Dismiss transitions
            ////////////////////////////////////////////////////////////

            TEST_METHOD (Dismiss_FromRevealing_TransitionsToDissolving)
            {
                HotkeyOverlay overlay;


                overlay.Show();
                overlay.Dismiss();



                Assert::IsTrue (overlay.GetPhase() == OverlayPhase::Dissolving,
                                L"Should transition to Dissolving from Revealing");
            }




            TEST_METHOD (Dismiss_FromHolding_TransitionsToDissolving)
            {
                HotkeyOverlay overlay;


                overlay.Show();

                // Advance to Holding
                overlay.Update (5.0f);

                overlay.Dismiss();



                Assert::IsTrue (overlay.GetPhase() == OverlayPhase::Dissolving,
                                L"Should transition to Dissolving from Holding");
            }




            TEST_METHOD (Dismiss_FromHidden_IsNoop)
            {
                HotkeyOverlay overlay;


                overlay.Dismiss();



                Assert::IsTrue (overlay.GetPhase() == OverlayPhase::Hidden,
                                L"Should remain Hidden");
            }




            TEST_METHOD (Dismiss_FromDissolving_IsNoop)
            {
                HotkeyOverlay overlay;


                overlay.Show();
                overlay.Dismiss();
                overlay.Dismiss();



                Assert::IsTrue (overlay.GetPhase() == OverlayPhase::Dissolving,
                                L"Should remain Dissolving");
            }




            ////////////////////////////////////////////////////////////
            //  T094: Update during dissolving
            ////////////////////////////////////////////////////////////

            TEST_METHOD (Update_Dissolving_TransitionsToHiddenWhenFaded)
            {
                HotkeyOverlay overlay;


                overlay.Show();
                overlay.Dismiss();

                // Update with enough time for per-character dissolve to complete
                for (int i = 0; i < 200; i++)
                {
                    overlay.Update (0.02f);
                }



                Assert::IsTrue (overlay.GetPhase() == OverlayPhase::Hidden,
                                L"Should transition to Hidden when all characters faded");
            }




            TEST_METHOD (DissolveComplete_AllCharactersHidden)
            {
                HotkeyOverlay overlay;


                overlay.Show();
                overlay.Dismiss();

                // Update with enough time for full dissolve
                for (int i = 0; i < 200; i++)
                {
                    overlay.Update (0.02f);
                }

                auto chars = overlay.GetCharacters();



                for (const auto & ch : chars)
                {
                    if (!ch.isSpace)
                    {
                        Assert::IsTrue (ch.phase == CharPhase::Hidden,
                                        L"All non-space characters should be Hidden after dissolve");
                    }
                }
            }




            TEST_METHOD (Update_Holding_AutoDismissesAfterTimeout)
            {
                HotkeyOverlay overlay;


                overlay.Show();

                // Advance to Holding
                overlay.Update (5.0f);

                Assert::IsTrue (overlay.GetPhase() == OverlayPhase::Holding,
                                L"Should be in Holding");

                // Update past the hold duration — should auto-dismiss
                overlay.Update (5.0f);



                Assert::IsTrue (overlay.GetPhase() == OverlayPhase::Dissolving,
                                L"Hotkey overlay should auto-dismiss after hold timeout");
            }




            ////////////////////////////////////////////////////////////
            //  T095: Complete animation loop
            ////////////////////////////////////////////////////////////

            TEST_METHOD (TestCompleteAnimationLoop)
            {
                HotkeyOverlay overlay;


                // Show → Revealing
                overlay.Show();
                Assert::IsTrue (overlay.GetPhase() == OverlayPhase::Revealing);

                // Advance to Holding
                overlay.Update (5.0f);
                Assert::IsTrue (overlay.GetPhase() == OverlayPhase::Holding);

                // Dismiss → Dissolving
                overlay.Dismiss();
                Assert::IsTrue (overlay.GetPhase() == OverlayPhase::Dissolving);

                // Advance to Hidden
                for (int i = 0; i < 200; i++)
                {
                    overlay.Update (0.02f);
                }



                Assert::IsTrue (overlay.GetPhase() == OverlayPhase::Hidden,
                                L"Should complete full animation loop back to Hidden");
                Assert::IsFalse (overlay.IsActive(), L"Should not be active after full loop");
            }
    };




}  // namespace MatrixRainTests
