# Contract — Trackbar Tick-Mark Conventions

**Feature**: `006-multimon-gpu-efficiency`
**Applies to**: all `msctls_trackbar32` controls in the MatrixRain configuration dialog (`IDD_MATRIXRAIN_SAVER_CONFIG`).

This contract pins the visual conventions for trackbar ticks. Discrete sliders (the new advanced graphics controls) and percentage sliders (existing + Glow Intensity used as on/off) follow different rules, captured below.

---

## Rule 1 — Discrete sliders

Sliders whose positions correspond to a small, finite set of named choices.

- Style: `TBS_AUTOTICKS | TBS_TOP | WS_TABSTOP`.
- Range: from `0` (or `1` for `IDC_GLOWPASSES_SLIDER`) up to `n-1` (or `n`), step 1.
- Each tick position has a label drawn beneath it. Implementation: `LTEXT` statics in the `.rc` aligned at the tick screen-x positions in dialog units, computed once during layout authoring.
- The slider's vertical footprint is taller than a percentage slider to leave room for the labels.

Controls covered:

| Control                  | Range  | Default | Labels (left → right)                               |
|--------------------------|--------|---------|-----------------------------------------------------|
| `IDC_GLOWPASSES_SLIDER`  | 1..4   | 3       | `"1"` `"2"` `"3"` `"4"`                              |
| `IDC_GLOWRES_SLIDER`     | 0..3   | 2       | `"Eighth"` `"Quarter"` `"Half"` `"Full"`             |
| `IDC_GLOWSMOOTH_SLIDER`  | 0..2   | 2       | `"Low"` `"Medium"` `"High"`                          |

The right-side value indicator (an existing pattern, `IDC_*_LABEL`) mirrors the currently-selected tick's label text (e.g., `"Quarter"`), not the raw integer position.

---

## Rule 2 — Percentage sliders

Sliders whose positions represent percentages or quasi-percentage values.

- Style: `TBS_AUTOTICKS | WS_TABSTOP` (existing).
- Ticks are unlabeled.
- The slider does NOT snap to ticks (free positioning).
- Tick frequency chosen so a tick falls at the midpoint of the range, with a target of ~21 ticks total. Per-slider settings:

| Control                       | Range    | Mid | `TBM_SETTICFREQ` | Tick count | Notes |
|-------------------------------|----------|-----|------------------|------------|-------|
| `IDC_DENSITY_SLIDER`          | 0..100   | 50  | 5                | 21         | Exact midpoint tick at 50. |
| `IDC_ANIMSPEED_SLIDER`        | 1..100   | —   | 5                | 21         | `TBM_SETTICFREQ = 5` yields ticks 1, 6, …, 96; add one explicit tick at 100 via `TBM_SETTIC` for a total of 21. Midpoint (50.5) is non-integer; closest tick is 51. |
| `IDC_GLOWINTENSITY_SLIDER`    | 0..200   | 100 | 10               | 21         | Exact midpoint tick at 100. Value indicator reads `"0% (glow disabled)"` at position 0 (FR-031). |
| `IDC_GLOWSIZE_SLIDER`         | 50..200  | 125 | 5                | 31         | freq=10 would land ticks 50, 60, …, 120, 130, …, 200 with no tick at 125 (violating the midpoint rule); freq=5 keeps the midpoint at 125 at the cost of denser ticks. |

The right-side value indicator continues to show the integer percent value followed by `"%"`, with the documented `"0% (glow disabled)"` special-case for Glow Intensity.

---

## Rule 3 — Initialization

The existing helper `InitializeSlider(hDlg, sliderId, labelId, min, max, current)` in `ConfigDialog.cpp` is extended to:
1. Send `TBM_SETRANGE` (already done).
2. Send `TBM_SETTICFREQ` per the per-slider table above (new).
3. For `IDC_ANIMSPEED_SLIDER`, additionally send `TBM_SETTIC, 0, 100` after `TBM_SETTICFREQ` to add the explicit tick at 100 (new).
4. Send `TBM_SETPOS` (already done).
5. Format and set the label text (already done) — extended to special-case `IDC_GLOWINTENSITY_SLIDER` value 0 (FR-031) and to use mapped text for discrete sliders (FR-022, FR-029, FR-030).

The `WM_HSCROLL` handler is similarly extended to update the value indicator using the mapped/special-case text.

---

## Rule 4 — Non-trackbar quality controls

Out of scope for this contract but listed here for completeness:

- `IDC_QUALITY_PRESET_COMBO`: combobox (`CBS_DROPDOWNLIST | WS_TABSTOP`). Entries: `"Low"`, `"Medium"`, `"High"`, `"Custom"`. No tick concept.
- `IDC_GRAPHICS_ADVANCED_CHECK`: checkbox (`BS_AUTOCHECKBOX | WS_TABSTOP`). Drives dialog dynamic resize per R-010.
- `IDC_*_INFO`: owner-drawn buttons (`BS_OWNERDRAW | WS_TABSTOP`) for information tips. Hover + Space/Enter trigger the shared tooltip per R-009.
