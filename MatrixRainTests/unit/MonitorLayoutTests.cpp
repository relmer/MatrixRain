#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\MonitorInfo.h"
#include "..\..\MatrixRainCore\MonitorLayout.h"





namespace MatrixRainTests
{


    static MonitorInfo MakeMonitor (LONG left, LONG top, LONG right, LONG bottom, UINT dpi, bool isPrimary)
    {
        MonitorInfo monitor;
        monitor.m_bounds    = { left, top, right, bottom };
        monitor.m_dpi       = dpi;
        monitor.m_isPrimary = isPrimary;
        monitor.m_handle    = nullptr;

        return monitor;
    }




    static int CountPrimaries (const std::vector<MonitorPlacement> & placements)
    {
        int count = 0;

        for (const MonitorPlacement & placement : placements)
        {
            if (placement.isPrimary)
            {
                count++;
            }
        }

        return count;
    }




    TEST_CLASS (MonitorLayoutTests)
    {
        public:
            TEST_METHOD (Empty_ReturnsNoPlacements)
            {
                std::vector<MonitorPlacement> placements = PlanFullscreenPlacements (std::vector<MonitorInfo>{});

                Assert::AreEqual (size_t (0), placements.size());
            }




            TEST_METHOD (SingleMonitor_OnePlacement_IsPrimaryWithCorrectBounds)
            {
                std::vector<MonitorInfo> monitors;
                monitors.push_back (MakeMonitor (0, 0, 1920, 1080, 96, true));

                std::vector<MonitorPlacement> placements = PlanFullscreenPlacements (monitors);

                Assert::AreEqual (size_t (1), placements.size());
                Assert::AreEqual (LONG (0), placements[0].position.x);
                Assert::AreEqual (LONG (0), placements[0].position.y);
                Assert::AreEqual (LONG (1920), placements[0].size.cx);
                Assert::AreEqual (LONG (1080), placements[0].size.cy);
                Assert::IsTrue (placements[0].isPrimary);
            }




            TEST_METHOD (SingleMonitor_NoPrimaryFlag_FirstBecomesPrimary)
            {
                std::vector<MonitorInfo> monitors;
                monitors.push_back (MakeMonitor (0, 0, 1280, 720, 96, false));

                std::vector<MonitorPlacement> placements = PlanFullscreenPlacements (monitors);

                Assert::AreEqual (size_t (1), placements.size());
                Assert::IsTrue (placements[0].isPrimary);
            }




            TEST_METHOD (TwoMonitors_PrimaryFirst_PreservesCountAndBounds)
            {
                std::vector<MonitorInfo> monitors;
                monitors.push_back (MakeMonitor (0, 0, 1920, 1080, 96, true));
                monitors.push_back (MakeMonitor (1920, 0, 3840, 1080, 144, false));

                std::vector<MonitorPlacement> placements = PlanFullscreenPlacements (monitors);

                Assert::AreEqual (size_t (2), placements.size());

                Assert::IsTrue (placements[0].isPrimary);
                Assert::AreEqual (LONG (0), placements[0].position.x);
                Assert::AreEqual (LONG (1920), placements[0].size.cx);

                Assert::IsFalse (placements[1].isPrimary);
                Assert::AreEqual (LONG (1920), placements[1].position.x);
                Assert::AreEqual (LONG (1920), placements[1].size.cx);

                Assert::AreEqual (1, CountPrimaries (placements));
            }




            TEST_METHOD (TwoMonitors_PrimarySecond_SecondIsFlagged)
            {
                std::vector<MonitorInfo> monitors;
                monitors.push_back (MakeMonitor (0, 0, 1920, 1080, 96, false));
                monitors.push_back (MakeMonitor (1920, 0, 3840, 1080, 144, true));

                std::vector<MonitorPlacement> placements = PlanFullscreenPlacements (monitors);

                Assert::AreEqual (size_t (2), placements.size());
                Assert::IsFalse (placements[0].isPrimary);
                Assert::IsTrue (placements[1].isPrimary);
                Assert::AreEqual (1, CountPrimaries (placements));
            }




            TEST_METHOD (TwoMonitors_NoPrimaryFlag_FirstBecomesPrimary)
            {
                std::vector<MonitorInfo> monitors;
                monitors.push_back (MakeMonitor (0, 0, 1920, 1080, 96, false));
                monitors.push_back (MakeMonitor (1920, 0, 3840, 1080, 144, false));

                std::vector<MonitorPlacement> placements = PlanFullscreenPlacements (monitors);

                Assert::AreEqual (size_t (2), placements.size());
                Assert::IsTrue (placements[0].isPrimary);
                Assert::IsFalse (placements[1].isPrimary);
                Assert::AreEqual (1, CountPrimaries (placements));
            }




            TEST_METHOD (MultipleMonitorsFlaggedPrimary_FirstFlaggedWins)
            {
                std::vector<MonitorInfo> monitors;
                monitors.push_back (MakeMonitor (0, 0, 1920, 1080, 96, false));
                monitors.push_back (MakeMonitor (1920, 0, 3840, 1080, 96, true));
                monitors.push_back (MakeMonitor (3840, 0, 5760, 1080, 96, true));

                std::vector<MonitorPlacement> placements = PlanFullscreenPlacements (monitors);

                Assert::AreEqual (size_t (3), placements.size());
                Assert::IsTrue (placements[1].isPrimary);
                Assert::IsFalse (placements[2].isPrimary);
                Assert::AreEqual (1, CountPrimaries (placements));
            }




            TEST_METHOD (DegenerateMonitorDropped_OnlyValidPlacements)
            {
                std::vector<MonitorInfo> monitors;
                monitors.push_back (MakeMonitor (0, 0, 1920, 1080, 96, true));
                monitors.push_back (MakeMonitor (1920, 0, 1920, 1080, 96, false));   // zero width

                std::vector<MonitorPlacement> placements = PlanFullscreenPlacements (monitors);

                Assert::AreEqual (size_t (1), placements.size());
                Assert::AreEqual (LONG (1920), placements[0].size.cx);
                Assert::IsTrue (placements[0].isPrimary);
            }




            TEST_METHOD (PrimaryFlaggedMonitorIsDegenerate_FirstSurvivorBecomesPrimary)
            {
                std::vector<MonitorInfo> monitors;
                monitors.push_back (MakeMonitor (0, 0, 1920, 1080, 96, false));
                monitors.push_back (MakeMonitor (1920, 0, 1920, 1080, 96, true));    // primary but zero width

                std::vector<MonitorPlacement> placements = PlanFullscreenPlacements (monitors);

                Assert::AreEqual (size_t (1), placements.size());
                Assert::AreEqual (LONG (0), placements[0].position.x);
                Assert::IsTrue (placements[0].isPrimary);
                Assert::AreEqual (1, CountPrimaries (placements));
            }




            TEST_METHOD (AllMonitorsDegenerate_ReturnsEmpty)
            {
                std::vector<MonitorInfo> monitors;
                monitors.push_back (MakeMonitor (0, 0, 0, 0, 96, true));
                monitors.push_back (MakeMonitor (100, 100, 100, 200, 96, false));    // zero width
                monitors.push_back (MakeMonitor (200, 200, 300, 200, 96, false));    // zero height

                std::vector<MonitorPlacement> placements = PlanFullscreenPlacements (monitors);

                Assert::AreEqual (size_t (0), placements.size());
            }




            TEST_METHOD (InvertedBounds_TreatedAsDegenerate)
            {
                std::vector<MonitorInfo> monitors;
                monitors.push_back (MakeMonitor (1920, 1080, 0, 0, 96, true));       // right<left, bottom<top

                std::vector<MonitorPlacement> placements = PlanFullscreenPlacements (monitors);

                Assert::AreEqual (size_t (0), placements.size());
            }




            TEST_METHOD (NegativeVirtualScreenCoordinates_PreservedInPlacement)
            {
                std::vector<MonitorInfo> monitors;
                monitors.push_back (MakeMonitor (0, 0, 1920, 1080, 96, true));
                monitors.push_back (MakeMonitor (-1920, -200, 0, 880, 96, false));   // left-of/above primary

                std::vector<MonitorPlacement> placements = PlanFullscreenPlacements (monitors);

                Assert::AreEqual (size_t (2), placements.size());
                Assert::AreEqual (LONG (-1920), placements[1].position.x);
                Assert::AreEqual (LONG (-200), placements[1].position.y);
                Assert::AreEqual (LONG (1920), placements[1].size.cx);
                Assert::AreEqual (LONG (1080), placements[1].size.cy);
            }
    };


}
