#pragma once

#include "ScreenSaverSettings.h"





class RegistrySettingsProvider
{
public:
    static HRESULT Load (ScreenSaverSettings & settings);
    static HRESULT Save (const ScreenSaverSettings & settings);
    
    // For testing: allows overriding the registry key path
    static void SetRegistryKeyPath (LPCWSTR path);
    static void ResetRegistryKeyPath();

private:
    static constexpr LPCWSTR DEFAULT_REGISTRY_KEY_PATH = L"Software\\relmer\\MatrixRain";
    static LPCWSTR s_registryKeyPath;
    
    static LPCWSTR GetRegistryKeyPath();
    
    static constexpr LPCWSTR VALUE_DENSITY                = L"Density";
    static constexpr LPCWSTR VALUE_COLOR_SCHEME           = L"ColorScheme";
    static constexpr LPCWSTR VALUE_ANIMATION_SPEED        = L"AnimationSpeed";
    static constexpr LPCWSTR VALUE_GLOW_INTENSITY         = L"GlowIntensity";
    static constexpr LPCWSTR VALUE_GLOW_SIZE              = L"GlowSize";
    static constexpr LPCWSTR VALUE_START_FULLSCREEN       = L"StartFullscreen";
    static constexpr LPCWSTR VALUE_SHOW_DEBUG_STATS       = L"ShowDebugStats";
    static constexpr LPCWSTR VALUE_SHOW_FADE_TIMERS       = L"ShowFadeTimers";
    static constexpr LPCWSTR VALUE_LAST_SAVED             = L"LastSaved";
    
    static HRESULT ReadInt    (HKEY hKey, LPCWSTR valueName, int & outValue);
    static HRESULT ReadBool   (HKEY hKey, LPCWSTR valueName, bool & outValue);
    static HRESULT ReadString (HKEY hKey, LPCWSTR valueName, std::wstring & outValue);
    
    static HRESULT WriteInt    (HKEY hKey, LPCWSTR valueName, int value);
    static HRESULT WriteBool   (HKEY hKey, LPCWSTR valueName, bool value);
    static HRESULT WriteString (HKEY hKey, LPCWSTR valueName, const std::wstring & value);
};

