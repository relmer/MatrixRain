#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\ScanlineStyleMapping.h"





namespace MatrixRainTests
{


    TEST_CLASS (ScanlineStyleMappingTests)
    {
        public:
        TEST_METHOD (ScanlineLineCount_AtStyle1_IsAbout981)
        {
            const float lines = ScanlineLineCount (1);
            Assert::IsTrue (std::abs (lines - 981.0f) <= 2.0f,
                            L"Expected ~981 lines at style=1");
        }


        TEST_METHOD (ScanlineLineCount_AtStyle25_IsAbout622)
        {
            const float lines = ScanlineLineCount (25);
            Assert::IsTrue (std::abs (lines - 622.0f) <= 2.0f,
                            L"Expected ~622 lines at style=25");
        }


        TEST_METHOD (ScanlineLineCount_AtStyle50_IsAbout387)
        {
            const float lines = ScanlineLineCount (50);
            Assert::IsTrue (std::abs (lines - 387.0f) <= 2.0f,
                            L"Expected ~387 lines at style=50");
        }


        TEST_METHOD (ScanlineLineCount_AtStyle75_IsAbout241)
        {
            const float lines = ScanlineLineCount (75);
            Assert::IsTrue (std::abs (lines - 241.0f) <= 2.0f,
                            L"Expected ~241 lines at style=75");
        }


        TEST_METHOD (ScanlineLineCount_AtStyle100_IsAbout150)
        {
            const float lines = ScanlineLineCount (100);
            Assert::IsTrue (std::abs (lines - 150.0f) <= 2.0f,
                            L"Expected ~150 lines at style=100");
        }


        TEST_METHOD (ScanlineLineCount_AtEndpoint1_IsClampInvariant)
        {
            // Inputs at or below the [1,100] endpoint must produce identical output.
            const float linesAt1     = ScanlineLineCount (1);
            const float linesBelow1  = ScanlineLineCount (0);
            const float linesFarBelow = ScanlineLineCount (-50);


            Assert::AreEqual (linesAt1, linesBelow1,    0.0001f);
            Assert::AreEqual (linesAt1, linesFarBelow,  0.0001f);
        }


        TEST_METHOD (ScanlineLineCount_AtEndpoint100_IsClampInvariant)
        {
            // Inputs at or above the [1,100] endpoint must produce identical output.
            const float linesAt100     = ScanlineLineCount (100);
            const float linesAbove100  = ScanlineLineCount (101);
            const float linesFarAbove  = ScanlineLineCount (200);


            Assert::AreEqual (linesAt100, linesAbove100, 0.0001f);
            Assert::AreEqual (linesAt100, linesFarAbove, 0.0001f);
        }
    };


}
