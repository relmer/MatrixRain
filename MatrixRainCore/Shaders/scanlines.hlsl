// ATTRIBUTION: Adapted from crt-pi by Davide Berra (MIT)
// Upstream URL: https://github.com/libretro/glsl-shaders/blob/master/crt/shaders/crt-pi.glsl
// Upstream collection SHA: 42fa8a98ab19bdaffb53280746a30819eb21f807
// SPDX-License-Identifier: MIT
//
// MatrixRain modifications (v1.5 T050, contracts/scanline-shader.md, research.md R6):
//   Forked from ..\Casso\Casso\Shaders\CRT\scanlines.hlsl with two changes:
//     - line-count is supplied per-frame by the CPU via g_linesPerHeight
//       (ScanlineLineCount(style)) instead of the hardcoded 192.0; this is
//       what makes the Style slider drive line density 981..150 (FR-023).
//     - luminance gating is removed (no `lum` / `weight` lerp); scanlines
//       darken every pixel uniformly so dark or empty regions still carry
//       the CRT pattern (FR-024).
//   Resulting shape: darken = lerp(1 - g_intensity, 1, bright).

cbuffer ScanlineCb : register(b0)
{
    float g_intensity;        // [0..1]     scanline darkening strength
    float g_linesPerHeight;   // ~150..~981 number of lines spanning render height
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

float4 main (PSInput i) : SV_TARGET
{
    float4 c       = tex.Sample (sam, i.uv);
    float  linePos = i.uv.y * g_linesPerHeight;
    float  gap     = sin (linePos * 3.14159265);
    float  bright  = gap * gap;
    float  darken  = lerp (1.0 - g_intensity, 1.0, bright);
    c.rgb *= darken;
    return c;
}
