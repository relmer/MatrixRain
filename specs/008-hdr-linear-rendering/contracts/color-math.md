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
Color4 InstanceLinearColor (Color4 srgbColor, float brightness, float highlightGain) noexcept
```

- Returns `SrgbToLinear(srgb * brightness * (1 + 0.3 * brightness)) * highlightGain`
  per RGB channel. Alpha is passed through unchanged.
- With `highlightGain == 1` and full atlas coverage, `LinearToSrgb` of the
  result equals the v1.6 shader's output for the same inputs (clamped to 1).
  **This is the regression test that pins FR-005.**

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
- `ToneMapHighlights`:
  - Identity when `max(rgb) <= 1`.
  - For `m = max(rgb) > 1`, maps `m` through a smooth shoulder to
    `m' < headroom`, with `m' → headroom` as `m → ∞`, and scales all channels
    by `m' / m` (hue preserved: channel ratios unchanged within 1e-5).
  - C¹-continuous at `m = 1` (no visible kink).
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
