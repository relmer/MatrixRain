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
////////////////////////////////////////////////////////////////////////////////

HRESULT ScreenSaverInstaller::Install()
{
    return E_NOTIMPL;
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScreenSaverInstaller::Uninstall
//
////////////////////////////////////////////////////////////////////////////////

HRESULT ScreenSaverInstaller::Uninstall (IRegistryProvider & /*registry*/)
{
    return E_NOTIMPL;
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScreenSaverInstaller::IsElevated
//
////////////////////////////////////////////////////////////////////////////////

bool ScreenSaverInstaller::IsElevated()
{
    return false;
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScreenSaverInstaller::RequestElevation
//
////////////////////////////////////////////////////////////////////////////////

HRESULT ScreenSaverInstaller::RequestElevation (LPCWSTR /*pszSwitch*/)
{
    return E_NOTIMPL;
}
