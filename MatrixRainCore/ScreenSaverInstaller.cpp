#include "pch.h"

#include "WindowsRegistryProvider.h"
#include "ScreenSaverInstaller.h"





////////////////////////////////////////////////////////////////////////////////
//
//  WindowsRegistryProvider::ReadString
//
////////////////////////////////////////////////////////////////////////////////

HRESULT WindowsRegistryProvider::ReadString (HKEY           hkeyParent,
                                             LPCWSTR        pszSubKey,
                                             LPCWSTR        pszValueName,
                                             std::wstring & value)
{
    HRESULT hr      = S_OK;
    HKEY    hkey    = nullptr;
    DWORD   cbData  = 0;
    DWORD   dwType  = 0;
    LONG    lResult = 0;



    lResult = RegOpenKeyExW (hkeyParent, pszSubKey, 0, KEY_READ, &hkey);
    CBRAEx (lResult == ERROR_SUCCESS, HRESULT_FROM_WIN32 (lResult));

    // Query the required buffer size
    lResult = RegQueryValueExW (hkey, pszValueName, nullptr, &dwType, nullptr, &cbData);
    CBRAEx (lResult == ERROR_SUCCESS, HRESULT_FROM_WIN32 (lResult));
    CBRAEx (dwType == REG_SZ && cbData > 0, HRESULT_FROM_WIN32 (ERROR_INVALID_DATA));

    // Allocate and read the value
    {
        std::wstring buffer (cbData / sizeof (WCHAR), L'\0');

        lResult = RegQueryValueExW (hkey, pszValueName, nullptr, nullptr,
                                    reinterpret_cast<LPBYTE> (buffer.data()), &cbData);
        CBRAEx (lResult == ERROR_SUCCESS, HRESULT_FROM_WIN32 (lResult));

        // Trim the null terminator included in cbData
        buffer.resize (cbData / sizeof (WCHAR) - 1);
        value = std::move (buffer);
    }


    
Error:
    if (hkey)
    {
        RegCloseKey (hkey);
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  WindowsRegistryProvider::WriteString
//
////////////////////////////////////////////////////////////////////////////////

HRESULT WindowsRegistryProvider::WriteString (HKEY    hkeyParent,
                                              LPCWSTR pszSubKey,
                                              LPCWSTR pszValueName,
                                              LPCWSTR pszValue)
{
    HRESULT hr      = S_OK;
    HKEY    hkey    = nullptr;
    LONG    lResult;



    lResult = RegOpenKeyExW (hkeyParent, pszSubKey, 0, KEY_SET_VALUE, &hkey);
    CBRAEx (lResult == ERROR_SUCCESS, HRESULT_FROM_WIN32 (lResult));

    lResult = RegSetValueExW (hkey, pszValueName, 0, REG_SZ,
                              reinterpret_cast<const BYTE *> (pszValue),
                              static_cast<DWORD> ((wcslen (pszValue) + 1) * sizeof (WCHAR)));
    CBRAEx (lResult == ERROR_SUCCESS, HRESULT_FROM_WIN32 (lResult));

Error:
    if (hkey)
    {
        RegCloseKey (hkey);
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  WindowsRegistryProvider::DeleteValue
//
////////////////////////////////////////////////////////////////////////////////

HRESULT WindowsRegistryProvider::DeleteValue (HKEY    hkeyParent,
                                              LPCWSTR pszSubKey,
                                              LPCWSTR pszValueName)
{
    HRESULT hr     = S_OK;
    HKEY    hkey    = nullptr;
    LONG    lResult;



    lResult = RegOpenKeyExW (hkeyParent, pszSubKey, 0, KEY_SET_VALUE, &hkey);
    CBRAEx (lResult == ERROR_SUCCESS, HRESULT_FROM_WIN32 (lResult));

    lResult = RegDeleteValueW (hkey, pszValueName);
    CBRAEx (lResult == ERROR_SUCCESS, HRESULT_FROM_WIN32 (lResult));

Error:
    if (hkey)
    {
        RegCloseKey (hkey);
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScreenSaverInstaller::CheckPolicy
//
//  Checks a single registry-based policy.  Sets fBlocked to true and populates
//  errorMessage if the policy value matches the blocking condition.  Returns
//  S_OK if the check completed (regardless of whether it blocked).
//
////////////////////////////////////////////////////////////////////////////////

HRESULT ScreenSaverInstaller::CheckPolicy (const ScreenSaverInstaller::PolicyCheck & check, bool & fBlocked, std::wstring & errorMessage)
{
    HRESULT hr      = S_OK;
    HKEY    hkey    = nullptr;
    LONG    lResult = 0;
    DWORD   dwValue = 0;
    DWORD   cbValue = sizeof (dwValue);
    DWORD   dwType  = 0;
    LPCWSTR pszRoot = (check.hkeyRoot == HKEY_LOCAL_MACHINE) ? L"HKLM" : L"HKCU";



    lResult = RegOpenKeyExW (check.hkeyRoot, check.pszSubKey, 0, KEY_READ, &hkey);
    BAIL_OUT_IF (lResult != ERROR_SUCCESS, S_OK);

    lResult = RegQueryValueExW (hkey, check.pszValueName, nullptr, &dwType,
                                reinterpret_cast<LPBYTE> (&dwValue), &cbValue);
    BAIL_OUT_IF (lResult != ERROR_SUCCESS || dwType != REG_DWORD, S_OK);

    fBlocked = check.fBlockIfNonZero ? (dwValue > 0) : (dwValue == check.dwBlockingValue);
    BAIL_OUT_IF (!fBlocked, S_OK);

    errorMessage = std::format (L"{}\n\n{}\\{}\\{} = {}",
                                check.pszMessage, 
                                pszRoot,
                                check.pszSubKey, 
                                check.pszValueName, 
                                dwValue);



Error:
    if (hkey)
    {
        RegCloseKey (hkey);
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScreenSaverInstaller::CheckScreenSaverPolicies
//
//  Checks for Group Policy or MDM/Intune settings that would prevent a
//  screensaver from functioning.  Returns S_OK always (unless registry read
//  fails).  Sets fBlocked to true and populates errorMessage if a blocking
//  policy is detected.  Call this BEFORE elevation so the user sees the
//  warning without a UAC prompt.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT ScreenSaverInstaller::CheckScreenSaverPolicies (bool & fBlocked, std::wstring & errorMessage)
{
    HRESULT hr = S_OK;



    fBlocked = false;

    for (const auto & check : s_krgPolicyChecks)
    {
        hr = CheckPolicy (check, fBlocked, errorMessage);
        CHR (hr);

        if (fBlocked)
            break;
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScreenSaverInstaller::Install
//
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
////////////////////////////////////////////////////////////////////////////////

HRESULT ScreenSaverInstaller::Install()
{
    HRESULT           hr                          = S_OK;
    WCHAR             szSourcePath[MAX_PATH];
    WCHAR             szTargetPath[MAX_PATH];
    WCHAR             szRunDll32Cmd[MAX_PATH * 2];
    DWORD             cchPath                     = 0;
    BOOL              fSuccess                    = FALSE;
    bool              fElevated                   = false;
    SHELLEXECUTEINFOW sei                         = { sizeof (sei) };



    // FR-006: Verify elevation
    fElevated = s_pfnIsElevated();
    CBREx (fElevated, E_ACCESSDENIED);

    // Get the running executable's path (source)
    cchPath = GetModuleFileNameW (nullptr, szSourcePath, _countof (szSourcePath));
    CWRA (cchPath > 0 && cchPath < _countof (szSourcePath));

    // Build target path: %SystemRoot%\System32\MatrixRain.scr
    cchPath = GetSystemDirectoryW (szTargetPath, _countof (szTargetPath));
    CWRA (cchPath);
    CBRAEx (cchPath < _countof (szTargetPath), HRESULT_FROM_WIN32 (ERROR_INSUFFICIENT_BUFFER));

    hr = PathCchAppend (szTargetPath, _countof (szTargetPath), kpszScrFilename);
    CHRA (hr);

    // FR-003 / edge case: Skip copy if source == target (running from installed .scr)
    if (_wcsicmp (szSourcePath, szTargetPath) != 0)
    {
        // FR-008: CopyFileW with bFailIfExists=FALSE to overwrite existing
        fSuccess = CopyFileW (szSourcePath, szTargetPath, FALSE);
        CWRA (fSuccess);
    }

    // FR-004: Invoke desk.cpl InstallScreenSaver to set as active + open Settings
    StringCchPrintfW (szRunDll32Cmd, _countof (szRunDll32Cmd),
                      L"desk.cpl,InstallScreenSaver %ls", szTargetPath);

    sei.lpVerb       = L"open";
    sei.lpFile       = L"rundll32.exe";
    sei.lpParameters = szRunDll32Cmd;
    sei.nShow        = SW_SHOWNORMAL;

    fSuccess = ShellExecuteExW (&sei);
    CWRA (fSuccess);

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScreenSaverInstaller::Uninstall
//
//  Uninstall MatrixRain as the system screensaver.
//
//  1. Deletes %SystemRoot%\System32\MatrixRain.scr
//  2. If MatrixRain was the active screensaver, clears registry entries
//
//  Returns S_OK on success, S_FALSE if not installed (informational).
//  Returns E_ACCESSDENIED if the process is not elevated.
//  Registry operations use the provided IRegistryProvider for testability.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT ScreenSaverInstaller::Uninstall (IRegistryProvider & registry, IFileSystemProvider & fileSystem)
{
    HRESULT hr                     = S_OK;
    WCHAR   szTargetPath[MAX_PATH];
    DWORD   cchPath                = 0;
    BOOL    fSuccess               = FALSE;
    bool    fElevated              = false;
    DWORD   dwAttrs                = 0;



    // FR-006: Verify elevation
    fElevated = s_pfnIsElevated();
    CBREx (fElevated, E_ACCESSDENIED);

    // Build target path: %SystemRoot%\System32\MatrixRain.scr
    cchPath = fileSystem.GetSystemDirectory (szTargetPath, _countof (szTargetPath));
    CWRA (cchPath);
    CBRAEx (cchPath < _countof (szTargetPath), HRESULT_FROM_WIN32 (ERROR_INSUFFICIENT_BUFFER));

    hr = PathCchAppend (szTargetPath, _countof (szTargetPath), kpszScrFilename);
    CHRA (hr);

    // FR-009: Gracefully handle missing .scr file (but surface access-denied)
    dwAttrs = fileSystem.GetFileAttributes (szTargetPath);
    if (dwAttrs == INVALID_FILE_ATTRIBUTES)
    {
        DWORD dwError = fileSystem.GetLastError();

        BAIL_OUT_IF (dwError == ERROR_FILE_NOT_FOUND || dwError == ERROR_PATH_NOT_FOUND, S_FALSE);
        CHRA (HRESULT_FROM_WIN32 (dwError));
    }

    // FR-013/014/015: Clean up registry if MatrixRain was the active screensaver
    // (do this before file deletion so registry is cleaned even if delete fails)
    CleanupRegistryForUninstall (registry, fileSystem);

    // FR-005: Remove the .scr file
    fSuccess = fileSystem.DeleteFile (szTargetPath);
    if (!fSuccess)
    {
        DWORD dwError = fileSystem.GetLastError();
        CHR (HRESULT_FROM_WIN32 (dwError));
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScreenSaverInstaller::IsElevated
//
//  Check if the current process is running with administrator privileges.
//
////////////////////////////////////////////////////////////////////////////////

bool ScreenSaverInstaller::IsElevated()
{
    HRESULT         hr        = S_OK;
    HANDLE          hToken    = nullptr;
    BOOL            fSuccess  = FALSE;
    TOKEN_ELEVATION elevation = {};
    DWORD           cbSize    = 0;



    fSuccess = OpenProcessToken (GetCurrentProcess(), TOKEN_QUERY, &hToken);
    CWRA (fSuccess);

    cbSize   = sizeof (elevation);
    fSuccess = GetTokenInformation (hToken, TokenElevation, &elevation, sizeof (elevation), &cbSize);
    CWRA (fSuccess);



Error:
    if (hToken != nullptr)
    {
        CloseHandle (hToken);
    }

    return elevation.TokenIsElevated != 0;
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScreenSaverInstaller::RequestElevation
//
//  Re-launch the current process with elevated privileges (triggers UAC).
//
//  pszSwitch: The command-line switch to pass to the elevated process
//             (e.g., L"/install" or L"/uninstall")
//
//  Returns S_OK if the elevated process was launched successfully.
//  Returns E_ACCESSDENIED if the user declined the UAC prompt.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT ScreenSaverInstaller::RequestElevation (LPCWSTR pszSwitch)
{
    HRESULT           hr                  = S_OK;
    WCHAR             szExePath[MAX_PATH];
    DWORD             cchPath             = 0;
    BOOL              fSuccess            = FALSE;
    SHELLEXECUTEINFOW sei                 = { sizeof (sei) };



    cchPath = GetModuleFileNameW (nullptr, szExePath, _countof (szExePath));
    CWRA (cchPath > 0 && cchPath < _countof (szExePath));

    sei.lpVerb       = L"runas";
    sei.lpFile       = szExePath;
    sei.lpParameters = pszSwitch;
    sei.nShow        = SW_SHOWNORMAL;

    fSuccess = ShellExecuteExW (&sei);
    CWRA (fSuccess);



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScreenSaverInstaller::CleanupRegistryForUninstall
//
//  Clean up screensaver registry entries during uninstall.
//
//  Reads SCRNSAVE.EXE from HKCU\Control Panel\Desktop.
//  If it points to MatrixRain.scr: deletes the value and sets
//  ScreenSaveActive to "0".
//  If it points to a different screensaver or is missing: does nothing.
//
//  Separated from Uninstall() for unit test coverage via mock registry.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT ScreenSaverInstaller::CleanupRegistryForUninstall (IRegistryProvider & registry, IFileSystemProvider & fileSystem)
{
    static constexpr LPCWSTR kpszDesktopKey   = L"Control Panel\\Desktop";
    static constexpr LPCWSTR kpszScrnsaveExe  = L"SCRNSAVE.EXE";
    static constexpr LPCWSTR kpszScreenActive = L"ScreenSaveActive";

    HRESULT      hr                       = S_OK;
    std::wstring currentScr;
    WCHAR        szExpectedPath[MAX_PATH];
    DWORD        cchPath                  = 0;



    // Build the expected path to compare against
    cchPath = fileSystem.GetSystemDirectory (szExpectedPath, _countof (szExpectedPath));
    CWRA (cchPath);
    CBRAEx (cchPath < _countof (szExpectedPath), HRESULT_FROM_WIN32 (ERROR_INSUFFICIENT_BUFFER));

    hr = PathCchAppend (szExpectedPath, _countof (szExpectedPath), kpszScrFilename);
    CHRA (hr);

    // Read the current SCRNSAVE.EXE value
    hr = registry.ReadString (HKEY_CURRENT_USER, kpszDesktopKey, kpszScrnsaveExe, currentScr);
    CHR (hr);  // Key missing or unreadable — nothing to clean up

    // Normalize short path names (e.g., MATRIX~1.SCR → MatrixRain.scr)
    // The CPL may store 8.3 short names in the registry
    {
        WCHAR szLongPath[MAX_PATH];
        DWORD cchLong = fileSystem.GetLongPathName (currentScr.c_str(), szLongPath, _countof (szLongPath));
        if (cchLong > 0 && cchLong < _countof (szLongPath))
        {
            currentScr = szLongPath;
        }
    }

    // FR-015: Only clean up if MatrixRain is the active screensaver
    BAIL_OUT_IF (_wcsicmp (currentScr.c_str(), szExpectedPath) != 0, S_OK);

    // FR-013: Delete SCRNSAVE.EXE value
    registry.DeleteValue (HKEY_CURRENT_USER, kpszDesktopKey, kpszScrnsaveExe);

    // FR-014: Set ScreenSaveActive to "0"
    registry.WriteString (HKEY_CURRENT_USER, kpszDesktopKey, kpszScreenActive, L"0");



Error:
    return hr;
}
