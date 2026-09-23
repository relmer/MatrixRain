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

    //  The GPU's polynomial curves, measured against the exact ones in 8-bit
    //  code values (1/255). The bounds are the fits' verified worst cases
    //  (ColorConstants.h) with a little room for the sweep landing between
    //  the points the fit was checked at; a coefficient typo blows through
    //  either by orders of magnitude.
    static constexpr float kDecodeCodeBound     = 0.02f;
    static constexpr float kEncodeCodeBound     = 0.10f;
    static constexpr float kCodeValue           = 255.0f;

    //  Step for the polynomial sweeps: ten thousand points across [0, 1].
    static constexpr float kFineSweepStep       = 0.0001f;





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

        TEST_METHOD (InstanceDisplayColor_MatchesTheDocumentedFormula)
        {
            const Color4 srgb       (0.25f, 0.50f, 0.75f, 0.6f);
            const float  brightness = 0.8f;
            const float  scale      = brightness * (1.0f + 0.3f * brightness) * brightness;

            const Color4 result = InstanceDisplayColor (srgb, brightness);

            Assert::AreEqual (0.25f * scale, result.r, 1e-6f, L"Red");
            Assert::AreEqual (0.50f * scale, result.g, 1e-6f, L"Green");
            Assert::AreEqual (0.75f * scale, result.b, 1e-6f, L"Blue");
        }


        TEST_METHOD (InstanceDisplayColor_PassesAlphaThroughUntouched)
        {
            const Color4 srgb (1.0f, 1.0f, 1.0f, 0.42f);

            Assert::AreEqual (0.42f, InstanceDisplayColor (srgb, 0.5f).a, 0.0f,
                              L"Alpha must survive unchanged");
        }


        TEST_METHOD (InstanceDisplayColor_AtZeroBrightness_IsBlack)
        {
            const Color4 result = InstanceDisplayColor (Color4 (1.0f, 1.0f, 1.0f, 1.0f), 0.0f);

            Assert::AreEqual (0.0f, result.r, 0.0f, L"A fully faded character emits no light");
            Assert::AreEqual (0.0f, result.g, 0.0f, L"...green");
            Assert::AreEqual (0.0f, result.b, 0.0f, L"...blue");
        }


        TEST_METHOD (InstanceDisplayColor_IsNotClipped_TheShaderClipsIt)
        {
            //  A white head at full brightness is 1.3. The shader clips at
            //  white exactly where v1.6's 8-bit target did, after coverage;
            //  clipping here would clip in the wrong place for edge pixels.
            const Color4 result = InstanceDisplayColor (Color4 (1.0f, 1.0f, 1.0f, 1.0f), 1.0f);

            Assert::AreEqual (1.3f, result.r, 1e-6f, L"Self-glow overshoot must reach the shader intact");
        }


        ////////////////////////////////////////////////////////////////////////
        // FR-005 regression: the displayed pixel must not move
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD (InstanceDisplayColor_ReproducesV16Output_ForEveryColorSchemeAndBrightness)
        {
            //  THE test that pins FR-005. At full coverage the shader displays
            //  min(1, InstanceDisplayColor), so that clipped value must be the
            //  pixel v1.6 put on screen. If this ever fails, the rain has
            //  changed color or brightness and a user would see it.
            const ColorScheme schemes[] = { ColorScheme::Green,
                                            ColorScheme::Blue,
                                            ColorScheme::Red,
                                            ColorScheme::Amber };

            for (ColorScheme scheme : schemes)
            {
                const Color4 srgb = GetColorRGB (scheme);

                for (float brightness = 0.0f; brightness <= 1.0f; brightness += kBrightnessStep)
                {
                    const Color4 displayed = InstanceDisplayColor (srgb, brightness);

                    Assert::AreEqual (V16ShaderOutput (srgb.r, brightness), std::min (1.0f, displayed.r),
                                      kRegressionTolerance, L"Red channel must match v1.6");
                    Assert::AreEqual (V16ShaderOutput (srgb.g, brightness), std::min (1.0f, displayed.g),
                                      kRegressionTolerance, L"Green channel must match v1.6");
                    Assert::AreEqual (V16ShaderOutput (srgb.b, brightness), std::min (1.0f, displayed.b),
                                      kRegressionTolerance, L"Blue channel must match v1.6");
                }
            }
        }


        TEST_METHOD (InstanceDisplayColor_ReproducesV16Output_ForWhiteHeadsAndCustomColors)
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
                    const Color4 displayed = InstanceDisplayColor (srgb, brightness);

                    Assert::AreEqual (V16ShaderOutput (srgb.r, brightness), std::min (1.0f, displayed.r),
                                      kRegressionTolerance, L"Red channel must match v1.6");
                    Assert::AreEqual (V16ShaderOutput (srgb.g, brightness), std::min (1.0f, displayed.g),
                                      kRegressionTolerance, L"Green channel must match v1.6");
                    Assert::AreEqual (V16ShaderOutput (srgb.b, brightness), std::min (1.0f, displayed.b),
                                      kRegressionTolerance, L"Blue channel must match v1.6");
                }
            }
        }


        ////////////////////////////////////////////////////////////////////////
        // The GPU's polynomial curves
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD (SrgbToLinearPolynomial_MatchesTheCurve_ToWellUnderOneCodeValue)
        {
            //  Decoding error is judged where it would show: after an exact
            //  re-encode, in code values. A linear-light error near black is
            //  worth many more code values than the same error near white.
            float worst = 0.0f;



            for (float encoded = 0.0f; encoded <= 1.0f; encoded += kFineSweepStep)
            {
                const float back = LinearToSrgb (SrgbToLinearPolynomial (encoded));


                worst = std::max (worst, std::abs (back - encoded) * kCodeValue);
            }

            Assert::IsTrue (worst < kDecodeCodeBound,
                            (L"Decode polynomial worst error in code values: " + std::to_wstring (worst)).c_str());
        }

        TEST_METHOD (LinearToSrgbPolynomial_MatchesTheCurve_ToWellUnderOneCodeValue)
        {
            float worst = 0.0f;



            for (float linear = 0.0f; linear <= 1.0f; linear += kFineSweepStep)
            {
                const float exact = LinearToSrgb (linear);
                const float fit   = LinearToSrgbPolynomial (linear);


                worst = std::max (worst, std::abs (fit - exact) * kCodeValue);
            }

            Assert::IsTrue (worst < kEncodeCodeBound,
                            (L"Encode polynomial worst error in code values: " + std::to_wstring (worst)).c_str());
        }

        TEST_METHOD (Polynomials_AreContinuousAtTheKnees)
        {
            //  The fits were made over the curved segments only; they have to
            //  meet the straight segments without a step. The sweeps above
            //  are unlikely to land exactly on a knee, so check both sides.
            const float encodedKnee = ColorMathConstants::kEncodedKnee;
            const float linearKnee  = ColorMathConstants::kLinearKnee;
            const float nudge       = 1e-6f;



            Assert::AreEqual (LinearToSrgb (SrgbToLinearPolynomial (encodedKnee - nudge)),
                              LinearToSrgb (SrgbToLinearPolynomial (encodedKnee + nudge)),
                              kDecodeCodeBound / kCodeValue,
                              L"Decode polynomial must meet the straight segment at the knee");

            Assert::AreEqual (LinearToSrgbPolynomial (linearKnee - nudge),
                              LinearToSrgbPolynomial (linearKnee + nudge),
                              kEncodeCodeBound / kCodeValue,
                              L"Encode polynomial must meet the straight segment at the knee");
        }

        TEST_METHOD (Polynomials_AreMonotonicallyIncreasing)
        {
            //  A fit that wiggles would put a visible reversal into a smooth
            //  fade. Neither may ever go down.
            float previousLinear  = SrgbToLinearPolynomial (0.0f);
            float previousEncoded = LinearToSrgbPolynomial (0.0f);



            for (float value = kFineSweepStep; value <= 1.0f; value += kFineSweepStep)
            {
                const float currentLinear  = SrgbToLinearPolynomial (value);
                const float currentEncoded = LinearToSrgbPolynomial (value);


                Assert::IsTrue (currentLinear  >= previousLinear,  L"Decode polynomial went down");
                Assert::IsTrue (currentEncoded >= previousEncoded, L"Encode polynomial went down");

                previousLinear  = currentLinear;
                previousEncoded = currentEncoded;
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
