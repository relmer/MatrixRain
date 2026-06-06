# Data Model — Multi-Monitor User Control and GPU Efficiency

**Feature**: `006-multimon-gpu-efficiency`

This feature is a desktop application with no database. "Data" here means:
- **Settings entities** persisted in the Windows registry.
- **In-memory configuration entities** that drive rendering and the dialog state.
- **Pure-function lookup tables** mapping quality presets to their concrete knob values.

No new file formats, no new IPC schemas. All persistence is via the existing `RegistrySettingsProvider`.

---

## Entity 1 — `MultiMonitorSetting`

**Purpose**: User-controlled flag for whether MatrixRain should span all monitors.

**Fields**:
| Field            | Type | Default | Constraints                          |
|------------------|------|---------|--------------------------------------|
| `m_enabled`      | bool | `true`  | — (no constraint beyond bool domain) |

**Source location**: `MatrixRainCore\ScreenSaverSettings.h` (new field `m_multiMonitorEnabled`).
**Persistence**: registry value `MultiMonitor` (DWORD; 0 = disabled, 1 = enabled).
**State transitions**: free toggle. Effect on running app: live rebuild within 1 second.

---

## Entity 2 — `GpuAdapterSetting`

**Purpose**: User's preferred rendering adapter.

**Fields**:
| Field              | Type            | Default | Constraints                                                |
|--------------------|-----------------|---------|------------------------------------------------------------|
| `m_description`    | `std::wstring`  | `L""`   | UTF-16; empty = "use system default"; up to 128 chars (the DXGI description length cap) |

**Source location**: `MatrixRainCore\ScreenSaverSettings.h` (new field `m_gpuAdapter`).
**Persistence**: registry value `GpuAdapter` (REG_SZ).
**Validation rules**: at startup, the description is looked up against the currently-enumerated adapter list via `ResolveAdapter`. No match = silently fall back to default (FR-011, FR-014).
**State transitions**: free choice from the enumerated list; persists immediately on dialog OK.

---

## Entity 3 — `AdapterInfo` *(in-memory, enumerated)*

**Purpose**: Describes one rendering-capable GPU adapter discovered at runtime.

**Fields**:
| Field              | Type           | Source                                          |
|--------------------|----------------|-------------------------------------------------|
| `m_description`    | `std::wstring` | `DXGI_ADAPTER_DESC1::Description`               |
| `m_luid`           | `LUID`         | `DXGI_ADAPTER_DESC1::AdapterLuid`               |
| `m_dedicatedVramMb`| `unsigned`     | `DXGI_ADAPTER_DESC1::DedicatedVideoMemory / (1024*1024)` |
| `m_isSoftware`     | bool           | `(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0`|
| `m_isDefault`      | bool           | True if this adapter is the one returned by `IDXGIFactory6::EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_UNSPECIFIED, …)` |

**Source location**: `MatrixRainCore\IAdapterProvider.h`.
**Construction**: `WindowsAdapterProvider::EnumerateAdapters()` filters out software adapters before returning.
**Relationship**: `ResolveAdapter(std::vector<AdapterInfo>, std::wstring savedDescription)` returns the matching `LUID` (or `std::nullopt` for "use default").

---

## Entity 4 — `QualityPreset` *(enum)*

**Values**: `Low`, `Medium`, `High`, `Custom`.
**Source**: `MatrixRainCore\QualityPresets.h` (new `enum class QualityPreset : int { Low = 0, Medium = 1, High = 2, Custom = 3 }`).
**Persistence**: registry value `QualityPreset` (REG_SZ, exact string `"Low"`, `"Medium"`, `"High"`, or `"Custom"`).
**Default**: chosen on first run by `PickDefaultQualityPreset` (see Entity 8).

**State transitions**:
- User selects a named preset → all `AdvancedGraphicsValues` snap (Entity 5).
- User changes any `AdvancedGraphicsValues` field → preset auto-flips to `Custom` and `LastCustomGraphicsValues` (Entity 6) is updated.
- User selects `Custom` from the dropdown:
  - If `LastCustomGraphicsValues` has been saved before → restore those.
  - Else → leave `AdvancedGraphicsValues` at its current values.

---

## Entity 5 — `AdvancedGraphicsValues`

**Purpose**: The four numeric knobs that determine glow rendering quality.

**Fields**:
| Field                  | Type                          | Range / Domain                                | Default                       |
|------------------------|-------------------------------|-----------------------------------------------|-------------------------------|
| `m_glowIntensityPercent` | int                         | 0..200 (existing; 0 = glow fully disabled)    | 100 (existing)                |
| `m_blurPasses`           | int                         | 1..4 (4 discrete tick positions)              | 3 (the "High" preset)         |
| `m_bloomResolutionDivisor` | `enum ResolutionDivisor`  | `Full=1`, `Half=2`, `Quarter=4`, `Eighth=8`   | `Half`                        |
| `m_blurTaps`             | `enum BlurTaps`             | `Low=5`, `Medium=9`, `High=13`                | `High`                        |

**Source location**: `MatrixRainCore\QualityPresets.h`.

**Validation rules**:
- `m_glowIntensityPercent` clamped to `[0, 200]` on load.
- `m_blurPasses` clamped to `[1, 4]` on load (legacy 0 values silently become 1).
- `m_bloomResolutionDivisor` invalid → default `Half`.
- `m_blurTaps` invalid → default `High`.

**Live behavior**: change → `RenderSystem` sees new values on next frame via the existing shared-state snapshot path.

---

## Entity 6 — `LastCustomGraphicsValues`

**Purpose**: The user's most recent custom set of advanced control values, used to restore Custom after they navigate away and back.

**Fields**: same shape as Entity 5 (`m_glowIntensityPercent`, `m_blurPasses`, `m_bloomResolutionDivisor`, `m_blurTaps`).

**Existence semantics**: `std::optional<LastCustomGraphicsValues>` — does not exist until the user has customized at least once. Once set, it persists across restarts.

**Persistence**: four registry DWORD values:
- `LastCustom_GlowIntensity`
- `LastCustom_Passes`
- `LastCustom_Resolution` (stored as the divisor integer: 1/2/4/8)
- `LastCustom_Smoothness` (stored as the tap count: 5/9/13)

If any of these values is absent at load, `LastCustomGraphicsValues` is treated as empty (`nullopt`) and all four are ignored — partial state is intentionally not honored.

**Update trigger**: written whenever the user moves any advanced control (regardless of whether the active preset is named or `Custom`), so the "switch to Custom restores last custom" UX works correctly even if the user navigated through named presets in between.

---

## Entity 7 — `ShowAdvancedGraphicsSetting`

**Purpose**: Whether the advanced graphics controls are revealed in the configuration dialog.

**Fields**:
| Field            | Type | Default | Constraints |
|------------------|------|---------|-------------|
| `m_show`         | bool | `false` | —           |

**Persistence**: registry value `ShowAdvancedGraphics` (DWORD).
**Initial-show heuristic**: if the user's saved `QualityPreset` is `Custom`, the default is `true` (so they immediately see the controls that define their custom set). Otherwise `false`.
**Live behavior**: dialog resizes within 100ms of toggle; persisted on dialog OK.

---

## Entity 8 — `PickDefaultQualityPreset` *(pure function, not persisted)*

**Purpose**: First-run heuristic that picks a `QualityPreset` when no preset is saved.

**Inputs**:
| Input                | Type                              | Source                                       |
|----------------------|-----------------------------------|----------------------------------------------|
| `adapters`           | `const std::vector<AdapterInfo>&` | `WindowsAdapterProvider::EnumerateAdapters`  |
| `totalMonitorPixels` | `uint64_t`                        | sum of each connected monitor's width*height |

**Output**: `QualityPreset`.

**Algorithm**:
```
if any adapter in `adapters` is discrete (dedicatedVramMb >= 256 and not software):
    return High
elif totalMonitorPixels > 16_000_000:        // ~ two 4K displays
    return Low
else:
    return Medium
```

**Heuristic constants**:
| Constant                          | Value          | Justification                                                    |
|-----------------------------------|----------------|------------------------------------------------------------------|
| `kDiscreteVramThresholdMb`        | 256            | Modern integrated adapters reserve ≪256 MB dedicated; discrete adapters >=256 MB even at the low end |
| `kHeavyTotalPixelsThreshold`      | 16,000,000     | Approximately two 4K displays (3840×2160 × 2 = 16,588,800)        |

Both constants are `static constexpr` in `QualityPresets.cpp` so tests can pin them via dependency-of-the-test rather than a build flag.

---

## Entity Relationships

```
ScreenSaverSettings  (persisted)
├── m_multiMonitorEnabled : bool
├── m_gpuAdapter : wstring
├── m_qualityPreset : QualityPreset
├── m_advancedValues : AdvancedGraphicsValues   ← driven by m_qualityPreset OR
├── m_lastCustom : optional<LastCustomGraphicsValues>     custom drift
└── m_showAdvancedGraphics : bool

QualityPreset (enum) ──maps via──> AdvancedGraphicsValues (via lookup table)

AdapterProvider ──enumerates──> [AdapterInfo]
ResolveAdapter(adapters, savedDescription) ──> optional<LUID>
                                                  │
RenderSystem.Initialize(hwnd, w, h, optional<LUID>)
                                                  │
                                                  ▼
                                  D3D11CreateDevice(adapter|nullptr, type)
```

**Lock domains**:
- Settings struct: protected by `SharedState::mutex` for live updates (existing pattern).
- Adapter enumeration: called only at startup and on rebuild from the UI thread; no lock needed.
- Render thread: reads settings via the existing `SharedState::GetSnapshot()` lock-snapshot pattern.

**Persistence touch points** (registry key `HKCU\Software\relmer\MatrixRain`):

| Value name                  | Type    | Owner entity                  |
|-----------------------------|---------|-------------------------------|
| `MultiMonitor`              | DWORD   | `MultiMonitorSetting`         |
| `GpuAdapter`                | REG_SZ  | `GpuAdapterSetting`           |
| `QualityPreset`             | REG_SZ  | `QualityPreset` enum          |
| `LastCustom_GlowIntensity`  | DWORD   | `LastCustomGraphicsValues`    |
| `LastCustom_Passes`         | DWORD   | `LastCustomGraphicsValues`    |
| `LastCustom_Resolution`     | DWORD   | `LastCustomGraphicsValues`    |
| `LastCustom_Smoothness`     | DWORD   | `LastCustomGraphicsValues`    |
| `ShowAdvancedGraphics`      | DWORD   | `ShowAdvancedGraphicsSetting` |
| *(existing values)*         | various | unchanged                     |
