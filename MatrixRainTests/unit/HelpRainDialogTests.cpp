#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\HelpRainDialog.h"





namespace MatrixRainTests
{




    TEST_CLASS (HelpRainDialogTests)
    {
        public:




            ////////////////////////////////////////////////////////////
            //  T067: Window sizing — ComputeWindowSize
            ////////////////////////////////////////////////////////////

            TEST_METHOD (ComputeWindowSize_ReturnsPositiveDimensions)
            {
                UsageText usage (L'/');


                SIZE size = HelpRainDialog::ComputeWindowSize (usage, 96.0f);



                Assert::IsTrue (size.cx > 0, L"Width should be positive");
                Assert::IsTrue (size.cy > 0, L"Height should be positive");
            }




            TEST_METHOD (ComputeWindowSize_AtLeastTwiceTextBoundingBox)
            {
                UsageText usage (L'/');


                // At default DPI (96), the window should be at least 2x the text content
                SIZE size = HelpRainDialog::ComputeWindowSize (usage, 96.0f);



                // The window should have substantial size since it's 2x the text bounding box
                // Text content for 5 switches + headers should produce something
                // reasonable (at least a few hundred pixels in each dimension)
                Assert::IsTrue (size.cx >= 200, L"Width should be at least 200px (2x text bounding box)");
                Assert::IsTrue (size.cy >= 200, L"Height should be at least 200px (2x text bounding box)");
            }




            TEST_METHOD (ComputeWindowSize_CappedAt80PercentWorkArea)
            {
                UsageText usage (L'/');


                // Get primary monitor work area for comparison
                RECT workArea;
                SystemParametersInfoW (SPI_GETWORKAREA, 0, &workArea, 0);

                int maxWidth  = static_cast<int> ((workArea.right - workArea.left) * 0.8f);
                int maxHeight = static_cast<int> ((workArea.bottom - workArea.top) * 0.8f);

                SIZE size = HelpRainDialog::ComputeWindowSize (usage, 96.0f);



                Assert::IsTrue (size.cx <= maxWidth, L"Width should not exceed 80% of work area width");
                Assert::IsTrue (size.cy <= maxHeight, L"Height should not exceed 80% of work area height");
            }




            ////////////////////////////////////////////////////////////
            //  T068: Phase 1 reveal animation state
            ////////////////////////////////////////////////////////////

            TEST_METHOD (Constructor_PreComputesCharacterPositions)
            {
                UsageText usage (L'/');


                HelpRainDialog dialog (usage);

                const auto & positions = dialog.GetCharacterPositions();



                // Should have one entry per non-space character
                Assert::IsFalse (positions.empty(), L"Should have pre-computed character positions");

                // Count non-space characters in formatted text
                const std::wstring & text = usage.GetFormattedText();
                size_t nonSpaceCount      = 0;

                for (wchar_t ch : text)
                {
                    if (ch != L' ' && ch != L'\n' && ch != L'\r')
                    {
                        nonSpaceCount++;
                    }
                }

                Assert::AreEqual (nonSpaceCount, positions.size(),
                                  L"Character positions count should match non-space character count");
            }




            TEST_METHOD (Constructor_AnimationSystemNullBeforeShow)
            {
                UsageText usage (L'/');


                HelpRainDialog dialog (usage);



                // AnimationSystem is created in CreateRenderPipeline (called
                // from Show), so it should be null after construction alone.
                Assert::IsNull (dialog.GetAnimationSystem(),
                                L"AnimationSystem should be null before Show()");
            }




            TEST_METHOD (Constructor_InitializesRevealedFlagsToFalse)
            {
                UsageText usage (L'/');


                HelpRainDialog dialog (usage);

                const auto & flags     = dialog.GetRevealedFlags();
                const auto & positions = dialog.GetCharacterPositions();



                Assert::AreEqual (positions.size(), flags.size(),
                                  L"Revealed flags should match character positions count");

                for (size_t i = 0; i < flags.size(); i++)
                {
                    Assert::AreEqual (0.0f, flags[i], L"All flags should start at zero opacity");
                }
            }




            TEST_METHOD (Constructor_StartsInRevealingPhase)
            {
                UsageText usage (L'/');


                HelpRainDialog dialog (usage);



                Assert::IsTrue (dialog.GetAnimationPhase() == AnimationPhase::Revealing,
                                L"Should start in Revealing phase");
            }




            TEST_METHOD (Constructor_RevealFrontStartsAboveWindow)
            {
                UsageText usage (L'/');


                HelpRainDialog dialog (usage);



                Assert::IsTrue (dialog.GetRevealFrontY() < 0.0f,
                                L"Reveal front should start above the window");
            }




            ////////////////////////////////////////////////////////////
            //  T069: IsRevealComplete
            ////////////////////////////////////////////////////////////

            TEST_METHOD (IsRevealComplete_InitiallyFalse)
            {
                UsageText usage (L'/');


                HelpRainDialog dialog (usage);



                Assert::IsFalse (dialog.IsRevealComplete(),
                                 L"Reveal should not be complete initially");
            }




            TEST_METHOD (IsRevealComplete_TrueAfterFullReveal)
            {
                UsageText usage (L'/');


                HelpRainDialog dialog (usage);

                // With one-char-per-column-per-frame, columns may need
                // multiple full sweeps to reveal all stacked characters.
                // At 150px/s and 60fps, one sweep ≈ 240 frames.  Use 3600
                // frames (~60s) for ample margin.
                for (int i = 0; i < 3600; i++)
                {
                    dialog.Update (1.0f / 60.0f);
                }



                Assert::IsTrue (dialog.IsRevealComplete(),
                                L"Reveal should be complete after simulating enough frames");
            }




            ////////////////////////////////////////////////////////////
            //  T083: Phase 2 background phase
            ////////////////////////////////////////////////////////////

            TEST_METHOD (Phase2_TransitionsAfterRevealComplete)
            {
                UsageText usage (L'/');


                HelpRainDialog dialog (usage);

                // Transition to Background happens as soon as all characters
                // are revealed (columns respawn during Revealing).  Same
                // timing as IsRevealComplete test.  Use 3600 frames (60s).
                for (int i = 0; i < 3600; i++)
                {
                    dialog.Update (1.0f / 60.0f);
                }



                Assert::IsTrue (dialog.GetAnimationPhase() == AnimationPhase::Background,
                                L"Should transition to Background phase after reveal completes");
            }




            ////////////////////////////////////////////////////////////
            //  T085: Reveal queue drain rate
            ////////////////////////////////////////////////////////////

            TEST_METHOD (RevealFrontY_AdvancesWithUpdate)
            {
                UsageText usage (L'/');


                HelpRainDialog dialog (usage);

                float initialFront = dialog.GetRevealFrontY();

                // Simulate 10 frames at 60 fps
                for (int i = 0; i < 10; i++)
                {
                    dialog.Update (1.0f / 60.0f);
                }

                float advancedFront = dialog.GetRevealFrontY();



                // Reveal front should have moved downward (increasing Y)
                Assert::IsTrue (advancedFront > initialFront,
                                L"Reveal front should advance downward with each update");

                // Verify approximate distance: 10 frames * (1/60)s * 150px/s ≈ 25px
                float expectedAdvance = 10.0f * (1.0f / 60.0f) * HelpRainDialog::kRevealSpeed;
                float actualAdvance   = advancedFront - initialFront;

                Assert::IsTrue (abs (actualAdvance - expectedAdvance) < 1.0f,
                                L"Reveal front should advance at kRevealSpeed");
            }
    };




}  // namespace MatrixRainTests
