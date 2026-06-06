#include "pch.h"

#include "ApplicationState.h"
#include "ConfigDialogController.h"





ConfigDialogController::ConfigDialogController (ISettingsProvider & settingsProvider) :
    m_settingsProvider (settingsProvider)
{
}





HRESULT ConfigDialogController::Initialize()
{
    HRESULT hr = S_OK;
    
    
    
    // Load settings from provider (falls back to defaults if no data exists)
    hr = m_settingsProvider.Load (m_settings);
    
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
    
    // Propagate to ApplicationState in live mode
    if (m_snapshot.isLiveMode && m_snapshot.applicationStateRef)
    {
        m_snapshot.applicationStateRef->OnDensityChanged (m_settings.m_densityPercent);
    }
}





void ConfigDialogController::UpdateColorScheme (const std::wstring & colorSchemeKey)
{
    // Only update if valid color scheme
    if (IsValidColorSchemeKey (colorSchemeKey))
    {
        // Normalize to lowercase for storage
        std::wstring normalizedKey = colorSchemeKey;
        std::transform (normalizedKey.begin(), normalizedKey.end(), normalizedKey.begin(), ::towlower);
        m_settings.m_colorSchemeKey = normalizedKey;
        
        // Propagate to ApplicationState in live mode
        if (m_snapshot.isLiveMode && m_snapshot.applicationStateRef)
        {
            m_snapshot.applicationStateRef->SetColorScheme (ParseColorSchemeKey (normalizedKey));
        }
    }
}





void ConfigDialogController::UpdateAnimationSpeed (int animationSpeedPercent)
{
    m_settings.m_animationSpeedPercent = ScreenSaverSettings::ClampPercent (animationSpeedPercent, 
                                                                             ScreenSaverSettings::MIN_ANIMATION_SPEED_PERCENT, 
                                                                             ScreenSaverSettings::MAX_ANIMATION_SPEED_PERCENT);
    
    // Propagate to ApplicationState in live mode
    if (m_snapshot.isLiveMode && m_snapshot.applicationStateRef)
    {
        m_snapshot.applicationStateRef->SetAnimationSpeed (m_settings.m_animationSpeedPercent);
    }
}





void ConfigDialogController::UpdateGlowIntensity (int glowIntensityPercent)
{
    m_settings.m_glowIntensityPercent = ScreenSaverSettings::ClampPercent (glowIntensityPercent, 
                                                                            ScreenSaverSettings::MIN_GLOW_INTENSITY_PERCENT, 
                                                                            ScreenSaverSettings::MAX_GLOW_INTENSITY_PERCENT);
    
    // Propagate to ApplicationState in live mode
    if (m_snapshot.isLiveMode && m_snapshot.applicationStateRef)
    {
        m_snapshot.applicationStateRef->SetGlowIntensity (m_settings.m_glowIntensityPercent);
    }
}





void ConfigDialogController::UpdateGlowSize (int glowSizePercent)
{
    m_settings.m_glowSizePercent = ScreenSaverSettings::ClampPercent (glowSizePercent, 
                                                                      ScreenSaverSettings::MIN_GLOW_SIZE_PERCENT, 
                                                                      ScreenSaverSettings::MAX_GLOW_SIZE_PERCENT);
    
    // Propagate to ApplicationState in live mode
    if (m_snapshot.isLiveMode && m_snapshot.applicationStateRef)
    {
        m_snapshot.applicationStateRef->SetGlowSize (m_settings.m_glowSizePercent);
    }
}





void ConfigDialogController::UpdateStartFullscreen (bool startFullscreen)
{
    m_settings.m_startFullscreen = startFullscreen;

    if (m_snapshot.isLiveMode && m_snapshot.applicationStateRef)
    {
        m_snapshot.applicationStateRef->ApplySettings (m_settings);
    }
}





void ConfigDialogController::UpdateMultiMonitorEnabled (bool multiMonitorEnabled)
{
    m_settings.m_multiMonitorEnabled = multiMonitorEnabled;
}





void ConfigDialogController::UpdateGpuAdapter (const std::wstring & description)
{
    m_settings.m_gpuAdapter = description;
}





void ConfigDialogController::UpdateQualityPreset (QualityPreset preset)
{
    m_settings.m_qualityPreset  = preset;
    m_settings.m_advancedValues = ApplyPresetSnap (preset, m_settings.m_advancedValues, m_settings.m_lastCustom);

    // Live mode: push the snapped advanced values to ApplicationState so
    // SharedState picks them up via the registered callback and the render
    // thread renders the new preset on the next frame (US5 live preview).
    if (m_snapshot.isLiveMode && m_snapshot.applicationStateRef)
    {
        m_snapshot.applicationStateRef->SetAdvancedGraphics (m_settings.m_advancedValues);
    }
}





void ConfigDialogController::UpdateAdvancedGraphicsValues (const AdvancedGraphicsValues & values)
{
    m_settings.m_advancedValues = values;

    // Per the locked custom-drift behavior: any direct advanced edit
    // updates LastCustom (always - even if the values happen to coincide
    // with a named preset row, the fact that the user touched a knob
    // makes their current state the canonical "last custom" set).
    m_settings.m_lastCustom = values;

    // Recompute the displayed preset selection (typically flips to Custom).
    m_settings.m_qualityPreset = DetectActivePreset (values);

    if (m_snapshot.isLiveMode && m_snapshot.applicationStateRef)
    {
        m_snapshot.applicationStateRef->SetAdvancedGraphics (values);
    }
}




void ConfigDialogController::UpdateShowDebugStats (bool showDebugStats)
{
    m_settings.m_showDebugStats = showDebugStats;
}





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





HRESULT ConfigDialogController::ApplyChanges()
{
    HRESULT hr = S_OK;
    
    
    
    // Persist settings to registry
    hr = m_settingsProvider.Save (m_settings);
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
