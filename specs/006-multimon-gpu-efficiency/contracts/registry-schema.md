# Contract — Registry Schema

**Feature**: `006-multimon-gpu-efficiency`
**Registry key**: `HKEY_CURRENT_USER\Software\relmer\MatrixRain` (existing).

This contract documents the new registry values introduced by this feature. Existing values are unchanged.

---

## New values

| Value name                  | Type    | Domain                          | Default      | Owner                                                                            |
|-----------------------------|---------|---------------------------------|--------------|----------------------------------------------------------------------------------|
| `MultiMonitor`              | DWORD   | `0` (disabled), `1` (enabled)   | `1`          | `RegistrySettingsProvider::ReadBool` / `WriteBool`                               |
| `GpuAdapter`                | REG_SZ  | UTF-16 string; `""` = default   | `""`         | `RegistrySettingsProvider::ReadString` / `WriteString`                           |
| `QualityPreset`             | REG_SZ  | `"Low"`, `"Medium"`, `"High"`, `"Custom"`, or `""` (= "not yet set; pick on next launch") | `""` | `RegistrySettingsProvider::ReadString` / `WriteString`                           |
| `LastCustom_GlowIntensity`  | DWORD   | `0..200`                        | absent       | `RegistrySettingsProvider::ReadInt` / `WriteInt`                                 |
| `LastCustom_Passes`         | DWORD   | `1..4`                          | absent       | `RegistrySettingsProvider::ReadInt` / `WriteInt`                                 |
| `LastCustom_Resolution`     | DWORD   | one of `1`, `2`, `4`, `8`       | absent       | `RegistrySettingsProvider::ReadInt` / `WriteInt`                                 |
| `LastCustom_Smoothness`     | DWORD   | one of `5`, `9`, `13`           | absent       | `RegistrySettingsProvider::ReadInt` / `WriteInt`                                 |
| `ShowAdvancedGraphics`      | DWORD   | `0`, `1`                        | `0` (or `1` if `QualityPreset == "Custom"`)              | `RegistrySettingsProvider::ReadBool` / `WriteBool`             |

## Read/write rules

- All `LastCustom_*` values are loaded together: if **any** is absent, `LastCustomGraphicsValues` is treated as empty (`nullopt`) and all four are ignored. Partial state is never honored.
- Reading clamps out-of-domain values to the nearest valid value (same convention as existing `m_glowIntensityPercent`).
- Writing happens via `Save()` on dialog OK and via the live-update controller path (`UpdateMultiMonitorEnabled`, `UpdateGpuAdapter`, `UpdateQualityPreset`, `UpdateAdvancedGraphicsValues`, `UpdateShowAdvancedGraphics`).
- No migration logic — absent values trigger the documented defaults at load time.

## Backward compatibility

Existing installations whose registry contains only the v1.3 values continue to work: `MultiMonitor` defaults to `1` (preserves today's behavior); `GpuAdapter` defaults to `""` (uses system default — same as today); `QualityPreset` defaults to `""` which on first non-empty save will be replaced by the first-run heuristic's choice; `LastCustom_*` absent → `LastCustomGraphicsValues = nullopt`; `ShowAdvancedGraphics` defaults to `0`.

The visible behavior for a user who upgrades and never opens the dialog is **identical** to v1.3, because the default preset (High) is calibrated to today's exact knob values.
