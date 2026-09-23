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
[baseline/](baseline/), named `<case>-dpi<percent>.png`, lossless.

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

## Benchmark: WARP (T005)

600 frames per preset, GPU timestamp queries, defaults settings, 100% scale
only — the sweep guards appearance, and tripling the benchmark's runtime would
buy nothing SC-006 asks for.

| Preset | Frames | Mean GPU ms | p95 GPU ms |
|---|---|---|---|
| Low | 600 | 10.292 | 16.682 |
| Medium | 600 | 24.107 | 32.550 |
| High | 600 | 16.310 | 16.992 |

WARP is a software rasterizer, so these are not frame-rate predictions and the
presets do not order the way they will on a GPU (Medium lands slower than High
here). They exist as a repeatable before-and-after on identical hardware, which
is all SC-006 needs from them. **Hardware numbers are the real gate.**

## Benchmark: hardware (T005) — NOT YET CAPTURED

Run `--adapter hardware --mode benchmark` and record the result here.

## Manual FPS baseline (T001) — NOT YET CAPTURED

Needs the v1.6.0 build run on each monitor, statistics overlay on (`S`), at
each quality preset. Record FPS and GPU% per preset per monitor, with
resolution, DPI and GPU model.

| Monitor | Resolution | DPI | Preset | FPS | GPU% |
|---|---|---|---|---|---|
| | | | | | |
