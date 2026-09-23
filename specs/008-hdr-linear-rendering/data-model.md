# Data Model: HDR Output and Linear-Light Rendering

**Feature**: [spec.md](spec.md) | **Research**: [research.md](research.md) | **Date**: 2026-09-21

All types live in `MatrixRainCore`. "Pure" means no D3D/DXGI/Win32 calls, so
they are unit-testable per Constitution I (TDD).

## 1. `OutputMode` (pure)

```text
enum class OutputMode { Sdr, Hdr }
```

The presentation mode of one monitor. `Sdr` → 8-bit sRGB swap chain; `Hdr` →
FP16 scRGB swap chain.

## 2. `DisplayLuminance` (pure value)

Per-monitor facts read from the OS, refreshed at 1 Hz and on display changes.

| Field | Type | Source | Validation |
|---|---|---|---|
| `hdrEnabled` | `bool` | DXGI output color space is PQ/BT.2020 | — |
| `scRgbSupported` | `bool` | swap chain reports present support for scRGB (`RGB_FULL_G10_NONE_P709`) | Checked separately from `hdrEnabled`; both feed `SelectOutputMode` |
| `sdrWhiteNits` | `float` | DisplayConfig SDR white level (`/1000 * 80`) | Clamp to [80, 480]; default 80 when unavailable |
| `reportedPeakNits` | `float` | `DXGI_OUTPUT_DESC1::MaxLuminance` | May be 0 or implausible |
| `deviceName` | `std::wstring` | `DXGI_OUTPUT_DESC1::DeviceName` | Used to pair DXGI and DisplayConfig |

Derived (pure functions, see [contracts/color-math.md](contracts/color-math.md)):

| Derived value | Rule |
|---|---|
| `effectivePeakNits` | `reportedPeakNits` if in [80, 10 000], else 400 |
| `headroom` | `max(1, effectivePeakNits / sdrWhiteNits)`, so ≥ 1 always (FR-026) |
| `sdrWhiteScale` | `sdrWhiteNits / 80` (the scRGB multiplier for SDR white) |

## 3. `HdrSettings` (persisted, part of `ScreenSaverSettings`)

| Field | Type | Range | Default | Registry value |
|---|---|---|---|---|
| `m_hdrMode` | `HdrMode` (`Auto`, `Off`) | — | `Auto` | `HdrMode` (DWORD 0 = Auto, 1 = Off) |
| `m_highlightBrightness` | `int` | 0–100 | 80 | `HighlightBrightness` (DWORD) |

Validation: out-of-range registry values are clamped on read, and unknown
`HdrMode` values fall back to `Auto`, matching the existing
`ClampPercent`-style handling. Both fields take part in the dialog snapshot,
Cancel rollback and Reset to defaults (FR-023).

Phase 1 and Phase 2 builds may carry these fields with no UI. They have no
effect until Phase 3.

## 4. `OutputTransformCb` (per monitor, per pass)

The CPU mirror of the output constant buffer (`b1`) read by every pass that
can be the last one. It is uploaded once per frame, and again whenever
`isFinalPass` changes between passes. See
[contracts/output-transform.md](contracts/output-transform.md).

| Field | Type | SDR | HDR, Phase 2 | HDR, Phase 3 |
|---|---|---|---|---|
| `mode` | `uint` | 0 (sRGB encode) | 1 (scRGB) | 1 |
| `sdrWhiteScale` | `float` | unused | `sdrWhiteNits / 80` | same |
| `headroom` | `float` | 1 | 1 (clamp at white) | `DisplayLuminance.headroom` with HDR mode Auto; 1 with Off (research R14) |
| `isFinalPass` | `uint` | 1 when this pass writes the back buffer; 0 when it writes an intermediate (e.g. the composite ahead of scanlines) | same | same |

`highlightGain` is **not** here: it is applied per instance to streak heads on
the CPU (research R8), so it can be tested without the GPU and doesn't touch
trails.

## 5. `highlightGain` (pure function result)

`HighlightGain(headroom, highlightBrightness, hdrMode, outputMode)`:
- `1` when `outputMode == Sdr`, when `hdrMode == Off`, or in Phase 2.
- Otherwise `headroom ^ (highlightBrightness / 100)`.

Each instance carries `highlightGain ^ HighlightWeight (isHead, brightness)`
in `CharacterInstanceData::highlightGain`: the whole gain for a head, a share
growing with brightness for a trail glyph, 1 below the floor and for
overlays (research R14, option B). The glyph shader applies it after it
linearizes and clips the pixel; the instance color is gamma-space and cannot
carry it. Overlays and scanlines never receive it.

## 6. Render-target set (per `RenderSystem`)

| Target | Format after this feature | Size |
|---|---|---|
| Scene | `R11G11B10_FLOAT` (research R2, revised in T020) | full |
| Bloom, blur temp | `R11G11B10_FLOAT` | ÷ resolution divisor |
| Highlight, highlight blur temp (Phase 3) | `R11G11B10_FLOAT`, the scene above SDR white, in color | ÷ resolution divisor; created only while this monitor's highlight gain is above 1 (research R14) |
| Post-bloom | `R11G11B10_FLOAT` | full |
| Back buffer | `B8G8R8A8_UNORM` (SDR) / `R16G16B16A16_FLOAT` (HDR) | full |
| D2D target bitmap | matches back buffer | full |
| Glyph and overlay atlases | unchanged (`B8G8R8A8_UNORM`, coverage) | — |

## 7. State transitions (per monitor)

```text
            detect: HDR on + scRGB supported
   ┌─────┐ ─────────────────────────────────▶ ┌─────┐
   │ Sdr │                                     │ Hdr │
   └─────┘ ◀───────────────────────────────── └─────┘
            detect: HDR off, unsupported, preview window,
            or any HDR initialization failure
```

- **Entry**: at swap-chain creation, mode = `SelectOutputMode(...)`
  (see contract).
- **Triggers for re-selection**: factory stale, `WM_DISPLAYCHANGE`,
  `WM_DPICHANGED`, 1 Hz safety poll, device-lost rebuild.
- **Transition action**: in-place reconfiguration (release views → resize
  buffers with the new format → set color space → recreate views and D2D
  bitmap). On failure, fall back to `Sdr` and log once (FR-015).
- **Invariant**: `OutputTransformCb.mode` always matches the back buffer
  format actually in use.
