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




            TEST_METHOD (Show_CharactersInitializedToHiddenForReveal)
            {
                HelpHintOverlay overlay;


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

                    if (ch.phase != CharPhase::Settled)
                    {
                        allResolved = false;
                        break;
                    }
                }

                Assert::IsTrue (allResolved, L"All non-space characters should be Settled after large Update");
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

            TEST_METHOD (Show_WhileRevealing_ResetsToHidden)
            {
                HelpHintOverlay overlay;


                overlay.Show();

                // Partially reveal
                overlay.Update (0.1f);

                // Re-trigger
                overlay.Show();


                Assert::AreEqual (static_cast<int>(OverlayPhase::Revealing), static_cast<int>(overlay.GetPhase()), L"Should be Revealing after re-trigger");

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
                Assert::IsTrue (allHidden, L"Characters should be reset to Hidden for reveal");
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




            ////////////////////////////////////////////////////////////
            //  SetDpiScale scales layout proportionally
            ////////////////////////////////////////////////////////////

            TEST_METHOD (SetDpiScale_ScalesLayoutProportionally)
            {
                HelpHintOverlay overlay1x;
                HelpHintOverlay overlay2x;


                overlay1x.UpdateLayout (1920.0f, 1080.0f);

                overlay2x.SetDpiScale (2.0f);
                overlay2x.UpdateLayout (3840.0f, 2160.0f);

                D2D1_RECT_F rect1x = overlay1x.GetBoundingRect();
                D2D1_RECT_F rect2x = overlay2x.GetBoundingRect();

                float width1x  = rect1x.right  - rect1x.left;
                float height1x = rect1x.bottom - rect1x.top;
                float width2x  = rect2x.right  - rect2x.left;
                float height2x = rect2x.bottom - rect2x.top;

                // At 2× DPI with 2× viewport, block should be 2× larger in pixels
                Assert::IsTrue (abs (width2x  - width1x  * 2.0f) < 1.0f, L"Width should double at 2× DPI");
                Assert::IsTrue (abs (height2x - height1x * 2.0f) < 1.0f, L"Height should double at 2× DPI");

                // Both should still be centered in their respective viewports
                float centerX2x = (rect2x.left + rect2x.right) / 2.0f;
                float centerY2x = (rect2x.top + rect2x.bottom) / 2.0f;

                Assert::IsTrue (abs (centerX2x - 1920.0f) < 1.0f, L"Should be horizontally centered at 2× DPI");
                Assert::IsTrue (abs (centerY2x - 1080.0f) < 1.0f, L"Should be vertically centered at 2× DPI");
            }




            ////////////////////////////////////////////////////////////
            //  Default DPI scale is 1.0 (backward compatible)
            ////////////////////////////////////////////////////////////

            TEST_METHOD (DefaultDpiScale_IsOne)
            {
                HelpHintOverlay overlay;


                // With default dpiScale=1.0 the scaled accessors should
                // return the base constants unchanged.
                Assert::AreEqual (HelpHintOverlay::BASE_CHAR_WIDTH,  overlay.GetCharWidth(),  L"Default char width should match base");
                Assert::AreEqual (HelpHintOverlay::BASE_CHAR_HEIGHT, overlay.GetCharHeight(), L"Default char height should match base");
                Assert::AreEqual (HelpHintOverlay::BASE_PADDING,     overlay.GetPadding(),    L"Default padding should match base");
            }
    };




}  // namespace MatrixRainTests
