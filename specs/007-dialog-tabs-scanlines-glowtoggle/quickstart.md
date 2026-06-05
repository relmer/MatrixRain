# Quickstart: v1.5 Settings Dialog Overhaul, Scanlines & Glow Toggle

This quickstart walks a contributor through validating the v1.5 feature
end-to-end after implementation, and describes the suggested phase
ordering for the `/speckit.tasks` and `/speckit.implement` runs that
follow this plan.

## Build & run

```powershell
# Build (Debug x64)
& "C:\Program Files\Microsoft Visual Studio\18\Enterprise\MSBuild\Current\Bin\MSBuild.exe" `
    MatrixRain.sln /p:Configuration=Debug /p:Platform=x64 /m

# Run unit tests
& "C:\Program Files\Microsoft Visual Studio\18\Enterprise\MSBuild\Current\Bin\MSBuild.exe" `
    MatrixRain.sln /p:Configuration=Debug /p:Platform=x64 /m /t:MatrixRainTests
vstest.console.exe x64\Debug\MatrixRainTests.dll /Settings:MatrixRainTests.runsettings

# Launch the screensaver in config mode
x64\Debug\MatrixRain.exe /c
```

## Manual acceptance walk-through (matches spec User Stories)

### US1 — Two-tab property sheet + live perf readout

1. Launch with `/c`. Confirm the dialog has two tabs labeled `Visuals`
   and `Performance` (the Performance tab's title will populate with
   live numbers within ~1 second).
2. Confirm there is no Apply button.
3. Move the Density slider on Visuals; the preview behind the dialog
   updates immediately (no Apply press needed).
4. Switch to Performance; within 1 s the tab label should read
   something like `Performance (60 fps, 12% GPU)`. Initial display
   before first publication shows `Performance (0 fps, 0% GPU)`.
5. Change Density again, then click Cancel. The preview reverts to the
   pre-dialog Density value.

### US2 — Glow Enabled toggle

1. Open dialog → Performance tab. Toggle Glow Enabled OFF.
2. Bloom disappears in the preview immediately.
3. Switch to Visuals. Glow Intensity and Glow Size sliders are
   visually disabled. Hover one — tooltip reads
   `Glow is disabled on the performance tab.`
4. Switch back to Performance. Glow Passes / Resolution / Smoothness /
   Quality Preset are all disabled. Hover one — tooltip reads
   `Glow is disabled.`
5. OK to dismiss. Re-launch the screensaver. Bloom remains off (the
   toggle persisted to `GlowEnabled=0`).

### US3 — CRT scanlines

1. On a fresh-install registry state (delete the four `Scanlines*`
   values from the registry root, then launch `/c`):
2. Visuals tab shows Scanlines Enabled (checked), Intensity (30),
   Style (50) in that order. The preview shows visible scanlines.
3. Slide Style from 50 → 1. Scanlines become barely visible (~981
   lines).
4. Slide Style 1 → 100. Scanlines become chunky Apple-II bands (~150
   lines).
5. Toggle Scanlines Enabled OFF. Preview is bit-identical to no
   scanline pass.

### US4 — Custom color picker

1. Color combo → Custom… → ChooseColorW opens pre-populated with
   `RGB(0,255,0)` on first use. Pick a non-default colour, OK.
2. Preview rain renders in the chosen colour. Combo shows `Custom…` (the dropdown label and the selected-item display use the same string; no string mutation on selection).
3. Color combo → Custom… again (re-click the same item). Chooser
   re-opens pre-populated with the previously chosen colour (this
   exercises the subclass-based same-item-commit detection path).
4. Edit a palette swatch in the chooser, click OK. Re-open the chooser
   later — the edited swatch is retained.
5. Click Cancel on the *outer* property sheet. Active colour reverts
   to whatever it was at dialog-open. Palette edits are retained.

### US5 — Fade-timer removal

```powershell
# From repo root:
git --no-pager grep -i -n "fadetimer\|fade.timer" -- MatrixRain MatrixRainCore MatrixRainTests
# Expected output: nothing (zero matches in production OR test code)
```

If the legacy `ShowFadeTimers` REG_DWORD exists from a v1.4 install,
launching v1.5 should produce no warnings or behaviour change.

### US6 — Upgrade visible change

1. Take a registry snapshot from a v1.4 install (no `Scanlines*`
   values present).
2. Launch v1.5 against that snapshot.
3. Scanlines render on the first frame (intentional v1.5 visual
   evolution). The v1.5 CHANGELOG entry calls out the one-click
   disable path (Visuals → Scanlines Enabled → OFF).

## Suggested phase ordering for `/speckit.tasks`

This ordering is what the plan recommends and what the user-input
constraints request:

1. **Foundational** — manifest verification (no change expected),
   resource.h new control IDs, `MatrixRainCore/Shaders/` directory,
   `ScanlineStyleMapping.{h,cpp}` skeleton + unit tests (red).
2. **US4 / FR-036, FR-037: fade-timer removal** — all symbol/file
   deletions per R8 inventory, paired test deletions, registry
   silent-ignore test added. Single logical commit per touched
   subsystem (ApplicationState removal, SharedState removal,
   RenderSystem removal, controller removal, .rc removal,
   RegistrySettingsProvider removal). Pre-requisite for US1 so the
   dialog rewrite operates on a clean slate.
3. **US1 — property-sheet structure + tab title timer** — split
   `MatrixRain.rc` into 2 page templates, replace
   `ConfigDialog.{cpp,h}` with PropertySheetW host + 2 page DLGPROCs,
   add `MonitorRenderContext::m_publishedFps` publisher + tests, wire
   1 Hz timer + `TabCtrl_SetItem` title update. Add controller
   accessors / snapshot fields for the 5 new rollback-eligible
   settings (even if controls do not exist yet — tests can drive them
   directly).
4. **US2 — Glow Enabled toggle** — checkbox + cross-tab `EnableWindow`
   propagation (R7) + `RenderSystem` bloom pipeline bypass +
   `GlowEnabled` registry round-trip + tooltips + tests.
5. **US3 — Scanlines** — shader port + `ScanlineCb` struct + per-frame
   upload + extra render target + 3 controls on Visuals tab + 3
   registry values + tests.
6. **US5 — Custom color picker** — `ColorScheme::Custom` enum variant
   + Color combo entry + subclass-based same-item-commit detection
   (CB_GETDROPPEDSTATE on WM_LBUTTONUP / WM_KEYUP) + ChooseColorW
   integration + palette REG_BINARY round-trip (unconditional persist)
   + `CustomColor` registry round-trip + tests.
7. **US6 — CHANGELOG / docs / migration note** — CHANGELOG entry
   calling out the intentional scanlines-on default + one-click
   disable path; README screenshot refresh; doc cross-link to
   `specs/007-.../plan.md` in `.github/copilot-instructions.md`
   (already updated by Phase 1 step 3 of this plan).
8. **Polish** — ARM64 build verification, final CHANGELOG line for
   the spec status flip to "Implemented", spec frontmatter
   `Status: Implemented` flip.

## Definition of done

- All 6 user stories pass their manual walkthrough above.
- `vstest.console.exe` reports 100% pass on the full test suite
  (the 410+ remaining tests after the fade-timer deletions; spec
  SC-008).
- `git grep -i fadetimer` returns zero matches across
  `MatrixRain*` directories.
- x64 Debug, x64 Release, ARM64 Debug, ARM64 Release all build with
  zero warnings (`/W4 /WX`).
- `git log --oneline 007-dialog-tabs-scanlines-glowtoggle ^main`
  shows one commit per logical task with conventional-commits
  format.
