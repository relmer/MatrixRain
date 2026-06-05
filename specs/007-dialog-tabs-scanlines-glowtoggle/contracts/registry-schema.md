# Contract: Registry Schema — v1.5 Additions and Legacy Handling

Registry root key (unchanged from v1.4): the existing screensaver-settings
key managed by `RegistrySettingsProvider`.

## v1.5 Additions

| Value Name | Type | Default | Read Behaviour | Write Behaviour |
|---|---|---|---|---|
| `GlowEnabled` | `REG_DWORD` | `1` | missing → `1`; any non-zero → `true`; `0` → `false` | written by controller commit (OK path) |
| `ScanlinesEnabled` | `REG_DWORD` | `1` | missing → `1`; non-zero → `true`; `0` → `false` | written by controller commit (OK path) |
| `ScanlinesIntensity` | `REG_DWORD` | `30` | missing → `30`; clamp `[1,100]` | written by controller commit (OK path) |
| `ScanlinesStyle` | `REG_DWORD` | `50` | missing → `50`; clamp `[1,100]` | written by controller commit (OK path) |
| `CustomColor` | `REG_DWORD` | absent | missing → no in-memory custom-color override (chooser default `RGB(0,255,0)` used); present → 24-bit RGB unpacked into `COLORREF` | written by controller commit (OK path) iff scheme is Custom AND a chooser-OK has occurred during this session |
| `CustomColorPalette` | `REG_BINARY` | absent → 64 zero bytes | size must be exactly 64; any other size → treat as absent (zero-filled) | written **unconditionally on chooser-OK** (FR-035); NOT subject to outer property-sheet Cancel rollback |

## Removed (v1.4 → v1.5)

| Value Name | Disposition |
|---|---|
| `ShowFadeTimers` | Read code path removed entirely. A pre-existing value left over from v1.4 is silently ignored on subsequent runs (no read, no write, no delete). Per FR-037 + Edge Cases. |

## Rollback Eligibility (cross-reference with `ConfigDialogSnapshot`)

Rollback-eligible (snapshot at dialog-open; restored on Cancel/X/Alt+F4):
- `GlowEnabled`, `ScanlinesEnabled`, `ScanlinesIntensity`, `ScanlinesStyle`,
  `CustomColor`.

**Explicitly NOT rollback-eligible (FR-004, FR-035):**
- `CustomColorPalette` — palette edits made inside the ChooseColorW dialog
  are persisted at chooser-OK time and survive an outer property-sheet
  Cancel. Test coverage:
  `ConfigDialogControllerTests::PaletteSurvivesOuterCancel`.

## Validation Tests (new in `RegistrySettingsProviderTests.cpp`)

- `GlowEnabled_MissingValueDefaultsToTrue`
- `ScanlinesEnabled_MissingValueDefaultsToTrue`
- `ScanlinesIntensity_MissingValueDefaultsTo30`
- `ScanlinesIntensity_ClampsBelowOneToOne`
- `ScanlinesIntensity_ClampsAboveHundredToHundred`
- `ScanlinesStyle_MissingValueDefaultsTo50`
- `CustomColor_AbsentMeansNoUserSelection`
- `CustomColor_RoundTrip`
- `CustomColorPalette_AbsentZeroFillsSixtyFourBytes`
- `CustomColorPalette_WrongSizeTreatedAsAbsent`
- `CustomColorPalette_SixtyFourByteRoundTrip`
- `LegacyShowFadeTimersIsSilentlyIgnored` (FR-037)
