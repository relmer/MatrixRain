#include "pch.h"

#include "ScreenSaverInstaller.h"





////////////////////////////////////////////////////////////////////////////////
//
//  WindowsRegistryProvider::ReadString
//
////////////////////////////////////////////////////////////////////////////////

HRESULT WindowsRegistryProvider::ReadString (HKEY    hKey,
                                             LPCWSTR pszSubKey,
                                             LPCWSTR pszValueName,
                                             std::wstring & value)
{
    HRESULT hr     = S_OK;
    HKEY    hkOpen = nullptr;
    WCHAR   szBuffer[MAX_PATH];
    DWORD   cbBuffer = sizeof (szBuffer);
    DWORD   dwType   = 0;
    LONG    lResult  = 0;


    lResult = RegOpenKeyExW (hKey, pszSubKey, 0, KEY_READ, &hkOpen);
    CBRA (lResult == ERROR_SUCCESS);

    lResult = RegQueryValueExW (hkOpen, pszValueName, nullptr, &dwType,
                                reinterpret_cast<LPBYTE> (szBuffer), &cbBuffer);
    if (lResult != ERROR_SUCCESS || dwType != REG_SZ)
    {
        hr = HRESULT_FROM_WIN32 (lResult);
        goto Error;
    }

    value = szBuffer;

Error:
    if (hkOpen)
        RegCloseKey (hkOpen);

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  WindowsRegistryProvider::WriteString
//
////////////////////////////////////////////////////////////////////////////////

HRESULT WindowsRegistryProvider::WriteString (HKEY    hKey,
                                              LPCWSTR pszSubKey,
                                              LPCWSTR pszValueName,
                                              LPCWSTR pszValue)
{
    HRESULT hr     = S_OK;
    HKEY    hkOpen = nullptr;
    LONG    lResult;


    lResult = RegOpenKeyExW (hKey, pszSubKey, 0, KEY_SET_VALUE, &hkOpen);
    CBRA (lResult == ERROR_SUCCESS);

    lResult = RegSetValueExW (hkOpen, pszValueName, 0, REG_SZ,
                              reinterpret_cast<const BYTE *> (pszValue),
                              static_cast<DWORD> ((wcslen (pszValue) + 1) * sizeof (WCHAR)));
    CBRA (lResult == ERROR_SUCCESS);

Error:
    if (hkOpen)
        RegCloseKey (hkOpen);

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  WindowsRegistryProvider::DeleteValue
//
////////////////////////////////////////////////////////////////////////////////

HRESULT WindowsRegistryProvider::DeleteValue (HKEY    hKey,
                                              LPCWSTR pszSubKey,
                                              LPCWSTR pszValueName)
{
    HRESULT hr     = S_OK;
    HKEY    hkOpen = nullptr;
    LONG    lResult;


    lResult = RegOpenKeyExW (hKey, pszSubKey, 0, KEY_SET_VALUE, &hkOpen);
    CBRA (lResult == ERROR_SUCCESS);

    lResult = RegDeleteValueW (hkOpen, pszValueName);
    CBRA (lResult == ERROR_SUCCESS);

Error:
    if (hkOpen)
        RegCloseKey (hkOpen);

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScreenSaverInstaller::Install
//
//  Copies the running executable to System32 as MatrixRain.scr, then invokes
//  desk.cpl InstallScreenSaver to set it as active and open Screen Saver Settings.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT ScreenSaverInstaller::Install()
{
    HRESULT hr = S_OK;
    WCHAR   szSourcePath[MAX_PATH];
    WCHAR   szTargetPath[MAX_PATH];
    WCHAR   szSystem32[MAX_PATH];
    WCHAR   szRunDll32Cmd[MAX_PATH * 2];
    DWORD   cchPath = 0;


    // FR-006: Verify elevation (not an assertion — expected to fail for non-admin callers)
    if (!IsElevated())
    {
        hr = E_ACCESSDENIED;
        goto Error;
    }

    // Get the running executable's path (source)
    cchPath = GetModuleFileNameW (nullptr, szSourcePath, _countof (szSourcePath));
    CBRA (cchPath > 0 && cchPath < _countof (szSourcePath));

    // Build target path: %SystemRoot%\System32\MatrixRain.scr
    cchPath = GetSystemDirectoryW (szSystem32, _countof (szSystem32));
    CBRA (cchPath > 0);

    StringCchPrintfW (szTargetPath, _countof (szTargetPath), L"%ls\\MatrixRain.scr", szSystem32);

    // FR-003 / edge case: Skip copy if source == target (running from installed .scr)
    if (_wcsicmp (szSourcePath, szTargetPath) != 0)
    {
        // FR-008: CopyFileW with bFailIfExists=FALSE to overwrite existing
        CWRA (CopyFileW (szSourcePath, szTargetPath, FALSE));
    }

    // FR-004: Invoke desk.cpl InstallScreenSaver to set as active + open Settings
    StringCchPrintfW (szRunDll32Cmd, _countof (szRunDll32Cmd),
                      L"desk.cpl,InstallScreenSaver %ls", szTargetPath);

    {
        SHELLEXECUTEINFOW sei = { sizeof (sei) };

        sei.lpVerb       = L"open";
        sei.lpFile       = L"rundll32.exe";
        sei.lpParameters = szRunDll32Cmd;
        sei.nShow        = SW_SHOWNORMAL;

        CWRA (ShellExecuteExW (&sei));
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScreenSaverInstaller::Uninstall
//
////////////////////////////////////////////////////////////////////////////////

HRESULT ScreenSaverInstaller::Uninstall (IRegistryProvider & registry)
{
    HRESULT hr = S_OK;
    WCHAR   szTargetPath[MAX_PATH];
    WCHAR   szSystem32[MAX_PATH];
    DWORD   cchPath = 0;


    // FR-006: Verify elevation (not an assertion — expected to fail for non-admin callers)
    if (!IsElevated())
    {
        hr = E_ACCESSDENIED;
        goto Error;
    }

    // Build target path: %SystemRoot%\System32\MatrixRain.scr
    cchPath = GetSystemDirectoryW (szSystem32, _countof (szSystem32));
    CBRA (cchPath > 0);

    StringCchPrintfW (szTargetPath, _countof (szTargetPath), L"%ls\\MatrixRain.scr", szSystem32);

    // FR-009: Gracefully handle missing .scr file
    if (GetFileAttributesW (szTargetPath) == INVALID_FILE_ATTRIBUTES)
    {
        hr = S_FALSE;
        goto Error;
    }

    // FR-005: Remove the .scr file
    CWRA (DeleteFileW (szTargetPath));

    // FR-013/014/015: Clean up registry if MatrixRain was the active screensaver
    CleanupRegistryForUninstall (registry);

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScreenSaverInstaller::IsElevated
//
////////////////////////////////////////////////////////////////////////////////

bool ScreenSaverInstaller::IsElevated()
{
    HANDLE hToken   = nullptr;
    BOOL   fSuccess = FALSE;


    fSuccess = OpenProcessToken (GetCurrentProcess(), TOKEN_QUERY, &hToken);
    if (fSuccess)
    {
        TOKEN_ELEVATION elevation = {};
        DWORD           cbSize    = sizeof (elevation);

        fSuccess = GetTokenInformation (hToken, TokenElevation, &elevation, sizeof (elevation), &cbSize);
        CloseHandle (hToken);

        if (fSuccess)
        {
            return elevation.TokenIsElevated != 0;
        }
    }

    return false;
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScreenSaverInstaller::RequestElevation
//
////////////////////////////////////////////////////////////////////////////////

HRESULT ScreenSaverInstaller::RequestElevation (LPCWSTR pszSwitch)
{
    HRESULT            hr = S_OK;
    WCHAR              szExePath[MAX_PATH];
    DWORD              cchPath = 0;
    SHELLEXECUTEINFOW  sei     = { sizeof (sei) };


    cchPath = GetModuleFileNameW (nullptr, szExePath, _countof (szExePath));
    CBRA (cchPath > 0 && cchPath < _countof (szExePath));

    sei.lpVerb       = L"runas";
    sei.lpFile       = szExePath;
    sei.lpParameters = pszSwitch;
    sei.nShow        = SW_SHOWNORMAL;

    if (!ShellExecuteExW (&sei))
    {
        hr = HRESULT_FROM_WIN32 (GetLastError());
        goto Error;
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScreenSaverInstaller::CleanupRegistryForUninstall
//
//  Reads SCRNSAVE.EXE and clears it if it points to MatrixRain.scr.
//  Does nothing if a different screensaver is active or the key is missing.
//
////////////////////////////////////////////////////////////////////////////////

void ScreenSaverInstaller::CleanupRegistryForUninstall (IRegistryProvider & registry)
{
    static constexpr LPCWSTR kpszDesktopKey    = L"Control Panel\\Desktop";
    static constexpr LPCWSTR kpszScrnsaveExe   = L"SCRNSAVE.EXE";
    static constexpr LPCWSTR kpszScreenActive   = L"ScreenSaveActive";

    HRESULT      hr = S_OK;
    std::wstring currentScr;
    WCHAR        szExpectedPath[MAX_PATH];
    WCHAR        szSystem32[MAX_PATH];
    DWORD        cchPath = 0;


    // Build the expected path to compare against
    cchPath = GetSystemDirectoryW (szSystem32, _countof (szSystem32));
    if (cchPath == 0)
        return;

    StringCchPrintfW (szExpectedPath, _countof (szExpectedPath), L"%ls\\MatrixRain.scr", szSystem32);

    // Read the current SCRNSAVE.EXE value
    hr = registry.ReadString (HKEY_CURRENT_USER, kpszDesktopKey, kpszScrnsaveExe, currentScr);
    if (FAILED (hr))
        return;  // Key missing or unreadable — nothing to clean up

    // FR-015: Only clean up if MatrixRain is the active screensaver
    if (_wcsicmp (currentScr.c_str(), szExpectedPath) != 0)
        return;

    // FR-013: Delete SCRNSAVE.EXE value
    registry.DeleteValue (HKEY_CURRENT_USER, kpszDesktopKey, kpszScrnsaveExe);

    // FR-014: Set ScreenSaveActive to "0"
    registry.WriteString (HKEY_CURRENT_USER, kpszDesktopKey, kpszScreenActive, L"0");
}
