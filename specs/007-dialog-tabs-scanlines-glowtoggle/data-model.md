# Phase 1 Data Model: Settings Dialog Overhaul, Scanlines & Glow Toggle (v1.5)

Scope: only the *new or modified* entities introduced by this feature. All
v1.4 entities (Density, Speed, Glow*, Quality preset state, monitor layout,
adapter selection, frame limiter, etc.) are unchanged and inherited as-is
from `specs/006-multimon-gpu-efficiency/data-model.md`.

---

## 1. `ScreenSaverSettings` — added / removed fields

| Field | Type | Default | Range / Domain | Rollback-eligible? | Registry Value |
|---|---|---|---|---|---|
| `m_glowEnabled` | `bool` | `true` | `{false, true}` | Yes | `GlowEnabled` DWORD |
| `m_scanlinesEnabled` | `bool` | `true` | `{false, true}` | Yes | `ScanlinesEnabled` DWORD |
| `m_scanlinesIntensity` | `int` | `30` | `1..100` (percent) | Yes | `ScanlinesIntensity` DWORD |
| `m_scanlinesStyle` | `int` | `50` | `1..100` (unit-less) | Yes | `ScanlinesStyle` DWORD |
| `m_customColor` | `COLORREF` | `RGB(0,255,0)` (committed only on first chooser OK) | full 24-bit RGB | Yes | `CustomColor` DWORD |
| `m_customColorPalette` | `std::array<COLORREF, 16>` | all zero | full 24-bit RGB per slot | **No** (see FR-035) | `CustomColorPalette` REG_BINARY (64 bytes) |
| ~~`m_showFadeTimers`~~ | ~~`bool`~~ | — | — | — | ~~`ShowFadeTimers`~~ removed (FR-036) |

Validation rules:
- `m_scanlinesIntensity` and `m_scanlinesStyle` MUST be clamped to `[1,100]`
  on read and on write (defensive against tampered registry values).
- `m_customColor` is only written to the registry after a successful
  chooser-OK; absence in registry means "the user has never picked a custom
  colour" and the chooser pre-populates with the `RGB(0,255,0)` default
  (FR-030).
- `m_customColorPalette` size mismatch on read (≠ 64 bytes) MUST be
  treated as "no saved palette" and the field zero-initialised — no
  partial reads.

State transitions:
- `m_glowEnabled` toggling false → `RenderSystem` bypasses bloom pipeline
  entirely on the next frame (FR-015). Toggling back true → bloom resumes
  with the same `m_bloom*` parameter values that were already in
  `ScreenSaverSettings` (those parameters are NOT reset by the toggle;
  Edge Case "Glow toggle off with quality preset on Custom" confirms this).
- `m_scanlinesEnabled` toggling false → scanline post-pass bypassed
  (FR-028b). Toggling back true → pass re-enabled with the persisted
  Intensity/Style values.

---

## 2. `ColorScheme` enum — added variant

```cpp
enum class ColorScheme
{
    Green                   = 0,
    Blue                    = 1,
    Red                     = 2,
    Amber                   = 3,
    __StaticColorCount,
    ColorCycle              = __StaticColorCount,    // = 4
    Custom                  = 5,   // NEW (FR-033) — RGB from ScreenSaverSettings::m_customColor
};
```

- The numeric ordinals of every pre-existing variant (Green=0, Blue=1,
  Red=2, Amber=3, ColorCycle=4) MUST remain stable — existing v1.4
  registry hives store the scheme as its integer ordinal, and the
  `006` plan's `ColorSchemeRegistryRoundTrip` test asserts these
  positions. Per FR-039 / SC-007 the v1.4 → v1.5 upgrade MUST be
  byte-identical for every v1.4 setting.
- `Custom` is appended at ordinal `5` so it cannot collide with any
  prior value. Do NOT introduce a `White` variant — it doesn't exist
  on the v1.4 baseline.
- The `__StaticColorCount` sentinel is the existing v1.4 convention
  for distinguishing "static palette entries" from `ColorCycle`; leave
  it where it is. `Custom` sits past it because Custom isn't a static
  palette entry either — it sources its RGB from settings.

---

## 3. `ConfigDialogSnapshot` — added rollback fields

```cpp
struct ConfigDialogSnapshot
{
    // ... existing v1.4 fields (density, speed, glow*, color scheme,
    //     quality preset, multimon, GPU adapter, etc.) unchanged ...

    // v1.5 additions (FR-004, FR-044):
    bool      glowEnabled;
    bool      scanlinesEnabled;
    int       scanlinesIntensity;
    int       scanlinesStyle;
    COLORREF  customColor;

    // INTENTIONALLY NOT INCLUDED (FR-035):
    //   m_customColorPalette  — persisted unconditionally on chooser-OK,
    //   not part of the rollback set.
};
```

Snapshot lifecycle (extended from v1.4 unchanged contract):
- `ConfigDialogController::EnterLiveMode()` copies all snapshot fields
  (including the 5 new ones) from `ScreenSaverSettings` at dialog-open.
- `ConfigDialogController::CancelLiveMode()` restores all snapshot fields
  back to `ScreenSaverSettings`, pushes the restored state through
  `SharedState`, and triggers a rebuild only if a rebuild-worthy field
  changed (the 5 new fields are *not* rebuild-worthy — none of them affect
  device/context creation, only render-time behaviour).

---

## 4. `SharedState` — added live fields + snapshot mirrors

`SharedState` carries the dialog → render-thread per-frame settings push.
For each of the 5 rollback-eligible v1.5 settings, add both a live field
and a snapshot mirror (the snapshot mirror is what the render thread reads
when the controller is in live-preview mode; the live field is what the
dialog thread writes from its control handlers).

```cpp
// In SharedState (additions only):

// Live (dialog thread writes, render thread reads):
std::atomic<bool>    liveGlowEnabled         {true};
std::atomic<bool>    liveScanlinesEnabled    {true};
std::atomic<int>     liveScanlinesIntensity  {30};
std::atomic<int>     liveScanlinesStyle      {50};
std::atomic<DWORD>   liveCustomColor         {0x0000FF00};   // RGB(0,255,0)

// Snapshot mirrors (filled by EnterLiveMode, restored by CancelLiveMode):
bool    snapshotGlowEnabled;
bool    snapshotScanlinesEnabled;
int     snapshotScanlinesIntensity;
int     snapshotScanlinesStyle;
DWORD   snapshotCustomColor;

// REMOVED (FR-036):
//   std::atomic<bool> showDebugFadeTimes;
//   bool              snapshotShowDebugFadeTimes;
```

Atomics are used for the live values because the render thread reads them
without locking once per frame as part of the existing per-frame settings
copy. Integers and DWORDs are 4-byte lock-free on every supported
architecture; the bool atomics are 1-byte lock-free on x64/ARM64.

---

## 5. `RenderParams` — added / removed fields

`RenderParams` is the per-frame snapshot that the render thread copies
out of `SharedState` at the top of each frame.

```cpp
struct RenderParams
{
    // ... existing v1.4 fields unchanged ...

    // v1.5 additions:
    bool      glowEnabled;
    bool      scanlinesEnabled;
    float     scanlinesIntensity;      // normalised [0..1] from settings 1..100
    float     scanlinesLineCount;      // CPU-computed 1000 * pow(0.15, style/100)
    COLORREF  customColor;             // used only when colorScheme == Custom

    // REMOVED (FR-036):
    //   bool showDebugFadeTimes;
};
```

The render thread does the percent-to-float and style-to-line-count
conversions once per frame from the integer settings (cheap), avoiding
having to mutate `SharedState` writers when the underlying numeric format
ever changes.

---

## 6. `MonitorRenderContext` — added field

```cpp
class MonitorRenderContext
{
    // ... existing v1.4 members ...

    std::atomic<float>  m_publishedFps     {0.0f};   // FR-010 (R3)
    std::atomic<bool>   m_hasPublishedFps  {false};  // sentinel — see R3
};
```

Write contract: render thread, once per frame after `FPSCounter::Tick()`,
issues:
```cpp
m_publishedFps   .store (currentFps, std::memory_order_relaxed);
m_hasPublishedFps.store (true,       std::memory_order_relaxed);
```

Read contract: dialog thread, in the 1 Hz `WM_TIMER` handler:
```cpp
bool   hasFps = ctx.m_hasPublishedFps.load (std::memory_order_relaxed);
float  fps    = ctx.m_publishedFps   .load (std::memory_order_relaxed);
```

`hasFps == false` → substitute `--` per FR-012.

---

## 7. Scanline shader CPU/GPU contract (`ScanlineCb`)

```cpp
// CPU side, mirrors HLSL cbuffer 1:1 (sizeof must equal 16):
struct alignas(16) ScanlineCb
{
    float intensity;        // [0..1]     from settings 1..100 / 100.0f
    float linesPerHeight;   // ~150..~981 from settings 1..100 via mapping fn
    float _padding0;        // 0.0f
    float _padding1;        // 0.0f
};
static_assert (sizeof (ScanlineCb) == 16, "ScanlineCb must be 16 bytes");
```

Style → line count mapping (FR-023), unit-tested in
`ScanlineStyleMappingTests.cpp`:

```cpp
inline float ScanlineLineCount (int style) noexcept
{
    // style is clamped to [1, 100] by the caller (settings load + dialog)
    return 1000.0f * std::pow (0.15f, static_cast<float> (style) / 100.0f);
}
```

Reference values (tolerance ±2 lines):

| Style | Lines |
|---:|---:|
| 1   | ~981 |
| 25  | ~622 |
| 50  | ~387 |
| 75  | ~241 |
| 100 | ~150 |

---

## 8. Registry schema additions (see also `contracts/registry-schema.md`)

| Value Name | Type | Default | Range | Rollback-eligible |
|---|---|---|---|---|
| `GlowEnabled` | REG_DWORD | 1 | 0 or 1 | Yes |
| `ScanlinesEnabled` | REG_DWORD | 1 | 0 or 1 | Yes |
| `ScanlinesIntensity` | REG_DWORD | 30 | 1..100 | Yes |
| `ScanlinesStyle` | REG_DWORD | 50 | 1..100 | Yes |
| `CustomColor` | REG_DWORD | absent (chooser default RGB(0,255,0)) | full 24-bit RGB packed | Yes |
| `CustomColorPalette` | REG_BINARY | absent → 64 zero bytes | 64 bytes (16 × COLORREF) exactly | **No** |

Legacy `ShowFadeTimers` REG_DWORD: silently ignored on read (FR-037).
Removal is achieved by deleting all read code paths; no active cleanup is
performed (per Edge Case "Old registry value `ShowFadeTimers` present").

---

## 9. Cross-tab control enable/disable propagation

The `Glow Enabled` checkbox owns this matrix:

| Control | Page | Disabled when GlowEnabled == false |
|---|---|---|
| Glow Intensity slider + label trio + info button | Visuals | yes |
| Glow Size slider + label trio + info button | Visuals | yes |
| Quality Preset slider + label trio + info button | Performance | yes |
| Glow Passes slider + label trio + info button | Performance | yes |
| Glow Resolution slider + label trio + info button | Performance | yes |
| Glow Smoothness slider + label trio + info button | Performance | yes |

The `Scanlines Enabled` checkbox owns:

| Control | Page | Disabled when ScanlinesEnabled == false |
|---|---|---|
| Scanlines Intensity slider + label trio + info button | Visuals | yes |
| Scanlines Style slider + label trio + info button | Visuals | yes |

Tooltips when disabled (FR-018 / FR-019):
- Visuals-tab glow controls: `Glow is disabled on the performance tab.`
- Performance-tab glow controls: `Glow is disabled.`
- (Spec does not require tooltips on disabled scanline controls; none added.)
