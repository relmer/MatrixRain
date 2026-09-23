#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\FrameMetrics.h"





namespace MatrixRainTests
{


    //  Rec.709 weights, repeated here so the tests state the expected answer
    //  independently of the implementation's own constants.
    static constexpr float kLumaR    = 0.2126f;
    static constexpr float kLumaG    = 0.7152f;
    static constexpr float kLumaB    = 0.0722f;

    //  Frame size used by the comparison tests. Small enough to build by hand,
    //  large enough that a 99th percentile means something.
    static constexpr UINT  kDiffSide = 100;





    ////////////////////////////////////////////////////////////////////////////
    //
    //  SrgbToLinearReference
    //
    //  IEC 61966-2-1, written out here so the expectation does not borrow the
    //  implementation's decode.
    //
    ////////////////////////////////////////////////////////////////////////////

    static float SrgbToLinearReference (float value)
    {
        if (value <= 0.04045f)
        {
            return value / 12.92f;
        }

        return std::pow ((value + 0.055f) / 1.055f, 2.4f);
    }





    ////////////////////////////////////////////////////////////////////////////
    //
    //  MakeUniformBgra
    //
    ////////////////////////////////////////////////////////////////////////////

    static std::vector<uint8_t> MakeUniformBgra (UINT width, UINT height, uint8_t blue, uint8_t green, uint8_t red)
    {
        std::vector<uint8_t> frame (static_cast<size_t> (width) * height * 4);

        for (size_t pixel = 0; pixel < static_cast<size_t> (width) * height; ++pixel)
        {
            frame[pixel * 4 + 0] = blue;
            frame[pixel * 4 + 1] = green;
            frame[pixel * 4 + 2] = red;
            frame[pixel * 4 + 3] = 255;
        }

        return frame;
    }





    TEST_CLASS (FrameMetricsTests)
    {
        public:

        ////////////////////////////////////////////////////////////////////////
        // MeanLuminance
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD (MeanLuminance_OfUniform8Bit_IsTheDecodedRec709Luma)
        {
            const std::vector<uint8_t> frame    = MakeUniformBgra (16, 16, 40, 200, 90);
            const float                expected = kLumaR * SrgbToLinearReference (90.0f  / 255.0f)
                                                + kLumaG * SrgbToLinearReference (200.0f / 255.0f)
                                                + kLumaB * SrgbToLinearReference (40.0f  / 255.0f);

            Assert::AreEqual (expected, MeanLuminance (frame, 16, 16), 1e-5f,
                              L"A uniform frame's mean luminance is that color's linear Rec.709 luma");
        }


        TEST_METHOD (MeanLuminance_OfUniformLinear_IsTheRec709Luma)
        {
            std::vector<float> frame (16 * 16 * 4);

            for (size_t pixel = 0; pixel < 16 * 16; ++pixel)
            {
                frame[pixel * 4 + 0] = 0.25f;
                frame[pixel * 4 + 1] = 0.50f;
                frame[pixel * 4 + 2] = 0.75f;
                frame[pixel * 4 + 3] = 1.0f;
            }

            const float expected = kLumaR * 0.25f + kLumaG * 0.50f + kLumaB * 0.75f;

            Assert::AreEqual (expected, MeanLuminance (std::span<const float> (frame), 16, 16), 1e-6f,
                              L"Linear input must be weighted straight through, with no decode");
        }


        TEST_METHOD (MeanLuminance_OfBlack_IsZero)
        {
            const std::vector<uint8_t> frame = MakeUniformBgra (8, 8, 0, 0, 0);

            Assert::AreEqual (0.0f, MeanLuminance (frame, 8, 8), 1e-7f,
                              L"Black must measure as no light at all");
        }


        TEST_METHOD (MeanLuminance_OfWhite_IsOne)
        {
            const std::vector<uint8_t> frame = MakeUniformBgra (8, 8, 255, 255, 255);

            Assert::AreEqual (1.0f, MeanLuminance (frame, 8, 8), 1e-5f,
                              L"White must measure as full linear luminance");
        }


        TEST_METHOD (MeanLuminance_AveragesOverTheWholeFrame)
        {
            //  Half white, half black: the mean must land halfway.
            std::vector<uint8_t> frame = MakeUniformBgra (8, 8, 0, 0, 0);

            for (size_t pixel = 0; pixel < 32; ++pixel)
            {
                frame[pixel * 4 + 0] = 255;
                frame[pixel * 4 + 1] = 255;
                frame[pixel * 4 + 2] = 255;
            }

            Assert::AreEqual (0.5f, MeanLuminance (frame, 8, 8), 1e-5f,
                              L"The metric must average over every pixel, not sample one");
        }


        TEST_METHOD (MeanLuminance_WithAShortBuffer_IsZero)
        {
            const std::vector<uint8_t> frame = MakeUniformBgra (4, 4, 255, 255, 255);

            Assert::AreEqual (0.0f, MeanLuminance (frame, 8, 8), 1e-7f,
                              L"A buffer too small for the stated size must not be read past its end");
            Assert::AreEqual (0.0f, MeanLuminance (frame, 0, 0), 1e-7f,
                              L"An empty frame has no luminance to report");
        }


        ////////////////////////////////////////////////////////////////////////
        // CompareFrames
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD (CompareFrames_OfIdenticalFrames_ReportsNoDifference)
        {
            const std::vector<uint8_t> frame  = MakeUniformBgra (kDiffSide, kDiffSide, 10, 90, 30);
            const FrameDifference      result = CompareFrames (frame, frame, kDiffSide, kDiffSide, 0.0f);

            Assert::AreEqual (0.0f, result.m_maxDifference,  1e-7f, L"Max difference");
            Assert::AreEqual (0.0f, result.m_meanDifference, 1e-7f, L"Mean difference");
            Assert::AreEqual (0.0f, result.m_p99Difference,  1e-7f, L"99th percentile");
            Assert::AreEqual (static_cast<size_t> (0), result.m_pixelsOverThreshold,
                              L"A frame compared with itself has changed nowhere");
        }


        TEST_METHOD (CompareFrames_FindsASingleChangedPixelAndItsPlace)
        {
            const std::vector<uint8_t> baseline  = MakeUniformBgra (kDiffSide, kDiffSide, 0, 0, 0);
            std::vector<uint8_t>       candidate = baseline;
            const size_t               changed   = static_cast<size_t> (7) * kDiffSide + 3;

            candidate[changed * 4 + 1] = 40;

            const FrameDifference result = CompareFrames (baseline, candidate, kDiffSide, kDiffSide, 1.0f);

            Assert::AreEqual (40.0f, result.m_maxDifference, 1e-7f,
                              L"The largest difference is the one pixel that moved");
            Assert::AreEqual (3L, result.m_maxDifferenceAt.x, L"Its x, so a reader can go and look");
            Assert::AreEqual (7L, result.m_maxDifferenceAt.y, L"Its y");
            Assert::AreEqual (static_cast<size_t> (1), result.m_pixelsOverThreshold,
                              L"Exactly one pixel is over the threshold");
        }


        TEST_METHOD (CompareFrames_TakesTheLargestChannelDifference)
        {
            //  A hue shift can leave overall brightness alone, so the metric
            //  must look per channel rather than at luminance.
            const std::vector<uint8_t> baseline  = MakeUniformBgra (kDiffSide, kDiffSide, 100, 100, 100);
            const std::vector<uint8_t> candidate = MakeUniformBgra (kDiffSide, kDiffSide, 100, 100, 130);

            const FrameDifference result = CompareFrames (baseline, candidate, kDiffSide, kDiffSide, 1.0f);

            Assert::AreEqual (30.0f, result.m_maxDifference,  1e-7f, L"Red moved by 30, so the difference is 30");
            Assert::AreEqual (30.0f, result.m_meanDifference, 1e-5f, L"Every pixel moved, so the mean matches");
        }


        TEST_METHOD (CompareFrames_MeanAveragesOverTheWholeFrame)
        {
            //  A quarter of the frame moves by 40; the mean must be 10.
            const std::vector<uint8_t> baseline  = MakeUniformBgra (kDiffSide, kDiffSide, 0, 0, 0);
            std::vector<uint8_t>       candidate = baseline;
            const size_t               quarter   = static_cast<size_t> (kDiffSide) * kDiffSide / 4;

            for (size_t pixel = 0; pixel < quarter; ++pixel)
            {
                candidate[pixel * 4 + 1] = 40;
            }

            const FrameDifference result = CompareFrames (baseline, candidate, kDiffSide, kDiffSide, 1.0f);

            Assert::AreEqual (10.0f, result.m_meanDifference, 1e-4f,
                              L"Mean difference is over every pixel, changed or not");
            Assert::AreEqual (quarter, result.m_pixelsOverThreshold,
                              L"Every changed pixel must be counted");
        }


        TEST_METHOD (CompareFrames_P99SurvivesAChangeInASmallPartOfTheFrame)
        {
            //  The top 2% of pixels move. The mean barely stirs; the 99th
            //  percentile must see it. This is the case the metric exists for:
            //  a ring around every streak head is a small fraction of a frame.
            const std::vector<uint8_t> baseline   = MakeUniformBgra (kDiffSide, kDiffSide, 0, 0, 0);
            std::vector<uint8_t>       candidate  = baseline;
            const size_t               pixelCount = static_cast<size_t> (kDiffSide) * kDiffSide;
            const size_t               changed    = pixelCount * 2 / 100;

            for (size_t pixel = 0; pixel < changed; ++pixel)
            {
                candidate[pixel * 4 + 1] = 200;
            }

            const FrameDifference result = CompareFrames (baseline, candidate, kDiffSide, kDiffSide, 1.0f);

            Assert::AreEqual (200.0f, result.m_p99Difference, 1e-7f,
                              L"A change in 2% of the frame must show up in the 99th percentile");
            Assert::IsTrue (result.m_meanDifference < 5.0f,
                            L"...even though the mean stays near nothing, which is why both are reported");
        }


        TEST_METHOD (CompareFrames_IgnoresAlpha)
        {
            const std::vector<uint8_t> baseline  = MakeUniformBgra (kDiffSide, kDiffSide, 50, 50, 50);
            std::vector<uint8_t>       candidate = baseline;

            for (size_t pixel = 0; pixel < static_cast<size_t> (kDiffSide) * kDiffSide; ++pixel)
            {
                candidate[pixel * 4 + 3] = 0;
            }

            const FrameDifference result = CompareFrames (baseline, candidate, kDiffSide, kDiffSide, 0.0f);

            Assert::AreEqual (0.0f, result.m_maxDifference, 1e-7f,
                              L"Alpha is not displayed, so a change in it is not a visible difference");
        }


        TEST_METHOD (CompareFrames_WithAShortBuffer_ReportsNothing)
        {
            const std::vector<uint8_t> full  = MakeUniformBgra (kDiffSide, kDiffSide, 255, 255, 255);
            const std::vector<uint8_t> tooSmall = MakeUniformBgra (4, 4, 0, 0, 0);

            const FrameDifference result = CompareFrames (full, tooSmall, kDiffSide, kDiffSide, 1.0f);

            Assert::AreEqual (0.0f, result.m_maxDifference, 1e-7f,
                              L"A mismatched buffer must be refused, not read past its end");
            Assert::AreEqual (static_cast<size_t> (0), result.m_pixelsOverThreshold, L"...and count nothing");
        }
    };


}
