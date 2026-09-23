#ifndef MATRIXRAIN_SHADERS_COLORTRANSFER_HLSLI
#define MATRIXRAIN_SHADERS_COLORTRANSFER_HLSLI

#include "ColorConstants.h"

//
//  The sRGB transfer function in both directions, per channel and per color.
//
//  The curved segments are polynomial fits, not pow(): see ColorConstants.h
//  for why and for how close they are (well under a tenth of an 8-bit code
//  value). These are transliterations of SrgbToLinearPolynomial and
//  LinearToSrgbPolynomial in ColorMath.cpp and MUST stay so; the coefficients
//  come from the same header, so the two sides cannot drift apart, and the
//  unit tests on the C++ side are the tests of these.
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

    float acc = MR_SRGB_DECODE_C5;

    acc = acc * encoded + MR_SRGB_DECODE_C4;
    acc = acc * encoded + MR_SRGB_DECODE_C3;
    acc = acc * encoded + MR_SRGB_DECODE_C2;
    acc = acc * encoded + MR_SRGB_DECODE_C1;
    acc = acc * encoded + MR_SRGB_DECODE_C0;

    return acc;
}

float LinearToSrgbChannel(float linearValue)
{
    float clamped = saturate(linearValue);

    if (clamped <= MR_SRGB_LINEAR_KNEE)
    {
        return clamped * MR_SRGB_LINEAR_SLOPE;
    }

    float root = sqrt(clamped);
    float acc  = MR_SRGB_ENCODE_C5;

    acc = acc * root + MR_SRGB_ENCODE_C4;
    acc = acc * root + MR_SRGB_ENCODE_C3;
    acc = acc * root + MR_SRGB_ENCODE_C2;
    acc = acc * root + MR_SRGB_ENCODE_C1;
    acc = acc * root + MR_SRGB_ENCODE_C0;

    return acc;
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
