#pragma once

#include "IRegistryProvider.h"





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

    HRESULT ReadString (HKEY           hKey,
                        LPCWSTR        pszSubKey,
                        LPCWSTR        pszValueName,
                        std::wstring & value) override;

    HRESULT WriteString (HKEY    hKey,
                         LPCWSTR pszSubKey,
                         LPCWSTR pszValueName,
                         LPCWSTR pszValue) override;

    HRESULT DeleteValue (HKEY    hKey,
                         LPCWSTR pszSubKey,
                         LPCWSTR pszValueName) override;
};
