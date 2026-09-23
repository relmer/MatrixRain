#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\ColorMath.h"
#include "..\..\MatrixRainCore\InMemoryDisplayLuminanceProvider.h"
#include "..\..\MatrixRainCore\OutputModeTracker.h"





namespace MatrixRainTests
{


    static constexpr float kNitsTolerance = 1e-4f;





    ////////////////////////////////////////////////////////////////////////////
    //
    //  Snapshots the tests feed the tracker
    //
    ////////////////////////////////////////////////////////////////////////////

    static DisplayLuminance SdrDisplay (float sdrWhiteNits = 80.0f)
    {
        DisplayLuminance luminance;



        luminance.hdrEnabled       = false;
        luminance.scRgbSupported   = true;
        luminance.sdrWhiteNits     = sdrWhiteNits;
        luminance.reportedPeakNits = 0.0f;

        return luminance;
    }

    static DisplayLuminance HdrDisplay (float sdrWhiteNits = 200.0f, float peakNits = 1000.0f)
    {
        DisplayLuminance luminance;



        luminance.hdrEnabled       = true;
        luminance.scRgbSupported   = true;
        luminance.sdrWhiteNits     = sdrWhiteNits;
        luminance.reportedPeakNits = peakNits;

        return luminance;
    }





    //  T023 (US2): the display luminance math of contracts/color-math.md.
    TEST_CLASS (DisplayLuminanceMathTests)
    {
        public:

            TEST_METHOD (EffectivePeakNits_TrustsAPlausibleReport)
            {
                Assert::AreEqual (80.0f,    EffectivePeakNits (80.0f),    kNitsTolerance);
                Assert::AreEqual (1000.0f,  EffectivePeakNits (1000.0f),  kNitsTolerance);
                Assert::AreEqual (10000.0f, EffectivePeakNits (10000.0f), kNitsTolerance);
            }




            TEST_METHOD (EffectivePeakNits_FallsBackTo400_OutsideThePlausibleRange)
            {
                Assert::AreEqual (400.0f, EffectivePeakNits (0.0f),     kNitsTolerance, L"Unknown (0) must fall back");
                Assert::AreEqual (400.0f, EffectivePeakNits (-50.0f),   kNitsTolerance, L"Negative must fall back");
                Assert::AreEqual (400.0f, EffectivePeakNits (79.9f),    kNitsTolerance, L"Below 80 must fall back");
                Assert::AreEqual (400.0f, EffectivePeakNits (10001.0f), kNitsTolerance, L"Above 10 000 must fall back");
            }




            TEST_METHOD (Headroom_IsPeakOverWhite)
            {
                Assert::AreEqual (5.0f,  Headroom (1000.0f, 200.0f), kNitsTolerance);
                Assert::AreEqual (2.5f,  Headroom (1000.0f, 400.0f), kNitsTolerance);
                Assert::AreEqual (5.0f,  Headroom (400.0f,  80.0f),  kNitsTolerance);
            }




            TEST_METHOD (Headroom_IsNeverBelowOne)
            {
                // FR-026: a white level at or above the peak, or a white level
                // that cannot be trusted, gives no headroom rather than less
                // than none.
                Assert::AreEqual (1.0f, Headroom (200.0f, 200.0f), kNitsTolerance, L"White at peak");
                Assert::AreEqual (1.0f, Headroom (200.0f, 480.0f), kNitsTolerance, L"White above peak");
                Assert::AreEqual (1.0f, Headroom (80.0f,  0.0f),   kNitsTolerance, L"Zero white clamps to 80, and 80 / 80 is 1");
                Assert::AreEqual (1.0f, Headroom (80.0f,  -1.0f),  kNitsTolerance, L"Negative white clamps to 80");
                Assert::IsTrue   (Headroom (400.0f, 0.0f) >= 1.0f,  L"Zero white must not divide by zero");
            }




            TEST_METHOD (SdrWhiteScale_IsNitsOver80)
            {
                Assert::AreEqual (1.0f, SdrWhiteScale (80.0f),  kNitsTolerance);
                Assert::AreEqual (2.5f, SdrWhiteScale (200.0f), kNitsTolerance);
                Assert::AreEqual (6.0f, SdrWhiteScale (480.0f), kNitsTolerance);
            }




            TEST_METHOD (SdrWhiteScale_ClampsTheWhiteLevelFirst)
            {
                Assert::AreEqual (1.0f, SdrWhiteScale (0.0f),    kNitsTolerance, L"Zero must not black the screen");
                Assert::AreEqual (1.0f, SdrWhiteScale (-80.0f),  kNitsTolerance, L"Negative must not black the screen");
                Assert::AreEqual (6.0f, SdrWhiteScale (5000.0f), kNitsTolerance, L"Absurd must not blind the viewer");
            }




            TEST_METHOD (SdrWhiteNitsFromDisplayConfig_Is1000Per80Nits)
            {
                Assert::AreEqual (80.0f,  SdrWhiteNitsFromDisplayConfig (1000), kNitsTolerance);
                Assert::AreEqual (200.0f, SdrWhiteNitsFromDisplayConfig (2500), kNitsTolerance);
                Assert::AreEqual (480.0f, SdrWhiteNitsFromDisplayConfig (6000), kNitsTolerance);
            }




            TEST_METHOD (SdrWhiteNitsFromDisplayConfig_ClampsTo80Through480)
            {
                Assert::AreEqual (80.0f,  SdrWhiteNitsFromDisplayConfig (0),      kNitsTolerance, L"Missing reads as 80");
                Assert::AreEqual (80.0f,  SdrWhiteNitsFromDisplayConfig (500),    kNitsTolerance, L"Below 80 clamps up");
                Assert::AreEqual (480.0f, SdrWhiteNitsFromDisplayConfig (100000), kNitsTolerance, L"Above 480 clamps down");
            }
    };





    //  T025 (US2): the OutputModeTracker state machine of data-model §7,
    //  driven by the scripted provider.
    TEST_CLASS (OutputModeTrackerTests)
    {
        public:

            TEST_METHOD (StartsInSdr_WithUnitWhiteScale)
            {
                OutputModeTracker tracker (ScreenSaverMode::Normal);



                Assert::IsTrue   (OutputMode::Sdr == tracker.CurrentMode());
                Assert::AreEqual (1.0f, tracker.SdrWhiteScale(), kNitsTolerance);
                Assert::AreEqual (1.0f, tracker.Headroom(),      kNitsTolerance);
            }




            TEST_METHOD (SdrThenHdrThenSdr_AsksForEachSwitchOnce)
            {
                InMemoryDisplayLuminanceProvider provider ({ SdrDisplay(), HdrDisplay(), SdrDisplay() });
                OutputModeTracker                tracker  (ScreenSaverMode::Normal);
                OutputModeDecision               decision;



                decision = tracker.Observe (provider.Query (nullptr));
                Assert::IsTrue (OutputModeAction::None == decision.action, L"SDR on an SDR tracker: nothing to do");

                decision = tracker.Observe (provider.Query (nullptr));
                Assert::IsTrue (OutputModeAction::Reconfigure == decision.action, L"HDR appeared: reconfigure");
                Assert::IsTrue (OutputMode::Hdr == decision.mode);
                tracker.ReportReconfigureResult (OutputMode::Hdr, true);
                Assert::IsTrue (OutputMode::Hdr == tracker.CurrentMode());

                decision = tracker.Observe (provider.Query (nullptr));
                Assert::IsTrue (OutputModeAction::Reconfigure == decision.action, L"HDR went away: reconfigure");
                Assert::IsTrue (OutputMode::Sdr == decision.mode);
                tracker.ReportReconfigureResult (OutputMode::Sdr, true);
                Assert::IsTrue (OutputMode::Sdr == tracker.CurrentMode());
            }




            TEST_METHOD (UnchangedInput_ReturnsNone)
            {
                InMemoryDisplayLuminanceProvider provider ({ HdrDisplay() });
                OutputModeTracker                tracker  (ScreenSaverMode::Normal);



                tracker.ReportReconfigureResult (OutputMode::Hdr, true);   // already presenting in HDR

                for (int i = 0; i < 5; ++i)
                {
                    Assert::IsTrue (OutputModeAction::None == tracker.Observe (provider.Query (nullptr)).action);
                }

                Assert::AreEqual (5, provider.QueryCount());
            }




            TEST_METHOD (WhiteLevelChange_ReturnsNone_ButUpdatesTheScale)
            {
                InMemoryDisplayLuminanceProvider provider ({ HdrDisplay (200.0f), HdrDisplay (320.0f) });
                OutputModeTracker                tracker  (ScreenSaverMode::Normal);



                tracker.ReportReconfigureResult (OutputMode::Hdr, true);

                Assert::IsTrue   (OutputModeAction::None == tracker.Observe (provider.Query (nullptr)).action);
                Assert::AreEqual (2.5f, tracker.SdrWhiteScale(), kNitsTolerance, L"200 nits is 2.5 in scRGB");
                Assert::AreEqual (5.0f, tracker.Headroom(),      kNitsTolerance, L"1000 over 200");

                Assert::IsTrue   (OutputModeAction::None == tracker.Observe (provider.Query (nullptr)).action, L"The slider moved: still no reconfigure");
                Assert::AreEqual (4.0f,   tracker.SdrWhiteScale(), kNitsTolerance, L"320 nits is 4.0 in scRGB");
                Assert::AreEqual (3.125f, tracker.Headroom(),      kNitsTolerance, L"1000 over 320");
            }




            TEST_METHOD (ReconfigureFailure_LeavesTheTrackerInSdr_AndDoesNotRetryTheSameFacts)
            {
                InMemoryDisplayLuminanceProvider provider ({ HdrDisplay() });
                OutputModeTracker                tracker  (ScreenSaverMode::Normal);
                OutputModeDecision               decision = tracker.Observe (provider.Query (nullptr));



                Assert::IsTrue (OutputModeAction::Reconfigure == decision.action);

                tracker.ReportReconfigureResult (OutputMode::Hdr, false);

                Assert::IsTrue (OutputMode::Sdr == tracker.CurrentMode(), L"A refused switch leaves the back buffer in SDR");

                for (int i = 0; i < 3; ++i)
                {
                    Assert::IsTrue (OutputModeAction::None == tracker.Observe (provider.Query (nullptr)).action,
                                    L"The same facts must not be retried every second");
                }
            }




            TEST_METHOD (ReconfigureFailure_IsRetried_OnceTheDisplayFactsChange)
            {
                InMemoryDisplayLuminanceProvider provider ({ HdrDisplay(), SdrDisplay(), HdrDisplay() });
                OutputModeTracker                tracker  (ScreenSaverMode::Normal);



                Assert::IsTrue (OutputModeAction::Reconfigure == tracker.Observe (provider.Query (nullptr)).action);
                tracker.ReportReconfigureResult (OutputMode::Hdr, false);

                Assert::IsTrue (OutputModeAction::None == tracker.Observe (provider.Query (nullptr)).action, L"HDR off while in SDR: nothing to do, and the refusal clears");
                Assert::IsTrue (OutputModeAction::Reconfigure == tracker.Observe (provider.Query (nullptr)).action, L"HDR back on: worth another try");
            }




            TEST_METHOD (PreviewWindow_NeverLeavesSdr)
            {
                InMemoryDisplayLuminanceProvider provider ({ HdrDisplay() });
                OutputModeTracker                tracker  (ScreenSaverMode::ScreenSaverPreview);



                for (int i = 0; i < 3; ++i)
                {
                    Assert::IsTrue (OutputModeAction::None == tracker.Observe (provider.Query (nullptr)).action);
                    Assert::IsTrue (OutputMode::Sdr == tracker.CurrentMode());
                }

                Assert::AreEqual (2.5f, tracker.SdrWhiteScale(), kNitsTolerance, L"The white scale still follows the display");
            }




            TEST_METHOD (Provider_RepeatsItsLastSnapshot_AndReportsStaleOnce)
            {
                InMemoryDisplayLuminanceProvider provider ({ SdrDisplay (80.0f), HdrDisplay (240.0f) });



                Assert::IsFalse (provider.Query (nullptr).hdrEnabled);
                Assert::IsTrue  (provider.Query (nullptr).hdrEnabled);
                Assert::IsTrue  (provider.Query (nullptr).hdrEnabled, L"The last snapshot repeats");

                provider.SetStale (true);
                Assert::IsTrue  (provider.IsStale());
                Assert::IsFalse (provider.IsStale(), L"Stale clears after one read, like a refreshed factory");
            }
    };
}
