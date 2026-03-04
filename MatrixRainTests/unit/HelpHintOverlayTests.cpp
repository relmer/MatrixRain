#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\HelpHintOverlay.h"





namespace MatrixRainTests
{




    TEST_CLASS (HelpHintOverlayTests)
    {
        public:




            ////////////////////////////////////////////////////////////
            //  T024: Initial state — Hidden, not active, chars empty
            ////////////////////////////////////////////////////////////

            TEST_METHOD (InitialState_PhaseIsHidden)
            {
                HelpHintOverlay overlay;



                Assert::IsFalse (overlay.IsActive(), L"Should not be active initially");
                Assert::AreEqual (static_cast<int>(OverlayPhase::Hidden), static_cast<int>(overlay.GetPhase()), L"Phase should be Hidden");
            }




            TEST_METHOD (InitialState_CharactersInitialized)
            {
                HelpHintOverlay overlay;


                auto chars = overlay.GetCharacters();



                Assert::AreEqual (overlay.GetRows() * overlay.GetCols(), static_cast<int>(chars.size()), L"Character count should match rows*cols");
                Assert::IsTrue (overlay.GetRows() == 3, L"Should have 3 rows");
                Assert::IsTrue (overlay.GetCols() > 0, L"Should have positive columns");
            }




            ////////////////////////////////////////////////////////////
            //  T025: Show() transitions to Revealing
            ////////////////////////////////////////////////////////////

            TEST_METHOD (Show_TransitionsToRevealing)
            {
                HelpHintOverlay overlay;


                overlay.Show();



                Assert::IsTrue (overlay.IsActive(), L"Should be active after Show");
                Assert::AreEqual (static_cast<int>(OverlayPhase::Revealing), static_cast<int>(overlay.GetPhase()), L"Phase should be Revealing");
            }




            TEST_METHOD (Show_CharactersInitializedToScrambling)
            {
                HelpHintOverlay overlay;


                overlay.Show();


                auto chars = overlay.GetCharacters();
                bool hasScrambling = false;

                for (const auto & ch : chars)
                {
                    if (!ch.isSpace && ch.phase == CharPhase::Scrambling)
                    {
                        hasScrambling = true;
                        break;
                    }
                }

                Assert::IsTrue (hasScrambling, L"Non-space characters should be Scrambling after Show");
            }




            TEST_METHOD (Show_SpacesMarkedAsSpace)
            {
                HelpHintOverlay overlay;


                overlay.Show();


                auto chars = overlay.GetCharacters();

                for (const auto & ch : chars)
                {
                    if (ch.isSpace)
                    {
                        Assert::AreEqual (static_cast<int>(CharPhase::Hidden), static_cast<int>(ch.phase), L"Space chars should be Hidden");
                    }
                }
            }




            ////////////////////////////////////////////////////////////
            //  T026: Update during Revealing — scramble + resolve
            ////////////////////////////////////////////////////////////

            TEST_METHOD (Update_Revealing_CharactersResolve)
            {
                HelpHintOverlay overlay;


                overlay.Show();

                // Update with large delta to force all characters past their scramble timers
                overlay.Update (5.0f);


                auto chars = overlay.GetCharacters();
                bool allResolved = true;

                for (const auto & ch : chars)
                {
                    if (ch.isSpace) continue;

                    if (ch.phase != CharPhase::Resolved)
                    {
                        allResolved = false;
                        break;
                    }
                }

                Assert::IsTrue (allResolved, L"All non-space characters should be Resolved after large Update");
            }




            ////////////////////////////////////////////////////////////
            //  T027: Revealing → Holding transition
            ////////////////////////////////////////////////////////////

            TEST_METHOD (RevealComplete_TransitionsToHolding)
            {
                HelpHintOverlay overlay;


                overlay.Show();

                // Force all characters to resolve
                overlay.Update (5.0f);



                Assert::AreEqual (static_cast<int>(OverlayPhase::Holding), static_cast<int>(overlay.GetPhase()), L"Should transition to Holding after all resolved");
            }




            ////////////////////////////////////////////////////////////
            //  T028: Holding → Dissolving transition
            ////////////////////////////////////////////////////////////

            TEST_METHOD (HoldComplete_TransitionsToDissolving)
            {
                HelpHintOverlay overlay;


                overlay.Show();

                // Reveal all characters
                overlay.Update (5.0f);

                // Now in Holding — advance past hold duration (default 3.0s)
                overlay.Update (4.0f);



                Assert::AreEqual (static_cast<int>(OverlayPhase::Dissolving), static_cast<int>(overlay.GetPhase()), L"Should transition to Dissolving after hold");
            }




            ////////////////////////////////////////////////////////////
            //  T029: Dissolving → Hidden transition
            ////////////////////////////////////////////////////////////

            TEST_METHOD (DissolveComplete_TransitionsToHidden)
            {
                HelpHintOverlay overlay;


                overlay.Show();

                // Reveal
                overlay.Update (5.0f);

                // Hold
                overlay.Update (4.0f);

                // Dissolve — large time to ensure all characters finish
                overlay.Update (10.0f);



                Assert::AreEqual (static_cast<int>(OverlayPhase::Hidden), static_cast<int>(overlay.GetPhase()), L"Should transition to Hidden after dissolve");
                Assert::IsFalse (overlay.IsActive(), L"Should not be active after dissolve");
            }




            TEST_METHOD (DissolveComplete_AllCharactersHidden)
            {
                HelpHintOverlay overlay;


                overlay.Show();
                overlay.Update (5.0f);   // Reveal
                overlay.Update (4.0f);   // Hold
                overlay.Update (10.0f);  // Dissolve


                auto chars = overlay.GetCharacters();

                for (const auto & ch : chars)
                {
                    Assert::AreEqual (static_cast<int>(CharPhase::Hidden), static_cast<int>(ch.phase), L"All characters should be Hidden");
                }
            }




            ////////////////////////////////////////////////////////////
            //  T030: UpdateLayout computes centered rect
            ////////////////////////////////////////////////////////////

            TEST_METHOD (UpdateLayout_ComputesCenteredRect)
            {
                HelpHintOverlay overlay;


                overlay.UpdateLayout (1920.0f, 1080.0f);


                D2D1_RECT_F rect = overlay.GetBoundingRect();

                // Rect should be centered
                float centerX = (rect.left + rect.right) / 2.0f;
                float centerY = (rect.top + rect.bottom) / 2.0f;

                Assert::IsTrue (abs (centerX - 960.0f) < 1.0f, L"Should be horizontally centered");
                Assert::IsTrue (abs (centerY - 540.0f) < 1.0f, L"Should be vertically centered");

                Assert::IsTrue (rect.right > rect.left, L"Should have positive width");
                Assert::IsTrue (rect.bottom > rect.top, L"Should have positive height");
            }




            ////////////////////////////////////////////////////////////
            //  T031: GetCharacters returns correct layout
            ////////////////////////////////////////////////////////////

            TEST_METHOD (GetCharacters_CorrectRowColLayout)
            {
                HelpHintOverlay overlay;


                auto chars = overlay.GetCharacters();
                int  rows  = overlay.GetRows();
                int  cols  = overlay.GetCols();



                Assert::AreEqual (rows * cols, static_cast<int>(chars.size()), L"Should have rows*cols characters");

                // Verify first character of each row
                for (int r = 0; r < rows; r++)
                {
                    Assert::AreEqual (r, chars[r * cols].row, L"Row should match position");
                    Assert::AreEqual (0, chars[r * cols].col, L"First column should be 0");
                }

                // Verify last character of first row
                Assert::AreEqual (0, chars[cols - 1].row, L"Last char of row 0 should be row 0");
                Assert::AreEqual (cols - 1, chars[cols - 1].col, L"Last char should be at cols-1");
            }




            ////////////////////////////////////////////////////////////
            //  T042: Show() while Revealing resets to Scrambling
            ////////////////////////////////////////////////////////////

            TEST_METHOD (Show_WhileRevealing_ResetsToScrambling)
            {
                HelpHintOverlay overlay;


                overlay.Show();

                // Partially reveal
                overlay.Update (0.1f);

                // Re-trigger
                overlay.Show();


                Assert::AreEqual (static_cast<int>(OverlayPhase::Revealing), static_cast<int>(overlay.GetPhase()), L"Should be Revealing after re-trigger");

                auto chars = overlay.GetCharacters();
                bool hasScrambling = false;
                for (const auto & ch : chars)
                {
                    if (!ch.isSpace && ch.phase == CharPhase::Scrambling)
                    {
                        hasScrambling = true;
                        break;
                    }
                }
                Assert::IsTrue (hasScrambling, L"Characters should be reset to Scrambling");
            }




            ////////////////////////////////////////////////////////////
            //  T043: Show() while Holding resets to Revealing
            ////////////////////////////////////////////////////////////

            TEST_METHOD (Show_WhileHolding_ResetsToRevealing)
            {
                HelpHintOverlay overlay;


                overlay.Show();
                overlay.Update (5.0f);  // All resolved → Holding

                Assert::AreEqual (static_cast<int>(OverlayPhase::Holding), static_cast<int>(overlay.GetPhase()), L"Should be Holding");

                // Re-trigger
                overlay.Show();


                Assert::AreEqual (static_cast<int>(OverlayPhase::Revealing), static_cast<int>(overlay.GetPhase()), L"Should reset to Revealing");
            }




            ////////////////////////////////////////////////////////////
            //  T044: Show() while Dissolving reverses dissolve
            ////////////////////////////////////////////////////////////

            TEST_METHOD (Show_WhileDissolving_ResetsToRevealing)
            {
                HelpHintOverlay overlay;


                overlay.Show();
                overlay.Update (5.0f);  // Reveal → Holding
                overlay.Update (4.0f);  // Hold → Dissolving

                Assert::AreEqual (static_cast<int>(OverlayPhase::Dissolving), static_cast<int>(overlay.GetPhase()), L"Should be Dissolving");

                // Re-trigger
                overlay.Show();


                Assert::AreEqual (static_cast<int>(OverlayPhase::Revealing), static_cast<int>(overlay.GetPhase()), L"Should reset to Revealing from Dissolving");
            }




            ////////////////////////////////////////////////////////////
            //  T045: Dismiss() from Revealing → Dissolving
            ////////////////////////////////////////////////////////////

            TEST_METHOD (Dismiss_FromRevealing_TransitionsToDissolving)
            {
                HelpHintOverlay overlay;


                overlay.Show();

                overlay.Dismiss();


                Assert::AreEqual (static_cast<int>(OverlayPhase::Dissolving), static_cast<int>(overlay.GetPhase()), L"Dismiss from Revealing should go to Dissolving");
            }




            ////////////////////////////////////////////////////////////
            //  T046: Dismiss() from Holding → Dissolving
            ////////////////////////////////////////////////////////////

            TEST_METHOD (Dismiss_FromHolding_TransitionsToDissolving)
            {
                HelpHintOverlay overlay;


                overlay.Show();
                overlay.Update (5.0f);  // All resolved → Holding

                Assert::AreEqual (static_cast<int>(OverlayPhase::Holding), static_cast<int>(overlay.GetPhase()), L"Should be Holding");

                overlay.Dismiss();


                Assert::AreEqual (static_cast<int>(OverlayPhase::Dissolving), static_cast<int>(overlay.GetPhase()), L"Dismiss from Holding should go to Dissolving");
            }




            ////////////////////////////////////////////////////////////
            //  T047: Dismiss() from Dissolving is no-op
            ////////////////////////////////////////////////////////////

            TEST_METHOD (Dismiss_FromDissolving_IsNoOp)
            {
                HelpHintOverlay overlay;


                overlay.Show();
                overlay.Update (5.0f);  // Reveal → Holding
                overlay.Update (4.0f);  // Hold → Dissolving

                Assert::AreEqual (static_cast<int>(OverlayPhase::Dissolving), static_cast<int>(overlay.GetPhase()), L"Should be Dissolving");

                overlay.Dismiss();


                Assert::AreEqual (static_cast<int>(OverlayPhase::Dissolving), static_cast<int>(overlay.GetPhase()), L"Dismiss from Dissolving should be no-op");
            }




            ////////////////////////////////////////////////////////////
            //  T048: Dismiss() from Hidden is no-op
            ////////////////////////////////////////////////////////////

            TEST_METHOD (Dismiss_FromHidden_IsNoOp)
            {
                HelpHintOverlay overlay;


                overlay.Dismiss();


                Assert::AreEqual (static_cast<int>(OverlayPhase::Hidden), static_cast<int>(overlay.GetPhase()), L"Dismiss from Hidden should be no-op");
                Assert::IsFalse (overlay.IsActive(), L"Should remain inactive");
            }
    };




}  // namespace MatrixRainTests
