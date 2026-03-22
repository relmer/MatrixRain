#pragma once





////////////////////////////////////////////////////////////////////////////////
//
//  IRegistryProvider
//
//  Abstraction for Windows Registry operations to enable mock-based testing.
//  Production code uses the real Windows Registry; tests inject a mock.
//
////////////////////////////////////////////////////////////////////////////////

class IRegistryProvider
{
public:

    virtual ~IRegistryProvider() = default;

    virtual HRESULT ReadString (HKEY    hKey,
                                LPCWSTR pszSubKey,
                                LPCWSTR pszValueName,
                                std::wstring & value) = 0;

    virtual HRESULT WriteString (HKEY    hKey,
                                 LPCWSTR pszSubKey,
                                 LPCWSTR pszValueName,
                                 LPCWSTR pszValue) = 0;

    virtual HRESULT DeleteValue (HKEY    hKey,
                                 LPCWSTR pszSubKey,
                                 LPCWSTR pszValueName) = 0;
};





////////////////////////////////////////////////////////////////////////////////
//
//  WindowsRegistryProvider
//
//  Real Windows Registry implementation of IRegistryProvider.
//
////////////////////////////////////////////////////////////////////////////////

class WindowsRegistryProvider : public IRegistryProvider
{
public:

    HRESULT ReadString (HKEY    hKey,
                        LPCWSTR pszSubKey,
                        LPCWSTR pszValueName,
                        std::wstring & value) override;

    HRESULT WriteString (HKEY    hKey,
                         LPCWSTR pszSubKey,
                         LPCWSTR pszValueName,
                         LPCWSTR pszValue) override;

    HRESULT DeleteValue (HKEY    hKey,
                         LPCWSTR pszSubKey,
                         LPCWSTR pszValueName) override;
};





////////////////////////////////////////////////////////////////////////////////
//
//  ScreenSaverInstaller
//
//  Provides Install/Uninstall functionality for registering MatrixRain as a
//  Windows screensaver.  All methods are static except Uninstall, which accepts
//  an IRegistryProvider for testability.
//
////////////////////////////////////////////////////////////////////////////////

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
    //  Registry operations use the provided IRegistryProvider for testability.
    //
    static HRESULT Uninstall (IRegistryProvider & registry);


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


    //  Clean up screensaver registry entries during uninstall.
    //
    //  Reads SCRNSAVE.EXE from HKCU\Control Panel\Desktop.
    //  If it points to MatrixRain.scr: deletes the value and sets ScreenSaveActive to "0".
    //  If it points to a different screensaver or is missing: does nothing.
    //
    //  Separated from Uninstall() for unit test coverage via mock registry.
    //
    static void CleanupRegistryForUninstall (IRegistryProvider & registry);
};
