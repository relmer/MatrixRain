#include "pch.h"


#include "MatrixRain/FPSCounter.h"


using namespace MatrixRain;
namespace MatrixRainTests
{
    TEST_CLASS(FPSCounterTests)
    {
    public:
        // T173: Test FPSCounter initialization
        TEST_METHOD(TestFPSCounterInitialization)
        {
            FPSCounter counter;
            
            // FPS should start at 0 before any updates
            Assert::AreEqual(0.0f, counter.GetFPS(), L"Initial FPS should be 0");
        }

        // T174: Test FPSCounter calculates average FPS over 1-second window
        TEST_METHOD(TestFPSCounterCalculatesAverageFPS)
        {
            FPSCounter counter;
            
            // Simulate 60 FPS by updating every ~16.67ms
            float deltaTime = 1.0f / 60.0f;
            
            // Update for slightly more than 1 second to ensure calculation happens
            for (int i = 0; i < 61; i++)  // Extra frame to trigger calculation
            {
                counter.Update(deltaTime);
            }
            
            // Should calculate approximately 60 FPS (allow tolerance for floating point)
            float fps = counter.GetFPS();
            Assert::IsTrue(fps >= 55.0f && fps <= 65.0f, L"FPS should be approximately 60");
        }

        // Test FPS calculation with varying frame times
        TEST_METHOD(TestFPSCounterWithVaryingFrameTimes)
        {
            FPSCounter counter;
            
            // Simulate 30 FPS (slower)
            float deltaTime = 1.0f / 30.0f;
            
            for (int i = 0; i < 30; i++)
            {
                counter.Update(deltaTime);
            }
            
            float fps = counter.GetFPS();
            Assert::IsTrue(fps >= 29.0f && fps <= 31.0f, L"FPS should be approximately 30");
        }

        // Test FPS counter resets properly when frame rate changes
        TEST_METHOD(TestFPSCounterAdaptsToFrameRateChanges)
        {
            FPSCounter counter;
            
            // Start with 60 FPS
            for (int i = 0; i < 60; i++)
            {
                counter.Update(1.0f / 60.0f);
            }
            
            // Switch to 30 FPS - should adapt over the 1-second window
            for (int i = 0; i < 60; i++)
            {
                counter.Update(1.0f / 30.0f);
            }
            
            float fps = counter.GetFPS();
            // After enough updates at 30 FPS, should converge to ~30
            Assert::IsTrue(fps >= 25.0f && fps <= 35.0f, L"FPS should adapt to new frame rate");
        }
    };
}
