#pragma once

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
    static HRESULT Install                     ();
    static HRESULT Uninstall                   (IRegistryProvider & registry);
    static bool    IsElevated                  ();
    static HRESULT RequestElevation            (LPCWSTR pszSwitch);
    static void    CleanupRegistryForUninstall (IRegistryProvider & registry);

private:
    static constexpr LPCWSTR kpszScrFilename = L"MatrixRain.scr";
};
