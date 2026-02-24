#include "pch.h"

#include "RegistrySettingsProvider.h"




// Static member initialization
LPCWSTR RegistrySettingsProvider::s_registryKeyPath = DEFAULT_REGISTRY_KEY_PATH;




////////////////////////////////////////////////////////////////////////////////
//
//  RegistrySettingsProvider::SetRegistryKeyPath
//
//  For testing: allows overriding the registry key path
//
////////////////////////////////////////////////////////////////////////////////

void RegistrySettingsProvider::SetRegistryKeyPath (LPCWSTR path)
{
    s_registryKeyPath = path;
}




////////////////////////////////////////////////////////////////////////////////
//
//  RegistrySettingsProvider::ResetRegistryKeyPath
//
//  Resets the registry key path to the default
//
////////////////////////////////////////////////////////////////////////////////

void RegistrySettingsProvider::ResetRegistryKeyPath()
{
    s_registryKeyPath = DEFAULT_REGISTRY_KEY_PATH;
}




////////////////////////////////////////////////////////////////////////////////
//
//  RegistrySettingsProvider::GetRegistryKeyPath
//
//  Returns the current registry key path
//
////////////////////////////////////////////////////////////////////////////////

LPCWSTR RegistrySettingsProvider::GetRegistryKeyPath()
{
    return s_registryKeyPath;
}


////////////////////////////////////////////////////////////////////////////////
//
//  RegistrySettingsProvider::Load
//
//  Loads settings from the registry. Returns S_OK if settings were loaded,
//  or S_FALSE if the registry key doesn't exist (use defaults).
//
////////////////////////////////////////////////////////////////////////////////

HRESULT RegistrySettingsProvider::Load (ScreenSaverSettings & settings)
{
    HRESULT hr    = S_OK;
    HKEY    hKey  = nullptr;
    LSTATUS lstat = ERROR_SUCCESS;
    
    
    
    // Try to open the registry key
    lstat = RegOpenKeyExW (HKEY_CURRENT_USER, GetRegistryKeyPath(), 0, KEY_READ, &hKey);
    
    // If the key doesn't exist, return S_FALSE (use defaults)
    BAIL_OUT_IF (lstat == ERROR_FILE_NOT_FOUND, S_FALSE);
    
    // Any other error is a failure
    CBRA (lstat == ERROR_SUCCESS);
    
    // Read values (ignore individual failures - use defaults for missing values)
    ReadInt    (hKey, VALUE_DENSITY,          settings.m_densityPercent);
    ReadString (hKey, VALUE_COLOR_SCHEME,     settings.m_colorSchemeKey);
    ReadInt    (hKey, VALUE_ANIMATION_SPEED,  settings.m_animationSpeedPercent);
    ReadInt    (hKey, VALUE_GLOW_INTENSITY,   settings.m_glowIntensityPercent);
    ReadInt    (hKey, VALUE_GLOW_SIZE,        settings.m_glowSizePercent);
    ReadBool   (hKey, VALUE_START_FULLSCREEN, settings.m_startFullscreen);
    ReadBool   (hKey, VALUE_SHOW_DEBUG_STATS, settings.m_showDebugStats);
    ReadBool   (hKey, VALUE_SHOW_FADE_TIMERS, settings.m_showFadeTimers);

    // Clamp all values to valid ranges
    settings.Clamp();


Error:
    if (hKey != nullptr)
    {
        RegCloseKey (hKey);
    }
    
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  RegistrySettingsProvider::Save
//
//  Saves settings to the registry. Creates the key if it doesn't exist.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT RegistrySettingsProvider::Save (const ScreenSaverSettings & settings)
{
    HRESULT hr            = S_OK;
    HKEY    hKey          = nullptr;
    LSTATUS lstat         = ERROR_SUCCESS;
    DWORD   dwDisposition;
    
    
    
    // Create or open the registry key
    lstat = RegCreateKeyExW (HKEY_CURRENT_USER,
                             GetRegistryKeyPath(),
                             0,
                             nullptr,
                             REG_OPTION_NON_VOLATILE,
                             KEY_WRITE,
                             nullptr,
                             &hKey,
                             &dwDisposition);
    CBRA (lstat == ERROR_SUCCESS);
    
    // Write all values
    hr = WriteInt (hKey, VALUE_DENSITY, settings.m_densityPercent);
    CHR (hr);
    
    hr = WriteString (hKey, VALUE_COLOR_SCHEME, settings.m_colorSchemeKey);
    CHR (hr);
    
    hr = WriteInt (hKey, VALUE_ANIMATION_SPEED, settings.m_animationSpeedPercent);
    CHR (hr);
    
    hr = WriteInt (hKey, VALUE_GLOW_INTENSITY, settings.m_glowIntensityPercent);
    CHR (hr);
    
    hr = WriteInt (hKey, VALUE_GLOW_SIZE, settings.m_glowSizePercent);
    CHR (hr);
    
    hr = WriteBool (hKey, VALUE_START_FULLSCREEN, settings.m_startFullscreen);
    CHR (hr);
    
    hr = WriteBool (hKey, VALUE_SHOW_DEBUG_STATS, settings.m_showDebugStats);
    CHR (hr);
    
    hr = WriteBool (hKey, VALUE_SHOW_FADE_TIMERS, settings.m_showFadeTimers);
    CHR (hr);


Error:
    if (hKey != nullptr)
    {
        RegCloseKey (hKey);
    }
    
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  RegistrySettingsProvider::ReadInt
//
////////////////////////////////////////////////////////////////////////////////

HRESULT RegistrySettingsProvider::ReadInt (HKEY hKey, LPCWSTR valueName, int & outValue)
{
    HRESULT hr      = S_OK;
    DWORD   dwType  = 0;
    DWORD   dwValue;
    DWORD   cbValue = sizeof (dwValue);
    LSTATUS lstat   = ERROR_SUCCESS;
    
    
    
    lstat = RegQueryValueExW (hKey, valueName, nullptr, &dwType, reinterpret_cast<LPBYTE> (&dwValue), &cbValue);
    
    BAIL_OUT_IF (lstat == ERROR_FILE_NOT_FOUND, S_FALSE);
    CBRA (lstat == ERROR_SUCCESS);
    CBRA (dwType == REG_DWORD);
    
    outValue = static_cast<int> (dwValue);


Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  RegistrySettingsProvider::ReadBool
//
////////////////////////////////////////////////////////////////////////////////

HRESULT RegistrySettingsProvider::ReadBool (HKEY hKey, LPCWSTR valueName, bool & outValue)
{
    HRESULT hr      = S_OK;
    DWORD   dwType  = 0;
    DWORD   dwValue;
    DWORD   cbValue = sizeof (dwValue);
    LSTATUS lstat   = ERROR_SUCCESS;
    
    
    
    lstat = RegQueryValueExW (hKey, valueName, nullptr, &dwType, reinterpret_cast<LPBYTE> (&dwValue), &cbValue);
    
    BAIL_OUT_IF (lstat == ERROR_FILE_NOT_FOUND, S_FALSE);
    CBRA (lstat == ERROR_SUCCESS);
    CBRA (dwType == REG_DWORD);
    
    outValue = (dwValue != 0);


Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  RegistrySettingsProvider::ReadString
//
////////////////////////////////////////////////////////////////////////////////

HRESULT RegistrySettingsProvider::ReadString (HKEY hKey, LPCWSTR valueName, std::wstring & outValue)
{
    HRESULT hr      = S_OK;
    DWORD   dwType  = 0;
    DWORD   cbValue = 0;
    LSTATUS lstat   = ERROR_SUCCESS;
    
    
    
    // Query size first
    lstat = RegQueryValueExW (hKey, valueName, nullptr, &dwType, nullptr, &cbValue);
    
    BAIL_OUT_IF (lstat == ERROR_FILE_NOT_FOUND, S_FALSE);
    CBRA (lstat == ERROR_SUCCESS);
    CBRA (dwType == REG_SZ);
    CBRA (cbValue > 0);
    CBRA (cbValue <= 512);  // Reject unreasonably large strings (max 256 wide chars)
    
    // Allocate buffer and read string
    {
        std::vector<wchar_t> buffer (cbValue / sizeof (wchar_t));
        
        
        lstat = RegQueryValueExW (hKey, valueName, nullptr, &dwType, reinterpret_cast<LPBYTE> (buffer.data()), &cbValue);
        CBRA (lstat == ERROR_SUCCESS);
        
        outValue = buffer.data();
    }


Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  RegistrySettingsProvider::WriteInt
//
////////////////////////////////////////////////////////////////////////////////

HRESULT RegistrySettingsProvider::WriteInt (HKEY hKey, LPCWSTR valueName, int value)
{
    HRESULT hr      = S_OK;
    LSTATUS lstat   = ERROR_SUCCESS;
    DWORD   dwValue = static_cast<DWORD> (value);
    
    
    
    lstat = RegSetValueExW (hKey, valueName, 0, REG_DWORD, reinterpret_cast<const BYTE *> (&dwValue), sizeof (DWORD));
    CBRA (lstat == ERROR_SUCCESS);


Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  RegistrySettingsProvider::WriteBool
//
////////////////////////////////////////////////////////////////////////////////

HRESULT RegistrySettingsProvider::WriteBool (HKEY hKey, LPCWSTR valueName, bool value)
{
    HRESULT hr      = S_OK;
    LSTATUS lstat   = ERROR_SUCCESS;
    DWORD   dwValue = value ? 1 : 0;
    
    
    
    lstat = RegSetValueExW (hKey, valueName, 0, REG_DWORD, reinterpret_cast<const BYTE *> (&dwValue), sizeof (DWORD));
    CBRA (lstat == ERROR_SUCCESS);


Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  RegistrySettingsProvider::WriteString
//
////////////////////////////////////////////////////////////////////////////////

HRESULT RegistrySettingsProvider::WriteString (HKEY hKey, LPCWSTR valueName, const std::wstring & value)
{
    HRESULT hr      = S_OK;
    LSTATUS lstat   = ERROR_SUCCESS;
    DWORD   cbValue;
    
    
    
    cbValue = static_cast<DWORD> ((value.length() + 1) * sizeof (wchar_t));

    lstat  = RegSetValueExW (hKey, valueName, 0, REG_SZ, reinterpret_cast<const BYTE *> (value.c_str()), cbValue);
    CBRA (lstat == ERROR_SUCCESS);


Error:
    return hr;
}

