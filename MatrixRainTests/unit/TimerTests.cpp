#include "pch.h"

#include "../../MatrixRainCore/Timer.h"





namespace MatrixRainTests
{


    TEST_CLASS (TimerTests)
    {
        public:
        TEST_METHOD (Timer_Constructor_Initializes)
        {
            Timer timer;
            // Timer should start automatically on construction
            // Elapsed time should be very small (near zero)
            double elapsed = timer.GetElapsedSeconds();
            Assert::IsTrue (elapsed >= 0.0);
            Assert::IsTrue (elapsed < 0.01); // Less than 10ms
        }





        TEST_METHOD (Timer_GetElapsedSeconds_MeasuresTime)
        {
            Timer timer;
            timer.Start();

            // Sleep for approximately 50ms
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            double elapsed = timer.GetElapsedSeconds();

            // Should be at least 40ms (allowing for timing variations)
            Assert::IsTrue (elapsed >= 0.040);
            // Should be less than 100ms (very generous upper bound)
            Assert::IsTrue (elapsed < 0.100);
        }





        TEST_METHOD (Timer_GetElapsedMilliseconds_MeasuresTime)
        {
            Timer timer;
            timer.Start();

            // Sleep for approximately 50ms
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            double elapsed = timer.GetElapsedMilliseconds();

            // Should be at least 40ms
            Assert::IsTrue (elapsed >= 40.0);
            // Should be less than 100ms
            Assert::IsTrue (elapsed < 100.0);
        }





        TEST_METHOD (Timer_GetElapsedMilliseconds_MatchesSeconds)
        {
            Timer timer;
            timer.Start();

            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            double seconds = timer.GetElapsedSeconds();
            double milliseconds = timer.GetElapsedMilliseconds();

            // Milliseconds should be approximately 1000x seconds
            Assert::AreEqual (seconds * 1000.0, milliseconds, 0.1);
        }





        TEST_METHOD (Timer_Reset_RestartsTimer)
        {
            Timer timer;
            timer.Start();

            // Wait a bit
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            // Reset the timer
            timer.Reset();

            // Elapsed time should be near zero again
            double elapsed = timer.GetElapsedSeconds();
            Assert::IsTrue (elapsed >= 0.0);
            Assert::IsTrue (elapsed < 0.01); // Less than 10ms
        }





        TEST_METHOD (Timer_Start_RestartsTimer)
        {
            Timer timer;

            // Let some time pass
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            // Restart the timer
            timer.Start();

            // Elapsed time should be near zero
            double elapsed = timer.GetElapsedSeconds();
            Assert::IsTrue (elapsed >= 0.0);
            Assert::IsTrue (elapsed < 0.01); // Less than 10ms
        }





        TEST_METHOD (Timer_MultipleReads_ShowsIncreasingTime)
        {
            Timer timer;
            timer.Start();

            double elapsed1 = timer.GetElapsedSeconds();

            std::this_thread::sleep_for(std::chrono::milliseconds(20));

            double elapsed2 = timer.GetElapsedSeconds();

            std::this_thread::sleep_for(std::chrono::milliseconds(20));

            double elapsed3 = timer.GetElapsedSeconds();

            // Each reading should be larger than the previous
            Assert::IsTrue (elapsed2 > elapsed1);
            Assert::IsTrue (elapsed3 > elapsed2);
        }





        TEST_METHOD (Timer_HighResolution_SubMillisecondPrecision)
        {
            Timer timer;
            timer.Start();

            // Very short sleep (1ms)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

            double elapsed = timer.GetElapsedMilliseconds();

            // Should be able to measure time
            // (QueryPerformanceCounter should provide high precision)
            Assert::IsTrue (elapsed > 0.0);
            Assert::IsTrue (elapsed < 50.0); // Should be less than 50ms (generous for Windows scheduling)
        }





        TEST_METHOD (Timer_LongDuration_MaintainsAccuracy)
        {
            Timer timer;
            timer.Start();

            // Longer duration (200ms)
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            double elapsed = timer.GetElapsedMilliseconds();

            // Should be at least 180ms (allowing for variations)
            Assert::IsTrue (elapsed >= 180.0);
            // Should be less than 250ms
            Assert::IsTrue (elapsed < 250.0);
        }





        TEST_METHOD (Timer_ZeroElapsedTime_ImmediatelyAfterStart)
        {
            Timer timer;
            timer.Start();

            // Immediately check elapsed time
            double elapsed = timer.GetElapsedSeconds();

            // Should be very close to zero (but not necessarily exactly zero)
            Assert::IsTrue (elapsed >= 0.0);
            Assert::IsTrue (elapsed < 0.001); // Less than 1ms
        }
    };



}  // namespace MatrixRainTests

