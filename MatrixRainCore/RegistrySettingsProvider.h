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

    // v1.5 US5 (T061, FR-030, FR-031, FR-035, contracts/registry-schema.md):
    // CustomColor (REG_DWORD) is the user-picked streak color (COLORREF).
    // CustomColorPalette (REG_BINARY 64 bytes) is the 16-swatch palette the
    // ChooseColor dialog seeds its custom-color row from.  The palette
    // persists UNCONDITIONALLY across Save() / live-mode rollback per
    // FR-035 -- it lives outside the snapshot rollback set.
    static constexpr LPCWSTR VALUE_CUSTOM_COLOR              = L"CustomColor";
    static constexpr LPCWSTR VALUE_CUSTOM_COLOR_PALETTE      = L"CustomColorPalette";
    
    static HRESULT ReadInt    (HKEY hKey, LPCWSTR valueName, int & outValue);
    static HRESULT ReadBool   (HKEY hKey, LPCWSTR valueName, bool & outValue);
    static HRESULT ReadString (HKEY hKey, LPCWSTR valueName, std::wstring & outValue);
    static HRESULT ReadDword  (HKEY hKey, LPCWSTR valueName, DWORD & outValue);
    
    static HRESULT WriteInt    (HKEY hKey, LPCWSTR valueName, int value);
    static HRESULT WriteBool   (HKEY hKey, LPCWSTR valueName, bool value);
    static HRESULT WriteString (HKEY hKey, LPCWSTR valueName, const std::wstring & value);
    static HRESULT WriteDword  (HKEY hKey, LPCWSTR valueName, DWORD value);
    static HRESULT WriteBinary (HKEY hKey, LPCWSTR valueName, const void * pData, DWORD cbData);
};

