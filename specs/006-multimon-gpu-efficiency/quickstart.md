# Quickstart — Multi-Monitor User Control and GPU Efficiency

**Feature**: `006-multimon-gpu-efficiency`
**Audience**: developer or reviewer validating the feature on a workstation.

This document is the minimal recipe to (1) build the feature, (2) run the automated tests, (3) launch the binary, and (4) walk through each of the five user stories to confirm acceptance.

---

## 1. Build

```powershell
Set-Location C:\Users\relmer\source\repos\relmer\MatrixRain
& 'C:\Program Files\Microsoft Visual Studio\18\Enterprise\MSBuild\Current\Bin\MSBuild.exe' `
  MatrixRain.sln /p:Configuration=Debug /p:Platform=x64
```

- Do NOT pass `/m`. The PCH triggers transient `C3859`/`C1076` memory failures under parallel build on this machine.
- Warnings-as-errors (`/W4 /WX /sdl`) are enforced; any C4189 (unused variable) or C4100 (unreferenced parameter) breaks the build.

## 2. Tests

```powershell
& 'C:\Program Files\Microsoft Visual Studio\18\Enterprise\Common7\IDE\Extensions\TestPlatform\vstest.console.exe' `
  x64\Debug\MatrixRainTests.dll /Platform:x64 /InIsolation | `
  Select-String 'Total tests|Passed:|Failed:'
```

Expected output: total count = **354 (master baseline) + new tests added by this feature**. The new test files live in `MatrixRainTests/unit/`:
- `AdapterSelectionTests.cpp`
- `FrameLimiterTests.cpp`
- `DeviceLostTests.cpp`
- `MultiMonitorGateTests.cpp`
- `QualityPresetsTests.cpp`
- `RebuildCoalescerTests.cpp`
- + extensions to `RegistrySettingsProviderTests.cpp` and `ConfigDialogControllerTests.cpp`

All tests must pass. No skipped tests.

## 3. Launch

```powershell
Start-Process .\x64\Debug\MatrixRain.exe
```

For screensaver-mode validation:

```powershell
Start-Process .\x64\Debug\MatrixRain.exe -ArgumentList '/s'
```

For configuration-dialog-only:

```powershell
Start-Process .\x64\Debug\MatrixRain.exe -ArgumentList '/c'
```

## 4. Acceptance walkthrough

### US1 — Runtime topology and device-loss response (P1)

1. Connect a second monitor. Launch MatrixRain. Confirm it renders on both monitors.
2. Open Task Manager (Performance → GPU) and note the application's GPU utilization.
3. Disconnect the second monitor (physically unplug or disable in Display Settings → "Disconnect this display").
4. Within 1 second: only the remaining monitor continues rendering; GPU utilization drops to within 10% of "starting MatrixRain fresh with one monitor" (US1 SC-001).
5. Reconnect the second monitor. Within 1 second: MatrixRain begins rendering to it without restart (US1 SC-002).
6. Optional: disable the active GPU's driver via Device Manager. MatrixRain recovers automatically; no error dialog appears.

### US2 — Multi-monitor optional toggle (P1)

1. Launch MatrixRain on a multi-monitor system.
2. Press Enter (or right-click → Configure) to open the configuration dialog.
3. Find the "Render on all monitors" checkbox; verify it is checked by default.
4. Uncheck it and click OK.
5. Within 1 second: secondary monitors return to their normal desktop content; primary continues showing MatrixRain.
6. Restart MatrixRain. Verify it starts on the primary only (setting persisted).
7. Open the dialog, re-check the checkbox, OK. Verify all monitors resume rendering within 1 second.

### US3 — GPU adapter selection (P2)

*Requires a hybrid laptop or system with multiple non-software GPUs.*

1. Open the configuration dialog. Locate the GPU dropdown.
2. Verify each adapter is listed by its real name. The system default has `" (default)"` appended (e.g., `"NVIDIA GeForce RTX 3050 Ti (default)"`).
3. Verify software adapters (Microsoft Basic Render Driver) are NOT listed.
4. Select a non-default adapter and click OK.
5. Within 1 second: in Task Manager, MatrixRain's GPU usage switches to the selected GPU.
6. Restart MatrixRain. Verify it starts on the selected GPU.
7. To test the "adapter vanished" path: edit `HKCU\Software\relmer\MatrixRain\GpuAdapter` to a fake name (e.g., `"Acme GPU 9000"`). Restart MatrixRain. Verify it starts successfully on the default adapter without any error dialog.

### US4 — Frame-rate cap on high-refresh monitors (P2)

*Best validated on a system with a >60Hz monitor.*

1. Enable debug statistics in MatrixRain (Configuration → Show debug statistics).
2. On a >60Hz monitor: launch MatrixRain. Read the on-screen FPS. Expected: approximately 60.
3. On a 60Hz monitor: launch MatrixRain. Read the on-screen FPS. Expected: approximately 60 (existing behavior).
4. Mixed-refresh multi-monitor system: confirm each monitor independently shows ~60 FPS.
5. Optional: compare GPU utilization on the high-refresh monitor before and after this feature. Expected: ≥50% reduction (US4 SC-004).

### US5 — Graphics quality presets and advanced controls (P3)

1. Open the configuration dialog. Locate the "Quality" dropdown.
2. Verify entries: `Low`, `Medium`, `High`, `Custom`.
3. Switch through `Low` → `Medium` → `High`. Verify visible changes to glow appearance.
4. Locate the "Show advanced graphics settings" checkbox. Check it; the dialog grows to reveal three sliders (Passes / Resolution / Smoothness) and four `ⓘ` indicators next to the quality controls.
5. Move any advanced slider; verify the Quality dropdown switches to `Custom` automatically.
6. Switch the Quality back to `High`; verify all advanced sliders snap to High's values.
7. Switch back to `Custom`; verify the previously-customized values are restored.
8. Drag the Glow Intensity slider to 0%; verify the value label reads `"0% (glow disabled)"` and the glow effect is fully gone (not just darker).
9. Hover the mouse over each `ⓘ`; verify a tooltip appears containing descriptive text ending in one of:
   - `"Significant GPU performance impact."`
   - `"Moderate GPU performance impact."`
   - `"Small GPU performance impact."`
10. Tab to each `ⓘ` (verify each is keyboard-focusable). Press Space. Verify the same tooltip appears for the focused indicator.
11. Uncheck "Show advanced graphics settings". Verify the dialog shrinks and the advanced controls are hidden; the OK/Cancel/Reset buttons remain accessible.
12. Restart MatrixRain. Verify all preset/advanced/`ShowAdvancedGraphics` choices persisted.

## 5. Common-pitfall checklist

When implementing or reviewing:
- All new system headers (`<dxgi1_6.h>`) go in `pch.h` only.
- `D3D11CreateDevice` MUST use `D3D_DRIVER_TYPE_UNKNOWN` whenever a non-null adapter is passed.
- Render thread MUST NOT call `DestroyWindow`, modify registry, or change the active adapter directly — it MUST POST to the UI thread via `WM_APP_REBUILD_CONTEXTS`.
- All new commits exclude `Version.h` (auto-bumped pre-build).
- All commit messages follow Conventional Commits and include the `Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>` trailer.
- One task = one commit (constitution principle IX).
