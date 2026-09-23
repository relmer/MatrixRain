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

//
//  ToneMapHighlights3: a line-for-line transliteration of ToneMapHighlights in
//  ColorMath.cpp (research R8, R14). Identity up to a knee most of the way
//  to the headroom (MR_TONEMAP_KNEE); above it an extended-Reinhard shoulder
//  on the brightest channel, slope 1 at the knee, approaching the headroom
//  without reaching it; one factor for all three channels so the hue does
//  not move. With no headroom it caps at white.
//

float3 ToneMapHighlights3(float3 rgb, float headroom)
{
    float  m      = max(max(rgb.r, rgb.g), rgb.b);
    float3 result = rgb;

    if (headroom <= 1.0)
    {
        if (m > 1.0)
        {
            result = rgb / m;
        }
    }
    else
    {
        float knee = 1.0 + MR_TONEMAP_KNEE * (headroom - 1.0);

        if (m > knee)
        {
            float span   = headroom - knee;
            float t      = (m - knee) / span;
            float mapped = knee + span * t / (1.0 + t);

            result = rgb * (mapped / m);
        }
    }

    return result;
}

float3 OutputTransform(float3 linearRgb)
{
    // An intermediate pass leaves the image in linear light for whatever comes
    // next.
    float3 result = linearRgb;

    if (g_isFinalPass != 0)
    {
        if (g_outputMode == 0)
        {
            result = LinearToSrgb3(linearRgb);
        }
        else
        {
            // HDR (scRGB): roll highlights off into the headroom (1 caps at
            // white), then place SDR white where the user's slider put it.
            result = ToneMapHighlights3(max(linearRgb, 0.0), g_headroom) * g_sdrWhiteScale;
        }
    }

    return result;
}

#endif
