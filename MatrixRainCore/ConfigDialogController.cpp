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


// Error:    // /WX would reject the unreferenced label; uncomment when the
            // first CHR/CBR/CWR call lands here.
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

    if (m_snapshot.isLiveMode && m_snapshot.applicationStateRef)
    {
        m_snapshot.applicationStateRef->ApplySettings (m_settings);
    }
}




////////////////////////////////////////////////////////////////////////////////
//
//  ConfigDialogController::UpdateMultiMonitorEnabled
//
////////////////////////////////////////////////////////////////////////////////

void ConfigDialogController::UpdateMultiMonitorEnabled (bool multiMonitorEnabled)
{
    m_settings.m_multiMonitorEnabled = multiMonitorEnabled;

    if (m_snapshot.isLiveMode && m_snapshot.applicationStateRef)
    {
        m_snapshot.applicationStateRef->ApplySettings (m_settings);
    }
}




////////////////////////////////////////////////////////////////////////////////
//
//  ConfigDialogController::UpdateGpuAdapter
//
////////////////////////////////////////////////////////////////////////////////

void ConfigDialogController::UpdateGpuAdapter (const std::wstring & description)
{
    m_settings.m_gpuAdapter = description;

    if (m_snapshot.isLiveMode && m_snapshot.applicationStateRef)
    {
        m_snapshot.applicationStateRef->ApplySettings (m_settings);
    }
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

////////////////////////////////////////////////////////////////////////////////
//
//  v1.5 setters (US1 T025, data-model.md §3/§4)
//
//  Each setter mutates m_settings and (in live mode) propagates the new
//  value to ApplicationState via ApplySettings so the SharedState mirror
//  picks it up on the next render-thread snapshot.  Until US2/US3/US5 wire
//  individual SharedState atomics, ApplySettings is the coarse-grained
//  propagation path — it copies the whole struct, which is cheap.
//
////////////////////////////////////////////////////////////////////////////////

void ConfigDialogController::UpdateGlowEnabled (bool enabled)
{
    m_settings.m_glowEnabled = enabled;

    if (m_snapshot.isLiveMode && m_snapshot.applicationStateRef)
    {
        m_snapshot.applicationStateRef->ApplySettings (m_settings);
    }
}





void ConfigDialogController::UpdateScanlinesEnabled (bool enabled)
{
    m_settings.m_scanlinesEnabled = enabled;

    if (m_snapshot.isLiveMode && m_snapshot.applicationStateRef)
    {
        m_snapshot.applicationStateRef->ApplySettings (m_settings);
    }
}





void ConfigDialogController::UpdateScanlinesIntensity (int intensityPercent)
{
    m_settings.m_scanlinesIntensity = ScreenSaverSettings::ClampPercent (intensityPercent,
                                                                          ScreenSaverSettings::MIN_SCANLINES_INTENSITY_PERCENT,
                                                                          ScreenSaverSettings::MAX_SCANLINES_INTENSITY_PERCENT);

    if (m_snapshot.isLiveMode && m_snapshot.applicationStateRef)
    {
        m_snapshot.applicationStateRef->ApplySettings (m_settings);
    }
}





void ConfigDialogController::UpdateScanlinesStyle (int style)
{
    m_settings.m_scanlinesStyle = ScreenSaverSettings::ClampPercent (style,
                                                                      ScreenSaverSettings::MIN_SCANLINES_STYLE,
                                                                      ScreenSaverSettings::MAX_SCANLINES_STYLE);

    if (m_snapshot.isLiveMode && m_snapshot.applicationStateRef)
    {
        m_snapshot.applicationStateRef->ApplySettings (m_settings);
    }
}





void ConfigDialogController::UpdateCustomColor (COLORREF color)
{
    m_settings.m_customColor = color;

    if (m_snapshot.isLiveMode && m_snapshot.applicationStateRef)
    {
        m_snapshot.applicationStateRef->ApplySettings (m_settings);
    }
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
    // FR-035 carve-out: the custom-color palette is INTENTIONALLY NOT
    // reset.  Capture it before zeroing m_settings (defaults-init wipes
    // the array to all zeros) and restore it on the freshly defaulted
    // struct.  Mirror onto the snapshot so a subsequent CancelLiveMode
    // doesn't observe a stale palette (the snapshot itself never carried
    // the palette before, but ResetToDefaults is now the one writer to it
    // that the snapshot can't otherwise see).  Without this, Reset->OK
    // would write the zeroed palette to the registry and the user's 16
    // saved swatches would be lost permanently.
    std::array<COLORREF, 16> palette = m_settings.m_customColorPalette;

    m_settings                                       = ScreenSaverSettings();
    m_settings.m_customColorPalette                  = palette;
    m_snapshot.snapshotSettings.m_customColorPalette = palette;

    // Mini-phase 2.5 (cross-page Reset button): propagate to ApplicationState
    // in live mode so the live preview snaps back instantly.  ApplySettings
    // is the same coarse-grained propagation path UpdateGlowEnabled and the
    // UpdateScanlines* setters already use; the render thread observes the
    // defaults on the next snapshot regardless of which page (if any) is
    // currently active when the user clicks the frame-scope Reset button.
    if (m_snapshot.isLiveMode && m_snapshot.applicationStateRef)
    {
        m_snapshot.applicationStateRef->ApplySettings (m_settings);
    }
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

    // FR-035 carve-out: the custom-color palette is INTENTIONALLY NOT
    // rollback-eligible.  Preserve the live palette across the snapshot
    // restore so it survives Cancel exactly like the registry copy does.
    {
        std::array<COLORREF, 16> livePalette = m_settings.m_customColorPalette;

        // Revert current settings to snapshot (undoing live preview changes)
        m_settings                       = m_snapshot.snapshotSettings;
        m_settings.m_customColorPalette  = livePalette;
        m_snapshot.snapshotSettings.m_customColorPalette = livePalette;
    }

    // Propagate snapshot settings back to ApplicationState to visually revert animation
    if (m_snapshot.applicationStateRef)
    {
        m_snapshot.applicationStateRef->ApplySettings (m_settings);
    }
    
    // Clear live mode state
    m_snapshot.isLiveMode          = false;
    m_snapshot.applicationStateRef = nullptr;
    
    
Error:
    return hr;
}
