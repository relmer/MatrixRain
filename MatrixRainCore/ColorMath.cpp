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
//  InstanceLinearColor
//
////////////////////////////////////////////////////////////////////////////////

Color4 InstanceLinearColor (const Color4 & srgbColor, float brightness, float highlightGain) noexcept
{
    //  Exactly the v1.6 shader's scaling, kept in gamma space where it was
    //  authored, so the glyph core is unchanged by the move to linear light.
    const float scale = brightness * (1.0f + kGlyphSelfGlow * brightness);



    return Color4 (SrgbToLinear (srgbColor.r * scale) * highlightGain,
                   SrgbToLinear (srgbColor.g * scale) * highlightGain,
                   SrgbToLinear (srgbColor.b * scale) * highlightGain,
                   srgbColor.a);
}
