#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\FrameMetrics.h"





namespace MatrixRainTests
{


    //  Rec.709 weights, repeated here so the tests state the expected answer
    //  independently of the implementation's own constants.
    static constexpr float kLumaR        = 0.2126f;
    static constexpr float kLumaG        = 0.7152f;
    static constexpr float kLumaB        = 0.0722f;

    //  A synthetic halo big enough that its half-maximum radius sits well
    //  inside the frame, with the centre in the middle.
    static constexpr UINT  kHaloWidth    = 129;
    static constexpr UINT  kHaloHeight   = 129;
    static constexpr float kHaloSigma    = 12.0f;

    //  A Gaussian exp (-r^2 / 2 sigma^2) reaches half its peak at
    //  sigma * sqrt (2 ln 2).
    static constexpr float kHalfMaxScale = 1.1774100f;

    //  The tolerance T003 asks for on the halo radius.
    static constexpr float kRadiusTolerancePx = 0.5f;





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





    ////////////////////////////////////////////////////////////////////////////
    //
    //  GaussianAt
    //
    ////////////////////////////////////////////////////////////////////////////

    static float GaussianAt (UINT x, UINT y, POINT center, float sigma)
    {
        const float dx = static_cast<float> (static_cast<LONG> (x) - center.x);
        const float dy = static_cast<float> (static_cast<LONG> (y) - center.y);



        return std::exp (-(dx * dx + dy * dy) / (2.0f * sigma * sigma));
    }





    ////////////////////////////////////////////////////////////////////////////
    //
    //  MakeGaussianLinear
    //
    //  A radially symmetric white halo in linear float RGBA.
    //
    ////////////////////////////////////////////////////////////////////////////

    static std::vector<float> MakeGaussianLinear (UINT width, UINT height, POINT center, float sigma)
    {
        std::vector<float> frame (static_cast<size_t> (width) * height * 4);

        for (UINT y = 0; y < height; ++y)
        {
            for (UINT x = 0; x < width; ++x)
            {
                const size_t offset = (static_cast<size_t> (y) * width + x) * 4;
                const float  value  = GaussianAt (x, y, center, sigma);

                frame[offset + 0] = value;
                frame[offset + 1] = value;
                frame[offset + 2] = value;
                frame[offset + 3] = 1.0f;
            }
        }

        return frame;
    }





    ////////////////////////////////////////////////////////////////////////////
    //
    //  MakeGaussianBgra
    //
    //  The same halo, sRGB-encoded into 8 bits -- what a back-buffer read-back
    //  of an SDR frame actually looks like.
    //
    ////////////////////////////////////////////////////////////////////////////

    static std::vector<uint8_t> MakeGaussianBgra (UINT width, UINT height, POINT center, float sigma)
    {
        std::vector<uint8_t> frame (static_cast<size_t> (width) * height * 4);

        for (UINT y = 0; y < height; ++y)
        {
            for (UINT x = 0; x < width; ++x)
            {
                const size_t offset  = (static_cast<size_t> (y) * width + x) * 4;
                const float  linear  = GaussianAt (x, y, center, sigma);
                const float  encoded = (linear <= 0.0031308f)
                                       ? linear * 12.92f
                                       : 1.055f * std::pow (linear, 1.0f / 2.4f) - 0.055f;
                const uint8_t stored = static_cast<uint8_t> (std::lround (encoded * 255.0f));

                frame[offset + 0] = stored;
                frame[offset + 1] = stored;
                frame[offset + 2] = stored;
                frame[offset + 3] = 255;
            }
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
                              L"A uniform frame's mean luminance is that colour's linear Rec.709 luma");
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
        // HaloFalloffRadius
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD (HaloFalloffRadius_OfALinearGaussian_MatchesTheAnalyticHalfMaximum)
        {
            const POINT              center   = { static_cast<LONG> (kHaloWidth  / 2),
                                                  static_cast<LONG> (kHaloHeight / 2) };
            const std::vector<float> frame    = MakeGaussianLinear (kHaloWidth, kHaloHeight, center, kHaloSigma);
            const float              expected = kHaloSigma * kHalfMaxScale;
            const float              measured = HaloFalloffRadius (std::span<const float> (frame),
                                                                   kHaloWidth,
                                                                   kHaloHeight,
                                                                   center);

            Assert::AreEqual (expected, measured, kRadiusTolerancePx,
                              L"The measured radius must match sigma * sqrt (2 ln 2) within half a pixel");
        }


        TEST_METHOD (HaloFalloffRadius_OfAnSrgbGaussian_MatchesTheAnalyticHalfMaximum)
        {
            const POINT                center   = { static_cast<LONG> (kHaloWidth  / 2),
                                                    static_cast<LONG> (kHaloHeight / 2) };
            const std::vector<uint8_t> frame    = MakeGaussianBgra (kHaloWidth, kHaloHeight, center, kHaloSigma);
            const float                expected = kHaloSigma * kHalfMaxScale;
            const float                measured = HaloFalloffRadius (frame, kHaloWidth, kHaloHeight, center);

            Assert::AreEqual (expected, measured, kRadiusTolerancePx,
                              L"8-bit input must be decoded before measuring, or the radius comes out wrong");
        }


        TEST_METHOD (HaloFalloffRadius_ScalesWithTheHaloWidth)
        {
            const POINT              center = { static_cast<LONG> (kHaloWidth  / 2),
                                                static_cast<LONG> (kHaloHeight / 2) };
            const std::vector<float> narrow = MakeGaussianLinear (kHaloWidth, kHaloHeight, center, 6.0f);
            const std::vector<float> wide   = MakeGaussianLinear (kHaloWidth, kHaloHeight, center, 18.0f);

            const float narrowRadius = HaloFalloffRadius (std::span<const float> (narrow),
                                                          kHaloWidth, kHaloHeight, center);
            const float wideRadius   = HaloFalloffRadius (std::span<const float> (wide),
                                                          kHaloWidth, kHaloHeight, center);

            Assert::AreEqual (6.0f  * kHalfMaxScale, narrowRadius, kRadiusTolerancePx, L"Narrow halo");
            Assert::AreEqual (18.0f * kHalfMaxScale, wideRadius,   kRadiusTolerancePx, L"Wide halo");
            Assert::IsTrue (wideRadius > narrowRadius,
                            L"A wider glow must report a larger radius -- this is the metric's whole job");
        }


        TEST_METHOD (HaloFalloffRadius_OnABlackFrame_IsZero)
        {
            const std::vector<uint8_t> frame  = MakeUniformBgra (32, 32, 0, 0, 0);
            const POINT                center = { 16, 16 };

            Assert::AreEqual (0.0f, HaloFalloffRadius (frame, 32, 32, center), 1e-7f,
                              L"With no light at the centre there is no falloff to measure");
        }


        TEST_METHOD (HaloFalloffRadius_OnAFlatFrame_IsZero)
        {
            const std::vector<uint8_t> frame  = MakeUniformBgra (32, 32, 255, 255, 255);
            const POINT                center = { 16, 16 };

            Assert::AreEqual (0.0f, HaloFalloffRadius (frame, 32, 32, center), 1e-7f,
                              L"A frame that never falls to half its peak has no half-maximum radius");
        }


        TEST_METHOD (HaloFalloffRadius_WithACentreOutsideTheFrame_IsZero)
        {
            const std::vector<uint8_t> frame = MakeUniformBgra (32, 32, 255, 255, 255);

            Assert::AreEqual (0.0f, HaloFalloffRadius (frame, 32, 32, POINT { -1, 16 }), 1e-7f,
                              L"A centre left of the frame must be rejected, not sampled");
            Assert::AreEqual (0.0f, HaloFalloffRadius (frame, 32, 32, POINT { 16, 32 }), 1e-7f,
                              L"A centre below the frame must be rejected, not sampled");
        }
    };


}
