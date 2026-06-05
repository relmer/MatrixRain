# Contract: PropertySheetW Layout, Flags, and Runtime Updates

## Header (`PROPSHEETHEADERW`)

| Field | Value | Why |
|---|---|---|
| `dwSize` | `sizeof(PROPSHEETHEADERW)` | required |
| `dwFlags` | `PSH_PROPSHEETPAGE \| PSH_MODELESS \| PSH_NOAPPLYNOW \| PSH_PROPTITLE \| PSH_USECALLBACK` | modeless for live preview (R1); no Apply (FR-002); PROPTITLE+per-page USETITLE pair for runtime title updates (R2); callback for `PSCB_PRECREATE` ExStyle tweaks if needed |
| `hwndParent` | parent (preview HWND or NULL for standalone config) | unchanged from v1.4 |
| `hInstance` | app instance | required |
| `pszCaption` | string-table loaded `IDS_CONFIG_DIALOG_CAPTION` | sheet caption (title bar) |
| `nPages` | `2` | Visuals + Performance |
| `nStartPage` | `0` | Visuals first |
| `ppsp` | pointer to `PROPSHEETPAGEW[2]` (see below) | the two pages |
| `pfnCallback` | `&PropSheetCallback` | minimal; logs `PSCB_INITIALIZED` for diagnostics |

Return value: the property-sheet frame `HWND` (because `PSH_MODELESS` is set).
Stored in `Application::m_hConfigDialog` exactly as the v1.4 single-dialog
HWND was. The existing `IsDialogMessage(m_hConfigDialog, &msg)` branch in
`Application::RunMessageLoop` continues to work unmodified (R1).

## Pages (`PROPSHEETPAGEW`)

Both pages share this template:

| Field | Visuals page | Performance page |
|---|---|---|
| `dwSize` | `sizeof(PROPSHEETPAGEW)` | same |
| `dwFlags` | `PSP_USETITLE \| PSP_PREMATURE` | `PSP_USETITLE \| PSP_PREMATURE` |
| `hInstance` | app instance | same |
| `pszTemplate` | `MAKEINTRESOURCEW(IDD_VISUALS_PAGE)` | `MAKEINTRESOURCEW(IDD_PERFORMANCE_PAGE)` |
| `pszTitle` | string-table `IDS_VISUALS_TAB_TITLE` (`L"Visuals"`) | string-table `IDS_PERFORMANCE_TAB_TITLE_INITIAL` (`L"Performance"` — replaced on first timer tick) |
| `pfnDlgProc` | `VisualsPageDlgProc` | `PerformancePageDlgProc` |
| `lParam` | pointer to shared page-state struct | same |

`PSP_USETITLE` is required so each page provides its own tab label (rather
than inheriting from the dialog template's caption); this is what makes
`TabCtrl_SetItem` safe to call against the per-page tab item at runtime
(R2).

`PSP_PREMATURE` forces both pages' HWNDs to exist immediately after
`PropertySheetW` returns, so the cross-tab `EnableWindow` propagation
(R7) can fire from the initial Glow-Enabled / Scanlines-Enabled state
without waiting for the user to first navigate to the inactive tab.

## Per-tick title update (`WM_TIMER`, 1 Hz)

```cpp
// In the property-sheet frame's WM_TIMER handler (timer ID 1, 1000 ms):
HWND       hTab     = PropSheet_GetTabControl (hSheet);
WCHAR      fpsBuf  [8];
WCHAR      gpuBuf  [8];
WCHAR      title   [64];
TCITEMW    item    = {};


item.mask = TCIF_TEXT;

if (g_hasPublishedFps) swprintf_s (fpsBuf, L"%u", static_cast<unsigned> (g_publishedFps));
else                   StringCchCopyW (fpsBuf, ARRAYSIZE (fpsBuf), L"--");

if (g_hasPdhGpu)       swprintf_s (gpuBuf, L"%u", static_cast<unsigned> (g_pdhGpuPercent));
else                   StringCchCopyW (gpuBuf, ARRAYSIZE (gpuBuf), L"--");

swprintf_s (title, g_perfTitleFormat, fpsBuf, gpuBuf);   // "Performance (%s fps, %s%% GPU)"
item.pszText = title;

TabCtrl_SetItem (hTab, 1 /* Performance is page index 1 */, &item);
```

Contract:
- Timer fires at most once per second (`SetTimer (hSheet, IDT_PERF, 1000, NULL)`).
- Format string is loaded once at sheet creation via
  `LoadStringW (g_hInstance, IDS_PERFTAB_TITLE_FORMAT, g_perfTitleFormat, ...)`.
- `g_publishedFps` / `g_hasPublishedFps` are reads from
  `MonitorRenderContext::m_publishedFps` / `m_hasPublishedFps` (see
  `fps-publisher.md`), selecting the primary monitor's context.
- `g_pdhGpuPercent` / `g_hasPdhGpu` come from the same PDH counter the
  debug overlay uses (FR-011); `g_hasPdhGpu == false` until the first PDH
  collection returns non-zero data.
- Timer is killed in the property-sheet `PSCB_INITIALIZED` callback's
  cleanup path or in the frame `WM_DESTROY` handler.

## Dismissal semantics (FR-004a, SC-011)

| Path | Action |
|---|---|
| OK button | `PSN_APPLY` on each page returns `PSNRET_NOERROR`; `PropertySheet` posts `WM_DESTROY` to the frame; controller commits live values to registry; no rollback. |
| Cancel button | `PSN_RESET` on each page; controller invokes `CancelLiveMode()` (restores snapshot, pushes restored values into SharedState); frame destroyed. |
| X (sheet caption) / Alt+F4 | Routed by the default dialog proc through `IDCANCEL` (`WM_SYSCOMMAND` `SC_CLOSE` → `WM_COMMAND IDCANCEL`); identical to Cancel above. No `WM_CLOSE` handler needed. |

In all three dismissal paths, the `CustomColorPalette` written at
chooser-OK time is preserved (it was already in the registry; nothing
removes it) — see `registry-schema.md`.
