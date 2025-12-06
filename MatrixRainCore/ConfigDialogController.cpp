#include "pch.h"

#include "ConfigDialogController.h"
#include "RegistrySettingsProvider.h"





HRESULT ConfigDialogController::Initialize()
{
    HRESULT hr = S_OK;
    
    
    
    // Load settings from registry (falls back to defaults if key doesn't exist)
    hr = RegistrySettingsProvider::Load (m_settings);
    
    // hr == S_FALSE means key didn't exist, used defaults (not an error)
    // hr == S_OK means loaded from registry successfully
    // Any other HRESULT is an actual error, but we continue with defaults
    if (FAILED (hr))
    {
        // Reset to defaults on error
        m_settings = ScreenSaverSettings();
    }
    
    // Save original settings for Cancel operation
    m_originalSettings = m_settings;
    
    return S_OK;  // Always succeed - worst case we use defaults
}





void ConfigDialogController::UpdateDensity (int densityPercent)
{
    m_settings.m_densityPercent = ScreenSaverSettings::ClampDensityPercent (densityPercent);
}





void ConfigDialogController::UpdateColorScheme (const std::wstring & colorSchemeKey)
{
    // Only update if valid color scheme
    if (m_isValidColorScheme (colorSchemeKey))
    {
        // Normalize to lowercase for storage
        std::wstring normalizedKey = colorSchemeKey;
        std::transform (normalizedKey.begin(), normalizedKey.end(), normalizedKey.begin(), ::towlower);
        m_settings.m_colorSchemeKey = normalizedKey;
    }
}





void ConfigDialogController::UpdateAnimationSpeed (int animationSpeedPercent)
{
    m_settings.m_animationSpeedPercent = ScreenSaverSettings::ClampPercent (animationSpeedPercent, 
                                                                             ScreenSaverSettings::MIN_ANIMATION_SPEED_PERCENT, 
                                                                             ScreenSaverSettings::MAX_ANIMATION_SPEED_PERCENT);
}





void ConfigDialogController::UpdateGlowIntensity (int glowIntensityPercent)
{
    m_settings.m_glowIntensityPercent = ScreenSaverSettings::ClampPercent (glowIntensityPercent, 
                                                                            ScreenSaverSettings::MIN_GLOW_INTENSITY_PERCENT, 
                                                                            ScreenSaverSettings::MAX_GLOW_INTENSITY_PERCENT);
}





void ConfigDialogController::UpdateGlowSize (int glowSizePercent)
{
    m_settings.m_glowSizePercent = ScreenSaverSettings::ClampPercent (glowSizePercent, 
                                                                       ScreenSaverSettings::MIN_GLOW_SIZE_PERCENT, 
                                                                       ScreenSaverSettings::MAX_GLOW_SIZE_PERCENT);
}





void ConfigDialogController::UpdateStartFullscreen (bool startFullscreen)
{
    m_settings.m_startFullscreen = startFullscreen;
}





void ConfigDialogController::UpdateShowDebugStats (bool showDebugStats)
{
    m_settings.m_showDebugStats = showDebugStats;
}





void ConfigDialogController::UpdateShowFadeTimers (bool showFadeTimers)
{
    m_settings.m_showFadeTimers = showFadeTimers;
}





HRESULT ConfigDialogController::ApplyChanges()
{
    HRESULT hr = S_OK;
    
    
    
    // Persist settings to registry
    hr = RegistrySettingsProvider::Save (m_settings);
    CHR (hr);
    
    // Update original settings to match saved state
    m_originalSettings = m_settings;
    
    
Error:
    return hr;
}





void ConfigDialogController::CancelChanges()
{
    // Restore original settings (discard pending changes)
    m_settings = m_originalSettings;
}





void ConfigDialogController::ResetToDefaults()
{
    // Reset to factory defaults
    m_settings = ScreenSaverSettings();
}





bool ConfigDialogController::m_isValidColorScheme (const std::wstring & key) const
{
    std::wstring lowerKey = key;
    std::transform (lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::towlower);
    
    return lowerKey == L"green"  ||
           lowerKey == L"blue"   ||
           lowerKey == L"red"    ||
           lowerKey == L"amber"  ||
           lowerKey == L"cycle";
}






////////////////////////////////////////////////////////////////////////////////
//
//  ConfigDialogController::InitializeLiveMode
//
//  Initializes live overlay mode with ApplicationState reference for immediate
//  updates. Creates snapshot for Cancel rollback capability.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT ConfigDialogController::InitializeLiveMode (ApplicationState * appState)
{
    HRESULT hr = S_OK;
    
    
    
    // Validate ApplicationState pointer
    CBRAEx (appState != nullptr, E_POINTER);
    
    // Create snapshot of current settings for Cancel rollback
    m_snapshot.snapshotSettings     = m_settings;
    m_snapshot.isLiveMode           = true;
    m_snapshot.applicationStateRef  = appState;  // Stored for use in T052
    
    
Error:
    return hr;
}






////////////////////////////////////////////////////////////////////////////////
//
//  ConfigDialogController::ApplyLiveMode
//
//  Persists current settings to registry and clears live mode snapshot.
//  Called when user clicks OK in live overlay dialog.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT ConfigDialogController::ApplyLiveMode()
{
    HRESULT hr = S_OK;
    
    
    
    // Verify we're in live mode
    CBRA (m_snapshot.isLiveMode);
    
    // Persist settings to registry
    hr = RegistrySettingsProvider::Save (m_settings);
    CHR (hr);
    
    // Clear live mode state
    m_snapshot.isLiveMode          = false;
    m_snapshot.applicationStateRef = nullptr;
    
    
Error:
    return hr;
}






////////////////////////////////////////////////////////////////////////////////
//
//  ConfigDialogController::CancelLiveMode
//
//  Reverts ApplicationState to snapshot values and clears live mode state.
//  Called when user clicks Cancel in live overlay dialog.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT ConfigDialogController::CancelLiveMode()
{
    HRESULT hr = S_OK;
    
    
    
    // Verify we're in live mode
    CBRA (m_snapshot.isLiveMode);
    
    // Revert current settings to snapshot (undoing live preview changes)
    m_settings = m_snapshot.snapshotSettings;
    
    // TODO T052: Propagate snapshot settings back to ApplicationState
    // to visually revert animation to pre-dialog state
    
    // Clear live mode state
    m_snapshot.isLiveMode          = false;
    m_snapshot.applicationStateRef = nullptr;
    
    
Error:
    return hr;
}
