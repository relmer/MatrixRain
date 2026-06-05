#include "pch.h"

#include "RegistrySettingsProvider.h"





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
    lstat = RegOpenKeyExW (HKEY_CURRENT_USER, m_registryKeyPath, 0, KEY_READ, &hKey);
    
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
    ReadBool   (hKey, VALUE_GLOW_ENABLED,     settings.m_glowEnabled);                // v1.5 T038 (FR-020, FR-038)

    // v1.5 T049 (FR-027, FR-028, FR-038, contracts/registry-schema.md):
    // Scanlines defaults are baked into ScreenSaverSettings (true / 30 / 50),
    // so ReadBool/ReadInt no-ops on absent keys leave them intact.  After
    // the loads we run a one-shot clamp on the two int fields to defend
    // against tampered registries.
    ReadBool   (hKey, VALUE_SCANLINES_ENABLED,   settings.m_scanlinesEnabled);
    ReadInt    (hKey, VALUE_SCANLINES_INTENSITY, settings.m_scanlinesIntensity);
    ReadInt    (hKey, VALUE_SCANLINES_STYLE,     settings.m_scanlinesStyle);
    ReadBool   (hKey, VALUE_START_FULLSCREEN, settings.m_startFullscreen);
    ReadBool   (hKey, VALUE_SHOW_DEBUG_STATS, settings.m_showDebugStats);
    ReadBool   (hKey, VALUE_MULTIMONITOR,     settings.m_multiMonitorEnabled);
    ReadString (hKey, VALUE_GPU_ADAPTER,      settings.m_gpuAdapter);

    // Quality preset: REG_SZ matching the enum class names.  Empty/missing
    // value triggers the first-run heuristic in Application::Initialize.
    {
        std::wstring presetName;

        if (ReadString (hKey, VALUE_QUALITY_PRESET, presetName) == S_OK)
        {
            if      (presetName == L"Low")    settings.m_qualityPreset = QualityPreset::Low;
            else if (presetName == L"Medium") settings.m_qualityPreset = QualityPreset::Medium;
            else if (presetName == L"High")   settings.m_qualityPreset = QualityPreset::High;
            else if (presetName == L"Custom") settings.m_qualityPreset = QualityPreset::Custom;
        }
    }

    // Last-custom advanced values: ALL FOUR DWORDs must be present or we
    // treat the persisted LastCustom as missing.  Partial state is never
    // honored (registry-schema contract).
    {
        int passes        = 0;
        int resolution    = 0;
        int smoothness    = 0;
        int glowIntensity = 0;

        bool havePasses     = ReadInt (hKey, VALUE_LASTCUSTOM_PASSES,         passes)        == S_OK;
        bool haveResolution = ReadInt (hKey, VALUE_LASTCUSTOM_RESOLUTION,     resolution)    == S_OK;
        bool haveSmoothness = ReadInt (hKey, VALUE_LASTCUSTOM_SMOOTHNESS,     smoothness)    == S_OK;
        bool haveIntensity  = ReadInt (hKey, VALUE_LASTCUSTOM_GLOW_INTENSITY, glowIntensity) == S_OK;

        if (havePasses && haveResolution && haveSmoothness && haveIntensity)
        {
            AdvancedGraphicsValues v;

            v.m_glowIntensityPercent = std::clamp (glowIntensity, 0,   200);
            v.m_blurPasses           = std::clamp (passes,        1,   4);

            switch (resolution)
            {
                case 1: v.m_bloomResolutionDivisor = ResolutionDivisor::Full;    break;
                case 2: v.m_bloomResolutionDivisor = ResolutionDivisor::Half;    break;
                case 4: v.m_bloomResolutionDivisor = ResolutionDivisor::Quarter; break;
                case 8: v.m_bloomResolutionDivisor = ResolutionDivisor::Eighth;  break;
                default: v.m_bloomResolutionDivisor = ResolutionDivisor::Half;
            }

            switch (smoothness)
            {
                case 5:  v.m_blurTaps = BlurTaps::Low;    break;
                case 9:  v.m_blurTaps = BlurTaps::Medium; break;
                case 13: v.m_blurTaps = BlurTaps::High;   break;
                default: v.m_blurTaps = BlurTaps::High;
            }

            settings.m_lastCustom = v;
        }
    }

    // When QualityPreset == Custom, restore the advanced values from
    // LastCustom (if present).  Otherwise advanced values follow the
    // named preset's lookup row.
    if (settings.m_qualityPreset == QualityPreset::Custom && settings.m_lastCustom.has_value())
    {
        settings.m_advancedValues = *settings.m_lastCustom;
    }
    else if (settings.m_qualityPreset != QualityPreset::Custom)
    {
        settings.m_advancedValues = LookupPresetValues (settings.m_qualityPreset);
    }

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
                             m_registryKeyPath,
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

    // v1.5 T038 (FR-020, FR-038): persist GlowEnabled as REG_DWORD.
    hr = WriteBool (hKey, VALUE_GLOW_ENABLED, settings.m_glowEnabled);
    CHR (hr);

    // v1.5 T049 (FR-027, FR-028, FR-038): persist scanlines triple.  Clamp
    // on write so out-of-range C++ state can't propagate to the registry.
    hr = WriteBool (hKey, VALUE_SCANLINES_ENABLED, settings.m_scanlinesEnabled);
    CHR (hr);

    hr = WriteInt (hKey, VALUE_SCANLINES_INTENSITY,
                   std::clamp (settings.m_scanlinesIntensity,
                               ScreenSaverSettings::MIN_SCANLINES_INTENSITY_PERCENT,
                               ScreenSaverSettings::MAX_SCANLINES_INTENSITY_PERCENT));
    CHR (hr);

    hr = WriteInt (hKey, VALUE_SCANLINES_STYLE,
                   std::clamp (settings.m_scanlinesStyle,
                               ScreenSaverSettings::MIN_SCANLINES_STYLE,
                               ScreenSaverSettings::MAX_SCANLINES_STYLE));
    CHR (hr);
    
    hr = WriteBool (hKey, VALUE_START_FULLSCREEN, settings.m_startFullscreen);
    CHR (hr);
    
    hr = WriteBool (hKey, VALUE_SHOW_DEBUG_STATS, settings.m_showDebugStats);
    CHR (hr);

    hr = WriteBool (hKey, VALUE_MULTIMONITOR, settings.m_multiMonitorEnabled);
    CHR (hr);

    hr = WriteString (hKey, VALUE_GPU_ADAPTER, settings.m_gpuAdapter);
    CHR (hr);

    // QualityPreset name as REG_SZ.
    {
        const wchar_t * name = L"High";

        switch (settings.m_qualityPreset)
        {
            case QualityPreset::Low:    name = L"Low";    break;
            case QualityPreset::Medium: name = L"Medium"; break;
            case QualityPreset::High:   name = L"High";   break;
            case QualityPreset::Custom: name = L"Custom"; break;
        }

        hr = WriteString (hKey, VALUE_QUALITY_PRESET, std::wstring (name));
        CHR (hr);
    }

    // LastCustom values: always persist when present so a future switch to
    // Custom can restore them.  When absent, intentionally leave any prior
    // values in the registry (no-op write would be a silent migration
    // gotcha).  The all-or-nothing read enforces consistency.
    if (settings.m_lastCustom.has_value())
    {
        hr = WriteInt (hKey, VALUE_LASTCUSTOM_GLOW_INTENSITY, settings.m_lastCustom->m_glowIntensityPercent);
        CHR (hr);

        hr = WriteInt (hKey, VALUE_LASTCUSTOM_PASSES,         settings.m_lastCustom->m_blurPasses);
        CHR (hr);

        hr = WriteInt (hKey, VALUE_LASTCUSTOM_RESOLUTION,     static_cast<int> (settings.m_lastCustom->m_bloomResolutionDivisor));
        CHR (hr);

        hr = WriteInt (hKey, VALUE_LASTCUSTOM_SMOOTHNESS,     static_cast<int> (settings.m_lastCustom->m_blurTaps));
        CHR (hr);
    }


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

