#pragma once

#include "ISettingsProvider.h"





class RegistrySettingsProvider : public ISettingsProvider
{
public:
    explicit RegistrySettingsProvider (LPCWSTR registryKeyPath = REGISTRY_KEY_PATH) :
        m_registryKeyPath (registryKeyPath)
    {
    }

    HRESULT Load (ScreenSaverSettings & settings) override;
    HRESULT Save (const ScreenSaverSettings & settings) override;

private:
    static constexpr LPCWSTR REGISTRY_KEY_PATH = L"Software\\relmer\\MatrixRain";

    LPCWSTR m_registryKeyPath;
    
    static constexpr LPCWSTR VALUE_DENSITY                = L"Density";
    static constexpr LPCWSTR VALUE_COLOR_SCHEME           = L"ColorScheme";
    static constexpr LPCWSTR VALUE_ANIMATION_SPEED        = L"AnimationSpeed";
    static constexpr LPCWSTR VALUE_GLOW_INTENSITY         = L"GlowIntensity";
    static constexpr LPCWSTR VALUE_GLOW_SIZE              = L"GlowSize";
    static constexpr LPCWSTR VALUE_START_FULLSCREEN       = L"StartFullscreen";
    static constexpr LPCWSTR VALUE_SHOW_DEBUG_STATS       = L"ShowDebugStats";
    static constexpr LPCWSTR VALUE_LAST_SAVED             = L"LastSaved";
    
    static HRESULT ReadInt    (HKEY hKey, LPCWSTR valueName, int & outValue);
    static HRESULT ReadBool   (HKEY hKey, LPCWSTR valueName, bool & outValue);
    static HRESULT ReadString (HKEY hKey, LPCWSTR valueName, std::wstring & outValue);
    
    static HRESULT WriteInt    (HKEY hKey, LPCWSTR valueName, int value);
    static HRESULT WriteBool   (HKEY hKey, LPCWSTR valueName, bool value);
    static HRESULT WriteString (HKEY hKey, LPCWSTR valueName, const std::wstring & value);
};

