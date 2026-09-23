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

    // The darkening multiplies LINEAR light, which is what a raster actually
    // does to a phosphor: half the darkening means half the light. Applied to
    // gamma-encoded values, as v1.6 did, the same factor removed rather more
    // light than intended and the effect read as a gray veil over the image
    // instead of a raster behind it (FR-001).
    c.rgb *= darken;

    // When this pass runs it is always the last one, so it always encodes.
    return float4 (OutputTransform (c.rgb), c.a);
}
