# Quickstart: Validating HDR Output and Linear-Light Rendering

**Feature**: [spec.md](spec.md) | **Plan**: [plan.md](plan.md)

How to prove each phase works. Math and interface details are in
[data-model.md](data-model.md) and [contracts/](contracts/); this guide only
covers running and checking.

## Prerequisites

- Visual Studio 2026 with the x64 (and optionally ARM64) MSVC toolset.
- For Phases 2–3: at least one monitor with **Windows HDR** available
  (Settings → System → Display → *Use HDR*). A second, SDR-only monitor for the
  mixed-mode checks.
- For SC-003 and SC-005: a luminance meter or colorimeter. Without one, do the
  side-by-side visual checks and note the criterion as visually verified.
- A v1.6.0 build (`x64\Release\MatrixRain.exe` from the `v1.6.0` tag) for
  side-by-side comparisons.

## Build and unit tests (every phase)

```powershell
.\scripts\Invoke-MatrixRainBuild.ps1 -Configuration Release
.\scripts\Invoke-MatrixRainTests.ps1 -Configuration Release -Platform Auto
```

Expected: 0 warnings, all tests pass, including `ColorMathTests` and
`OutputModeSelectionTests` ([contracts/color-math.md](contracts/color-math.md)).

## Phase 1: linear light, SDR output

| # | Check | Expected | Covers |
|---|---|---|---|
| 1 | Run v1.6 and the new build side by side at default settings on an SDR monitor | Same overall brightness, glow size, color | SC-001, FR-005 |
| 2 | Lower Density, watch a single streak's tail fade on black up close | No stepped bands | SC-002, FR-003 |
| 3 | Watch two heads pass each other | Overlapping halos brighter than either; no flat plateau | US1-3 |
| 4 | Toggle scanlines; sweep Intensity 1→100 | Darkening scales with Intensity; density per glyph unchanged from v1.6 | US1-4, FR-007 |
| 5 | Cycle every color scheme and a custom color | Hues match v1.6 | US1-5 |
| 6 | Each quality preset, Performance tab readout | FPS within 5% of v1.6 on the same machine | SC-006, FR-008 |
| 7 | Calibration harness (research R3) in compare mode | Every case within the difference thresholds in `baseline.md`, and the difference images show only the intended improvements | SC-001 |

## Phase 2: native HDR at SDR brightness

| # | Check | Expected | Covers |
|---|---|---|---|
| 1 | HDR on, run new build and v1.6 in turn | Rain brightness and color match | US2-1, SC-003 |
| 2 | Meter a full-white test patch vs the Windows SDR brightness setting | Within 5% | SC-003 |
| 3 | Move the Windows SDR content brightness slider while running | Rain follows within ~1 s | FR-012 |
| 4 | Check the help hint (shown at start), hotkey reference (`?`), usage (`MatrixRain /?`), and statistics (`S`) | Sharp, normal brightness | FR-014, SC-008 |
| 5 | HDR + SDR monitors together | Each correct; same appearance | FR-010, SC-007 |
| 6 | Toggle HDR in Settings 10× while running | Correct within 2 s every time; no crash | FR-011, SC-004 |
| 7 | Screensaver preview in Windows' screensaver settings | Renders SDR, as in v1.6 | FR-017 |
| 8 | Remote Desktop session | Falls back to SDR, no error | FR-015 |
| 9 | Force device loss (disable and re-enable the GPU) with HDR on | Recovers in HDR | FR-016 |
| 10 | Auto HDR on in Windows | No double processing; looks as in check 1 | research R11 |

## Phase 3: highlight headroom

| # | Check | Expected | Covers |
|---|---|---|---|
| 1 | HDR mode Auto, defaults | Heads visibly brighter than SDR white; trails unchanged | US3-1, FR-018, FR-020 |
| 2 | Meter a head on a ≥600-nit panel at ≤250-nit SDR brightness | ≥2× SDR white | SC-005 |
| 3 | Highlight brightness 100, dense rain | Highlights roll off smoothly; no clip or hue shift | FR-019 |
| 4 | Sweep the highlight brightness slider | Changes live; persists after restart | FR-022 |
| 5 | HDR mode Off | Identical to Phase 2 | US3-4 |
| 6 | Change HDR settings, press Cancel; separately press Reset to defaults | Both revert correctly | FR-023 |
| 7 | Dialog with no HDR monitor present | HDR group grayed with explanatory tooltip | FR-024 |
| 8 | Two HDR monitors with different peaks | Everything matches except highlight height | FR-025 |
| 9 | Low-headroom panel (HDR400-class) | Little or no boost, still smooth | FR-026 |
