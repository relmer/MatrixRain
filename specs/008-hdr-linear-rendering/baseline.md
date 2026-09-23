# Baseline measurements

Reference numbers for feature 008. Everything later in this feature is judged
against what is recorded here.

## How to reproduce

```powershell
# Build the core library first; the harness links its Release .lib.
.\scripts\Invoke-MatrixRainBuild.ps1 -Target Build -Configuration Release

& "C:\Program Files\Microsoft Visual Studio\18\Enterprise\MSBuild\Current\Bin\MSBuild.exe" `
    tools\HdrCalibration\HdrCalibration.vcxproj /p:Configuration=Calibration /p:Platform=x64 `
    /p:SolutionDir="$PWD\"

.\x64\Calibration\HdrCalibration.exe --adapter warp --mode reference   # capture baseline/*.png
.\x64\Calibration\HdrCalibration.exe --adapter warp --mode compare     # diff against it
.\x64\Calibration\HdrCalibration.exe --adapter warp --mode benchmark
```

## Reference capture (T005)

**Captured at**: branch `008-hdr-linear-rendering`, **before any pipeline
change**. Only the RNG source had moved at this point, so these frames are
v1.6's rendering.

**Adapter**: WARP, `d3d10warp.dll` **10.0.26100.9278**. WARP ships with Windows
and can change with it. If a comparison suddenly lights up, check this version
before suspecting the renderer.

**Frame**: 1920x1080, seed 1, 180 fixed steps of 1/60 s.

**Display scales**: every case is captured at 100%, 125% and 150%, giving row
pitches (cells) of 24, 30 and 36 px. 24 frames in total, in
[baseline/](baseline/), with file names of the form `<case>-dpi<percent>.png`, lossless.

The sweep is not decoration. Scanline pitch is derived from the cell, so at
Style 1 a 24 px cell gives a **~2.4 px pitch** — below the 3 px floor where a
fractional pitch beats against the pixel grid and individual gaps drop out —
while a 36 px cell gives a clean **~3.6 px**. A single-scale baseline would
have pinned the scanline cases to one side of that boundary and left the other
unguarded. It also means the two scales this project is actually developed on
(125% and 150%) are covered directly.

### Mean luminance per case and scale

| Case | dpi 100% | dpi 125% | dpi 150% | Settings |
|---|---|---|---|---|
| `defaults` | 0.045544 | 0.061437 | 0.071511 | Glow 100%, size 100%, scanlines off, green |
| `glow-min` | 0.033470 | 0.042154 | 0.047519 | Glow intensity 1% |
| `glow-max` | 0.067967 | 0.094461 | 0.109529 | Glow intensity 200% |
| `glow-size-min` | 0.047643 | 0.063576 | 0.073236 | Glow size 50% |
| `glow-size-max` | 0.043832 | 0.058779 | 0.068747 | Glow size 200% |
| `scanlines-1` | 0.032478 | 0.043679 | 0.050765 | Scanlines on, Style 1 |
| `scanlines-100` | 0.032621 | 0.043870 | 0.051008 | Scanlines on, Style 100 |
| `custom-color` | 0.024502 | 0.034407 | 0.041271 | Custom `RGB(0, 128, 255)` |

Luminance rises with scale because a bigger cell means more glyph ink per
pixel of screen. Compare like with like: a case is only ever measured against
its own scale.

### Capture verified

Re-running compare mode in a fresh process immediately after the capture
reported **0 difference on all 24 frames** (max, mean, p99 and pixels over
threshold all zero). That one result exercises three things at once: the
animation is deterministic from its seed, the PNG round trip is lossless, and
WARP is reproducible across processes.

The comparison was then checked for sensitivity by substituting the
`glow-size-max` frame for the `defaults` baseline. It reported max 54, mean
8.65, p99 30, 1 593 121 pixels over threshold, worst at 1298x492 — while every
other case stayed at zero. So a zero row means "unchanged", not "not looking".

That substitution is worth recording for another reason: it is a glow-size
change, the exact thing the retired `HaloFalloffRadius` metric moved only 5%
on, in the wrong direction. See research R3.

### Storage

27.3 MB for the 24 frames, about 9 MB per scale. That is a real addition to a
55 MB repository, taken deliberately: a baseline that is not in version control
is not a baseline. It is a **one-time** cost, because Phases 2 and 3 compare
against this same v1.6 capture rather than re-baselining — which is the correct
methodology anyway, not just the cheap one.

## Difference thresholds for Phase 1 (T018)

Phase 1 is **expected** to change these frames — removing banding from the
fading tails and fixing halo blending is the point of it. So the gate is not
"zero difference"; it is, per case **and per scale**:

| Measure | Threshold | Why |
|---|---|---|
| Mean luminance | within 5% of the matching cell above | Overall exposure must not drift (SC-001) |
| p99 difference | <= 12 code values | A change confined to a small part of the frame still shows here |
| Max difference | <= 48 code values | Isolated pixels may move a lot where banding steps used to sit |
| `.diff.png` inspection | Required | The numbers cannot tell an intended improvement from a regression |

These are starting values, set from the sensitivity check above: a change large
enough to read as "the glow got bigger" put p99 at 30 and max at 54, so the
gate sits comfortably below that while leaving room for the fades to smooth
out. **Revise them in this file when T018 runs**, with the reasoning.

## Phase 1 calibration result (T018)

**The goal is to match what v1.6 put on screen.** v1.6 is the reference and is
untouched; every change below is to the new pipeline.

### Where the line ended up

| Stage | Space | Why |
|---|---|---|
| Glyph compositing into the scene | linear light, float target | The one place linear light stays. Antialiased edges and overlapping quads composite as light; no 8-bit banding |
| Glyph color, fade, edge weight | v1.6's value computed in gamma space, converted per pixel | Reproduces v1.6's displayed pixel exactly, then linearizes it |
| Bloom extract, blur, saturation, composite | **encoded values**, as v1.6's were | Every attempt to run these in linear light and compensate afterward left something visibly different; see below |
| Scanline darkening | encoded strength, applied in linear light | The Intensity slider keeps its v1.6 meaning |
| Output | linear light, encoded once for SDR | Where HDR output attaches in Phase 2 |

There are **no fitted constants**. The extract thresholds are v1.6's 0.1 and
0.6, the Glow Intensity mapping is v1.6's, and the composite is v1.6's formula.

### Result

Mean luminance against baseline, at 100% scale:

| Case | Baseline | Now | Delta |
|---|---|---|---|
| `defaults` | 0.045544 | 0.046318 | +1.7% |
| `glow-min` | 0.033470 | 0.034784 | +3.9% |
| `glow-max` | 0.067967 | 0.068059 | +0.1% |
| `glow-size-min` | 0.047643 | 0.048221 | +1.2% |
| `glow-size-max` | 0.043832 | 0.044725 | +2.0% |
| `scanlines-1` | 0.032478 | 0.031786 | -2.1% |
| `scanlines-100` | 0.032621 | 0.031968 | -2.0% |
| `custom-color` | 0.024502 | 0.024342 | -0.7% |

**All eight cases are inside SC-001's 5%.** The largest residual, `glow-min`
at +3.9%, is the trails with almost no glow: linear-light compositing of
antialiased glyph edges over other glyphs and the float scene's freedom from
8-bit quantization. That is the linear-light part that was kept, showing at
the size it shows.

### How it got here

The calibration went through nine distinct mismatches. Every one had the same
root: a step v1.6 performed on ENCODED values was being performed on linear
light, and the encode curve is concave, so the two give different pictures.
The first eight were each matched by reproducing v1.6's result for that step
and compensating for the rest; the ninth ended the pattern.

1. **Glow strength.** 0.3 of linear light added to a dark gap lands at sRGB
   0.58; the same `bloomIntensity` gave about 8x too much glow, a fog between
   the streaks. Fitted a ceiling.
2. **Glow shape.** The curve lifts a halo's dim tail far more than its core;
   a Gaussian blurred in linear light reached the screen much wider (39.6%
   of core at 16 px against v1.6's 13.5%). Rob saw glow that no longer hugged
   the glyphs. Shaped the falloff with the curve's exponent.
3. **Trail fade.** v1.6's shader applied brightness twice, to the color and
   again as its blend alpha; only the first had moved to the CPU and the
   second ran as linear-light alpha. Trails 20% too bright. Both factors are
   in `InstanceDisplayColor`. The FR-005 regression test had pinned the
   shader's output rather than the screen's; it now pins the screen.
4. **Glyph edges.** The atlas is premultiplied white, so v1.6 showed
   `color * c * c`. First approximated with a coverage exponent, then done
   exactly per pixel in the glyph shader.
5. **Scanlines.** Darkening removed less from linear light, and the float
   post-bloom target kept overshoot v1.6's 8-bit image had clipped, so the
   raster vanished over the brightest pixels. Exponent plus a headroom clamp.
6. **Head halos.** The self-glow overshoot leaked through the half-resolution
   extract's bilinear sample before any clamp could act. Glyph shader clips at
   white where v1.6's target did.
7. **Glow hue.** The rain's green has an encoded blue/green ratio of 0.39 but
   a linear one of 0.13; a linear blur showed a third of v1.6's blue in the
   dim halo. Rob saw the build as greener and yellower. Gave the glow its
   source's encoded hue.
8. **Glow on glyph bodies.** v1.6's `scene + glow * (1 - scene)` attenuated
   the glow landing on a bright glyph, per channel; without it every glyph
   carried extra blue, a blue silhouette in the difference images. Rob asked
   why the silhouettes remained. Applied v1.6's composite in encoded space.
9. **Glow where streaks overlap.** The soft saturation, `1 - exp(-x)`, acted
   on linear magnitudes, which in dim regions are far smaller than encoded
   ones, so dense regions kept adding glow where v1.6's had already clamped,
   and the fitted ceiling then rescaled everything to the average: dense
   areas relatively brighter, sparse ones dimmer, mean unchanged. Rob saw
   the glow in the dense middle drowning the characters that made it, and
   the head cores still red in the difference images.

   Rather than a ninth compensation, the extract now hands the blur chain
   ENCODED values. From there the blur, saturation, hue, overlap behavior
   and composite are v1.6's own math, by construction, and items 1, 2, 7
   and 8 and the fitted ceiling, falloff exponent and intensity remap are
   all deleted. One subtlety made it exact: v1.6's extract bilinearly
   averaged already-encoded 8-bit texels, and encoding the average of linear
   texels is not the same -- the curve is concave, so encode(mean) is at
   least mean(encode), and for a 2x2 block with one lit texel it is 0.54
   against v1.6's 0.25. With a plain sample the glow came out 1.71x too
   strong at every setting. The extract now encodes each of the four texels
   and averages them, which is the value v1.6's sampler produced.

### What was given up, and what was not

Given up: overlapping halos adding as light does instead of saturating as
v1.6's did. Rob judged the v1.6 behavior better for legibility -- the added
glow in dense regions obscured the characters making it -- and that is the
kind of call FR-005 exists to protect.

Kept: glyph compositing in linear light, float intermediates throughout (no
8-bit banding in the scene, the blur chain or the post-bloom target), and the
linear-light output stage the HDR phases need. Phase 3 note: the extract's
encode saturates at white, so a head above SDR white blooms as a white head;
highlight bloom needs its own handling in Phase 3.

### What this means for SC-001

SC-001's 5% mean-luminance gate stands and is met on all eight cases. Neither
threshold nor gate was widened.

### A note on what the metrics missed

Of the nine mismatches, six were invisible to every number the harness
produced: glow shape, trail fade, the head leak, hue, glyph-body glow and the
overlap saturation. Mean luminance was within a few percent while the halo was
three times too wide; the FR-005 regression test passed while every trail was
20% too bright, because it pinned the wrong quantity; the head leak hid inside
a fitted constant; and the overlap error hid behind a ceiling fitted to the
average, which is exactly the kind of error an average cannot see. All six were
caught by Rob comparing renders by eye. Worth remembering when the Phase 2 and
Phase 3 gates are written: a scalar can confirm a suspicion but will not raise
one, a fitted constant can absorb a defect as easily as correct for one, and
matching a mean says nothing about matching a distribution.

## Performance baseline (T001, T005)

600 frames per preset, timed with D3D11 timestamp queries around the render,
defaults settings.

**Frames are deliberately not presented.** `RenderSystem::Present` uses a sync
interval of 1, so presenting inside the timing loop measures the display's
refresh rather than the rendering: the first run of this benchmark returned
~12 ms for *every* preset with a p95 pinned at 16.6 ms, which is 60 Hz, not GPU
work. The window is hidden and nothing reads the back buffer in benchmark mode,
so there is nothing to present. The harness also blocks on each frame's queries,
so frames do not overlap and a per-frame figure means what it says.

These are therefore measures of **rendering cost**, not of achievable frame
rate. That is what a 5% regression gate needs: frame rate on this hardware is
pinned to the refresh rate at every preset and has no headroom in which a
regression could show itself.

### Hardware: NVIDIA GeForce RTX 5070 Ti

| Configuration | Preset | Mean GPU ms | p95 GPU ms |
|---|---|---|---|
| 1920x1080 @ 100% (reference) | Low | 0.086 | 0.090 |
| 1920x1080 @ 100% (reference) | Medium | 0.118 | 0.123 |
| 1920x1080 @ 100% (reference) | High | 0.158 | 0.162 |
| 3840x2160 @ 125% (landscape 37", primary) | Low | 0.204 | 0.219 |
| 3840x2160 @ 125% (landscape 37", primary) | Medium | 0.323 | 0.336 |
| 3840x2160 @ 125% (landscape 37", primary) | High | 0.439 | 0.476 |
| 2160x3840 @ 150% (portrait 32") | Low | 0.154 | 0.163 |
| 2160x3840 @ 150% (portrait 32") | Medium | 0.258 | 0.271 |
| 2160x3840 @ 150% (portrait 32") | High | 0.406 | 0.431 |

Reproduce with, for example:

```powershell
.\x64\Calibration\HdrCalibration.exe --adapter hardware --mode benchmark --frame 3840x2160 --dpi 125
```

Presets order correctly (Low < Medium < High) at every configuration, and p95
sits within ~5% of the mean, so the measurement is stable enough that a 5%
regression is detectable rather than lost in noise.

The portrait monitor costs less than the landscape one despite the same pixel
count, because its 150% scaling means larger cells and so fewer glyphs to draw.

### WARP (software rasterizer), 1920x1080 @ 100%

| Preset | Mean GPU ms | p95 GPU ms |
|---|---|---|
| Low | 5.492 | 6.264 |
| Medium | 10.492 | 11.236 |
| High | 16.060 | 16.871 |

Not a frame-rate prediction; useful as a repeatable before-and-after that does
not depend on GPU driver or thermal state.

### How T001 was satisfied

T001 originally called for reading the in-app statistics overlay on each
monitor at each preset. That turned out to need the measured monitor to be the
Windows primary — statistics render only on the primary context, and GPU% from
PDH is process-wide rather than per monitor — and changing the primary monitor
has side effects on a working desktop that are not worth a benchmark.

The harness numbers above replace those readings and improve on them: they are
per-configuration, measured on the GPU timeline rather than read off a screen,
repeatable to within a few percent, and they need no change to the desktop.
Each monitor is represented by its real resolution and scaling.

**Optional spot-check, not yet done**: run the app windowed with multi-monitor
disabled, maximized on each monitor in turn, statistics on, and confirm it
holds the refresh rate at every preset. That confirms the frame rate is pinned,
which is the assumption the cost-based gate rests on.

## Performance gate result (T020)

**Same-session comparison.** The T005 build (commit `4be6023`, built in a
scratch worktree) and the final Phase 1 build were run alternately, two rounds
each, 600 frames per preset at defaults, same protocol as above. The
alternation matters: the T005 table above was recorded in an earlier session,
when this GPU was running about 20% slower than it did during T020, so
comparing today's build against that table would have hidden a third of the
regression. The gate is judged against the same-session numbers only.

### Hardware: NVIDIA GeForce RTX 5070 Ti, mean GPU ms, two rounds averaged

| Configuration | Preset | T005 | Final | Delta |
|---|---|---|---|---|
| 1920x1080 @ 100% | Low | 0.069 | 0.069 | 0% |
| 1920x1080 @ 100% | Medium | 0.095 | 0.097 | +2.1% |
| 1920x1080 @ 100% | High | 0.124 | 0.128 | +3.6% |
| 3840x2160 @ 125% (landscape) | Low | 0.163 | 0.166 | +2.2% |
| 3840x2160 @ 125% (landscape) | Medium | 0.255 | 0.273 | **+7.1%** |
| 3840x2160 @ 125% (landscape) | High | 0.366 | 0.385 | **+5.2%** |
| 2160x3840 @ 150% (portrait) | Low | 0.128 | 0.128 | 0% |
| 2160x3840 @ 150% (portrait) | Medium | 0.218 | 0.232 | **+6.7%** |
| 2160x3840 @ 150% (portrait) | High | 0.332 | 0.346 | +4.4% |

Run-to-run noise is about 1.5%. Six of nine are inside the 5% gate. Medium on
both real monitors and High on the landscape one are over it, by 0.2 to 2.1
points, which is 14 to 19 microseconds per frame.

### Where the cost went, and what came back

The starting point, with `R16G16B16A16_FLOAT` for the scene and post-bloom
targets as research R2 originally chose, was +6% to +9% at 1080p, +17% to
+34% at 4K landscape and +21% to +49% at portrait. Each step below was a
timing-only probe with the image checked afterward.

1. **The shader math was not it.** Removing the glyph shader's decode, the
   extract's per-texel encode and the composite's encode, one at a time, moved
   nothing outside noise on hardware.
2. **The format was.** FP16 full-resolution targets are 8 bytes per pixel
   where v1.6 wrote 4, paid on the clear and on the composite's read, and the
   cost scaled with pixel count. `R11G11B10_FLOAT` (4 bytes, still float)
   brought every full-screen pass back to v1.6's cost. `R8G8B8A8_UNORM_SRGB`
   was tried too, hardware encode on write with a linear blend: v1.6's exact
   precision, but slower than R11 everywhere (+16% at portrait). Research R2
   records the precision analysis of R11.
3. **Blending into R11 costs more than into 8-bit when glyphs are large**: 14
   microseconds per frame at portrait 150%, nothing at landscape 125%, shown
   by removing the glyph draw from both builds. Glyph quads are deliberately
   taller than the row pitch, so most blended pixels carry zero coverage. The
   glyph shader now discards those. Bit-identical image on all eight cases,
   and portrait Low went from 0.143 to T005's 0.128.
4. **The composite's SDR fast path**: when the composite writes the SDR back
   buffer it now hands over the encoded result directly instead of decoding
   it for `OutputTransform` to encode again, an identity costing six `pow()`
   per pixel at full resolution. Pixel-neutral, checked against the
   calibration numbers.
5. **What remains is the bloom chain at Medium and High**, 12 to 20
   microseconds. Its C++ and its blur shaders are identical to T005's (diffed).
   It is the sum of three things each inside the noise floor on its own: the
   extract's four loads and twelve `pow()` at bloom resolution (about 4 µs),
   the composite's encode of the scene at full resolution (about 3 µs), and
   the R11 chain against v1.6's 8-bit one (3 to 7 µs). The first two are
   what the v1.6 match requires (calibration item 9 above). An 8-bit chain
   was measured: about 1% back, and its rounding moved the calibration mean
   by +1.5%, so it was rejected. Two micro-optimizations, `Load` instead of
   `Sample` in the composite and the scene size from a constant buffer instead
   of `GetDimensions`, measured slower and were reverted.

### WARP (software rasterizer), 1920x1080 @ 100%, mean ms

| Preset | T005 | Final | Delta |
|---|---|---|---|
| Low | 5.6 | 7.4 to 7.8 | +30% to +40% |
| Medium | 10.6 | 16.4 to 17.7 | +55% to +67% |
| High | 15.9 | 25.4 to 28.0 | +60% to +76% |

Two runs of the final build are shown because WARP is noisier than the GPU.
Attribution by the same probes: removing the three encode/decode steps from
the shaders takes WARP to 6.0 / 13.8 / 23.7, so the `pow()` math that costs
nothing on hardware is most of the Low regression on a software rasterizer;
8-bit scene targets take it to 7.0 / 16.1 / 25.0. The rest is the blur chain
reading and writing float textures instead of 8-bit ones. WARP is the fallback
for a machine with no usable GPU driver (a remote desktop session, some VMs).
At the Low preset it still renders a 1080p frame in under 8 ms.

### Verdict

The gate as written is not met at Medium (both monitors) and High (landscape)
on hardware, by 0.2 to 2.1 points, and not met on WARP by a wide margin. The
frame rate is pinned to the refresh rate at every preset on this hardware and
has no headroom in which 14 to 19 microseconds could show. The remaining cost
is the encode work the v1.6 match requires, plus the float format the HDR
phases require. Retuning the Medium preset (the remedy tasks.md lists) would
change what Medium looks like, and was not done without a say-so. Rob decides.

