# Contract: Color Math and Mode Selection (pure, unit-tested)

**Module**: `MatrixRainCore/ColorMath.{h,cpp}` and `MatrixRainCore/OutputModeSelection.{h,cpp}`

**Tests**: `MatrixRainTests/unit/ColorMathTests.cpp` and `OutputModeSelectionTests.cpp`

These are the functions FR-027 requires to be tested. The HLSL output
transform ([output-transform.md](output-transform.md)) must be a line-for-line
transliteration of `LinearToSrgb` and `ToneMapHighlights`. The constants are
shared via a single header of `constexpr` values that the HLSL source strings
reference by value.

## sRGB transfer

```text
float SrgbToLinear (float encoded) noexcept   // IEC 61966-2-1 piecewise
float LinearToSrgb (float linear)  noexcept   // inverse; input clamped to [0,1]
```

- Round-trip `LinearToSrgb(SrgbToLinear(x)) == x` within 1e-5 for all
  x ∈ [0,1].
- Endpoints: 0 → 0, 1 → 1. Continuous at the 0.04045 / 0.0031308 knee.
- `SrgbToLinear(0.5) ≈ 0.2140`.

## Instance color (research R1)

```text
Color4 InstanceDisplayColor (Color4 srgbColor, float brightness) noexcept
```

- Returns `srgb * brightness * (1 + 0.3 * brightness) * brightness` per RGB
  channel, in **gamma space, unclipped**. Alpha is passed through unchanged.
- Brightness appears twice because v1.6's shader applied it twice: to the
  color, with the self-glow, and again as the alpha it blended with. Over the
  black scene both multiply the displayed pixel.
- The glyph pixel shader finishes the job per pixel: it multiplies by
  coverage squared (the atlas is premultiplied white, so v1.6's `tex.rgb` and
  `tex.a` were both coverage), clips at 1 exactly where v1.6's 8-bit target
  did, and only then converts to linear light. Doing the conversion after
  coverage is what makes the match exact for every color and every
  antialiased edge; converting on the CPU and shaping coverage separately was
  exact only where the sRGB curve is a pure power law, and read 15% dark on
  a mid-tone custom color.
- `min(1, InstanceDisplayColor)` equals **the pixel v1.6 put on screen** at
  full coverage -- shader output times shader alpha, clamped -- not the
  shader's output alone. **This is the regression test that pins FR-005.** An
  earlier version pinned the shader output and passed while the trails
  visibly drifted.
- Highlight gain (Phase 3) is applied in the shader after linearization, not
  here: it is a linear-light multiplier and this value is not linear.

## Display luminance

```text
float EffectivePeakNits (float reportedPeakNits) noexcept      // 400 if outside [80, 10000]
float Headroom          (float effectivePeakNits, float sdrWhiteNits) noexcept  // >= 1
float SdrWhiteScale     (float sdrWhiteNits) noexcept           // nits / 80
float SdrWhiteNitsFromDisplayConfig (uint32_t sdrWhiteLevel) noexcept // level / 1000 * 80, clamped [80, 480]
```

## Highlights (Phase 3)

```text
float HighlightGain (float headroom, int highlightBrightness, HdrMode, OutputMode) noexcept
void  ToneMapHighlights (float rgb[3], float headroom) noexcept
```

- `HighlightGain` is 1 for `OutputMode::Sdr` or `HdrMode::Off`; otherwise
  `headroom ^ (clamp(highlightBrightness, 0, 100) / 100)`. It is monotonic in
  the brightness setting and equals `headroom` at 100.
- `float HighlightWeight (bool isHead, float brightness) noexcept` (research
  R14, option B): 1 for a head; for a trail glyph
  `kTrailHighlightShare * r^2` with
  `r = max (0, (brightness - kTrailHighlightFloor) / (1 - kTrailHighlightFloor))`,
  so 0 at and below the floor, rising to the share at full brightness,
  monotonic in brightness, never above 1. An instance's gain is
  `HighlightGain ^ HighlightWeight`.
- `ToneMapHighlights`:
  - Identity when `max(rgb) <= 1`.
  - For `m = max(rgb) > 1`, maps `m` through a smooth shoulder to
    `m' < headroom`, with `m' → headroom` as `m → ∞`, and scales all channels
    by `m' / m` (hue preserved: channel ratios unchanged within 1e-5).
  - C¹-continuous at the knee (no visible kink).
  - The curve (research R14, knee revised in T044): with
    `k = 1 + MR_TONEMAP_KNEE * (h - 1)`, identity up to `k`; above it
    `t = (m - k) / (h - k)` and `m' = k + (h - k) * t / (1 + t)`; `f'(k) = 1`,
    `f -> h`. SC-005 is tested through the gain and this curve together.
  - `headroom == 1` → output capped at 1 (the Phase 2 behavior).

## Output mode selection

```text
OutputMode SelectOutputMode (bool hdrEnabled,
                             bool scRgbPresentSupported,
                             ScreenSaverMode displayMode) noexcept
```

| `hdrEnabled` | `scRgbPresentSupported` | `displayMode` | Result |
|---|---|---|---|
| false | any | any | `Sdr` |
| true | false | any | `Sdr` |
| true | true | `ScreenSaverPreview` | `Sdr` (FR-017) |
| true | true | any other | `Hdr` |

`HdrMode::Off` does **not** force `Sdr` output: it only zeroes the highlight
gain. An HDR monitor still presents natively at SDR white, which is the
Phase 2 behavior (spec US3 scenario 4).
