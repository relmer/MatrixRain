// ATTRIBUTION: Adapted from crt-pi by Davide Berra (MIT)
// Upstream URL: https://github.com/libretro/glsl-shaders/blob/master/crt/shaders/crt-pi.glsl
// Upstream collection SHA: 42fa8a98ab19bdaffb53280746a30819eb21f807
// SPDX-License-Identifier: MIT
//
// MatrixRain modifications (v1.5 T050, contracts/scanline-shader.md, research.md R6):
//   Forked from ..\Casso\Casso\Shaders\CRT\scanlines.hlsl with two changes:
//     - line-count is supplied per-frame by the CPU via g_linesPerHeight
//       instead of the hardcoded 192.0; this is what makes the Style slider
//       drive line density (FR-023). The CPU derives it from the rain
//       CHARACTER CELL, not the panel: ScanlineLinesPerCell(style) picks
//       6..16 lines per cell and ScanlineLineCount() scales that by the
//       viewport height, so one Style setting holds the same raster-to-glyph
//       ratio on monitors of differing heights (a portrait 3840-tall panel
//       previously got 3.6 lines/cell where a 2160-tall one got 6.5).
//     - luminance gating is removed (no `lum` / `weight` lerp); scanlines
//       darken every pixel uniformly so dark or empty regions still carry
//       the CRT pattern (FR-024).
//     - the kernel is AREA-AVERAGED over the pixel instead of point sampled.
//   Resulting shape: darken = lerp(1 - g_intensity, 1, bright).
//
// Why the averaging matters MORE here than it did in Casso. This pass lays
// g_linesPerHeight cycles across the render height. The cell-anchored mapping
// now bounds the densest setting at a 2.25 px pitch, but the roll-off still
// earns its keep: a reduced characterScale on a short viewport, or a future
// widening of the range, can push the pitch back toward two pixels, where
// point sampling turns into a moire beat rather than scanlines. Casso got
// away with it because its luminance gate suppressed the pattern on dark
// pixels -- and this fork deliberately removed that gate (FR-024), over a
// field that is mostly dark background, with glyphs scrolling through it to
// make the beat crawl.
//
// For sin^2(pi*L) == (1 - cos(2*pi*L)) / 2 the mean over a pixel spanning dL
// cycles has a closed form:
//
//     mean = 1/2 - 1/2 * cos(2*pi*L) * sinc(dL),   sinc(x) = sin(pi*x)/(pi*x)
//
// -- the original kernel scaled by sinc. Since dL comes from ddy it tracks
// BOTH the slider and the display's resolution on its own: a dense setting
// fades toward flat on a 1080p panel instead of shimmering, while the same
// setting stays crisp on a 4K one.
//
// Cost: 11 -> 18 instruction slots (fxc /T ps_5_0 /O3) -- one extra sincos,
// one divide, one coarse derivative. The pass still issues a single texture
// sample and no branches, and it is bound by that fetch, not by ALU.

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
