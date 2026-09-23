#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\ColorMath.h"





namespace MatrixRainTests
{


    //  The round trip is required to hold to 1e-5 across the range
    //  (contracts/color-math.md).
    static constexpr float kRoundTripTolerance = 1e-5f;

    //  Step used to walk [0, 1] for the round-trip and monotonicity checks.
    static constexpr float kSweepStep          = 0.001f;





    TEST_CLASS (ColorMathTests)
    {
        public:

        ////////////////////////////////////////////////////////////////////////
        // sRGB transfer: endpoints and the documented sample
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD (SrgbToLinear_AtEndpoints_IsExact)
        {
            Assert::AreEqual (0.0f, SrgbToLinear (0.0f), 0.0f, L"Black must decode to exactly zero");
            Assert::AreEqual (1.0f, SrgbToLinear (1.0f), 1e-6f, L"White must decode to exactly one");
        }


        TEST_METHOD (LinearToSrgb_AtEndpoints_IsExact)
        {
            Assert::AreEqual (0.0f, LinearToSrgb (0.0f), 0.0f, L"Black must encode to exactly zero");
            Assert::AreEqual (1.0f, LinearToSrgb (1.0f), 1e-6f, L"White must encode to exactly one");
        }


        TEST_METHOD (SrgbToLinear_AtMidGrey_IsAboutPoint214)
        {
            //  The number that makes the whole feature necessary: half-way up
            //  the encoded scale is barely a fifth of the light.
            Assert::AreEqual (0.2140f, SrgbToLinear (0.5f), 5e-4f,
                              L"sRGB 0.5 must decode to ~0.2140 linear");
        }


        ////////////////////////////////////////////////////////////////////////
        // Round trip
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD (SrgbLinear_RoundTrips_AcrossTheWholeRange)
        {
            for (float encoded = 0.0f; encoded <= 1.0f; encoded += kSweepStep)
            {
                const float roundTripped = LinearToSrgb (SrgbToLinear (encoded));

                Assert::AreEqual (encoded, roundTripped, kRoundTripTolerance,
                                  L"Encoding a decoded value must return it unchanged");
            }
        }


        ////////////////////////////////////////////////////////////////////////
        // The knee
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD (SrgbToLinear_IsContinuousAtTheKnee)
        {
            const float below = SrgbToLinear (ColorMathConstants::kEncodedKnee - 1e-6f);
            const float above = SrgbToLinear (ColorMathConstants::kEncodedKnee + 1e-6f);

            Assert::AreEqual (below, above, 1e-6f,
                              L"The straight and curved segments must meet, or dark tones step");
        }


        TEST_METHOD (LinearToSrgb_IsContinuousAtTheKnee)
        {
            const float below = LinearToSrgb (ColorMathConstants::kLinearKnee - 1e-9f);
            const float above = LinearToSrgb (ColorMathConstants::kLinearKnee + 1e-9f);

            Assert::AreEqual (below, above, 1e-5f, L"The inverse must meet at its knee too");
        }


        TEST_METHOD (SrgbToLinear_KneesAreEachOthersImages)
        {
            //  The two thresholds are not independent numbers: each is the
            //  other's image under the transfer function. If someone edits one
            //  constant without the other, this is what catches it.
            Assert::AreEqual (ColorMathConstants::kLinearKnee,
                              SrgbToLinear (ColorMathConstants::kEncodedKnee),
                              1e-7f,
                              L"The encoded knee must decode to the linear knee");
        }


        ////////////////////////////////////////////////////////////////////////
        // Clamping and shape
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD (LinearToSrgb_ClampsOutOfRangeInput)
        {
            Assert::AreEqual (0.0f, LinearToSrgb (-0.5f),  0.0f,  L"Negative light encodes to black");
            Assert::AreEqual (1.0f, LinearToSrgb ( 4.0f),  1e-6f, L"Light above white encodes to white");
            Assert::AreEqual (1.0f, LinearToSrgb ( 1.0e6f), 1e-6f, L"...however far above");
        }


        TEST_METHOD (SrgbToLinear_DoesNotClampAboveOne)
        {
            //  Deliberately NOT clamped: linear light above 1 is what an HDR
            //  highlight is made of, and clamping here would destroy it before
            //  the output transform ever saw it.
            Assert::IsTrue (SrgbToLinear (2.0f) > 1.0f,
                            L"Values above white must decode to more than full light, not be clipped");
        }


        TEST_METHOD (SrgbToLinear_IsMonotonicallyIncreasing)
        {
            float previous = SrgbToLinear (0.0f);

            for (float encoded = kSweepStep; encoded <= 1.0f; encoded += kSweepStep)
            {
                const float current = SrgbToLinear (encoded);

                Assert::IsTrue (current > previous,
                                L"A brighter encoded value must always mean more light");
                previous = current;
            }
        }


        TEST_METHOD (SrgbToLinear_IsDarkerThanItsInputBelowWhite)
        {
            //  The curve lies below the diagonal everywhere inside the range.
            //  This is the whole reason gamma-space blending comes out wrong.
            for (float encoded = kSweepStep; encoded < 1.0f; encoded += 0.01f)
            {
                Assert::IsTrue (SrgbToLinear (encoded) < encoded,
                                L"Decoded light must sit below the encoded value everywhere below white");
            }
        }
    };


}
