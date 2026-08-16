# Contract: Scanline Shader (CPU ↔ GPU)

Ported from `..\Casso\Casso\Shaders\CRT\scanlines.hlsl` (crt-pi by Davide
Berra, MIT). MatrixRain modifications per FR-023, FR-024, FR-024a (see
research note R6).

## HLSL (`MatrixRainCore/Shaders/scanlines.hlsl`)

```hlsl
// ATTRIBUTION: Adapted from crt-pi by Davide Berra (MIT)
// Upstream URL: https://github.com/libretro/glsl-shaders/blob/master/crt/shaders/crt-pi.glsl
// SPDX-License-Identifier: MIT
// Casso modifications: simplified single-pass HLSL port of the scanline
//   darkening kernel only.
// MatrixRain modifications (v1.5):
//   - Cbuffer reduced to (intensity, linesPerHeight, padding, padding)
//   - kNativeScanlines removed; line count uploaded per-frame from CPU
//   - Source-luminance gating removed (FR-024a); darkening is uniform
//   - Kernel AREA-AVERAGED over the pixel rather than point sampled

cbuffer ScanlineCb : register (b0)
{
    float g_intensity;
    float g_linesPerHeight;
    float g_padding0;
    float g_padding1;
};

Texture2D    tex : register (t0);
SamplerState sam : register (s0);

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
```

### Why the kernel is area-averaged

The pass lays `g_linesPerHeight` cycles across the render height and the Style
slider drives that from 150 to 981. Point sampling a periodic signal needs
better than two pixels per cycle; at 981 lines that is 1.10 px on 1080p and
1.47 px on 1440p, so most of the slider's lower half sat below Nyquist and
resolved into a moire beat instead of scanlines. Removing the luminance gate
(FR-024a) took away the cover Casso had, over a field that is mostly dark
background with glyphs scrolling through it.

For `sin^2(pi*L) == (1 - cos(2*pi*L)) / 2`, the mean over a pixel spanning
`dL` cycles is the same kernel scaled by `sinc(dL)`. `dL` comes from `ddy`, so
it tracks both the slider and the display resolution without either being
passed in: a dense setting fades toward flat on a 1080p panel rather than
shimmering, and stays crisp on a 4K one.

Cost, `fxc /T ps_5_0 /O3`: 11 -> 18 instruction slots (one extra sincos, one
divide, one coarse derivative). Still one texture sample, still no branches.

## CPU mirror (`MatrixRainCore/RenderSystem.h` adjacent struct)

```cpp
struct alignas (16) ScanlineCb
{
    float intensity;
    float linesPerHeight;
    float _padding0;
    float _padding1;
};
static_assert (sizeof (ScanlineCb) == 16, "ScanlineCb must match HLSL b0 size");
```

## Per-frame upload

```cpp
ScanlineCb               cb           = {};
D3D11_MAPPED_SUBRESOURCE mapped       = {};


cb.intensity      = static_cast<float> (params.scanlinesIntensityPercent) / 100.0f;
cb.linesPerHeight = ScanlineLineCount (params.scanlinesStyle);

hr = m_context->Map (m_scanlineConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
CHRA (hr);
memcpy (mapped.pData, &cb, sizeof (cb));
m_context->Unmap (m_scanlineConstantBuffer.Get(), 0);
```

## Draw order (in `RenderSystem::Render`)

```
1. Clear backbuffer (or m_postBloomTarget if scanlines enabled)
2. Draw character streaks (existing path)
3. If glowEnabled:
       Bloom extract / blur passes (existing)
       Bloom composite -> writes to (scanlinesEnabled ? m_postBloomTarget : backbuffer)
   Else:
       Streaks already in (scanlinesEnabled ? m_postBloomTarget : backbuffer)
4. If scanlinesEnabled:
       Bind backbuffer as RTV
       Bind m_postBloomTarget as SRV (t0)
       Bind m_scanlineConstantBuffer (b0)
       Draw fullscreen triangle with scanline PS
5. Present
```

When `scanlinesEnabled == false` the scanline pass is skipped entirely
(zero draw calls, zero extra texture binding) — satisfies FR-028b's
"fully bypassed" requirement.

When `glowEnabled == false` AND `scanlinesEnabled == false`, characters
render directly into the backbuffer exactly as in v1.4 (no post-bloom
target involved) — satisfies FR-015.

## Style → line-count mapping

```cpp
// In MatrixRainCore/ScanlineStyleMapping.h
inline float ScanlineLineCount (int style) noexcept
{
    // style is expected in [1, 100]; defensive clamp:
    int  s = std::clamp (style, 1, 100);
    return 1000.0f * std::pow (0.15f, static_cast<float> (s) / 100.0f);
}
```

Test vector (`ScanlineStyleMappingTests.cpp`, ±2 lines tolerance):

| style | expected | actual (computed) |
|---:|---:|---:|
| 1   | 981 | `1000 * 0.15^0.01` ≈ 981.2 |
| 25  | 622 | `1000 * 0.15^0.25` ≈ 622.3 |
| 50  | 387 | `1000 * 0.15^0.50` ≈ 387.3 |
| 75  | 241 | `1000 * 0.15^0.75` ≈ 241.1 |
| 100 | 150 | `1000 * 0.15^1.00` = 150.0 |
