#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\MonitorInfo.h"
#include "..\..\MatrixRainCore\InMemoryMonitorProvider.h"
#include "..\..\MatrixRainCore\WindowsMonitorProvider.h"





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




    TEST_CLASS (MonitorEnumeratorTests)
    {
        public:
            TEST_METHOD (MonitorInfo_WidthHeight_ComputedFromBounds)
            {
                MonitorInfo monitor = MakeMonitor (100, 200, 1380, 920, 96, true);

                Assert::AreEqual (1280, monitor.Width());
                Assert::AreEqual (720, monitor.Height());
            }




            TEST_METHOD (InMemoryProvider_Empty_ReturnsNoMonitors)
            {
                InMemoryMonitorProvider provider (std::vector<MonitorInfo>{});

                Assert::AreEqual (size_t (0), provider.GetMonitors().size());
            }




            TEST_METHOD (InMemoryProvider_PreservesCountOrderAndFields)
            {
                std::vector<MonitorInfo> seed;
                seed.push_back (MakeMonitor (0, 0, 1920, 1080, 96, true));
                seed.push_back (MakeMonitor (1920, 0, 3840, 1080, 144, false));

                InMemoryMonitorProvider  provider (seed);
                std::vector<MonitorInfo> result = provider.GetMonitors();

                Assert::AreEqual (size_t (2), result.size());

                Assert::AreEqual (1920, result[0].Width());
                Assert::IsTrue (result[0].m_isPrimary);
                Assert::AreEqual (UINT (96), result[0].m_dpi);

                Assert::AreEqual (1920, result[1].Width());
                Assert::IsFalse (result[1].m_isPrimary);
                Assert::AreEqual (UINT (144), result[1].m_dpi);
                Assert::AreEqual (LONG (1920), result[1].m_bounds.left);
            }




            TEST_METHOD (WindowsProvider_LiveTopology_IsSelfConsistent)
            {
                WindowsMonitorProvider   provider;
                std::vector<MonitorInfo> monitors = provider.GetMonitors();

                // A headless/session-0 host may report no monitors; only assert
                // invariants when at least one display is present.
                if (monitors.empty())
                {
                    return;
                }

                int  primaryCount = 0;

                for (const MonitorInfo & monitor : monitors)
                {
                    Assert::IsTrue (monitor.Width() > 0);
                    Assert::IsTrue (monitor.Height() > 0);
                    Assert::IsTrue (monitor.m_dpi >= 96);
                    Assert::IsNotNull (monitor.m_handle);

                    if (monitor.m_isPrimary)
                    {
                        primaryCount++;
                    }
                }

                Assert::AreEqual (1, primaryCount);
            }
    };


}
