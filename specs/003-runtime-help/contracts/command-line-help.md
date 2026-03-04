# API Contracts: CommandLineHelp

**Module**: `CommandLineHelp` (MatrixRainCore.lib)
**Date**: 2026-03-04

## Public Interface

### Orchestration

```
static DisplayCommandLineHelp (switchPrefix: wchar_t) → HRESULT
```
- Top-level entry point for `/?`/`-?` handling
- Creates a `UsageText` with the detected prefix
- Creates a `HelpRainDialog` with the `UsageText` reference
- Shows the dialog (blocks until dismissed)
- Returns `S_OK` on success when the dialog is dismissed
- Returns error HRESULT if dialog creation or rendering fails
- After returning, the caller exits the process

## Flow

```
ParseCommandLine detects /?/-?
  → DetectSwitchPrefix() → switchPrefix
  → DisplayCommandLineHelp (switchPrefix)
    → UsageText (switchPrefix)
    → HelpRainDialog (usageText)
    → HelpRainDialog::Show()
    → Dialog dismissed
    → Return S_OK
  → Exit process
```

- No text grid or column activity map step — `HelpRainDialog` receives the `UsageText` directly and uses `IDWriteTextLayout` with proportional font for per-character positioning

## Thread Safety

- Runs on main thread before window creation
- No render thread exists during this flow
- No console attachment, no Ctrl+C handling needed
