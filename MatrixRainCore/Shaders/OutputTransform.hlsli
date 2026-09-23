#ifndef MATRIXRAIN_SHADERS_OUTPUTTRANSFORM_HLSLI
#define MATRIXRAIN_SHADERS_OUTPUTTRANSFORM_HLSLI

#include "ColorTransfer.hlsli"

//
//  Included by every shader that might be the LAST pass of a frame, so that
//  whichever one ends up writing the back buffer encodes the image exactly
//  once (FR-004). Which pass that is depends on the user's settings -- the
//  bloom composite, the glow-off scene copy, or the scanline pass -- so each
//  of them includes this and is told at upload time whether it is the one.
//
//  HDR presents scRGB: linear light with 1.0 at 80 nits, so SDR white is
//  sdrWhiteNits / 80 (research R4, R6). Phase 2 caps the image at SDR white
//  first (headroom 1, FR-013), which is what makes an HDR monitor show the
//  same picture as an SDR one at the brightness the user set; Phase 3 raises
//  the headroom and tone-maps into it.
//

cbuffer OutputCb : register(b1)
{
    uint  g_outputMode;     // 0 = SDR (sRGB encode), 1 = HDR (scRGB)
    float g_sdrWhiteScale;  // sdrWhiteNits / 80; ignored in SDR
    float g_headroom;       // >= 1; 1 caps at SDR white (Phase 2)
    uint  g_isFinalPass;    // 1: this pass writes the back buffer; 0: an intermediate
};

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
        return LinearToSrgb3(linearRgb);
    }

    // HDR (scRGB): cap at the headroom, then place SDR white where the
    // user's slider put it.
    return min(max(linearRgb, 0.0), g_headroom) * g_sdrWhiteScale;
}

#endif
