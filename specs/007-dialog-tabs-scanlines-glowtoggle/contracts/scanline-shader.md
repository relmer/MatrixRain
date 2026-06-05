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
```

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
