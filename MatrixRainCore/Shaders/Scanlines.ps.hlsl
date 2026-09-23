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
    c.rgb *= darken;
    return c;
}
