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
    //  Exactly what v1.6 put on screen, kept in gamma space where it was
    //  authored, so the glyph core is unchanged by the move to linear light.
    //
    //  Brightness appears twice because v1.6's shader applied it twice: once
    //  to the color, with the self-glow, and once more as the alpha it blended
    //  with. Over the black scene both multiply the displayed pixel, so the
    //  fade a viewer actually saw was b * b * (1 + 0.3 b). Moving only the
    //  first factor here and leaving the second as linear-light alpha made
    //  every fading trail visibly brighter than v1.6, because a fade applied
    //  in linear light removes far less light than the same fade applied to
    //  encoded values.
    const float scale = brightness * (1.0f + kGlyphSelfGlow * brightness) * brightness;



    return Color4 (SrgbToLinear (srgbColor.r * scale) * highlightGain,
                   SrgbToLinear (srgbColor.g * scale) * highlightGain,
                   SrgbToLinear (srgbColor.b * scale) * highlightGain,
                   srgbColor.a);
}
