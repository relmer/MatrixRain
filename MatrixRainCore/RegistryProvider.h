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

    virtual HRESULT ReadString (HKEY           hKey,
                                LPCWSTR        pszSubKey,
                                LPCWSTR        pszValueName,
                                std::wstring & value) = 0;

    virtual HRESULT WriteString (HKEY    hKey,
                                 LPCWSTR pszSubKey,
                                 LPCWSTR pszValueName,
                                 LPCWSTR pszValue) = 0;

    virtual HRESULT DeleteValue (HKEY    hKey,
                                 LPCWSTR pszSubKey,
                                 LPCWSTR pszValueName) = 0;
};
