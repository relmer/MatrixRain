# Contract: HDR Settings and Dialog Controls (Phase 3)

## Registry

Stored beside the existing settings under the screensaver's key.

| Value | Type | Range | Default | On bad data |
|---|---|---|---|---|
| `HdrMode` | DWORD | 0 = Auto, 1 = Off | 0 | Unknown → 0 |
| `HighlightBrightness` | DWORD | 0–100 | 80 | Clamped |

Missing values use the defaults, so v1.6 installs upgrade silently.

## Visuals tab: "HDR" group

| Control | ID | Type | Behavior |
|---|---|---|---|
| HDR mode | `IDC_HDR_MODE_COMBO` | Drop-down: "Auto", "Off" | Live; Cancel / Reset aware |
| Highlight brightness | `IDC_HIGHLIGHT_SLIDER` + `_VALUE` + `_PROMPT` | Trackbar 0–100, value shown as `NN%` | Live; grayed when HDR mode is Off |
| Info | `IDC_HDR_INFO` | ⓘ owner-draw button | Tooltip: "Affects monitors with Windows HDR turned on. Other monitors are unchanged." |

- The group is **grayed** while no monitor is currently in HDR output mode,
  with the tooltip "No monitor has HDR turned on in Windows." It updates when a
  monitor's mode changes while the dialog is open.

**As built (T042, T043)**:
- One row rather than a group box, so the page keeps its size: prompt
  "HDR highlights:", the info button, the mode combo at the front of the
  slider column, then the slider and its `NN%` value.
- Which monitors have HDR on comes from DisplayConfig
  (`DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO_2`, Windows 11 24H2
  and later; nothing older is supported for this), not from
  `SharedState::hdrMonitorCount`. It answers the same in the Control Panel
  dialog, which has no render contexts, and it also tells a monitor that
  supports HDR but has it off from one that cannot do HDR at all.
- When a monitor supports HDR but it is off, the grayed row's tooltip says
  so and where to turn it on: "A monitor supports HDR, but it is turned off
  in Windows. Turn it on in Settings > System > Display > Use HDR to use
  these settings." Otherwise it says "No monitor has HDR turned on in
  Windows." With the row enabled and HDR mode Off, the grayed slider's
  tooltip says highlights are Off.
- The info button stays enabled while the row is grayed, so what the
  setting does can always be read.
- The row is re-evaluated on the dialog's existing 1 s timer.
- Graying follows the sheet-scoped pattern from the v1.6 scanline fix: the
  helper takes the property sheet and resolves the Visuals page itself.
- The layout keeps the dialog's existing size; the group sits below the
  scanline controls.

## Live plumbing

`ConfigDialogController::UpdateHdrMode` / `UpdateHighlightBrightness` →
`ApplicationState` → `SharedState` atomics → `SharedState::Snapshot` →
`RenderParams` (`hdrMode`, `highlightBrightness`) → per-monitor
`HighlightGain`. This is the same path `scanlinesEnabled` uses today.
