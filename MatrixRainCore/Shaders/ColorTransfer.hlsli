#ifndef MATRIXRAIN_SHADERS_COLORTRANSFER_HLSLI
#define MATRIXRAIN_SHADERS_COLORTRANSFER_HLSLI

#include "ColorConstants.h"

//
//  The sRGB transfer function in both directions, per channel and per color.
//  Transliterations of SrgbToLinear and LinearToSrgb in ColorMath.cpp, and
//  they MUST stay so; the constants come from the same header, so the two
//  sides cannot drift apart.
//
//  NB: no parameter here may be called "linear" -- that is an HLSL
//  interpolation modifier keyword, and a parameter with that name fails to
//  compile.
//

float SrgbToLinearChannel(float encoded)
{
    if (encoded <= MR_SRGB_ENCODED_KNEE)
    {
        return encoded / MR_SRGB_LINEAR_SLOPE;
    }

    return pow((encoded + MR_SRGB_CURVE_OFFSET) / MR_SRGB_CURVE_SCALE, MR_SRGB_CURVE_GAMMA);
}

float LinearToSrgbChannel(float linearValue)
{
    float clamped = saturate(linearValue);

    if (clamped <= MR_SRGB_LINEAR_KNEE)
    {
        return clamped * MR_SRGB_LINEAR_SLOPE;
    }

    return MR_SRGB_CURVE_SCALE * pow(clamped, 1.0 / MR_SRGB_CURVE_GAMMA) - MR_SRGB_CURVE_OFFSET;
}

float3 SrgbToLinear3(float3 encoded)
{
    return float3(SrgbToLinearChannel(encoded.r),
                  SrgbToLinearChannel(encoded.g),
                  SrgbToLinearChannel(encoded.b));
}

float3 LinearToSrgb3(float3 linearRgb)
{
    return float3(LinearToSrgbChannel(linearRgb.r),
                  LinearToSrgbChannel(linearRgb.g),
                  LinearToSrgbChannel(linearRgb.b));
}

#endif
