#ifndef MATRIXRAIN_SHADERS_OUTPUTTRANSFORM_HLSLI
#define MATRIXRAIN_SHADERS_OUTPUTTRANSFORM_HLSLI

#include "ColorConstants.h"

//
//  Included by every shader that might be the LAST pass of a frame, so that
//  whichever one ends up writing the back buffer encodes the image exactly
//  once (FR-004). Which pass that is depends on the user's settings -- the
//  bloom composite, the glow-off scene copy, or the scanline pass -- so each
//  of them includes this and is told at upload time whether it is the one.
//
//  LinearToSrgbChannel is a transliteration of LinearToSrgb in ColorMath.cpp
//  and MUST stay one. The constants come from ColorConstants.h, which the C++
//  side includes as well, so the two cannot drift apart.
//
//  The HDR branch is a pass-through until US2 gives it a real transform.
//

cbuffer OutputCb : register(b1)
{
    uint  g_outputMode;     // 0 = SDR (sRGB encode), 1 = HDR (scRGB)
    float g_sdrWhiteScale;  // sdrWhiteNits / 80; ignored in SDR
    float g_headroom;       // >= 1; 1 caps at SDR white (Phase 2)
    uint  g_isFinalPass;    // 1: this pass writes the back buffer; 0: an intermediate
};

// NB: the parameter cannot be called "linear" -- that is an HLSL interpolation
// modifier keyword, and a parameter called that fails to compile.
float LinearToSrgbChannel(float linearValue)
{
    float clamped = saturate(linearValue);

    if (clamped <= MR_SRGB_LINEAR_KNEE)
    {
        return clamped * MR_SRGB_LINEAR_SLOPE;
    }

    return MR_SRGB_CURVE_SCALE * pow(clamped, 1.0 / MR_SRGB_CURVE_GAMMA) - MR_SRGB_CURVE_OFFSET;
}

float3 OutputTransform(float3 linearRgb)
{
    // An intermediate pass leaves the image in linear light for whatever comes
    // next.
    if (g_isFinalPass == 0)
    {
        return linearRgb;
    }

    if (g_outputMode == 0)
    {
        return float3(LinearToSrgbChannel(linearRgb.r),
                      LinearToSrgbChannel(linearRgb.g),
                      LinearToSrgbChannel(linearRgb.b));
    }

    // HDR (scRGB): completed in US2.
    return linearRgb;
}

#endif
