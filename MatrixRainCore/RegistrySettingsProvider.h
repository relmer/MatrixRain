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
    static constexpr LPCWSTR VALUE_GLOW_ENABLED           = L"GlowEnabled";          // v1.5 T038 (FR-020, FR-038)
    static constexpr LPCWSTR VALUE_SCANLINES_ENABLED      = L"ScanlinesEnabled";     // v1.5 T049 (FR-027, FR-028, FR-038)
    static constexpr LPCWSTR VALUE_SCANLINES_INTENSITY    = L"ScanlinesIntensity";
    static constexpr LPCWSTR VALUE_SCANLINES_STYLE        = L"ScanlinesStyle";
    static constexpr LPCWSTR VALUE_START_FULLSCREEN       = L"StartFullscreen";
    static constexpr LPCWSTR VALUE_SHOW_DEBUG_STATS       = L"ShowDebugStats";
    static constexpr LPCWSTR VALUE_MULTIMONITOR              = L"MultiMonitor";
    static constexpr LPCWSTR VALUE_GPU_ADAPTER               = L"GpuAdapter";
    static constexpr LPCWSTR VALUE_QUALITY_PRESET            = L"QualityPreset";
    static constexpr LPCWSTR VALUE_LASTCUSTOM_GLOW_INTENSITY = L"LastCustom_GlowIntensity";
    static constexpr LPCWSTR VALUE_LASTCUSTOM_PASSES         = L"LastCustom_Passes";
    static constexpr LPCWSTR VALUE_LASTCUSTOM_RESOLUTION     = L"LastCustom_Resolution";
    static constexpr LPCWSTR VALUE_LASTCUSTOM_SMOOTHNESS     = L"LastCustom_Smoothness";
    static constexpr LPCWSTR VALUE_LAST_SAVED                = L"LastSaved";
    
    static HRESULT ReadInt    (HKEY hKey, LPCWSTR valueName, int & outValue);
    static HRESULT ReadBool   (HKEY hKey, LPCWSTR valueName, bool & outValue);
    static HRESULT ReadString (HKEY hKey, LPCWSTR valueName, std::wstring & outValue);
    
    static HRESULT WriteInt    (HKEY hKey, LPCWSTR valueName, int value);
    static HRESULT WriteBool   (HKEY hKey, LPCWSTR valueName, bool value);
    static HRESULT WriteString (HKEY hKey, LPCWSTR valueName, const std::wstring & value);
};

