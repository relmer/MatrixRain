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

**Captured at**: commit `662c7f6` plus the frame-comparison change, on branch
`008-hdr-linear-rendering`, **before any pipeline change**. Only the RNG source
had moved at this point, so these frames are v1.6's rendering.

**Adapter**: WARP, `d3d10warp.dll` **10.0.26100.9278**. WARP ships with Windows
and can change with it. If a comparison suddenly lights up, check this version
before suspecting the renderer.

**Frame**: 1920x1080, seed 1, 180 fixed steps of 1/60 s, character scale and
DPI scale pinned to 1.0 so the result does not depend on the monitor the
harness happens to launch on.

Baseline frames are in [baseline/](baseline/), one PNG per case, lossless.

| Case | Mean luminance | Settings |
|---|---|---|
| `defaults` | 0.045544 | Glow 100%, size 100%, scanlines off, green |
| `glow-min` | 0.033470 | Glow intensity 1% |
| `glow-max` | 0.067967 | Glow intensity 200% |
| `glow-size-min` | 0.047643 | Glow size 50% |
| `glow-size-max` | 0.043832 | Glow size 200% |
| `scanlines-1` | 0.032478 | Scanlines on, Style 1 |
| `scanlines-100` | 0.032621 | Scanlines on, Style 100 |
| `custom-color` | 0.024502 | Custom `RGB(0, 128, 255)` |

### Capture verified

Re-running compare mode in a fresh process immediately after the capture
reported **0 difference on all eight cases** (max, mean, p99 and pixels over
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

## Difference thresholds for Phase 1 (T018)

Phase 1 is **expected** to change these frames — removing banding from the
fading tails and fixing halo blending is the point of it. So the gate is not
"zero difference"; it is:

| Measure | Threshold | Why |
|---|---|---|
| Mean luminance | within 5% of the row above | Overall exposure must not drift (SC-001) |
| p99 difference | ≤ 12 code values | A change confined to a small part of the frame still shows here |
| Max difference | ≤ 48 code values | Isolated pixels may move a lot where banding steps used to sit |
| `.diff.png` inspection | Required | The numbers cannot tell an intended improvement from a regression |

These are starting values, set from the sensitivity check above: a change large
enough to read as "the glow got bigger" put p99 at 30 and max at 54, so the
gate sits comfortably below that while leaving room for the fades to smooth
out. **Revise them in this file when T018 runs**, with the reasoning.

## Benchmark: WARP (T005)

600 frames per preset, GPU timestamp queries, defaults settings.

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
