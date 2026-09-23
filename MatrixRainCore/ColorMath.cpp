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
