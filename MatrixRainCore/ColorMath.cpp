#include "pch.h"

#include "ColorMath.h"





////////////////////////////////////////////////////////////////////////////////
//
//  SrgbToLinear
//
////////////////////////////////////////////////////////////////////////////////

float SrgbToLinear (float encoded) noexcept
{
    if (encoded <= ColorMathConstants::kEncodedKnee)
    {
        return encoded / ColorMathConstants::kLinearSlope;
    }

    return std::pow ((encoded + ColorMathConstants::kCurveOffset) / ColorMathConstants::kCurveScale,
                     ColorMathConstants::kCurveGamma);
}





////////////////////////////////////////////////////////////////////////////////
//
//  LinearToSrgb
//
////////////////////////////////////////////////////////////////////////////////

float LinearToSrgb (float linear) noexcept
{
    const float clamped = std::clamp (linear, 0.0f, 1.0f);


    if (clamped <= ColorMathConstants::kLinearKnee)
    {
        return clamped * ColorMathConstants::kLinearSlope;
    }

    return ColorMathConstants::kCurveScale
           * std::pow (clamped, 1.0f / ColorMathConstants::kCurveGamma)
           - ColorMathConstants::kCurveOffset;
}





////////////////////////////////////////////////////////////////////////////////
//
//  EvaluatePolynomial
//
//  Horner's rule, highest power first, in the order and precision the shader
//  uses so the two agree to the last bit that matters.
//
////////////////////////////////////////////////////////////////////////////////

static float EvaluatePolynomial (const float (& coefficients)[6], float t) noexcept
{
    float acc = coefficients[0];



    for (size_t i = 1; i < 6; ++i)
    {
        acc = acc * t + coefficients[i];
    }

    return acc;
}





////////////////////////////////////////////////////////////////////////////////
//
//  SrgbToLinearPolynomial
//
////////////////////////////////////////////////////////////////////////////////

float SrgbToLinearPolynomial (float encoded) noexcept
{
    if (encoded <= ColorMathConstants::kEncodedKnee)
    {
        return encoded / ColorMathConstants::kLinearSlope;
    }

    return EvaluatePolynomial (ColorMathConstants::kDecodePolynomial, encoded);
}





////////////////////////////////////////////////////////////////////////////////
//
//  LinearToSrgbPolynomial
//
////////////////////////////////////////////////////////////////////////////////

float LinearToSrgbPolynomial (float linear) noexcept
{
    const float clamped = std::clamp (linear, 0.0f, 1.0f);


    if (clamped <= ColorMathConstants::kLinearKnee)
    {
        return clamped * ColorMathConstants::kLinearSlope;
    }

    return EvaluatePolynomial (ColorMathConstants::kEncodePolynomial, std::sqrt (clamped));
}





////////////////////////////////////////////////////////////////////////////////
//
//  ClampSdrWhiteNits
//
////////////////////////////////////////////////////////////////////////////////

static float ClampSdrWhiteNits (float sdrWhiteNits) noexcept
{
    return std::clamp (sdrWhiteNits,
                       DisplayLuminanceConstants::kMinSdrWhiteNits,
                       DisplayLuminanceConstants::kMaxSdrWhiteNits);
}





////////////////////////////////////////////////////////////////////////////////
//
//  EffectivePeakNits
//
////////////////////////////////////////////////////////////////////////////////

float EffectivePeakNits (float reportedPeakNits) noexcept
{
    if (reportedPeakNits < DisplayLuminanceConstants::kMinPlausiblePeakNits
        || reportedPeakNits > DisplayLuminanceConstants::kMaxPlausiblePeakNits)
    {
        return DisplayLuminanceConstants::kDefaultPeakNits;
    }

    return reportedPeakNits;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Headroom
//
////////////////////////////////////////////////////////////////////////////////

float Headroom (float effectivePeakNits, float sdrWhiteNits) noexcept
{
    const float white = ClampSdrWhiteNits (sdrWhiteNits);



    return std::max (1.0f, effectivePeakNits / white);
}





////////////////////////////////////////////////////////////////////////////////
//
//  SdrWhiteScale
//
////////////////////////////////////////////////////////////////////////////////

float SdrWhiteScale (float sdrWhiteNits) noexcept
{
    return ClampSdrWhiteNits (sdrWhiteNits) / DisplayLuminanceConstants::kScRgbWhiteNits;
}





////////////////////////////////////////////////////////////////////////////////
//
//  SdrWhiteNitsFromDisplayConfig
//
////////////////////////////////////////////////////////////////////////////////

float SdrWhiteNitsFromDisplayConfig (uint32_t sdrWhiteLevel) noexcept
{
    const float nits = static_cast<float> (sdrWhiteLevel)
                       / static_cast<float> (DisplayLuminanceConstants::kDisplayConfigWhiteLevelUnit)
                       * DisplayLuminanceConstants::kScRgbWhiteNits;



    return ClampSdrWhiteNits (nits);
}





////////////////////////////////////////////////////////////////////////////////
//
//  HighlightGain
//
////////////////////////////////////////////////////////////////////////////////

float HighlightGain (float headroom, int highlightBrightness, HdrMode hdrMode, OutputMode outputMode) noexcept
{
    if (outputMode != OutputMode::Hdr || hdrMode == HdrMode::Off)
    {
        return 1.0f;
    }

    const int   setting  = std::clamp (highlightBrightness, 0, HighlightConstants::kMaxHighlightBrightness);
    const float exponent = static_cast<float> (setting) / static_cast<float> (HighlightConstants::kMaxHighlightBrightness);



    return std::pow (std::max (1.0f, headroom), exponent);
}





////////////////////////////////////////////////////////////////////////////////
//
//  HighlightWeight
//
////////////////////////////////////////////////////////////////////////////////

float HighlightWeight (bool isHead, float brightness) noexcept
{
    if (isHead)
    {
        return 1.0f;
    }

    const float floor = HighlightConstants::kTrailHighlightFloor;
    const float above = std::clamp ((brightness - floor) / (1.0f - floor), 0.0f, 1.0f);



    return HighlightConstants::kTrailHighlightShare * above * above;
}





////////////////////////////////////////////////////////////////////////////////
//
//  ToneMapHighlights
//
////////////////////////////////////////////////////////////////////////////////

void ToneMapHighlights (float (& rgb)[3], float headroom) noexcept
{
    const float m = std::max ({ rgb[0], rgb[1], rgb[2] });
    float       mapped;



    if (m <= 1.0f)
    {
        return;
    }

    if (headroom <= 1.0f)
    {
        mapped = 1.0f;
    }
    else
    {
        const float span = headroom - 1.0f;
        const float t    = (m - 1.0f) / span;


        mapped = 1.0f + span * t / (1.0f + t);
    }

    const float scale = mapped / m;



    rgb[0] *= scale;
    rgb[1] *= scale;
    rgb[2] *= scale;
}





////////////////////////////////////////////////////////////////////////////////
//
//  InstanceDisplayColor
//
////////////////////////////////////////////////////////////////////////////////

Color4 InstanceDisplayColor (const Color4 & srgbColor, float brightness) noexcept
{
    //  Exactly what v1.6 put on screen, kept in gamma space where it was
    //  authored.
    //
    //  Brightness appears twice because v1.6's shader applied it twice: once
    //  to the color, with the self-glow, and once more as the alpha it blended
    //  with. Over the black scene both multiply the displayed pixel, so the
    //  fade a viewer actually saw was b * b * (1 + 0.3 b). An earlier version
    //  moved only the first factor here and left the second as linear-light
    //  alpha, which made every fading trail visibly brighter than v1.6: a fade
    //  applied in linear light removes far less light than the same fade
    //  applied to encoded values.
    const float scale = brightness * (1.0f + kGlyphSelfGlow * brightness) * brightness;



    return Color4 (srgbColor.r * scale,
                   srgbColor.g * scale,
                   srgbColor.b * scale,
                   srgbColor.a);
}
