#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\RebuildCoalescer.h"

#include <atomic>
#include <thread>
#include <vector>




namespace MatrixRainTests
{


    TEST_CLASS (RebuildCoalescerTests)
    {
        public:

            TEST_METHOD (RequestRebuild_FirstCall_ReturnsTrue)
            {
                RebuildCoalescer coalescer;

                Assert::IsTrue (coalescer.RequestRebuild());
            }




            TEST_METHOD (RequestRebuild_RepeatedCalls_OnlyFirstReturnsTrue)
            {
                RebuildCoalescer coalescer;

                Assert::IsTrue  (coalescer.RequestRebuild());
                Assert::IsFalse (coalescer.RequestRebuild());
                Assert::IsFalse (coalescer.RequestRebuild());
                Assert::IsFalse (coalescer.RequestRebuild());
            }




            TEST_METHOD (Consume_AfterRequest_AllowsNextRequest)
            {
                RebuildCoalescer coalescer;

                Assert::IsTrue  (coalescer.RequestRebuild());
                Assert::IsFalse (coalescer.RequestRebuild());
                coalescer.Consume();
                Assert::IsTrue  (coalescer.RequestRebuild());
            }




            TEST_METHOD (Consume_WithoutPriorRequest_DoesNotCrash)
            {
                RebuildCoalescer coalescer;

                coalescer.Consume();
                Assert::IsTrue (coalescer.RequestRebuild());
            }




            TEST_METHOD (RequestRebuild_FromManyThreads_ExactlyOneReturnsTrue)
            {
                constexpr int    kThreadCount = 32;
                RebuildCoalescer coalescer;
                std::atomic<int> trueCount {0};
                std::atomic<int> startGate {0};


                std::vector<std::thread> threads;
                threads.reserve (kThreadCount);

                for (int i = 0; i < kThreadCount; ++i)
                {
                    threads.emplace_back ([&]()
                    {
                        // Spin-wait so all threads contend at roughly the same moment.
                        while (startGate.load (std::memory_order_acquire) == 0)
                        {
                        }

                        if (coalescer.RequestRebuild())
                        {
                            trueCount.fetch_add (1, std::memory_order_relaxed);
                        }
                    });
                }


                startGate.store (1, std::memory_order_release);

                for (std::thread & t : threads)
                {
                    t.join();
                }


                Assert::AreEqual (1, trueCount.load (std::memory_order_relaxed));
            }
    };


}
