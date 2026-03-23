#pragma once

#include "FileSystemProvider.h"
#include "RegistryProvider.h"





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
    using IsElevatedFn = bool (*)();

    static HRESULT Install                     ();
    static HRESULT Uninstall                   (IRegistryProvider & registry, IFileSystemProvider & fileSystem);
    static bool    IsElevated                  ();
    static HRESULT RequestElevation            (LPCWSTR pszSwitch);
    static HRESULT CleanupRegistryForUninstall (IRegistryProvider & registry, IFileSystemProvider & fileSystem);
    static HRESULT CheckScreenSaverPolicies    (bool & fBlocked, std::wstring & errorMessage);

    // Test seam: override to control elevation check in unit tests
    static inline IsElevatedFn s_pfnIsElevated = &IsElevated;

private:
    static constexpr LPCWSTR kpszScrFilename = L"MatrixRain.scr";

    struct PolicyCheck
    {
        HKEY    hkeyRoot;
        LPCWSTR pszSubKey;
        LPCWSTR pszValueName;
        DWORD   dwBlockingValue;
        bool    fBlockIfNonZero;
        LPCWSTR pszMessage;
    };

    static inline const PolicyCheck s_krgPolicyChecks[] =
    {
        {
            HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\Microsoft\\PolicyManager\\current\\device\\DeviceLock",
            L"MaxInactivityTimeDeviceLock",
            0, true,
            L"An MDM/Intune device lock policy is enforced on this device.\n\n"
            L"This policy prevents screensavers from activating, causing Windows "
            L"to lock the screen instead."
        },
        {
            HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\Policies\\Microsoft\\Windows\\Control Panel\\Desktop",
            L"ScreenSaveActive",
            0, false,
            L"A Group Policy on this device is preventing screensavers from being enabled."
        },
        {
            HKEY_CURRENT_USER,
            L"SOFTWARE\\Policies\\Microsoft\\Windows\\Control Panel\\Desktop",
            L"ScreenSaveActive",
            0, false,
            L"A policy on this account is preventing screensavers from being enabled."
        },
    };

    static HRESULT CheckPolicy (const PolicyCheck & check, bool & fBlocked, std::wstring & errorMessage);

};
