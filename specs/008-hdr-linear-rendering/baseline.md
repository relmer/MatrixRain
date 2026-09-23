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

Two constants in `Shaders/BloomComposite.ps.hlsl`, both measured:

| Constant | Value | What it does |
|---|---|---|
| `kBloomFalloff` | 2.4 (`MR_SRGB_CURVE_GAMMA`) | Shapes the halo's falloff |
| `kBloomCeiling` | 0.571 | Scales the glow's strength |

### Strength: the encode curve makes a little glow look like a lot

Glow is added in linear light and then encoded, and the encode curve is steep
near black: 0.3 of linear light added to a dark gap lands at sRGB 0.58. The
same `bloomIntensity` that looked right in gamma space produced about eight
times too much apparent glow, and dark gaps are most of the frame. Before
calibration the default case rendered as a green fog filling the space between
the streaks.

The measurement is the **glow's own contribution**, `defaults` minus
`glow-min` mean luminance, not total luminance. Total luminance also carries
the trail change below, which no bloom constant can or should absorb; tuning
against the total would have forced the glow well under v1.6's to compensate.

| | Glow contribution |
|---|---|
| v1.6 baseline | 0.012074 |
| After calibration | 0.012056 |
| Ratio | **1.00x** |

### Shape: mean luminance is blind to it

Getting the strength right left the glow visibly wrong in a way no scalar
metric could see. Rob caught it by eye: the glow was "brighter and more opaque,
and doesn't hug the chars as tightly... the glow on a streak looks like it's
the same width the whole way down."

The cause is the same concave encode curve, acting this time on the halo's
shape rather than its strength. It lifts the dim tail far more than the bright
core, so a Gaussian blurred in linear light reaches the screen much flatter and
wider than one blurred in gamma space. For a sigma-8 halo:

| Distance from core | v1.6 showed | Linear, unshaped | Excess |
|---|---|---|---|
| 16 px | 13.5% of core | 39.6% | 2.9x |
| 24 px | 1.1% | 9.5% | 8.6x |
| 32 px | 0.04% | 0.3% | 9.8x |

Raising the glow to the encode curve's own exponent before adding it cancels
that lift, so what lands on screen falls off as v1.6's did. This is not gamma
compensation creeping back into the blending -- light is still added in linear
light. It shapes the glow's PROFILE, which is art direction rather than
physics, and v1.6's art direction is what FR-005 protects.

Shaping also made the glow behave consistently across the sliders. Before it,
the glow cases scattered from -9.6% to +22.9% against baseline; after, they sit
within a point and a half of each other.

### Where the frame still differs, and why

Mean luminance against baseline, at 100% scale:

| Case | Baseline | Now | Delta |
|---|---|---|---|
| `defaults` | 0.045544 | 0.052650 | +15.6% |
| `glow-min` | 0.033470 | 0.040594 | +21.3% |
| `glow-max` | 0.067967 | 0.078842 | +16.0% |
| `glow-size-min` | 0.047643 | 0.055090 | +15.6% |
| `glow-size-max` | 0.043832 | 0.050135 | +14.4% |
| `scanlines-1` | 0.032478 | 0.049271 | +51.7% |
| `scanlines-100` | 0.032621 | 0.049231 | +50.9% |
| `custom-color` | 0.024502 | 0.020135 | -17.8% |

Two of these are **structural**: no constant reaches them, because they are the
corrections this feature exists to make.

**Trails, about +21%.** The glyph blend is `SRC_ALPHA / INV_SRC_ALPHA`.
Compositing a half-faded character over black gives `srgb * a` in gamma space
but `LinearToSrgb(linear(srgb) * a)` in linear light, and because the encode
curve is concave the second is always brighter for `a < 1`. Every partially
faded trail character is lighter than it was, most so mid-fade, which is where
most of a trail lives. The banding this feature set out to remove was a symptom
of the same wrong arithmetic. There is also a hue shift: the rain's green is
`RGB(0, 255, 100)`, so blue sits mid-range where the curve bends hardest while
green is already at maximum. The difference image is correspondingly almost
entirely blue, and trails read very slightly less saturated. `custom-color` is
low for the same per-channel reason, its blue being a dominant channel rather
than a secondary one.

**Scanlines, about +51%.** Darkening now multiplies linear light, so half the
darkening means half the light, as a raster does to a phosphor. Applied to
gamma-encoded values as v1.6 did, the same factor removed considerably more
light than intended, which is why the effect read as a gray veil over the image
rather than a raster behind it. This is FR-001 being satisfied.

### What this means for SC-001

**SC-001's 5% mean-luminance gate cannot be met** while alpha blending and
scanline darkening are correct. The two are mutually exclusive: hitting the
number requires deliberately reintroducing the gamma-space errors this phase
removed, which would also restore the fade banding.

The gate is therefore replaced, for Phase 1, by:

| Measure | Gate |
|---|---|
| Glow contribution | within 5% of baseline (met: 1.00x) |
| Glow falloff shape | matches v1.6 within measurement, and by eye |
| Trail and scanline changes | explained and expected, judged by eye on hardware (T019) |
| Everything else | within 25% of baseline, with a stated reason per case |

Recording that here rather than quietly widening a threshold: the original
number was written before it was clear that alpha compositing was in scope.

### A note on what the metrics missed

The glow shape error was invisible to every number the harness produced. Mean
luminance was within a few percent of target while the halo was nearly three
times too wide at 16 px. The difference images showed it plainly once someone
looked. Worth remembering when the Phase 2 and Phase 3 gates are written: a
scalar can confirm a suspicion but will not raise one.

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
disabled, maximised on each monitor in turn, statistics on, and confirm it
holds the refresh rate at every preset. That confirms the frame rate is pinned,
which is the assumption the cost-based gate rests on.
