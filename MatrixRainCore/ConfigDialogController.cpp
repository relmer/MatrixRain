#include "pch.h"

#include "ApplicationState.h"
#include "ConfigDialogController.h"





////////////////////////////////////////////////////////////////////////////////
//
//  ConfigDialogController::ConfigDialogController
//
////////////////////////////////////////////////////////////////////////////////

ConfigDialogController::ConfigDialogController (ISettingsProvider & settingsProvider) :
    m_settingsProvider (settingsProvider)
{
}




////////////////////////////////////////////////////////////////////////////////
//
//  ConfigDialogController::Initialize
//
//  Loads settings from the provider, falling back to defaults if the
//  load fails for any reason.  Always returns S_OK — worst case we
//  proceed with defaults rather than refuse to open the dialog.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT ConfigDialogController::Initialize()
{
    HRESULT hr = S_OK;


    hr = m_settingsProvider.Load (m_settings);

    // hr == S_FALSE means key didn't exist, used defaults (not an error)
    // hr == S_OK means loaded from registry successfully
    // Any other HRESULT is an actual error, but we continue with defaults
    if (FAILED (hr))
    {
        m_settings = ScreenSaverSettings();
    }

    m_originalSettings = m_settings;

    // No Error: label here — the function has no CHR/CBR call paths, and
    // /WX would reject the unreferenced label.  Adding one in the future
    // is trivial: change the last assignment to a CHR site, label here.
    return S_OK;
}




////////////////////////////////////////////////////////////////////////////////
//
//  ConfigDialogController::UpdateDensity
//
////////////////////////////////////////////////////////////////////////////////

void ConfigDialogController::UpdateDensity (int densityPercent)
{
    m_settings.m_densityPercent = ScreenSaverSettings::ClampDensityPercent (densityPercent);

    if (m_snapshot.isLiveMode && m_snapshot.applicationStateRef)
    {
        m_snapshot.applicationStateRef->OnDensityChanged (m_settings.m_densityPercent);
    }
}




////////////////////////////////////////////////////////////////////////////////
//
//  ConfigDialogController::UpdateColorScheme
//
////////////////////////////////////////////////////////////////////////////////

void ConfigDialogController::UpdateColorScheme (const std::wstring & colorSchemeKey)
{
    if (IsValidColorSchemeKey (colorSchemeKey))
    {
        std::wstring normalizedKey = colorSchemeKey;


        std::transform (normalizedKey.begin(), normalizedKey.end(), normalizedKey.begin(), ::towlower);
        m_settings.m_colorSchemeKey = normalizedKey;

        if (m_snapshot.isLiveMode && m_snapshot.applicationStateRef)
        {
            m_snapshot.applicationStateRef->SetColorScheme (ParseColorSchemeKey (normalizedKey));
        }
    }
}




////////////////////////////////////////////////////////////////////////////////
//
//  ConfigDialogController::UpdateAnimationSpeed
//
////////////////////////////////////////////////////////////////////////////////

void ConfigDialogController::UpdateAnimationSpeed (int animationSpeedPercent)
{
    m_settings.m_animationSpeedPercent = ScreenSaverSettings::ClampPercent (animationSpeedPercent,
                                                                            ScreenSaverSettings::MIN_ANIMATION_SPEED_PERCENT,
                                                                            ScreenSaverSettings::MAX_ANIMATION_SPEED_PERCENT);

    if (m_snapshot.isLiveMode && m_snapshot.applicationStateRef)
    {
        m_snapshot.applicationStateRef->SetAnimationSpeed (m_settings.m_animationSpeedPercent);
    }
}




////////////////////////////////////////////////////////////////////////////////
//
//  ConfigDialogController::UpdateGlowIntensity
//
////////////////////////////////////////////////////////////////////////////////

void ConfigDialogController::UpdateGlowIntensity (int glowIntensityPercent)
{
    m_settings.m_glowIntensityPercent = ScreenSaverSettings::ClampPercent (glowIntensityPercent,
                                                                           ScreenSaverSettings::MIN_GLOW_INTENSITY_PERCENT,
                                                                           ScreenSaverSettings::MAX_GLOW_INTENSITY_PERCENT);

    if (m_snapshot.isLiveMode && m_snapshot.applicationStateRef)
    {
        m_snapshot.applicationStateRef->SetGlowIntensity (m_settings.m_glowIntensityPercent);
    }
}




////////////////////////////////////////////////////////////////////////////////
//
//  ConfigDialogController::UpdateGlowSize
//
////////////////////////////////////////////////////////////////////////////////

void ConfigDialogController::UpdateGlowSize (int glowSizePercent)
{
    m_settings.m_glowSizePercent = ScreenSaverSettings::ClampPercent (glowSizePercent,
                                                                      ScreenSaverSettings::MIN_GLOW_SIZE_PERCENT,
                                                                      ScreenSaverSettings::MAX_GLOW_SIZE_PERCENT);

    if (m_snapshot.isLiveMode && m_snapshot.applicationStateRef)
    {
        m_snapshot.applicationStateRef->SetGlowSize (m_settings.m_glowSizePercent);
    }
}




////////////////////////////////////////////////////////////////////////////////
//
//  ConfigDialogController::UpdateStartFullscreen
//
////////////////////////////////////////////////////////////////////////////////

void ConfigDialogController::UpdateStartFullscreen (bool startFullscreen)
{
    m_settings.m_startFullscreen = startFullscreen;
}




////////////////////////////////////////////////////////////////////////////////
//
//  ConfigDialogController::UpdateMultiMonitorEnabled
//
////////////////////////////////////////////////////////////////////////////////

void ConfigDialogController::UpdateMultiMonitorEnabled (bool multiMonitorEnabled)
{
    m_settings.m_multiMonitorEnabled = multiMonitorEnabled;
}




////////////////////////////////////////////////////////////////////////////////
//
//  ConfigDialogController::UpdateGpuAdapter
//
////////////////////////////////////////////////////////////////////////////////

void ConfigDialogController::UpdateGpuAdapter (const std::wstring & description)
{
    m_settings.m_gpuAdapter = description;
}




////////////////////////////////////////////////////////////////////////////////
//
//  ConfigDialogController::UpdateQualityPreset
//
////////////////////////////////////////////////////////////////////////////////

void ConfigDialogController::UpdateQualityPreset (QualityPreset preset)
{
    m_settings.m_qualityPreset  = preset;
    m_settings.m_advancedValues = ApplyPresetSnap (preset, m_settings.m_advancedValues, m_settings.m_lastCustom);

    // Mirror the snapped glow intensity into the legacy top-level field so
    // RegistrySettingsProvider::Save persists the preset value.  See
    // ApplicationState::ApplyFirstRunQualityPreset for the same rationale.
    m_settings.m_glowIntensityPercent = m_settings.m_advancedValues.m_glowIntensityPercent;

    // Live mode: push the snapped advanced values to ApplicationState so
    // SharedState picks them up via the registered callback and the render
    // thread renders the new preset on the next frame (US5 live preview).
    if (m_snapshot.isLiveMode && m_snapshot.applicationStateRef)
    {
        m_snapshot.applicationStateRef->SetAdvancedGraphics (m_settings.m_advancedValues);
    }
}




////////////////////////////////////////////////////////////////////////////////
//
//  ConfigDialogController::UpdateAdvancedGraphicsValues
//
////////////////////////////////////////////////////////////////////////////////

void ConfigDialogController::UpdateAdvancedGraphicsValues (const AdvancedGraphicsValues & values)
{
    m_settings.m_advancedValues = values;

    // Per the locked custom-drift behavior: any direct advanced edit
    // updates LastCustom (always - even if the values happen to coincide
    // with a named preset row, the fact that the user touched a knob
    // makes their current state the canonical "last custom" set).
    m_settings.m_lastCustom = values;

    // Mirror glow intensity into the legacy top-level field (see
    // UpdateQualityPreset for rationale).
    m_settings.m_glowIntensityPercent = values.m_glowIntensityPercent;

    // Recompute the displayed preset selection (typically flips to Custom).
    m_settings.m_qualityPreset = DetectActivePreset (values);

    if (m_snapshot.isLiveMode && m_snapshot.applicationStateRef)
    {
        m_snapshot.applicationStateRef->SetAdvancedGraphics (values);
    }
}




////////////////////////////////////////////////////////////////////////////////
//
//  ConfigDialogController::UpdateShowDebugStats
//
////////////////////////////////////////////////////////////////////////////////

void ConfigDialogController::UpdateShowDebugStats (bool showDebugStats)
{
    m_settings.m_showDebugStats = showDebugStats;
}




////////////////////////////////////////////////////////////////////////////////
//
//  ConfigDialogController::UpdateShowFadeTimers
//
////////////////////////////////////////////////////////////////////////////////

void ConfigDialogController::UpdateShowFadeTimers (bool showFadeTimers)
{
    m_settings.m_showFadeTimers = showFadeTimers;
}




////////////////////////////////////////////////////////////////////////////////
//
//  ConfigDialogController::ApplyChanges
//
//  Persists the working settings to the registry and snapshots them as
//  the new originalSettings so a subsequent CancelChanges reverts to
//  this state rather than to whatever was on disk at dialog open.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT ConfigDialogController::ApplyChanges()
{
    HRESULT hr = S_OK;


    hr = m_settingsProvider.Save (m_settings);
    CHR (hr);

    m_originalSettings = m_settings;


Error:
    return hr;
}




////////////////////////////////////////////////////////////////////////////////
//
//  ConfigDialogController::CancelChanges
//
////////////////////////////////////////////////////////////////////////////////

void ConfigDialogController::CancelChanges()
{
    m_settings = m_originalSettings;
}




////////////////////////////////////////////////////////////////////////////////
//
//  ConfigDialogController::ResetToDefaults
//
////////////////////////////////////////////////////////////////////////////////

void ConfigDialogController::ResetToDefaults()
{
    m_settings = ScreenSaverSettings();
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
    hr = m_settingsProvider.Save (m_settings);
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
    
    // Propagate snapshot settings back to ApplicationState to visually revert animation
    if (m_snapshot.applicationStateRef)
    {
        m_snapshot.applicationStateRef->ApplySettings (m_snapshot.snapshotSettings);
    }
    
    // Clear live mode state
    m_snapshot.isLiveMode          = false;
    m_snapshot.applicationStateRef = nullptr;
    
    
Error:
    return hr;
}
