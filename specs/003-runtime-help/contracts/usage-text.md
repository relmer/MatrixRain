# API Contracts: UsageText

**Module**: `UsageText` (MatrixRainCore.lib)
**Date**: 2026-03-04

## Public Interface

### Construction

```
UsageText (switchPrefix: wchar_t)
```
- `switchPrefix` must be `/` or `-` (FR-001, FR-002)
- Populates the switch table with all supported command-line switches
- Pre-formats text with section grouping: "Options:" (`/c`, `/?`) and "Screensaver Options:" (`/s`, `/p`, `/a`)
- Does NOT include hotkeys (hotkeys are for the `?` key in-app overlay, not the `/?` dialog)
- Uses em dash from `UnicodeSymbols.h` for switch/description separator

### Queries

```
GetFormattedText () → const wstring&
```
- Returns the complete formatted text with section headers, switch names, and descriptions
- All switch references use the configured `switchPrefix`
- Suitable for passing directly to `IDWriteTextLayout` for proportional font rendering
- Includes header (app name + version), blank separator, "Options:" section, "Screensaver Options:" section

```
GetSwitchPrefix () → wchar_t
```
- Returns the prefix character used for switch display

```
GetPlainText () → wstring
```
- Returns the formatted text as plain text (identical to `GetFormattedText()` content)
- Plain text representation of the usage content

### Static Helpers

```
static DetectSwitchPrefix (commandLine: wstring_view) → wchar_t
```
- Scans the command line for `/?` or `-?`
- Returns the prefix character that was used
- Returns `/` as default if neither found (shouldn't happen if called correctly)

## Content Layout

The formatted text has this structure (proportional font, aligned via explicit x-offsets by the caller):

```
MatrixRain v1.1.x

Options:
  /c          — Show settings dialog
  /?          — Show this help

Screensaver Options:
  /s          — Run as screensaver
  /p <HWND>   — Preview in window
  /a          — Change screensaver settings
```

- Section headers are on their own line
- Switch entries are indented with switch name and description separated by em dash
- No hotkeys are included

## Thread Safety

- Immutable after construction — safe to read from any thread without synchronization
