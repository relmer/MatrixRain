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




            TEST_METHOD (Constructor_BuildsShuffledRevealQueue)
            {
                UsageText usage (L'/');


                HelpRainDialog dialog (usage);

                const auto & queue     = dialog.GetRevealQueue();
                const auto & positions = dialog.GetCharacterPositions();



                Assert::AreEqual (positions.size(), queue.size(),
                                  L"Reveal queue should have same count as character positions");

                // Queue should contain indices [0, N) in some order
                std::vector<size_t> sorted (queue.begin(), queue.end());
                std::ranges::sort (sorted);

                for (size_t i = 0; i < sorted.size(); i++)
                {
                    Assert::AreEqual (i, sorted[i],
                                      L"Reveal queue should be a permutation of [0, N)");
                }
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
                    Assert::IsFalse (flags[i], L"All flags should start as false");
                }
            }




            TEST_METHOD (Constructor_StartsInRevealingPhase)
            {
                UsageText usage (L'/');


                HelpRainDialog dialog (usage);



                Assert::IsTrue (dialog.GetAnimationPhase() == AnimationPhase::Revealing,
                                L"Should start in Revealing phase");
            }




            TEST_METHOD (Constructor_RevealQueueIndexStartsAtZero)
            {
                UsageText usage (L'/');


                HelpRainDialog dialog (usage);



                Assert::AreEqual (static_cast<size_t> (0), dialog.GetRevealQueueIndex(),
                                  L"Reveal queue index should start at 0");
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

                // Simulate full reveal by calling Update with enough time
                // The reveal should complete in kRevealDurationSeconds (3.0s)
                // Run in increments to simulate the animation loop
                for (int i = 0; i < 250; i++)
                {
                    dialog.Update (1.0f / 60.0f);
                }



                Assert::IsTrue (dialog.IsRevealComplete(),
                                L"Reveal should be complete after simulating 250 frames (~4.2s > 3.0s reveal duration)");
            }




            ////////////////////////////////////////////////////////////
            //  T083: Phase 2 background phase
            ////////////////////////////////////////////////////////////

            TEST_METHOD (Phase2_TransitionsAfterRevealComplete)
            {
                UsageText usage (L'/');


                HelpRainDialog dialog (usage);

                // Run enough frames to complete reveal (3s) and allow all reveal
                // streaks to travel past the window bottom before being removed.
                // At 300 px/s, streaks spawned last (~3s) start 96px above their
                // target and must travel past height(600) + trail(72) = 672px.
                // Worst case: target near bottom of centered text, head starts
                // near 0 and must reach 672.  672/300 = 2.24s after spawn.
                // Total: 3s reveal + 2.24s travel = ~5.3s.  Use 1200 frames
                // (20s) for generous margin against timing variance.
                for (int i = 0; i < 1200; i++)
                {
                    dialog.Update (1.0f / 60.0f);
                }



                Assert::IsTrue (dialog.GetAnimationPhase() == AnimationPhase::Background,
                                L"Should transition to Background phase after reveal completes");
            }




            ////////////////////////////////////////////////////////////
            //  T085: Reveal queue drain rate
            ////////////////////////////////////////////////////////////

            TEST_METHOD (RevealSpawnRate_CalibratedForDuration)
            {
                UsageText usage (L'/');


                HelpRainDialog dialog (usage);

                float  spawnRate = dialog.GetRevealSpawnRate();
                size_t charCount = dialog.GetCharacterPositions().size();

                // Rate should be N / kRevealDurationSeconds
                float expected = static_cast<float> (charCount) / HelpRainDialog::kRevealDurationSeconds;



                Assert::AreEqual (expected, spawnRate, 0.01f,
                                  L"Spawn rate should be N / kRevealDurationSeconds");
            }




            TEST_METHOD (RevealQueue_DrainsCompletelyWithinDuration)
            {
                UsageText usage (L'/');


                HelpRainDialog dialog (usage);

                size_t totalChars = dialog.GetCharacterPositions().size();
                float  elapsed    = 0.0f;
                float  dt         = 1.0f / 60.0f;

                // Simulate until queue is fully drained
                while (dialog.GetRevealQueueIndex() < totalChars && elapsed < 10.0f)
                {
                    dialog.Update (dt);
                    elapsed += dt;
                }



                Assert::AreEqual (totalChars, dialog.GetRevealQueueIndex(),
                                  L"Reveal queue should be fully drained");

                // Should have drained within kRevealDurationSeconds + some tolerance
                float maxAllowed = HelpRainDialog::kRevealDurationSeconds + 0.5f;
                Assert::IsTrue (elapsed <= maxAllowed,
                                std::format (L"Queue should drain within {:.1f}s, took {:.1f}s",
                                             maxAllowed, elapsed).c_str());
            }
    };




}  // namespace MatrixRainTests
