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

    static HRESULT Install();

    static HRESULT Uninstall (IRegistryProvider & registry);

    static bool    IsElevated();

    static HRESULT RequestElevation (LPCWSTR pszSwitch);

    static void    CleanupRegistryForUninstall (IRegistryProvider & registry);
};
