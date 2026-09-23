//
//  Scanline pass (contracts/scanline-shader.md, research R6).
//
//  ATTRIBUTION: Adapted from crt-pi by Davide Berra (MIT)
//  Upstream URL:
//    https://github.com/libretro/glsl-shaders/blob/master/crt/shaders/crt-pi.glsl
//  SPDX-License-Identifier: MIT
//
//  MatrixRain modifications (v1.5):
//   - line count uploaded per-frame from CPU via g_linesPerHeight
//   - source-luminance gating removed (FR-024a); darkening is uniform
//   - the kernel is AREA-AVERAGED over the pixel rather than point sampled,
//     which is what keeps the Style slider usable below 4K
//

#include "OutputTransform.hlsli"

cbuffer ScanlineCb : register(b0)
{
    float g_intensity;
    float g_linesPerHeight;
    float g_padding0;
    float g_padding1;
};

Texture2D    tex : register(t0);
SamplerState sam : register(s0);

struct PSInput
{
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD;
};

static const float kPi = 3.14159265;

float4 main (PSInput i) : SV_TARGET
{
    float4 c       = tex.Sample (sam, i.uv);
    float  linePos = i.uv.y * g_linesPerHeight;
    float  perPix  = max (abs (ddy (linePos)), 1e-6);
    float  rolloff = max (sin (kPi * perPix) / (kPi * perPix), 0.0);
    float  bright  = 0.5 - 0.5 * cos (2.0 * kPi * linePos) * rolloff;
    float  darken  = lerp (1.0 - g_intensity, 1.0, bright);

    // v1.6 darkened an 8-bit post-bloom texture, so anything the composite
    // pushed past white had already been clipped to white before the raster
    // touched it. Here the post-bloom target is float and keeps the overshoot,
    // and darkening a value above white then clipping it can land back at
    // white: the raster vanishes over the brightest pixels. Clamp to the
    // display's headroom first. In SDR that is 1.0, exactly v1.6's clip; in
    // HDR it is the real headroom, so Phase 3's highlights keep theirs.
    c.rgb = min (c.rgb, g_headroom);

    // Match v1.6 on screen. v1.6 multiplied the ENCODED pixel by darken, so
    // the same factor applied to linear light removes visibly less and the
    // scanlines came out about a third lighter than the baseline. Raising the
    // factor to the encode curve's exponent cancels that, the same way the
    // glyph coverage and the halo falloff are shaped. Light still combines in
    // linear light everywhere; this only reproduces the strength v1.6 gave
    // the Intensity slider, which is what FR-005 protects.
    c.rgb *= pow (darken, MR_SRGB_CURVE_GAMMA);

    // When this pass runs it is always the last one, so it always encodes.
    return float4 (OutputTransform (c.rgb), c.a);
}
