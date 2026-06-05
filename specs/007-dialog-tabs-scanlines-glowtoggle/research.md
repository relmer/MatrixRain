# Phase 0 Research: Settings Dialog Overhaul, Scanlines & Glow Toggle (v1.5)

All "NEEDS CLARIFICATION" items from the spec's Session 2026-02-23 have already
been resolved at spec-time. This document resolves the *implementation*
unknowns flagged in the plan kickoff so Phase 1 can produce concrete contracts
and a tasks plan with no ambiguity.

---

## R1 — Modeless `PropertySheetW` lifecycle & integration with the main message loop

**Decision**: Create the property sheet modeless via `PropertySheetW(&hdr)` with
`PSH_MODELESS` set on `PROPSHEETHEADER::dwFlags`. The call returns the outer
property-sheet `HWND` (when `PSH_MODELESS` is set, the function returns the
window handle instead of a result code). Store that handle in
`Application::m_hConfigDialog` exactly as today; the existing message-loop
branch `if (m_hConfigDialog && IsDialogMessage(m_hConfigDialog, &msg))` keeps
working unchanged because `IsDialogMessage` correctly handles property-sheet
frame windows (tab navigation, accelerator routing, default-button handling are
all property-sheet-aware when fed the frame HWND).

**Rationale**: Preserves the v1.3/v1.4 integration model. No changes to
`Application.cpp`'s loop required; only the *type* of window pointed at by
`m_hConfigDialog` changes (from a single dialog to a property-sheet frame).
Destruction is the same `DestroyWindow(m_hConfigDialog)` path. The
`PropSheet_*` macros all accept the frame HWND.

**Alternatives considered**:
- *Modal `PropertySheetW`* — rejected: would block the message loop and break
  live-preview while the dialog is open.
- *Custom tab control (`SysTabControl32`) inside the existing single dialog* —
  rejected: would lose runtime title updates (`PSM_SETTITLE` is property-sheet
  specific), lose the "no Apply" suppression flag, and reinvent tab-key
  navigation. More code, less platform integration.

**References**: `PropertySheetW`, `PROPSHEETHEADER`, `PROPSHEETPAGE`,
`IsDialogMessage` on MSDN. Property sheets are a thin wrapper around a tab
control hosting child dialogs; `IsDialogMessage` on the frame HWND walks the
active page automatically.

---

## R2 — `PSM_SETTITLE` runtime title updates

**Decision**: Set `PSH_PROPTITLE` on the header (so the title becomes
`<sheet-title> <Properties>` style — but for this app the *page* titles, not the
sheet title, are what we mutate; sheet caption is irrelevant for a modeless
config) and `PSP_USETITLE` on each `PROPSHEETPAGE` so each page provides its
own title string. Per-tick: `PropSheet_SetTitle(hSheet, dwStyle, pszNewTitle)`
updates the *property sheet caption*; to update the *tab text* we use
`TabCtrl_SetItem` on `PropSheet_GetTabControl(hSheet)` with `TCITEM::mask =
TCIF_TEXT` and `pszText = pszNewTitle`. This is the documented technique for
runtime tab-label changes and is supported on Win10 1809+ and Win11 22H2+
without any quirks (the underlying `SysTabControl32` has handled
`TCM_SETITEMW` since NT 3.51).

**Rationale**: `PSP_USETITLE` ensures the per-page title is owned by us (not
auto-generated from the dialog template caption), which is what makes
`TabCtrl_SetItem` safe — we are not fighting the property-sheet framework for
ownership of the string. The mixed-case format string `Performance (%u fps,
%u%% GPU)` is loaded once via `LoadStringW` at init, cached in a wide-string
member; per tick we `swprintf_s` into a stack buffer, then `TabCtrl_SetItem`.

**Alternatives considered**:
- *`PropSheet_SetTitle` alone* — rejected: that updates the *sheet caption*
  (which for a modeless sheet is the title-bar text), not the visible tab
  label.
- *Owner-drawn tab control* — overkill; the standard tab control re-paints
  itself on `TCM_SETITEMW`.

**References**: `PSM_SETTITLE`, `TCM_SETITEMW`, `PropSheet_GetTabControl` on
MSDN. Verified Windows 11 22H2 + Win10 22H2 behavior in dev notes from the
v1.4 multimon work (property-sheet stack has not regressed in any recent
update).

---

## R3 — Live FPS exposure from `MonitorRenderContext` to the dialog thread

**Decision**: Add `std::atomic<float> m_publishedFps {0.0f};` to
`MonitorRenderContext`. The render thread writes it once per frame after the
existing `FPSCounter::Tick()` call using `m_publishedFps.store(fps,
std::memory_order_relaxed)`. The dialog-thread timer reads it via
`m_publishedFps.load(std::memory_order_relaxed)`. Float load/store is
lock-free on every Windows architecture we target (x64 and ARM64 both
guarantee single-instruction aligned 4-byte loads/stores; `std::atomic<float>`
is guaranteed `is_always_lock_free` for 4-byte float on these targets).
Initial value `0.0f` is the sentinel that means "no frame rendered yet" — the
dialog code substitutes `--` (FR-012) whenever the value compares equal to
`0.0f` *and* no `TRUE` flag from a separate `std::atomic<bool>
m_hasPublishedFps` has been set. Using a small bool flag avoids the race where
a *real* 0.0 fps reading (e.g., paused) is indistinguishable from "never set".

**Rationale**: Simplest correct lock-free design. No mutex on the render hot
path. Memory ordering can be relaxed because the only consumer is a 1 Hz
display timer — we have no ordering dependency with any other render-thread
write.

**Alternatives considered**:
- *Mutex-guarded float* — rejected: locks on every render tick.
- *Push from render thread via `PostMessage`* — rejected: forces the render
  thread to know about UI HWNDs, breaks layering.

---

## R4 — `CBN_SELENDOK` vs `CBN_SELCHANGE` for re-clicking "Custom…"

**Decision**: The combo handler tracks the *last selected index* in a static
`int s_lastColorComboIndex` (or instance member on the page-specific state
struct attached via `DWLP_USER`). It listens to both `CBN_SELCHANGE` and
`CBN_SELENDOK`:

- `CBN_SELCHANGE` fires when the selection *index* actually changes. Handled
  normally: update `s_lastColorComboIndex`; if the new index is "Custom",
  open the chooser.
- `CBN_SELENDOK` fires when the user commits a selection (clicks an item or
  presses Enter in the dropdown), *even if the same item is committed* with
  certain comctl6 + theming combinations. Handler: if the committed index is
  the "Custom" index AND `s_lastColorComboIndex` was already "Custom", treat
  it as a same-item re-click and force-open the chooser.

This matches the documented Win32 combo behavior and is the same idiom used
inside the Settings app for "Pick a colour" controls.

**Rationale**: Per the Q2 clarification, the chooser MUST re-open even when
the user picks "Custom…" while Custom is already active. Relying on
`CBN_SELCHANGE` alone misses the same-item case; relying on `CBN_SELENDOK`
alone double-fires on normal selection changes. Tracking the last index lets
us deduplicate cleanly.

**Alternatives considered**:
- *Subclass the combo's edit/list child to intercept `WM_LBUTTONUP`* — rejected:
  fragile, theme-dependent, breaks keyboard equivalence.
- *Send `CB_SETCURSEL(-1)` immediately after the chooser closes* — rejected:
  visible flicker, and the "Custom" label needs to remain selected.

---

## R5 — `ChooseColorW` threading + custom-palette REG_BINARY round-trip

**Decision**: Call `ChooseColorW` synchronously on the dialog (UI) thread.
The chooser is modal-to-its-owner; passing the property-sheet frame HWND as
`hwndOwner` is correct. The 16-slot palette is held in a static
`COLORREF g_customColorPalette[16]` initialised on dialog open by
`RegistrySettingsProvider::LoadCustomColorPalette` (reads `CustomColorPalette`
REG_BINARY, 64 bytes exact — if size differs, zero-fill). Pass
`&g_customColorPalette[0]` to `CHOOSECOLORW::lpCustColors`. On chooser-OK,
unconditionally write the (possibly-mutated) palette back via
`RegistrySettingsProvider::SaveCustomColorPalette`, then write `CustomColor`
to the registry and push live (FR-031, FR-035). On chooser-Cancel: do nothing
(the palette is *not* written back even though Windows may have mutated the
in-memory buffer — this is per FR-032 + the FR-004 clarification that *only
the active CustomColor* rolls back, but palette writes only happen on OK).

Wait — re-reading FR-035: "edits made inside ChooseColorW are written through
to the registry at chooser-OK time and survive an outer property-sheet
Cancel". So palette persistence happens on chooser-OK regardless of outer
sheet OK/Cancel. ✓ This matches the decision above.

`COLORREF` layout matches `CHOOSECOLORW::lpCustColors` (16 × 4-byte COLORREFs
= 64 bytes); REG_BINARY round-trip is a verbatim `memcpy`.

**Rationale**: ChooseColorW is documented thread-safe per-thread; we never
call it from anywhere but the dialog thread. The 64-byte blob format is
stable since Windows 95 and is documented in the COMMDLG header.

**Alternatives considered**:
- *Store palette as 16 separate DWORD registry values* — rejected: more code,
  more registry I/O, no benefit.
- *Use the modern IFileDialog-style chooser* — there is no such API for
  colour; `ChooseColorW` remains the platform standard.

---

## R6 — Scanline shader port specifics

**Decision**: Port `..\Casso\Casso\Shaders\CRT\scanlines.hlsl` to
`MatrixRainCore/Shaders/scanlines.hlsl` (or embed as a `R"(...)"` raw string
literal inside `RenderSystem.cpp` next to the bloom shaders, matching project
convention). Modifications relative to upstream:

1. **Cbuffer collapsed**: Replace the wide Casso `CrtCb` (10 floats) with a
   minimal 16-byte struct:
   ```hlsl
   cbuffer ScanlineCb : register(b0)
   {
       float g_intensity;       // 0..1, from Scanlines Intensity slider / 100
       float g_linesPerHeight;  // CPU-computed: 1000 * pow(0.15, style/100)
       float g_padding0;        // 16-byte alignment
       float g_padding1;
   };
   ```
   CPU-side mirror is a 16-byte POD with `static_assert(sizeof(ScanlineCb) ==
   16)`. D3D11 constant buffer `ByteWidth` is rounded up to 16 anyway; the
   explicit padding keeps the C++ struct and HLSL declaration bit-identical
   so a future field add is obvious.

2. **`kNativeScanlines` removed**: The `static const float kNativeScanlines =
   192.0` line is deleted. The shader multiplies `i.uv.y * g_linesPerHeight`
   instead.

3. **Luminance gating removed** (FR-024a): Delete
   `float lum = max(c.r, max(c.g, c.b));` and
   `float weight = saturate(lum * 4.0);`. The `darken` calc becomes:
   ```hlsl
   float darken = lerp(1.0 - g_intensity, 1.0, bright);
   ```
   (i.e., the outer `lerp(1.0, ..., weight)` collapses to its second argument
   because `weight` is now implicitly 1.0 everywhere.)

4. **Attribution preserved**: Keep the 4-line SPDX + URL + upstream-SHA
   header verbatim. Add a one-line "MatrixRain modifications" note below the
   existing "Casso modifications" comment.

**Placement in the draw flow**: After the bloom composite pass writes into
the back-of-pipeline render target (currently the swapchain back buffer in
the bloom-disabled path, or the post-bloom composite target in the
bloom-enabled path), insert one extra ping-pong texture (`m_postBloomTarget`,
same DXGI_FORMAT and dimensions as the swapchain back buffer). The bloom
composite renders *into* `m_postBloomTarget` instead of directly into the
back buffer; the scanline pass samples `m_postBloomTarget` as `t0` and
renders into the back buffer. When scanlines are disabled (FR-028b),
`m_postBloomTarget` is bypassed: bloom composite renders directly into the
back buffer as today. When *both* glow and scanlines are disabled, the
existing direct-to-backbuffer character draw path is used unchanged. Only
*one* extra texture is needed (no ping-pong; one read, one write).

**Rationale**: Minimal shader surface (two floats) means the smallest
possible constant buffer, no wasted CPU push. Single extra render target
means one `CreateTexture2D` + one `CreateRenderTargetView` + one
`CreateShaderResourceView` per `RenderSystem`, all created in the existing
`CreateBloomResources`-style helper. Bypass paths keep the no-effect cost at
literally zero extra draw calls.

**Alternatives considered**:
- *Sample directly from the swapchain back buffer back into itself* —
  rejected: D3D11 disallows reading from the bound RTV.
- *Compute shader* — rejected: pixel shader is shorter, simpler, and the cost
  is already trivial.

---

## R7 — Cross-tab disabled-state propagation for Glow Enabled toggle

**Decision**: The Performance-tab `BS_AUTOCHECKBOX` handler for Glow Enabled
calls a single helper `ApplyGlowEnabledUI(hSheet, bool enabled)` that:

1. Walks the Performance page's HWND (the `HWND` returned by
   `PropSheet_IndexToHwnd(hSheet, 1)`) and `EnableWindow(FALSE)` on:
   Quality Preset slider, Glow Passes slider, Glow Resolution slider, Glow
   Smoothness slider, plus each one's prompt label, value label, and info
   button (per FR-017).
2. Walks the Visuals page's HWND (`PropSheet_IndexToHwnd(hSheet, 0)`) and
   `EnableWindow(FALSE)` on: Glow Intensity slider + label trio, Glow Size
   slider + label trio (per FR-016).
3. Updates the controller via `pController->UpdateGlowEnabled(enabled)`
   which pushes through `SharedState` so `RenderSystem` bypasses bloom on
   the next frame (FR-015).

Critically: `PropSheet_IndexToHwnd` returns a valid HWND for any page that
has been *created* — and property sheets *lazily* create pages only when the
user first navigates to them. To make cross-tab propagation work from
"dialog open", we set `PSP_PREMATURE` on each `PROPSHEETPAGE::dwFlags` so
both pages are created up-front at sheet-creation time. That gives us valid
HWNDs for both pages immediately and lets the initial sync (apply the
saved-GlowEnabled state to all glow controls on both tabs) run during the
sheet's `PSN_SETACTIVE` for the first-visible page.

**Rationale**: `PSP_PREMATURE` is the documented escape hatch for exactly
this scenario (controls on inactive pages need to be enabled/disabled
before the user visits them). Cost is negligible (two extra dialog
templates instantiated at open time instead of on-demand).

**Alternatives considered**:
- *Defer propagation to `PSN_SETACTIVE` on each page* — rejected: causes
  visible "flash" of mis-enabled state when the user first tabs over.
- *Single big "render state" struct passed to a page-init helper* — same end
  result; the helper approach above is just a thinner shim over
  `EnableWindow`.

---

## R8 — Fade-timer feature removal: file/symbol inventory

**Decision**: Pre-removal grep confirms the following touch list (production
+ tests). Removing is purely deletions; nothing is renamed or repurposed.

**Production symbols (delete)**:
- `MatrixRainCore/ApplicationState.{h,cpp}` — `ToggleDebugFadeTimes`,
  `SetShowDebugFadeTimes`, `GetShowDebugFadeTimes`, the corresponding
  change-callback registration, and the backing bool field.
- `MatrixRainCore/ScreenSaverSettings.h` — `m_showFadeTimers` field +
  accessor(s).
- `MatrixRainCore/RegistrySettingsProvider.{h,cpp}` —
  `VALUE_SHOW_FADE_TIMERS` constant + load/save code paths. **Add**: a
  silent-ignore read code path documented per FR-037 (no read at all is
  sufficient — registry values left over from prior installs are simply not
  touched, satisfying "silently ignored" without code).
- `MatrixRainCore/RenderSystem.{h,cpp}` — `RenderDebugFadeTimes` method +
  its single call site in the render loop.
- `MatrixRainCore/SharedState.h` — `showDebugFadeTimes` live field + the
  matching snapshot field.
- `MatrixRainCore/RenderParams.h` — `showDebugFadeTimes`.
- `MatrixRainCore/ConfigDialogController.{h,cpp}` —
  `UpdateShowFadeTimers`.
- `MatrixRain/MatrixRain.rc` — "Show fade timers" checkbox control entry
  (`IDC_SHOWFADETIMERS_CHECK`).
- `MatrixRain/resource.h` — `IDC_SHOWFADETIMERS_CHECK` definition.
- `MatrixRain/ConfigDialog.cpp` — `OnShowFadeTimersCheck`, all
  `CheckDlgButton (..., IDC_SHOWFADETIMERS_CHECK, ...)` calls in defaults
  apply / settings load / WM_COMMAND dispatch.

**Test files (delete cases, keep files)**:
- `MatrixRainTests/ConfigDialogControllerTests.cpp` — every test whose name
  contains `FadeTimer` or that exercises `UpdateShowFadeTimers`.
- `MatrixRainTests/ScreenSaverSettingsTests.cpp` — every test that asserts
  on `m_showFadeTimers`.
- `MatrixRainTests/RegistrySettingsProviderTests.cpp` — every test reading
  or writing `ShowFadeTimers`. **Add**: one new test that asserts a
  pre-existing legacy `ShowFadeTimers` REG_DWORD in the test registry hive
  does not cause `Load()` to fail and does not appear in the loaded
  settings (FR-037).

**Order**: Fade-timer removal MUST be the first implementation phase after
the foundational scaffolding (manifest verification, new resource IDs,
shader directory). Reason: the property-sheet tab restructure (US1)
touches `ConfigDialog.cpp` and `MatrixRain.rc` extensively; doing the
removal first means the restructure works on a clean dialog with no
dead control to keep accidentally re-adding.

**Alternatives considered**:
- *Leave the symbols in place and just hide the checkbox* — rejected per
  spec FR-036 (explicit excision; spec is unambiguous).
- *Combine fade-timer removal with the property-sheet rewrite in one
  commit* — rejected per constitution IX (one task = one commit; mixing
  loses bisectability).

---

## R9 — `Performance (%u fps, %u%% GPU)` format string mechanics

**Decision**: Add one string-table entry to `MatrixRain.rc`:

```
STRINGTABLE
BEGIN
    IDS_PERFTAB_TITLE_FORMAT  L"Performance (%s fps, %s%% GPU)"
END
```

Note the `%s` (string) substitutions rather than `%u` (unsigned int)
substitutions — this is the trick that lets us interpolate either a digit
string (`L"60"`) or the literal `L"--"` placeholder without branching on
two different format strings. The dialog-thread timer handler builds two
small wide-string buffers (one for fps, one for gpu%) — each is either
`swprintf_s(buf, L"%u", value)` when the reading is valid, or
`StringCchCopyW(buf, ARRAYSIZE(buf), L"--")` when not. Then a single
`swprintf_s(titleBuf, fmtString, fpsBuf, gpuBuf)` produces the final tab
title, which is fed to `TabCtrl_SetItem` per R2.

`LoadStringW(g_hInstance, IDS_PERFTAB_TITLE_FORMAT, ...)` is called once at
dialog-open time and cached in a wide-string member.

**Rationale**: Per-tick branching ("if fps available use fmt A else use fmt
B") squared into "if either is unavailable use fmt C/D/E" produces five
different format strings. Using two `%s` slots collapses that to one
format string and two trivial value-or-placeholder snippets.

The literal `%%` in the format string survives Resource Compiler
processing and `LoadStringW` unchanged; the second `%%` in
`%s%% GPU` produces a single literal `%` in the output, giving the
required visible text `45% GPU` etc.

**Alternatives considered**:
- *Two format strings (one with %u, one with literal `--`)* — rejected: more
  code, more strings to localise.
- *Format the entire title CPU-side without a .rc entry* — rejected: spec
  FR-009 mandates the format string lives in the .rc string table for
  future localisation.

---

## Post-research summary

All eight implementation unknowns and the format-string mechanics are
resolved. No outstanding NEEDS CLARIFICATION items remain. Phase 1 can
proceed to data-model + contracts + quickstart with concrete numbers and
flag matrices.
