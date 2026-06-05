#pragma once

#include "ColorScheme.h"
#include "ConfigDialogSnapshot.h"
#include "ISettingsProvider.h"
#include "ScreenSaverSettings.h"





class ApplicationState;





/// <summary>
/// Presenter/controller for the screensaver configuration dialog.
/// Implements MVP pattern: manages settings validation, persistence coordination,
/// and provides interface between dialog UI and RegistrySettingsProvider.
/// </summary>
class ConfigDialogController
{
public:
    explicit ConfigDialogController (ISettingsProvider & settingsProvider);

    /// <summary>
    /// Load current settings from provider.
    /// Falls back to defaults if no saved settings exist.
    /// </summary>
    /// <returns>S_OK on success, error HRESULT otherwise</returns>
    HRESULT Initialize();

    /// <summary>
    /// Update density percentage with validation and clamping.
    /// </summary>
    /// <param name="densityPercent">Density value (clamped to 0-100)</param>
    void UpdateDensity (int densityPercent);

    /// <summary>
    /// Update color scheme key. Rejects invalid schemes.
    /// </summary>
    /// <param name="colorSchemeKey">Color scheme identifier (green, blue, red, amber, cycle)</param>
    void UpdateColorScheme (const std::wstring & colorSchemeKey);

    /// <summary>
    /// Update animation speed percentage with validation and clamping.
    /// </summary>
    /// <param name="animationSpeedPercent">Animation speed value (clamped to 1-100)</param>
    void UpdateAnimationSpeed (int animationSpeedPercent);

    /// <summary>
    /// Update glow intensity percentage with validation and clamping.
    /// </summary>
    /// <param name="glowIntensityPercent">Glow intensity value (clamped to 0-200)</param>
    void UpdateGlowIntensity (int glowIntensityPercent);

    /// <summary>
    /// Update glow size percentage with validation and clamping.
    /// </summary>
    /// <param name="glowSizePercent">Glow size value (clamped to 50-200)</param>
    void UpdateGlowSize (int glowSizePercent);

    /// <summary>
    /// Update start fullscreen flag.
    /// </summary>
    /// <param name="startFullscreen">True to start in fullscreen mode</param>
    void UpdateStartFullscreen (bool startFullscreen);

    /// <summary>
    /// Update multi-monitor enabled flag.  When true, MatrixRain creates a
    /// render context per connected monitor in fullscreen mode; when false,
    /// it uses a single primary-only window.  The dialog handler is
    /// expected to post WM_APP_REBUILD_CONTEXTS after calling this so the
    /// running app picks up the new gate decision (see FR-003).  The
    /// CancelLiveMode path automatically reverts via the snapshot, but the
    /// dialog handler must likewise post a rebuild on Cancel for the
    /// reverted setting to take visual effect (see FR-031b).
    /// </summary>
    /// <param name="multiMonitorEnabled">True to span all monitors</param>
    void UpdateMultiMonitorEnabled (bool multiMonitorEnabled);

    /// <summary>
    /// Update the user-selected GPU adapter (by DXGI description string).
    /// Empty string means "use the system default adapter".  The dialog
    /// handler is expected to post WM_APP_REBUILD_CONTEXTS after calling
    /// this so the running app rebuilds its contexts on the new device
    /// (FR-015).  Cancel-revert is automatic via the snapshot, but the
    /// dialog handler must post a rebuild on Cancel for the reverted
    /// adapter selection to take visual effect (FR-031b).
    /// </summary>
    /// <param name="description">DXGI adapter description (Empty for default)</param>
    void UpdateGpuAdapter (const std::wstring & description);

    /// <summary>
    /// Update the current quality preset.  When set to a named preset,
    /// also snaps m_advancedValues to that preset's lookup row (so the
    /// dialog can reflect them back into the advanced sliders).  When
    /// set to Custom, restores LastCustom if saved, else leaves the
    /// advanced values at their current state.  Cancel-revert is
    /// automatic via the snapshot.
    /// </summary>
    void UpdateQualityPreset (QualityPreset preset);

    /// <summary>
    /// Update the four advanced graphics control values as a unit.  Always
    /// updates LastCustom (FR-023) AND recomputes m_qualityPreset via
    /// DetectActivePreset so the dialog can refresh the preset combo
    /// (typically flipping it to Custom when knobs drift off the table).
    /// </summary>
    void UpdateAdvancedGraphicsValues (const AdvancedGraphicsValues & values);

    /// <summary>
    /// Update show debug stats flag.
    /// </summary>
    /// <param name="showDebugStats">True to show debug statistics</param>
    void UpdateShowDebugStats (bool showDebugStats);

    // v1.5 setters (US1 T025, FR-004, FR-020, FR-027, FR-028, FR-030,
    // FR-033, FR-044) — mutate m_settings, clamp where applicable, and
    // (in live mode) propagate via m_snapshot.applicationStateRef.
    void UpdateGlowEnabled         (bool     enabled);
    void UpdateScanlinesEnabled    (bool     enabled);
    void UpdateScanlinesIntensity  (int      intensityPercent);
    void UpdateScanlinesStyle      (int      style);
    void UpdateCustomColor         (COLORREF color);

    /// <summary>
    /// Apply and persist all changes to registry.
    /// </summary>
    /// <returns>S_OK on success, error HRESULT otherwise</returns>
    HRESULT ApplyChanges();

    /// <summary>
    /// Discard pending changes (does not modify registry).
    /// </summary>
    void CancelChanges();

    /// <summary>
    /// Reset all settings to factory defaults.
    /// </summary>
    void ResetToDefaults();

    /// <summary>
    /// Get current settings (with pending modifications).
    /// </summary>
    /// <returns>Current settings structure</returns>
    const ScreenSaverSettings & GetSettings() const { return m_settings; }

    /// <summary>
    /// Initialize live overlay mode with ApplicationState reference.
    /// Creates snapshot for Cancel rollback and enables immediate updates.
    /// </summary>
    /// <param name="appState">Non-null ApplicationState pointer for real-time updates</param>
    /// <returns>S_OK on success, E_POINTER if appState is null, error HRESULT otherwise</returns>
    HRESULT InitializeLiveMode (ApplicationState * appState);

    /// <summary>
    /// Apply live mode changes (persist to registry and clear snapshot).
    /// </summary>
    /// <returns>S_OK on success, E_FAIL if not in live mode, error HRESULT otherwise</returns>
    HRESULT ApplyLiveMode();

    /// <summary>
    /// T033a (US1, FR-004a, SC-011): commit live-mode changes from the
    /// property-sheet `PSN_APPLY` path.  Persists every rollback-eligible
    /// field (the 5 new v1.5 fields PLUS the v1.4 fields that participate
    /// in live-mode rollback) to the registry via
    /// `m_settingsProvider.Save(m_settings)` and clears the snapshot.
    /// Functionally identical to `ApplyLiveMode()`; named to match the
    /// dismissal-semantics contract in `contracts/propertysheet.md` so the
    /// dialog code reads cleanly.
    /// </summary>
    HRESULT CommitLiveMode() { return ApplyLiveMode(); }

    /// <summary>
    /// Cancel live mode changes (revert ApplicationState to snapshot and clear).
    /// </summary>
    /// <returns>S_OK on success, E_FAIL if not in live mode</returns>
    HRESULT CancelLiveMode();

    /// <summary>
    /// Check if controller is in live overlay mode.
    /// </summary>
    /// <returns>True if live mode active, false for modal mode</returns>
    bool IsLiveMode() const { return m_snapshot.isLiveMode; }

    /// <summary>
    /// True if the currently-pending settings differ from the live-mode
    /// snapshot in any field that requires the monitor render contexts to
    /// be torn down and rebuilt (multi-monitor span, or selected GPU
    /// adapter).  Used by the dialog to suppress an unnecessary
    /// destroy/recreate flicker on OK / Cancel when nothing rebuild-worthy
    /// has changed.
    /// </summary>
    bool LiveModeRebuildRequired() const
    {
        if (!m_snapshot.isLiveMode) return false;
        if (m_settings.m_multiMonitorEnabled != m_snapshot.snapshotSettings.m_multiMonitorEnabled) return true;
        if (m_settings.m_gpuAdapter          != m_snapshot.snapshotSettings.m_gpuAdapter)          return true;
        return false;
    }

private:
    ISettingsProvider   & m_settingsProvider;            // Settings provider (not owned)
    ScreenSaverSettings  m_settings;                    // Current settings (may include pending changes)
    ScreenSaverSettings  m_originalSettings;            // Original settings loaded from provider
    ConfigDialogSnapshot m_snapshot;                    // Snapshot for live mode Cancel rollback
};


