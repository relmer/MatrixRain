#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\Overlay.h"





namespace MatrixRainTests
{
    static Overlay CreateHotkeyOverlay()
    {
        return Overlay (
            {{L"Space", L"Pause / Resume"}, {L"Enter", L"Settings dialog"}, {L"C", L"Cycle color scheme"},
             {L"S", L"Toggle statistics"}, {L"?", L"Help reference"}, {L"+", L"Increase rain density"},
             {L"-", L"Decrease rain density"}, {L"Alt+Enter", L"Toggle fullscreen"}, {L"Esc", L"Exit"}},
            OverlayTimingConfig {.revealDuration = 2.5f, .dismissDuration = 1.0f, .cycleInterval = 0.25f, .flashDuration = 1.0f, .holdDuration = 5.4f},
            OverlayLayoutConfig {.marginCols = 2, .gapChars = 6});
    }

    TEST_CLASS (HotkeyOverlayTests)
    {
    public:

        TEST_METHOD (InitialState_Hidden)
        {
            auto overlay = CreateHotkeyOverlay();
            Assert::AreEqual (static_cast<int>(OverlayPhase::Hidden), static_cast<int>(overlay.GetPhase()));
            Assert::IsFalse (overlay.IsActive());
        }

        TEST_METHOD (InitialState_CharactersInitialized)
        {
            auto overlay = CreateHotkeyOverlay();
            auto chars = overlay.GetCharacters();
            Assert::IsTrue (chars.size() > 0, L"Should have characters");
            for (const auto & ch : chars)
                Assert::AreEqual (static_cast<int>(CharPhase::Hidden), static_cast<int>(ch.phase), L"All chars should start Hidden");
        }

        TEST_METHOD (Show_TransitionsToRevealing)
        {
            auto overlay = CreateHotkeyOverlay();
            overlay.Show();
            Assert::AreEqual (static_cast<int>(OverlayPhase::Revealing), static_cast<int>(overlay.GetPhase()));
            Assert::IsTrue (overlay.IsActive());
        }

        TEST_METHOD (Show_CharactersInitializedToHiddenForReveal)
        {
            auto overlay = CreateHotkeyOverlay();
            overlay.Show();
            auto chars = overlay.GetCharacters();
            bool allHidden = true;
            for (const auto & ch : chars)
            {
                if (!ch.isSpace && ch.phase != CharPhase::Hidden) { allHidden = false; break; }
            }
            Assert::IsTrue (allHidden, L"Non-space characters should be Hidden after Show");
        }

        TEST_METHOD (Show_WhileRevealing_ResetsToRevealing)
        {
            auto overlay = CreateHotkeyOverlay();
            overlay.Show();
            overlay.Show();
            Assert::AreEqual (static_cast<int>(OverlayPhase::Revealing), static_cast<int>(overlay.GetPhase()));
        }

        TEST_METHOD (RevealComplete_TransitionsToHolding)
        {
            auto overlay = CreateHotkeyOverlay();
            overlay.Show();
            overlay.Update (5.0f, 0.0f, 1.0f, 0.0f);
            Assert::AreEqual (static_cast<int>(OverlayPhase::Holding), static_cast<int>(overlay.GetPhase()));
        }

        TEST_METHOD (Dismiss_FromRevealing_TransitionsToDissolving)
        {
            auto overlay = CreateHotkeyOverlay();
            overlay.Show();
            overlay.Dismiss();
            Assert::AreEqual (static_cast<int>(OverlayPhase::Dissolving), static_cast<int>(overlay.GetPhase()));
        }

        TEST_METHOD (Dismiss_FromHolding_TransitionsToDissolving)
        {
            auto overlay = CreateHotkeyOverlay();
            overlay.Show();
            overlay.Update (5.0f, 0.0f, 1.0f, 0.0f);
            overlay.Dismiss();
            Assert::AreEqual (static_cast<int>(OverlayPhase::Dissolving), static_cast<int>(overlay.GetPhase()));
        }

        TEST_METHOD (Dismiss_FromHidden_IsNoop)
        {
            auto overlay = CreateHotkeyOverlay();
            overlay.Dismiss();
            Assert::AreEqual (static_cast<int>(OverlayPhase::Hidden), static_cast<int>(overlay.GetPhase()));
        }

        TEST_METHOD (Dismiss_FromDissolving_IsNoop)
        {
            auto overlay = CreateHotkeyOverlay();
            overlay.Show();
            overlay.Dismiss();
            overlay.Dismiss();
            Assert::AreEqual (static_cast<int>(OverlayPhase::Dissolving), static_cast<int>(overlay.GetPhase()));
        }

        TEST_METHOD (Update_Dissolving_TransitionsToHiddenWhenFaded)
        {
            auto overlay = CreateHotkeyOverlay();
            overlay.Show();
            overlay.Dismiss();
            for (int i = 0; i < 200; i++)
                overlay.Update (0.02f, 0.0f, 1.0f, 0.0f);
            Assert::AreEqual (static_cast<int>(OverlayPhase::Hidden), static_cast<int>(overlay.GetPhase()));
        }

        TEST_METHOD (DissolveComplete_AllCharactersHidden)
        {
            auto overlay = CreateHotkeyOverlay();
            overlay.Show();
            overlay.Dismiss();
            for (int i = 0; i < 200; i++)
                overlay.Update (0.02f, 0.0f, 1.0f, 0.0f);
            auto chars = overlay.GetCharacters();
            for (const auto & ch : chars)
                Assert::AreEqual (static_cast<int>(CharPhase::Hidden), static_cast<int>(ch.phase), L"All should be Hidden");
        }

        TEST_METHOD (Update_Holding_AutoDismissesAfterTimeout)
        {
            auto overlay = CreateHotkeyOverlay();
            overlay.Show();
            overlay.Update (5.0f, 0.0f, 1.0f, 0.0f);
            Assert::AreEqual (static_cast<int>(OverlayPhase::Holding), static_cast<int>(overlay.GetPhase()));
            overlay.Update (6.0f, 0.0f, 1.0f, 0.0f);
            Assert::AreEqual (static_cast<int>(OverlayPhase::Dissolving), static_cast<int>(overlay.GetPhase()));
        }

        TEST_METHOD (TestCompleteAnimationLoop)
        {
            auto overlay = CreateHotkeyOverlay();
            overlay.Show();
            Assert::AreEqual (static_cast<int>(OverlayPhase::Revealing), static_cast<int>(overlay.GetPhase()));
            overlay.Update (5.0f, 0.0f, 1.0f, 0.0f);
            Assert::AreEqual (static_cast<int>(OverlayPhase::Holding), static_cast<int>(overlay.GetPhase()));
            overlay.Dismiss();
            Assert::AreEqual (static_cast<int>(OverlayPhase::Dissolving), static_cast<int>(overlay.GetPhase()));
            for (int i = 0; i < 200; i++)
                overlay.Update (0.02f, 0.0f, 1.0f, 0.0f);
            Assert::AreEqual (static_cast<int>(OverlayPhase::Hidden), static_cast<int>(overlay.GetPhase()));
            Assert::IsFalse (overlay.IsActive());
        }

        TEST_METHOD (Show_WhileDissolving_RestartsReveal)
        {
            auto overlay = CreateHotkeyOverlay();
            overlay.Show();
            overlay.Update (5.0f, 0.0f, 1.0f, 0.0f);
            overlay.Dismiss();
            Assert::AreEqual (static_cast<int>(OverlayPhase::Dissolving), static_cast<int>(overlay.GetPhase()));
            overlay.Show();
            Assert::AreEqual (static_cast<int>(OverlayPhase::Revealing), static_cast<int>(overlay.GetPhase()));
        }
    };
}
