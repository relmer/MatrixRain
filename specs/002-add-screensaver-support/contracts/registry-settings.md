# Contract: Registry Settings Schema

## Key

- Root: `HKEY_CURRENT_USER\Software\relmer\MatrixRain`

## Values

| Name | Type | Description | Default |
|------|------|-------------|---------|
| `Density` | `REG_DWORD` (0–100) | Density percentage (0 yields one streak head, 100 restores full density) | `100` |
| `ColorScheme` | `REG_SZ` | Stable key matching `ColorSchemeRepository` entries | Current application default |
| `AnimationSpeed` | `REG_DWORD` (1–100) | Animation speed percentage (historical default = 75%) | `75` |
| `GlowIntensity` | `REG_DWORD` (0–200) | Bloom intensity percentage (0 disables additive boost) | `100` |
| `GlowSize` | `REG_DWORD` (50–200) | Bloom blur radius percentage (controls halo width) | `100` |
| `StartFullscreen` | `REG_DWORD` (0/1) | Forces fullscreen on launch | `1` |
| `ShowDebugStats` | `REG_DWORD` (0/1) | Debug overlay toggle (ignored in `/s`/`/p`) | `0` |
| `ShowFadeTimers` | `REG_DWORD` (0/1) | Fade timer overlay toggle (ignored in `/s`/`/p`) | `0` |
| `LastSaved` | `REG_QWORD` | Unix timestamp (ms) when settings were last persisted | Omitted until first save |

## Access Pattern

- Read settings at startup; if the key or value is missing, use defaults and write them back when the user confirms changes.
- Write operations must call `RegCreateKeyExW` with `REG_OPTION_NON_VOLATILE` and use EHM macros for error propagation (`CHRA`, `CHR`).
- After saving, broadcast an internal `SettingsUpdated` event so running sessions can refresh without restarting, and keep a lightweight `RegNotifyChangeKeyValue` watcher active so any process changes to the key propagate into the running session.
