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
