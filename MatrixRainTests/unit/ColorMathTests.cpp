#include "Pch_MatrixRainTests.h"

#include "..\..\MatrixRainCore\ColorMath.h"
#include "..\..\MatrixRainCore\ColorScheme.h"





namespace MatrixRainTests
{


    //  The round trip is required to hold to 1e-5 across the range
    //  (contracts/color-math.md).
    static constexpr float kRoundTripTolerance = 1e-5f;

    //  Step used to walk [0, 1] for the round-trip and monotonicity checks.
    static constexpr float kSweepStep          = 0.001f;

    //  Tolerance for the FR-005 regression: the rebuilt pipeline must land on
    //  the same displayed color v1.6 produced, to well inside one 8-bit code
    //  value (1/255 = 0.0039).
    static constexpr float kRegressionTolerance = 1e-4f;

    //  Brightness step for the regression sweep.
    static constexpr float kBrightnessStep      = 0.01f;





    ////////////////////////////////////////////////////////////////////////////
    //
    //  V16ShaderOutput
    //
    //  What v1.6 put ON SCREEN for one channel at full atlas coverage. The
    //  shader wrote
    //      rgb = color * texture * brightness;
    //      rgb += rgb * 0.3 * brightness;
    //      a   = texture.a * brightness;
    //  and alpha-blended over a black scene, so the displayed pixel was
    //  rgb * a, clamped by the 8-bit target. Written out longhand, from the
    //  shader rather than from ColorMath, so the regression test compares the
    //  new pipeline against the OLD behavior and not against itself.
    //
    //  The earlier version of this helper stopped at the shader's OUTPUT and
    //  left out the alpha. It passed while every fading trail rendered brighter
    //  than v1.6, because the missing factor was being applied as linear-light
    //  alpha instead. The screen is what FR-005 is about, not the shader.
    //
    ////////////////////////////////////////////////////////////////////////////

    static float V16ShaderOutput (float srgbChannel, float brightness)
    {
        const float scaled = srgbChannel * brightness;
        const float rgb    = scaled + scaled * 0.3f * brightness;
        const float alpha  = brightness;



        return std::min (1.0f, rgb * alpha);
    }





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


        TEST_METHOD (SrgbToLinear_AtMidGray_IsAboutPoint214)
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


        ////////////////////////////////////////////////////////////////////////
        // Instance color
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD (InstanceLinearColor_MatchesTheDocumentedFormula)
        {
            const Color4 srgb       (0.25f, 0.50f, 0.75f, 0.6f);
            const float  brightness = 0.8f;
            const float  gain       = 1.0f;
            const float  scale      = brightness * (1.0f + 0.3f * brightness) * brightness;

            const Color4 result = InstanceLinearColor (srgb, brightness, gain);

            Assert::AreEqual (SrgbToLinear (0.25f * scale), result.r, 1e-6f, L"Red");
            Assert::AreEqual (SrgbToLinear (0.50f * scale), result.g, 1e-6f, L"Green");
            Assert::AreEqual (SrgbToLinear (0.75f * scale), result.b, 1e-6f, L"Blue");
        }


        TEST_METHOD (InstanceLinearColor_PassesAlphaThroughUntouched)
        {
            //  Alpha carries the trail fade, which the shader still applies
            //  exactly where it always did.
            const Color4 srgb (1.0f, 1.0f, 1.0f, 0.42f);

            Assert::AreEqual (0.42f, InstanceLinearColor (srgb, 0.5f, 1.0f).a, 0.0f,
                              L"Alpha must survive the conversion unchanged");
            Assert::AreEqual (0.42f, InstanceLinearColor (srgb, 1.0f, 4.0f).a, 0.0f,
                              L"...including when a highlight gain is applied");
        }


        TEST_METHOD (InstanceLinearColor_AppliesHighlightGainToRgbOnly)
        {
            const Color4 srgb   (0.5f, 0.5f, 0.5f, 1.0f);
            const Color4 plain  = InstanceLinearColor (srgb, 1.0f, 1.0f);
            const Color4 gained = InstanceLinearColor (srgb, 1.0f, 3.0f);

            Assert::AreEqual (plain.r * 3.0f, gained.r, 1e-6f, L"Gain scales linear light directly");
            Assert::AreEqual (plain.a,        gained.a, 0.0f,  L"Gain must not touch alpha");
        }


        TEST_METHOD (InstanceLinearColor_AtZeroBrightness_IsBlack)
        {
            const Color4 result = InstanceLinearColor (Color4 (1.0f, 1.0f, 1.0f, 1.0f), 0.0f, 1.0f);

            Assert::AreEqual (0.0f, result.r, 0.0f, L"A fully faded character emits no light");
            Assert::AreEqual (0.0f, result.g, 0.0f, L"...green");
            Assert::AreEqual (0.0f, result.b, 0.0f, L"...blue");
        }


        ////////////////////////////////////////////////////////////////////////
        // FR-005 regression: the displayed color must not move
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD (InstanceLinearColor_ReproducesV16Output_ForEveryColorSchemeAndBrightness)
        {
            //  THE test that pins FR-005. Convert to linear the new way, encode
            //  back for an SDR display, and the result must be the pixel v1.6
            //  would have written. If this ever fails, the rain has changed
            //  color or brightness and a user would see it.
            const ColorScheme schemes[] = { ColorScheme::Green,
                                            ColorScheme::Blue,
                                            ColorScheme::Red,
                                            ColorScheme::Amber };

            for (ColorScheme scheme : schemes)
            {
                const Color4 srgb = GetColorRGB (scheme);

                for (float brightness = 0.0f; brightness <= 1.0f; brightness += kBrightnessStep)
                {
                    const Color4 linear = InstanceLinearColor (srgb, brightness, 1.0f);

                    Assert::AreEqual (V16ShaderOutput (srgb.r, brightness), LinearToSrgb (linear.r),
                                      kRegressionTolerance, L"Red channel must match v1.6");
                    Assert::AreEqual (V16ShaderOutput (srgb.g, brightness), LinearToSrgb (linear.g),
                                      kRegressionTolerance, L"Green channel must match v1.6");
                    Assert::AreEqual (V16ShaderOutput (srgb.b, brightness), LinearToSrgb (linear.b),
                                      kRegressionTolerance, L"Blue channel must match v1.6");
                }
            }
        }


        TEST_METHOD (InstanceLinearColor_ReproducesV16Output_ForWhiteHeadsAndCustomColors)
        {
            //  Heads are drawn white, and a user-picked custom color is not in
            //  the scheme table, so both are checked separately.
            const Color4 colors[] = { Color4 (1.0f, 1.0f, 1.0f, 1.0f),   // head
                                      Color4 (0.0f, 0.5f, 1.0f, 1.0f),   // the harness custom color
                                      Color4 (1.0f, 0.0f, 0.0f, 1.0f),
                                      Color4 (0.0f, 0.0f, 0.0f, 1.0f) };

            for (const Color4 & srgb : colors)
            {
                for (float brightness = 0.0f; brightness <= 1.0f; brightness += kBrightnessStep)
                {
                    const Color4 linear = InstanceLinearColor (srgb, brightness, 1.0f);

                    Assert::AreEqual (V16ShaderOutput (srgb.r, brightness), LinearToSrgb (linear.r),
                                      kRegressionTolerance, L"Red channel must match v1.6");
                    Assert::AreEqual (V16ShaderOutput (srgb.g, brightness), LinearToSrgb (linear.g),
                                      kRegressionTolerance, L"Green channel must match v1.6");
                    Assert::AreEqual (V16ShaderOutput (srgb.b, brightness), LinearToSrgb (linear.b),
                                      kRegressionTolerance, L"Blue channel must match v1.6");
                }
            }
        }


        TEST_METHOD (InstanceLinearColor_ClipsAboveWhiteExactlyAsV16Did)
        {
            //  At full brightness the self-glow pushes a bright color past
            //  white. v1.6's 8-bit target clipped it; the new path must clip in
            //  the same place rather than quietly keeping the overshoot.
            const Color4 srgb   (1.0f, 1.0f, 1.0f, 1.0f);
            const Color4 linear = InstanceLinearColor (srgb, 1.0f, 1.0f);

            Assert::IsTrue (linear.r > 1.0f,
                            L"Linear light keeps the overshoot, so HDR can later use it");
            Assert::AreEqual (1.0f, LinearToSrgb (linear.r), 1e-6f,
                              L"...but encoding for SDR clips it to white, as v1.6 did");
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
