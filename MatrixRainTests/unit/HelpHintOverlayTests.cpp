#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\Overlay.h"





namespace MatrixRainTests
{
    // Helper: create a help hint overlay with the same entries/config as the original HelpHintOverlay
    static Overlay CreateHelpHintOverlay()
    {
        return Overlay (
            {{L"Settings", L"Enter"}, {L"Help", L"?"}, {L"Exit", L"Esc"}},
            OverlayTimingConfig {.revealDuration = 2.5f, .dismissDuration = 1.0f, .cycleInterval = 0.25f, .flashDuration = 1.0f, .holdDuration = 2.7f},
            OverlayLayoutConfig {.marginCols = 1, .gapChars = 6, .basePadding = 20.0f});
    }

    TEST_CLASS (HelpHintOverlayTests)
    {
    public:

        TEST_METHOD (InitialState_PhaseIsHidden)
        {
            auto overlay = CreateHelpHintOverlay();
            Assert::IsFalse (overlay.IsActive(), L"Should not be active initially");
            Assert::AreEqual (static_cast<int>(OverlayPhase::Hidden), static_cast<int>(overlay.GetPhase()), L"Phase should be Hidden");
        }

        TEST_METHOD (InitialState_CharactersInitialized)
        {
            auto overlay = CreateHelpHintOverlay();
            auto chars = overlay.GetCharacters();
            Assert::AreEqual (overlay.GetCharRows() * overlay.GetCols(), static_cast<int>(chars.size()), L"Character count should match rows*cols");
            Assert::IsTrue (overlay.GetCharRows() == 3, L"Should have 3 rows");
            Assert::IsTrue (overlay.GetCols() > 0, L"Should have positive columns");
        }

        TEST_METHOD (Show_TransitionsToRevealing)
        {
            auto overlay = CreateHelpHintOverlay();
            overlay.Show();
            Assert::IsTrue (overlay.IsActive(), L"Should be active after Show");
            Assert::AreEqual (static_cast<int>(OverlayPhase::Revealing), static_cast<int>(overlay.GetPhase()), L"Phase should be Revealing");
        }

        TEST_METHOD (Show_CharactersInitializedToHiddenForReveal)
        {
            auto overlay = CreateHelpHintOverlay();
            overlay.Show();
            auto chars = overlay.GetCharacters();
            bool allHidden = true;
            for (const auto & ch : chars)
            {
                if (!ch.isSpace && ch.phase != CharPhase::Hidden) { allHidden = false; break; }
            }
            Assert::IsTrue (allHidden, L"Non-space characters should be Hidden after Show");
        }

        TEST_METHOD (Show_SpacesMarkedAsSpace)
        {
            auto overlay = CreateHelpHintOverlay();
            overlay.Show();
            auto chars = overlay.GetCharacters();
            for (const auto & ch : chars)
            {
                if (ch.isSpace)
                    Assert::AreEqual (static_cast<int>(CharPhase::Hidden), static_cast<int>(ch.phase), L"Space chars should be Hidden");
            }
        }

        TEST_METHOD (Update_Revealing_CharactersResolve)
        {
            auto overlay = CreateHelpHintOverlay();
            overlay.Show();
            overlay.Update (5.0f, 0.0f, 1.0f, 0.0f);
            auto chars = overlay.GetCharacters();
            bool allResolved = true;
            for (const auto & ch : chars)
            {
                if (ch.isSpace) continue;
                if (ch.phase != CharPhase::Settled) { allResolved = false; break; }
            }
            Assert::IsTrue (allResolved, L"All non-space characters should be Settled after large Update");
        }

        TEST_METHOD (RevealComplete_TransitionsToHolding)
        {
            auto overlay = CreateHelpHintOverlay();
            overlay.Show();
            overlay.Update (5.0f, 0.0f, 1.0f, 0.0f);
            Assert::AreEqual (static_cast<int>(OverlayPhase::Holding), static_cast<int>(overlay.GetPhase()), L"Should transition to Holding");
        }

        TEST_METHOD (HoldComplete_TransitionsToDissolving)
        {
            auto overlay = CreateHelpHintOverlay();
            overlay.Show();
            overlay.Update (5.0f, 0.0f, 1.0f, 0.0f);
            overlay.Update (4.0f, 0.0f, 1.0f, 0.0f);
            Assert::AreEqual (static_cast<int>(OverlayPhase::Dissolving), static_cast<int>(overlay.GetPhase()), L"Should transition to Dissolving after hold");
        }

        TEST_METHOD (DissolveComplete_TransitionsToHidden)
        {
            auto overlay = CreateHelpHintOverlay();
            overlay.Show();
            overlay.Update (5.0f, 0.0f, 1.0f, 0.0f);
            overlay.Update (4.0f, 0.0f, 1.0f, 0.0f);
            overlay.Update (10.0f, 0.0f, 1.0f, 0.0f);
            Assert::AreEqual (static_cast<int>(OverlayPhase::Hidden), static_cast<int>(overlay.GetPhase()), L"Should transition to Hidden");
            Assert::IsFalse (overlay.IsActive(), L"Should not be active after dissolve");
        }

        TEST_METHOD (DissolveComplete_AllCharactersHidden)
        {
            auto overlay = CreateHelpHintOverlay();
            overlay.Show();
            overlay.Update (5.0f, 0.0f, 1.0f, 0.0f);
            overlay.Update (4.0f, 0.0f, 1.0f, 0.0f);
            overlay.Update (10.0f, 0.0f, 1.0f, 0.0f);
            auto chars = overlay.GetCharacters();
            for (const auto & ch : chars)
                Assert::AreEqual (static_cast<int>(CharPhase::Hidden), static_cast<int>(ch.phase), L"All characters should be Hidden");
        }

        TEST_METHOD (GetCharacters_CorrectRowColLayout)
        {
            auto overlay = CreateHelpHintOverlay();
            auto chars = overlay.GetCharacters();
            int rows = overlay.GetCharRows();
            int cols = overlay.GetCols();
            Assert::AreEqual (rows * cols, static_cast<int>(chars.size()), L"Should have rows*cols characters");
            for (int r = 0; r < rows; r++)
                Assert::AreEqual (r, chars[r * cols].row, L"Row should match position");
        }

        TEST_METHOD (Show_WhileRevealing_ResetsToHidden)
        {
            auto overlay = CreateHelpHintOverlay();
            overlay.Show();
            overlay.Update (0.1f, 0.0f, 1.0f, 0.0f);
            overlay.Show();
            Assert::AreEqual (static_cast<int>(OverlayPhase::Revealing), static_cast<int>(overlay.GetPhase()), L"Should be Revealing after re-trigger");
        }

        TEST_METHOD (Show_WhileHolding_ResetsToRevealing)
        {
            auto overlay = CreateHelpHintOverlay();
            overlay.Show();
            overlay.Update (5.0f, 0.0f, 1.0f, 0.0f);
            Assert::AreEqual (static_cast<int>(OverlayPhase::Holding), static_cast<int>(overlay.GetPhase()), L"Should be Holding");
            overlay.Show();
            Assert::AreEqual (static_cast<int>(OverlayPhase::Revealing), static_cast<int>(overlay.GetPhase()), L"Should reset to Revealing");
        }

        TEST_METHOD (Show_WhileDissolving_ResetsToRevealing)
        {
            auto overlay = CreateHelpHintOverlay();
            overlay.Show();
            overlay.Update (5.0f, 0.0f, 1.0f, 0.0f);
            overlay.Update (4.0f, 0.0f, 1.0f, 0.0f);
            Assert::AreEqual (static_cast<int>(OverlayPhase::Dissolving), static_cast<int>(overlay.GetPhase()), L"Should be Dissolving");
            overlay.Show();
            Assert::AreEqual (static_cast<int>(OverlayPhase::Revealing), static_cast<int>(overlay.GetPhase()), L"Should reset to Revealing");
        }

        TEST_METHOD (Dismiss_FromRevealing_TransitionsToDissolving)
        {
            auto overlay = CreateHelpHintOverlay();
            overlay.Show();
            overlay.Dismiss();
            Assert::AreEqual (static_cast<int>(OverlayPhase::Dissolving), static_cast<int>(overlay.GetPhase()), L"Dismiss from Revealing should go to Dissolving");
        }

        TEST_METHOD (Dismiss_FromHolding_TransitionsToDissolving)
        {
            auto overlay = CreateHelpHintOverlay();
            overlay.Show();
            overlay.Update (5.0f, 0.0f, 1.0f, 0.0f);
            overlay.Dismiss();
            Assert::AreEqual (static_cast<int>(OverlayPhase::Dissolving), static_cast<int>(overlay.GetPhase()), L"Dismiss from Holding should go to Dissolving");
        }

        TEST_METHOD (Dismiss_FromDissolving_IsNoOp)
        {
            auto overlay = CreateHelpHintOverlay();
            overlay.Show();
            overlay.Update (5.0f, 0.0f, 1.0f, 0.0f);
            overlay.Update (4.0f, 0.0f, 1.0f, 0.0f);
            Assert::AreEqual (static_cast<int>(OverlayPhase::Dissolving), static_cast<int>(overlay.GetPhase()), L"Should be Dissolving");
            overlay.Dismiss();
            Assert::AreEqual (static_cast<int>(OverlayPhase::Dissolving), static_cast<int>(overlay.GetPhase()), L"Dismiss from Dissolving should be no-op");
        }

        TEST_METHOD (Dismiss_FromHidden_IsNoOp)
        {
            auto overlay = CreateHelpHintOverlay();
            overlay.Dismiss();
            Assert::AreEqual (static_cast<int>(OverlayPhase::Hidden), static_cast<int>(overlay.GetPhase()), L"Dismiss from Hidden should be no-op");
            Assert::IsFalse (overlay.IsActive(), L"Should remain inactive");
        }

        TEST_METHOD (DefaultDpiScale_IsOne)
        {
            auto overlay = CreateHelpHintOverlay();
            Assert::AreEqual (16.0f, overlay.GetCharWidth(),  L"Default char width should match base");
            Assert::AreEqual (28.0f, overlay.GetCharHeight(), L"Default char height should match base");
            Assert::AreEqual (20.0f, overlay.GetPadding(),    L"Default padding should match base");
        }
    };
}
