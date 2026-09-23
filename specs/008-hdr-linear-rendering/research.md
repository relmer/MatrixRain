# Research: HDR Output and Linear-Light Rendering

**Feature**: [spec.md](spec.md) | **Plan**: [plan.md](plan.md) | **Date**: 2026-09-21

Each entry records the decision, why, and what else was considered. File and
line references are to `master` at v1.6.0 (`cf9bc0c`).

## Current pipeline (baseline)

Read from `MatrixRainCore/RenderSystem.cpp`:

| Stage | Target | Format | Notes |
|---|---|---|---|
| Glyphs (instanced) | `m_sceneTexture` | `R8G8B8A8_UNORM` | Blend `SRC_ALPHA / INV_SRC_ALPHA` ("over"), not additive. Pixel shader multiplies `color * atlas * brightness`, then adds a hard-coded `+30% * brightness`. |
| Help / hotkey / usage overlays | `m_sceneTexture` | same | D3D instanced from a D2D-built `B8G8R8A8` atlas, premultiplied blend, drawn before bloom so they glow. |
| Bloom extract | `m_bloomTexture` (÷ divisor) | `R8G8B8A8_UNORM` | `smoothstep(0.1, 0.6, max(luma, maxChannel))` on gamma-encoded values. |
| Blur H/V × passes | `m_blurTemp` ↔ `m_bloom` | `R8G8B8A8_UNORM` | 5/9/13-tap by quality. |
| Composite | back buffer, or `m_postBloom` when scanlines on | back buffer `B8G8R8A8_UNORM` | `scene + (1 - exp(-bloom*k)) * (1 - scene)`: a screen blend that assumes a [0,1] display-referred range. |
| Scanlines | back buffer | `B8G8R8A8_UNORM` | Multiplies `darken` into gamma-encoded color. |
| Statistics (FPS) | back buffer | `B8G8R8A8_UNORM` | Direct2D draws straight onto the swap-chain buffer (`m_d2dBitmap`). The only D2D-on-back-buffer path. |

Swap chain: `FLIP_DISCARD`, 2 buffers, `B8G8R8A8_UNORM`, no `SetColorSpace1`.
Devices: feature level 11.0 minimum (line 425). Streak heads are recognized by
being white (`BuildCharacterInstanceData`, `isWhite`).

**Correction to the spec's framing**: glyphs are alpha-composited, not added.
What is *additive* today is only the bloom composite. The spec wording has been
adjusted to "glyph compositing".

## R1. Working color space and where gamma is removed

**Decision**: Render in linear light, Rec.709 primaries. Convert each
instance's final display color to linear **on the CPU, per instance**, after
applying brightness and the head/trail color choice:
the CPU computes v1.6's gamma-space value
`display = color_srgb * brightness * (1 + 0.3 * brightness) * brightness`
per instance, and the glyph pixel shader computes
`SrgbToLinear(min(1, display * coverage * coverage))` per pixel. Coverage is
squared because the atlas is premultiplied white, so v1.6's `tex.rgb` and
`tex.a` were both coverage. The conversion happens after coverage and after
the clip, which is where v1.6's 8-bit target clipped, and that ordering is
what makes the match exact for every color and every antialiased edge.

The second `brightness` factor was missed in the first implementation. v1.6's
shader applied brightness both to the color and to the alpha it blended with,
so over the black scene the displayed pixel carried it twice. Leaving the
alpha factor in the shader meant it ran as linear-light alpha, and a fade in
linear light removes far less light than the same fade applied to encoded
values: every fading trail rendered visibly brighter, and slightly less
saturated, than v1.6. Rob spotted it by eye against the baseline; the
regression test had pinned the shader's output rather than the screen's, and
passed throughout.

**Rationale**: FR-005 requires the look at defaults to be preserved. The fade
curve (`brightness`) and the `+30%` boost were tuned perceptually in gamma
space. Converting the *product* keeps every fully covered glyph pixel at
the same brightness as v1.6 on SDR, before glow is added. Only antialiased edges,
overlaps and glow change, which are exactly the intended correctness fixes.
It is also cheaper than a per-pixel `pow`.

**Alternatives considered**:
- *Convert only the base color and keep `brightness` as a linear multiplier*:
  physically purer, but tails would fade visibly faster and darker (a 50%
  gamma-space brightness is ~21% linear), which violates FR-005.
- *`_SRGB` atlas/texture views for automatic decode*: the atlas holds
  coverage, not color, so decoding it would thin every glyph.

## R2. Intermediate formats and bandwidth

**Decision** (revised in T020, the performance gate; the original chose
`R16G16B16A16_FLOAT` for the two full-resolution targets):
- `m_sceneTexture`: `R11G11B10_FLOAT` (full resolution).
- `m_bloomTexture`, `m_blurTemp`: `R11G11B10_FLOAT` (÷ resolution divisor).
- `m_postBloomTarget`: `R11G11B10_FLOAT` (full resolution).

Every intermediate is float, so FR-001 and FR-003 hold: nothing is quantized
to 8 bits before the final encode, and values above 1.0 survive.

**Rationale**: measured, not assumed. With FP16 for the scene and post-bloom
targets the frame cost against T005 was +17% to +49% on the two real monitors
(`baseline.md`, T020). The cost is bandwidth on the two full-resolution
targets: 8 bytes per pixel where v1.6 wrote 4, paid on the clear and again on
the composite's read, and it scales with pixel count, not with the shader
math. Removing every added `pow()` from the glyph, extract and composite
shaders moved nothing outside run-to-run noise. `R11G11B10_FLOAT` is the same
4 bytes per pixel v1.6 used and brought the full-screen passes back to v1.6's
cost.

Its precision is enough. The mantissas are 6, 6 and 5 bits, so a value is held
to about 1.6% (red, green) or 3.1% (blue) of itself at every brightness. In the
dark that is far finer than 8-bit sRGB, which is what SC-002 cares about; near
white a step is about one to three 8-bit code values, and the rain's brightest
pixels are glyph bodies, not smooth gradients. Rendered against an FP16 build,
the mean bias is under 0.4 code values in every luminance band and the
largest per-pixel differences (10 to 13 code values) sit on a few dozen
antialiased edge pixels out of two million.

Nothing reads the scene's alpha: the glyph blend uses source alpha only and the
composite writes 1.0. The format has none, and that is fine.

One cost is specific to this format: on the RTX 5070 Ti, blending glyphs into
an `R11G11B10` target is slower than into 8-bit UNORM when the glyphs are
large (about 14 µs per frame at 2160x3840 @ 150%, nothing at 3840x2160 @
125%). Glyph quads are deliberately taller than the row pitch, so most of the
blended pixels carry zero coverage; the glyph shader now discards those, which
skips the blend for a bit-identical image and paid that cost back with room to
spare.

**Alternatives considered**, all measured in T020:
- *FP16 for the two full-resolution targets* (the original decision): +17% to
  +49%, bandwidth.
- *`R8G8B8A8_UNORM` holding linear values*: meets the gate and bands in the
  dark; `R10G10B10A2_UNORM` likewise (a 10-bit linear step at black is three
  sRGB code values). Both fail FR-003.
- *`R8G8B8A8_UNORM_SRGB` (hardware encode on write, linear blend)*: v1.6's
  exact precision, but the sRGB blend is slower still, +16% at the portrait
  monitor.
- *8-bit UNORM for the bloom chain*: the chain holds encoded values now, so
  8 bits would be v1.6's own precision there. Measured gain about 1%, and the
  rounding moved the calibration mean by +1.5%. Not worth it.

**Transfer curves**: `ColorTransfer.hlsli` evaluates both sRGB curves as
degree-5 polynomial fits (the encode in the square root of its input) rather
than `pow()`. On the desktop GPU that made no measurable difference; on a
Surface Pro 8's Iris Xe the `pow()` calls were the whole of a +5% to +9%
regression, and on WARP most of a +100%. The fits are within 0.013 (decode)
and 0.084 (encode) of an 8-bit code value, the coefficients sit in
`ColorConstants.h` beside the curve's constants, and `ColorMath.cpp` mirrors
them so unit tests can hold the shader's arithmetic to those bounds. Render
difference against `pow()`: at most 2 to 3 code values on a handful of pixels.

**Phase 2 note**: an HDR swap chain shows 10-bit steps, and a 1.6% to 3.1%
mantissa could show in a smooth halo at high brightness. Measure it then; if
it shows, the answer is FP16 only while the swap chain is scRGB, with its own
gate, not a change to the SDR path.

## R3. Bloom: v1.6's glow, computed on encoded values

**Decision** (revised during Phase 1 calibration): the bloom chain -- extract,
blur, soft saturation and composite -- runs on ENCODED values exactly as
v1.6's did, held in float textures, and the composited result is converted to
linear light for the output stage. The extract reads the linear scene and
encodes each texel before its bilinear average, which is the value v1.6's
sampler produced from its 8-bit scene. The thresholds, the soft saturation,
the `(1 - scene)` screen factor and the Glow Intensity mapping are v1.6's own.
There are no fitted constants.

**What changed from the original decision.** The original R3 put the chain in
linear light, dropped the screen factor as gamma-space compensation, and
planned to recalibrate the constants. That was tried. It produced, in
succession, a glow eight times too strong, a halo far wider than v1.6's, a
halo of the wrong hue, extra glow on every glyph body, and finally glow that
kept adding where streaks overlap while v1.6's had saturated -- which made the
dense regions bury the characters generating them. Each was matched by
computing that one term v1.6's way and converting afterward, until the
overlap behavior, which Rob judged worse for legibility, made the pattern
plain: every term of the glow's appearance wants to be v1.6's, so the chain
should simply be v1.6's. baseline.md records all nine mismatches and the
measurements.

**Rationale**: FR-005 protects the look, and the glow is most of the look.
Linear-light addition of overlapping halos is physically right and was the
one thing the linear chain offered visibly; Rob preferred v1.6's saturation.
The float textures keep the real benefit -- no 8-bit quantization anywhere in
the chain -- and the linear output stage keeps what the HDR phases need.

**Phase 3 note**: the extract's encode saturates at white, so a head above SDR
white blooms as a white head. Highlight bloom needs its own handling then;
research R14 is that design.

**Calibration method**: A deterministic reference frame (fixed seed, fixed
time) rendered on the WARP device by a small calibration harness, **kept as a
PNG** and compared pixel by pixel against the same frame rendered by the new
pipeline.

Comparing the frames replaced an earlier plan to reduce each frame to two
scalars: mean luminance and the 50%-falloff radius of an isolated head's halo.
The radius did not survive contact with a real frame. A streak head is a
saturated glyph, so half of its peak is reached at the edge of the letter's
own ink about 0.7 px out, and the number reported the stroke width rather than
the glow. Measured across the glow-size slider from 50% to 200% it moved by 5%,
in the wrong direction -- it would have certified a plainly visible change as
"within tolerance". A frame comparison needs no such proxy, and catches glow
SHAPE, hue shifts, glyph weight and banding as well, none of which anyone
would have thought to write a statistic for.
- **Determinism** needs a seam: randomness comes from three independent
  generators seeded by `std::random_device` (`AnimationSystem::m_generator`,
  `CharacterStreak::s_generator`, `CharacterSet`'s `s_gen`). A new
  `RandomSource` module gives one engine per thread that can be reseeded, and
  all three use it.
- **Reference capture**: v1.6 has neither the seam nor the harness, so the
  reference frame is captured on this branch once both land and *before* any
  pipeline change. At that point rendering is still byte-identical to v1.6.
- **Metrics** live in a unit-tested core module, `FrameMetrics`.
  `CompareFrames` reduces two frames to how far apart they are: max, mean and
  99th-percentile difference in 8-bit code values, how many pixels moved more
  than a threshold, and where the worst one is. `MeanLuminance` gives a
  one-number headline for exposure. Differences are stated in code values
  rather than in linear light because the question is whether anyone would SEE
  it, and the sRGB curve is what makes one code value about equally visible at
  any brightness. The tool's `main` only wires things together, and also
  writes an amplified difference image per case -- the numbers say how much
  moved, only the picture says what.
- **Reproducibility is verified, not assumed**: capturing the baseline and
  immediately re-running the comparison in a separate process reports zero
  difference on every case, which exercises determinism, the PNG round trip
  and WARP's repeatability at once. The WARP version is recorded with the
  baseline, since WARP ships with Windows and can change under it.
- **Settings coverage**: the reference set covers defaults plus several
  non-default glow, scanline and color settings (FR-006), each captured at
  display scale 100%, 125% and 150%. The scales matter because scanline pitch
  is derived from the cell: at Style 1 a 24 px cell gives a ~2.4 px pitch,
  under the 3 px floor where a fractional pitch beats against the pixel grid,
  while a 36 px cell gives a clean ~3.6 px. One scale would leave one side of
  that boundary unguarded.
- **Benchmark mode** (Constitution II): the same harness times N frames per
  quality preset and reports ms/frame. It runs on WARP for repeatability and
  on the hardware adapter for real numbers, and results are recorded against
  a threshold before and after each phase.

**Alternatives considered**: Tuning by eye only (not repeatable). Keeping
screen-blend composite in linear (still clips; fights Phase 3).

## R4. Final output transform (one place, all modes)

**Decision**: The **last full-screen pass** (the composite when scanlines are
off, the scanline pass when on) applies a shared `OutputTransform` function
driven by a small constant buffer:

| Mode | Swap chain | Transform |
|---|---|---|
| SDR | `B8G8R8A8_UNORM` | `saturate` → `LinearToSrgb` |
| HDR (Phase 2) | `R16G16B16A16_FLOAT`, scRGB | `min(x, 1) * (sdrWhiteNits / 80)` |
| HDR (Phase 3) | same | `ToneMapHighlights(x)` then `* (sdrWhiteNits / 80)` |

The encode is explicit in the shader rather than via an `_SRGB` render-target
view, so one code path serves both modes and the math has a C++ mirror that
unit tests can exercise (FR-027).

**Rationale**: Encoding once at output is FR-004. Folding it into the existing
last pass adds no extra full-screen pass.

**Alternatives considered**: `_SRGB` RTV on the flip-model back buffer (legal,
but splits SDR and HDR into two paths). A dedicated final pass (costs a
full-resolution read and write).

## R5. Per-monitor HDR detection

**Decision**: Each `RenderSystem` asks its swap chain for its containing
output (`IDXGISwapChain::GetContainingOutput`), queries `IDXGIOutput6::GetDesc1`,
and treats the monitor as HDR when `ColorSpace ==
DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020`. It records `MaxLuminance` and
`DeviceName`. `MaxFullFrameLuminance` is not needed: highlights are small
features, so the peak (not full-frame) rating is the right ceiling. It also confirms
`IDXGISwapChain3::CheckColorSpaceSupport(RGB_FULL_G10_NONE_P709)` reports
present support, reported separately as `scRgbSupported`, and falls back to
SDR on any failure (FR-015).

**Re-detection**: Detection re-runs when the DXGI factory reports it is stale
(`IDXGIFactory1::IsCurrent() == false`), after `WM_DISPLAYCHANGE` /
`WM_DPICHANGED`, and at 1 Hz as a safety net. A cached factory is recreated
when stale, since a stale factory keeps returning old output descriptions.

**Rationale**: This is Microsoft's documented method for Windows Advanced Color
detection. The containing output tracks the monitor each per-monitor window
actually sits on.

**Alternatives considered**: `DisplayConfig` advanced-color queries alone
(don't give peak luminance). Checking only at start-up (fails FR-011).

## R6. Matching the user's SDR content brightness

**Decision**: Read the per-display SDR white level with
`DisplayConfigGetDeviceInfo(DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL)`,
matching the DXGI output's `DeviceName` to the DisplayConfig source's GDI
device name. `SDRWhiteLevel` is in units where 1000 = 80 nits. Poll at 1 Hz
per monitor (FR-012 requires following the Windows slider live, and Windows
sends no notification). The call is behind an `IDisplayLuminanceProvider`
interface so unit tests use a fake.

**Rationale**: The documented source for the SDR slider value. 1 Hz polling
is ~1 ms/s per monitor, negligible.

**Alternatives considered**: Hard-coding 80 or 200 nits (ignores the user's
setting; fails SC-003).

## R7. Swap-chain reconfiguration and FR-011

**Decision**: When a monitor's detected mode changes, that monitor's
`RenderSystem` reconfigures in place: release back-buffer views and the D2D
bitmap, `ResizeBuffers` with the new format, `SetColorSpace1`, then recreate
the views and D2D bitmap. This is the same shape as the existing `Resize`
path. Other monitors are untouched.

**Interaction with the existing rebuild**: `WM_DISPLAYCHANGE` already triggers
a coalesced rebuild of *all* monitor contexts (`Application.cpp`,
`WM_APP_REBUILD_CONTEXTS`). Whether toggling HDR raises `WM_DISPLAYCHANGE` is
not reliably documented and must be verified on hardware. If it does, the
existing full rebuild handles the change correctly (new swap chains re-run
detection), at the cost of briefly re-initializing other monitors too, exactly
as any other display change does today. **Spec FR-011 was relaxed**
accordingly: the requirement is correct output without restart, consistent
with how other display changes behave. It no longer promises that other
monitors are never disturbed.

## R8. Highlight headroom and tone mapping (Phase 3)

**Decision**:
- **Which content** (revised in R14): streak heads (already recognized as
  white instances) get the full `highlightGain`; trail glyphs get a share of
  it that grows with their brightness and is none below a floor, so the
  bright glyphs just behind a head rise above SDR white in the scheme's
  color and the tail does not. Overlays and scanline gaps are unchanged
  (FR-020).
- **Gain**: `highlightGain = headroom ^ (highlight / 100)`, where
  `headroom = effectivePeakNits / sdrWhiteNits` and `highlight` is the 0–100
  setting (default 80). 0 means no boost; 100 means heads aim at the display's
  peak. On a 600-nit peak at 240-nit white this gives ~2.1× (SC-005); at 1000
  nits, ~3.1×.
- **Roll-off**: identity up to SDR white, then a smooth shoulder (extended
  Reinhard with the white point at `headroom`) above it, applied to the
  **maximum channel**. RGB is scaled by `mapped / original` so hue is
  preserved (FR-019). The exact curve, and how the part above white reaches
  the output past a glow chain that stops at white, are in R14.
- **Ceiling**: `effectivePeakNits = min(MaxLuminance, …)` with a conservative
  400-nit default when the value is missing or implausible (< 80 or
  > 10 000 nits). When `sdrWhiteNits >= effectivePeakNits`, headroom is 1 and
  the curve is identity (FR-026).

**Rationale**: A max-channel shoulder never clips and never shifts hue. Tying
the gain to each display's own headroom keeps trail brightness identical
across monitors while the highlight height follows the hardware (FR-025).

**Alternatives considered**: BT.2390 EETF (designed for PQ mastering; heavier,
no benefit here). Luminance-based Reinhard (can push saturated channels past
the peak). A fixed-nits gain (overshoots dim HDR panels).

## R9. Statistics overlay on an FP16 back buffer

**Decision**: Create the D2D target bitmap in the swap chain's current format:
`B8G8R8A8_UNORM` for SDR and `R16G16B16A16_FLOAT` for HDR, both premultiplied.
Brush colors go through the same sRGB→linear conversion and are scaled by
`sdrWhiteNits / 80` in HDR, so the text appears at SDR brightness (FR-014).

**Rationale**: D2D 1.1 supports FP16 targets on FL 10+. Drawing straight to
the back buffer keeps today's structure.

**Alternatives considered**: Render stats into an 8-bit offscreen and composite
(extra texture and pass for a debug readout).

## R10. Screensaver preview window

**Decision**: The `/p` preview context never enters HDR. Output-mode selection
takes the display mode as an input and returns SDR for `ScreenSaverPreview`
(FR-017), in the same style as `MultiMonitorGate`.

## R11. System-level SDR→HDR processing (Auto HDR)

**Decision**: No action beyond declaring the scRGB color space. Windows Auto
HDR applies only to SDR swap chains, so presenting natively in HDR removes
MatrixRain from Auto HDR processing on that monitor. Verify on hardware that
Auto HDR is not applied twice.

## R12. Settings

**Decision**: Two new persisted settings, following the v1.5 pattern
(`ScreenSaverSettings` → `RegistrySettingsProvider` → `SharedState` →
`RenderParams`, live via `ConfigDialogController`, Cancel/Reset aware):
- `HdrMode`: `enum class HdrMode { Auto, Off }`, default `Auto`.
- `HighlightBrightness`: int 0–100, default 80.

Both controls go on the **Visuals** tab in an "HDR" group, with an info button
stating they affect HDR displays only (FR-024). They are disabled when no
monitor is currently in HDR mode.

## R13. Phase boundaries in code

| Phase | Ships | Gated by |
|---|---|---|
| 1 | R1–R4 (SDR mode only), R10 | Always on; no setting. |
| 2 | R4 HDR mode, R5–R7, R9, R11 | Output mode = HDR when detected; transform capped at SDR white. |
| 3 | R8, R12 | `HdrMode::Auto` + `HighlightBrightness`. |

Each phase leaves the code shippable: Phase 2 simply never returns a gain
above 1.

## R14. Phase 3 design revision: highlights over a glow chain that stops at white

Phase 1 settled that the glow is v1.6's, computed on encoded values (R3), and
the composite converts the scene to encoded values to screen the glow over
it. Both conversions saturate at 1. So as Phase 1 left it, a head brightened
past SDR white loses its excess **twice**: the extract clips it before the
blur, so its glow is only a white head's glow, and the composite clips the
scene itself, so the head is not brighter either. The gain of R8 would have
no visible effect. This revision keeps every pixel Phase 1 and Phase 2 produce
bit for bit, and carries the part above white around the v1.6 chain instead
of through it.

**Decision**: split the scene at SDR white.

```text
sdr    = min (scene, 1)          per channel
excess = scene - sdr             per channel, >= 0; nonzero only on boosted heads
```

- **The SDR part** goes through the v1.6 glow chain exactly as today. The
  extract encodes `sdr` (which is what `LinearToSrgb`'s saturate already did),
  and the composite screens the glow over `encode (sdr)`. Unchanged code.
- **The excess** skips it:
  - the extract also averages `excess` over its four texels **in linear
    light** (light adds; there is nothing to match here) and writes it, in
    color, to a second render target, an `R11G11B10_FLOAT` highlight texture
    at the bloom resolution;
  - one pass of the existing 5-tap blur runs over that texture (the blur
    shaders are generic over `float4`). As first designed it took the glow's
    own kernel and pass count; measured, that cost as much again as the
    whole glow chain (0.23 ms at 4K High), because the blur is bound by
    texture samples, not bandwidth. The above-white part is the glow's
    core, so a short blur that stays near the glyph is also the right
    shape (T041a);
  - the composite adds, in linear light, after decoding its v1.6 result:

```text
linear = SrgbToLinear (v1.6 composite (encode (sdr), glow))
       + excess                                   the head above white
       + highlightGlow * kHighlightGlowStrength * (bloomIntensity / 2.5)
```

  `highlightGlow` is the blurred excess, in color: a boosted green trail
  glyph glows green above white, a white head white. The
  `bloomIntensity / 2.5` term makes the Glow Intensity slider scale the
  highlight glow the way it scales the ordinary glow, with its default at 1.
  `kHighlightGlowStrength` starts at 1 and is tuned by eye on hardware
  (T044), like the highlight brightness default.
- **Then `OutputTransform`** tone-maps into the display's headroom (R8).

**Why the SDR paths do not move**: in SDR, and in HDR with mode Off or a
highlight gain of 1, the glyph shader clips at 1 before it linearizes (Phase 1
item 6), alpha compositing of values in [0, 1] stays in [0, 1], so `excess` is
exactly 0 everywhere, `sdr` is the scene, and the composite reduces to
today's. The highlight texture and its blur passes are not created or run at
all unless this monitor's highlight gain is above 1, so SDR monitors, and
HDR monitors with mode Off, pay nothing. The calibration baselines therefore
keep passing unchanged, and they are the regression test for this.

**Which glyphs, and how much** (option B, chosen by Rob over heads only and
over boosting everything): each instance gets

```text
gain = G ^ w
G    = HighlightGain (headroom, highlightBrightness, hdrMode, outputMode)
w    = 1                                              for a head
w    = kTrailHighlightShare * r^2,
       r = max (0, (brightness - kTrailHighlightFloor) / (1 - kTrailHighlightFloor))
                                                      for a trail glyph
```

so a head gets the whole gain, a trail glyph near full brightness gets a
large share of it in its own color, and below the floor a glyph gets none.
`G ^ w` rather than a linear blend keeps equal steps of `w` equal steps of
perceived brightness, and makes `w = 0` exactly 1. Starting values
`kTrailHighlightShare = 0.6` and `kTrailHighlightFloor = 0.5`, tuned on
hardware in T044; `HighlightWeight (isHead, brightness)` is a pure function
with tests.

Boosting everything by brightness with no floor (the option not taken) is
the same as raising the SDR brightness for the rain alone: no extra
contrast, the glow's above-white part turning into fog, and mixed monitors
no longer matching (FR-025).

**Where the gain goes**: per instance, as a new `highlightGain` float in
`CharacterInstanceData` (1 for overlays and for every glyph below the floor),
and the glyph shader multiplies by it after it has linearized and clipped the
pixel: `SrgbToLinear3 (min (X c^2, 1)) * gain`. The self-glow overshoot that
v1.6 clipped (a full-brightness head is 1.3) stays clipped; the gain is the
only thing that lifts a glyph above white, so the slider alone sets its
height.
Premultiplied alpha is unaffected: rgb scales, alpha does not, so a boosted
head still covers what is behind it by its coverage.

**Tone mapping** (R8, made concrete): on the maximum channel `m`, with
`h = headroom`:

```text
m <= 1:  f(m) = m
m >  1:  t = (m - 1) / (h - 1);   f(m) = 1 + (h - 1) * t / (1 + t)
h <= 1:  f(m) = min (m, 1)
rgb *= f(m) / m
```

`f(1) = 1`, `f'(1) = 1` (C1 at white, so no kink where heads cross it), and
`f -> h` as `m -> infinity`, so nothing clips and the peak is approached, not
hit. Scaling all three channels by one factor preserves hue (FR-019).

**Headroom per mode**: the monitor's real headroom in HDR with mode Auto; 1
in HDR with mode Off ("never exceed SDR white", FR-021), which also keeps the
scanline shader's `min (c, g_headroom)` clamp where Phase 2 had it; unused in
SDR.

**Cost**: nothing on SDR monitors or with mode Off. With highlights on: one
extra render target on the extract (MRT, same pass), one `R11G11B10_FLOAT`
texture pair at the bloom resolution through one 5-tap blur pass, and a few
ALU operations in the composite. Measured (T041a) at 48 to 51 microseconds a
frame at 4K Medium and High on the desktop card, 13 to 15 at Low and at
1080p, on top of HDR output's own cost; see `baseline.md`. A single-channel texture
would halve that but could only carry white, and option B puts color above
white.

**Overlap**: with trail glyphs boosted, dense regions carry more above-white
light, and the highlight glow adds rather than saturating. If that brings
back the dense-region fog Rob rejected in Phase 1, the fix is v1.6's own
soft saturation, `1 - exp (-x)`, applied to the highlight glow before it is
added. T044 looks for it.

**Alternatives considered**:
- *A 16-bit float glow chain with the excess in its alpha channel*: no extra
  passes, but it carries only one channel above white, so the excess would
  have no color, and it puts a different format under the v1.6 color chain
  in highlight mode.
- *Deriving the highlight glow from the existing glow* (for example by how
  white the blurred glow is): no extra cost, but it cannot tell a head's glow
  from a white custom color's trail glow, and FR-020 says trails do not move.
- *Running the whole glow chain in linear light in HDR*: the R3 history is the
  reason not to; every step of it changed the look.
- *Applying the gain on the CPU to the instance color* (data-model §5 as first
  written): the instance color is gamma-space and clipped per pixel after
  coverage, so a linear gain cannot ride on it (T041 already records this).

## Resolved unknowns

All Technical Context items are resolved above. Three items **must be verified
on hardware** during implementation rather than researched further: whether an
HDR toggle raises `WM_DISPLAYCHANGE` (R7), the Auto HDR interaction (R11), and
the Phase 1 calibration constants (R3).
