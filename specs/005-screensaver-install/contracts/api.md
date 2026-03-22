# API Contracts: Screensaver Install/Uninstall

## ScreenSaverInstaller Public Interface

### ScreenSaverInstaller.h

```cpp
#pragma once

class ScreenSaverInstaller
{
public:

    //  Install MatrixRain as the system screensaver.
    //
    //  1. Copies the running executable to %SystemRoot%\System32\MatrixRain.scr
    //  2. Invokes desk.cpl InstallScreenSaver to set it as active and open Settings
    //
    //  Returns S_OK on success, or an appropriate HRESULT on failure.
    //  Returns E_ACCESSDENIED if the process is not elevated.
    //  Caller SHOULD check IsElevated() first to trigger UAC re-launch for
    //  optimal UX, but Install() also guards internally.
    //
    static HRESULT Install();


    //  Uninstall MatrixRain as the system screensaver.
    //
    //  1. Deletes %SystemRoot%\System32\MatrixRain.scr
    //  2. If MatrixRain was the active screensaver, clears registry entries
    //
    //  Returns S_OK on success, S_FALSE if not installed (informational).
    //  Returns E_ACCESSDENIED if the process is not elevated.
    //  Caller SHOULD check IsElevated() first to trigger UAC re-launch for
    //  optimal UX, but Uninstall() also guards internally.
    //
    //  Registry operations use an IRegistryProvider interface for testability.
    //  Production code passes the real Windows Registry implementation.
    //
    static HRESULT Uninstall();


    //  Check if the current process is running with administrator privileges.
    //
    static bool IsElevated();


    //  Re-launch the current process with elevated privileges (triggers UAC).
    //
    //  pszSwitch: The command-line switch to pass to the elevated process
    //             (e.g., L"/install" or L"/uninstall")
    //
    //  Returns S_OK if the elevated process was launched successfully.
    //  Returns E_ACCESSDENIED if the user declined the UAC prompt.
    //
    static HRESULT RequestElevation (LPCWSTR pszSwitch);
};
```

## ScreenSaverMode Enum Extension

```cpp
enum class ScreenSaverMode
{
    Normal,
    ScreenSaverFull,
    ScreenSaverPreview,
    SettingsDialog,
    PasswordChangeUnsupported,
    HelpRequested,
    Install,       // NEW
    Uninstall      // NEW
};
```

## ParseCommandLine Contract

The existing `ParseCommandLine` function signature is unchanged:

```cpp
HRESULT ParseCommandLine (LPCWSTR pszCommandLine, ScreenSaverModeContext & context);
```

**New behavior**: Recognizes `/install` and `/uninstall` (case-insensitive) as multi-character switches, setting `context.m_mode` to `ScreenSaverMode::Install` or `ScreenSaverMode::Uninstall` respectively.

## UsageText Extension

The `SwitchEntry` struct is extended with a `switchName` field for multi-character switches:

```cpp
struct SwitchEntry
{
    wchar_t      switchChar;   // Single-char switch (e.g., L'c'), or L'\0' for multi-char
    std::wstring switchName;   // Multi-char switch name (e.g., "install"), empty for single-char
    std::wstring argument;     // Optional argument (e.g., "<HWND>")
    std::wstring description;  // Help text description
};
```

When `switchChar == L'\0'`, the display logic uses `switchName` instead. New entries:

```cpp
m_switches =
{
    { L'c',  L"",          L"",  L"Show settings dialog"                        },
    { L'?',  L"",          L"",  L"Display this help message"                   },
    { L'\0', L"install",   L"",  L"Install MatrixRain as system screensaver"    },
    { L'\0', L"uninstall", L"",  L"Uninstall MatrixRain screensaver"            },
};
```

## main.cpp Dispatch Contract

Install and Uninstall modes are handled as early exits in `main.cpp`, before `Application::Initialize`:

```cpp
// Handle install/uninstall before application initialization
if (context.m_mode == ScreenSaverMode::Install || context.m_mode == ScreenSaverMode::Uninstall)
{
    if (!ScreenSaverInstaller::IsElevated())
    {
        LPCWSTR pszSwitch = (context.m_mode == ScreenSaverMode::Install)
            ? L"/install" : L"/uninstall";
        hr = ScreenSaverInstaller::RequestElevation (pszSwitch);
        goto Error;
    }

    hr = (context.m_mode == ScreenSaverMode::Install)
        ? ScreenSaverInstaller::Install()
        : ScreenSaverInstaller::Uninstall();
    goto Error;
}
```

## Error Handling

All public methods return `HRESULT`:

| Return Value | Meaning |
|-------------|---------|
| `S_OK` | Operation succeeded |
| `S_FALSE` | Informational (e.g., uninstall when not installed) |
| `E_ACCESSDENIED` | Not elevated / UAC declined |
| `HRESULT_FROM_WIN32(GetLastError())` | File operation failed (copy, delete) |
| `E_FAIL` | Registry operation failed |
