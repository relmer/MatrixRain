# Contract — Quality Preset Mapping

**Feature**: `006-multimon-gpu-efficiency`
**Header/impl**: `MatrixRainCore\QualityPresets.{h,cpp}` (new)

This contract pins the exact mapping between named quality presets and the underlying knob values, plus the rules for preset-snap, custom-drift detection, and first-run defaulting. All of these are pure functions and exhaustively unit-tested.

---

## The preset table

```cpp
enum class QualityPreset : int { Low = 0, Medium = 1, High = 2, Custom = 3 };
enum class ResolutionDivisor : int { Full = 1, Half = 2, Quarter = 4, Eighth = 8 };
enum class BlurTaps : int { Low = 5, Medium = 9, High = 13 };

struct AdvancedGraphicsValues
{
    int                m_glowIntensityPercent;   // 0..200; 0 = glow disabled
    int                m_blurPasses;             // 1..4
    ResolutionDivisor  m_bloomResolutionDivisor; // Full/Half/Quarter/Eighth
    BlurTaps           m_blurTaps;               // Low/Medium/High
};
```

| QualityPreset | GlowIntensity | Passes | Resolution | BlurTaps |
|---------------|---------------|--------|------------|----------|
| `Low`         | 75            | 1      | `Quarter`  | `Low`    |
| `Medium`      | 100           | 2      | `Half`     | `Medium` |
| `High`        | 100           | 3      | `Half`     | `High`   |
| `Custom`      | *(any)*       | *(any)*| *(any)*    | *(any)*  |

**`High` is calibrated to today's exact rendering** (3 passes, half resolution, 13-tap blur, glow intensity 100). Existing users who upgrade and never open the dialog see no visible change.

The `Eighth` resolution divisor is reachable only via `Custom`; no named preset uses it. (Available for the "I really need every GPU cycle back" scenario.)

`m_glowIntensityPercent == 0` disables the entire glow pipeline (extract + blur + composite all bypassed; direct-to-backbuffer fallback path is taken). This is enforced at the render layer; the value indicator label MUST read `"0% (glow disabled)"` per FR-031.

---

## Pure helpers

### `AdvancedGraphicsValues LookupPresetValues(QualityPreset preset)`

- For `Low`/`Medium`/`High`: returns the row from the table above.
- For `Custom`: precondition violated — undefined behavior (assert in debug, return `High` row in release). Callers MUST never pass `Custom`.

### `QualityPreset DetectActivePreset(const AdvancedGraphicsValues& current)`

- Returns the named preset whose row exactly matches `current`.
- Returns `Custom` if no named preset matches.
- Used after every advanced-control change to recompute the combo's displayed selection.

### `AdvancedGraphicsValues ApplyPresetSnap(QualityPreset preset, const AdvancedGraphicsValues& current, std::optional<AdvancedGraphicsValues> lastCustom)`

Snap rule when the user changes the preset combo:
- `preset ∈ {Low, Medium, High}` → return `LookupPresetValues(preset)`.
- `preset == Custom`:
  - if `lastCustom.has_value()` → return `*lastCustom`.
  - else → return `current` unchanged (the advanced controls keep showing the previous preset's values until the user touches one).

### `QualityPreset PickDefaultQualityPreset(const std::vector<AdapterInfo>& adapters, uint64_t totalMonitorPixels)`

First-run heuristic; runs only when no `QualityPreset` is saved.

```
if any adapter in `adapters` has m_dedicatedVramMb >= kDiscreteVramThresholdMb
                                  and !m_isSoftware:
    return High
elif totalMonitorPixels > kHeavyTotalPixelsThreshold:
    return Low
else:
    return Medium
```

Constants:
- `kDiscreteVramThresholdMb` = `256`.
- `kHeavyTotalPixelsThreshold` = `16'000'000` (≈ two 4K displays).

Both are `static constexpr` in `QualityPresets.cpp` and named in the test suite for explicit verification.

---

## Custom-drift behavior (state machine)

Inputs to the state machine:
- `activePreset : QualityPreset` (what the dropdown displays).
- `advanced : AdvancedGraphicsValues` (the four knob values).
- `lastCustom : optional<AdvancedGraphicsValues>` (persisted history).

Transitions:

| Event                                  | New `activePreset`                              | New `advanced`                                  | New `lastCustom`                       |
|----------------------------------------|-------------------------------------------------|-------------------------------------------------|----------------------------------------|
| User selects named preset P            | P                                               | `LookupPresetValues(P)`                         | unchanged                              |
| User selects `Custom`                  | `Custom`                                        | `ApplyPresetSnap(Custom, advanced, lastCustom)` | unchanged                              |
| User moves any advanced control        | `DetectActivePreset(newAdvanced)`               | `newAdvanced`                                   | `newAdvanced` (always — even if `activePreset` becomes a named one again by accident, e.g., user moved a control back to a preset's value) |
| Initial load (no saved preset)         | `PickDefaultQualityPreset(adapters, pixels)`    | `LookupPresetValues(default)`                   | `nullopt`                              |
| Initial load (`QualityPreset = "Custom"`, all `LastCustom_*` present) | `Custom`                                        | from `LastCustom_*` values                      | parsed from `LastCustom_*`             |
| Initial load (`QualityPreset = "Custom"`, any `LastCustom_*` missing)  | `PickDefaultQualityPreset(...)`                 | `LookupPresetValues(default)`                   | `nullopt`                              |

The "always update lastCustom on any advanced change" rule is intentional: even if the user happens to nudge a slider back to a named preset's exact value, the *fact that they touched it* makes their current state the canonical "last custom" set. This guarantees they can always recover whatever they were tinkering with by switching back to `Custom`.

---

## Persistence integration

The state machine above interacts with the registry per the rules in `registry-schema.md`. Specifically:
- `QualityPreset` (REG_SZ) is written on every preset change.
- `LastCustom_*` (DWORDs) are written whenever `lastCustom` is updated (i.e., on every advanced-control change).
- On startup, `LastCustomGraphicsValues` is loaded only when all four DWORDs are present; missing any one yields `nullopt`.
