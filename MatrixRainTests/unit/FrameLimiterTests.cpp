#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\FrameLimiter.h"

#include <chrono>




namespace MatrixRainTests
{


    TEST_CLASS (FrameLimiterTests)
    {
        public:

            //
            //  ShouldEngageFrameLimiter
            //

            TEST_METHOD (ShouldEngageFrameLimiter_AtOrBelow60_ReturnsFalse)
            {
                Assert::IsFalse (ShouldEngageFrameLimiter (0));
                Assert::IsFalse (ShouldEngageFrameLimiter (30));
                Assert::IsFalse (ShouldEngageFrameLimiter (59));
                Assert::IsFalse (ShouldEngageFrameLimiter (60));
            }




            TEST_METHOD (ShouldEngageFrameLimiter_Above60_ReturnsTrue)
            {
                Assert::IsTrue (ShouldEngageFrameLimiter (61));
                Assert::IsTrue (ShouldEngageFrameLimiter (75));
                Assert::IsTrue (ShouldEngageFrameLimiter (120));
                Assert::IsTrue (ShouldEngageFrameLimiter (144));
                Assert::IsTrue (ShouldEngageFrameLimiter (240));
            }




            //
            //  FrameLimiter
            //

            TEST_METHOD (WaitForNextFrame_FirstCall_ReturnsImmediately)
            {
                using clock = std::chrono::steady_clock;

                FrameLimiter        limiter (60);
                clock::time_point   t0 = clock::now();

                limiter.WaitForNextFrame();

                clock::time_point t1 = clock::now();
                auto              elapsed = std::chrono::duration_cast<std::chrono::milliseconds> (t1 - t0).count();

                Assert::IsTrue (elapsed < 5, L"First WaitForNextFrame should return effectively immediately");
            }




            TEST_METHOD (WaitForNextFrame_SecondCall_SleepsApproxFrameInterval)
            {
                using clock = std::chrono::steady_clock;

                FrameLimiter      limiter (60);
                clock::time_point t0;
                clock::time_point t1;


                limiter.WaitForNextFrame();   // primes m_lastFrameTime
                t0 = clock::now();
                limiter.WaitForNextFrame();   // should sleep ~16.6ms minus the (tiny) gap above
                t1 = clock::now();

                auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds> (t1 - t0).count();

                // Sleep granularity on Windows is usually >=10ms; allow a wide
                // tolerance so CI scheduler jitter does not flake the test.
                Assert::IsTrue (elapsedMs >= 10, L"Second WaitForNextFrame should sleep at least ~10ms at 60 fps");
                Assert::IsTrue (elapsedMs <= 50, L"Second WaitForNextFrame should not over-sleep beyond ~50ms");
            }
    };


}
