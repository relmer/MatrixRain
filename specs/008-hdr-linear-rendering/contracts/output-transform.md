# Contract: Output Transform (final pass)

**Where**: the last full-screen pass that writes the back buffer. That is the
bloom composite (or the glow-off scene copy) when scanlines are off, and the
scanline pass when on. Both call one shared HLSL function; there is no new pass.

## Constant buffer (register `b1`, 16 bytes)

```text
cbuffer OutputCb : register(b1)
{
    uint  g_outputMode;     // 0 = SDR (sRGB encode), 1 = HDR (scRGB)
    float g_sdrWhiteScale;  // sdrWhiteNits / 80; ignored in SDR
    float g_headroom;       // >= 1; 1 caps at SDR white (Phase 2)
    float g_padding0;
};
```

A `static_assert` pins the C++ mirror (`OutputTransformCb`) at 16 bytes, like
the existing `ScanlineCb`. `b1` is used because `b0` already holds the bloom
and scanline constants in those passes.

## Function

```text
float3 OutputTransform (float3 linearRgb)
{
    if (g_outputMode == 0)
        return LinearToSrgb (saturate (linearRgb));         // SDR
    return ToneMapHighlights (max (linearRgb, 0), g_headroom) * g_sdrWhiteScale; // scRGB
}
```

`LinearToSrgb` and `ToneMapHighlights` are transliterations of the C++
functions in [color-math.md](color-math.md), with the same constants.

## Invariants

- Inputs are linear light, ≥ 0, where 1.0 is SDR white.
- SDR output: exactly one sRGB encode per pixel, and no other pass encodes
  (FR-004).
- HDR output with `g_headroom == 1`: no pixel exceeds `g_sdrWhiteScale`
  (FR-013).
- Scanline darkening is applied **before** `OutputTransform`, in linear light
  (FR-001).
- The D2D statistics overlay draws after this pass, directly into the back
  buffer. In HDR its colors are pre-scaled by `g_sdrWhiteScale` (research R9).
