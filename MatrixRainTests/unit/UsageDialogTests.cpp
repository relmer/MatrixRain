#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\UsageDialog.h"
#include "..\..\MatrixRainCore\InMemorySettingsProvider.h"





namespace MatrixRainTests
{




    TEST_CLASS (UsageDialogTests)
    {
        private:
            InMemorySettingsProvider m_settingsProvider;

        public:




            ////////////////////////////////////////////////////////////
            //  T067: Window sizing — ComputeWindowSize
            ////////////////////////////////////////////////////////////

            TEST_METHOD (ComputeWindowSize_ReturnsPositiveDimensions)
            {
                UsageText usage (L'/');


                SIZE size = UsageDialog::ComputeWindowSize (usage, 96.0f);



                Assert::IsTrue (size.cx > 0, L"Width should be positive");
                Assert::IsTrue (size.cy > 0, L"Height should be positive");
            }




            TEST_METHOD (ComputeWindowSize_AtLeastTwiceTextBoundingBox)
            {
                UsageText usage (L'/');


                // At default DPI (96), the window should be at least 2x the text content
                SIZE size = UsageDialog::ComputeWindowSize (usage, 96.0f);



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

                SIZE size = UsageDialog::ComputeWindowSize (usage, 96.0f);



                Assert::IsTrue (size.cx <= maxWidth, L"Width should not exceed 80% of work area width");
                Assert::IsTrue (size.cy <= maxHeight, L"Height should not exceed 80% of work area height");
            }




            ////////////////////////////////////////////////////////////
            //  T068: Phase 1 reveal animation state
            ////////////////////////////////////////////////////////////

            TEST_METHOD (Constructor_PreComputesCharacterPositions)
            {
                UsageText usage (L'/');


                UsageDialog dialog (usage, m_settingsProvider);

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


                UsageDialog dialog (usage, m_settingsProvider);



                // AnimationSystem is created in CreateRenderPipeline (called
                // from Show), so it should be null after construction alone.
                Assert::IsNull (dialog.GetAnimationSystem(),
                                L"AnimationSystem should be null before Show()");
            }




            TEST_METHOD (Constructor_InitializesRevealedFlagsToFalse)
            {
                UsageText usage (L'/');


                UsageDialog dialog (usage, m_settingsProvider);

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


                UsageDialog dialog (usage, m_settingsProvider);



                Assert::IsTrue (dialog.GetAnimationPhase() == AnimationPhase::Revealing,
                                L"Should start in Revealing phase");
            }




            ////////////////////////////////////////////////////////////
            //  T069: IsRevealComplete
            ////////////////////////////////////////////////////////////

            TEST_METHOD (IsRevealComplete_InitiallyFalse)
            {
                UsageText usage (L'/');


                UsageDialog dialog (usage, m_settingsProvider);



                Assert::IsFalse (dialog.IsRevealComplete(),
                                 L"Reveal should not be complete initially");
            }




            TEST_METHOD (IsRevealComplete_TrueAfterFullReveal)
            {
                UsageText usage (L'/');


                UsageDialog dialog (usage, m_settingsProvider);

                // Scramble-reveal: all cells cycle random glyphs, then each
                // locks at a random time in [0, revealDuration].  After
                // revealDuration + flashDuration all should be Settled.
                // Use 200 frames at 60fps (3.3s) for comfortable margin.
                for (int i = 0; i < 200; i++)
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


                UsageDialog dialog (usage, m_settingsProvider);

                // Transition to Background happens as soon as all cells
                // settle.  200 frames at 60fps (3.3s) exceeds
                // revealDuration(2.5) + flash(0.15) comfortably.
                for (int i = 0; i < 200; i++)
                {
                    dialog.Update (1.0f / 60.0f);
                }



                Assert::IsTrue (dialog.GetAnimationPhase() == AnimationPhase::Background,
                                L"Should transition to Background phase after reveal completes");
            }




            ////////////////////////////////////////////////////////////
            //  T085: Scramble-reveal makes characters visible
            ////////////////////////////////////////////////////////////

            TEST_METHOD (Update_AllCharactersBecomeVisible)
            {
                UsageText usage (L'/');


                UsageDialog dialog (usage, m_settingsProvider);

                // After one update, all cells should be cycling at full
                // opacity (scramble-reveal starts all cells immediately).
                dialog.Update (1.0f / 60.0f);

                const auto & flags = dialog.GetRevealedFlags();



                for (size_t i = 0; i < flags.size(); i++)
                {
                    Assert::AreEqual (1.0f, flags[i],
                                      L"All characters should be visible after first update");
                }
            }
    };




}  // namespace MatrixRainTests
