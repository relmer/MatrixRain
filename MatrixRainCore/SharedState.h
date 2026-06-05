#pragma once

#include "ColorScheme.h"
#include "QualityPresets.h"
#include "ScreenSaverSettings.h"





////////////////////////////////////////////////////////////////////////////////
//
//  SharedState — Thread-safe shared state between UI and render threads
//
//  All fields in this struct are written by the UI thread (hotkeys, config
//  dialog) and read by the render thread (Update/Render).  Access must be
//  protected by the embedded mutex.
//
//  UI thread pattern:
//      {
//          std::lock_guard<std::mutex> lock (m_sharedState.mutex);
//          m_sharedState.colorScheme = newScheme;
//      }
//
//  Render thread pattern (snapshot to minimize lock hold time):
//      SharedState::Snapshot snapshot;
//      {
//          std::lock_guard<std::mutex> lock (m_sharedState.mutex);
//          snapshot = m_sharedState.GetSnapshot();
//      }
//      Update (snapshot, deltaTime);
//      Render (snapshot);
//
////////////////////////////////////////////////////////////////////////////////

struct SharedState
{
    mutable std::mutex mutex;


    // Rain density (0-100%)
    int         densityPercent        = ScreenSaverSettings::DEFAULT_DENSITY_PERCENT;

    // Color scheme
    ColorScheme colorScheme           = ColorScheme::Green;

    // Animation speed (1-100%)
    int         animationSpeedPercent = ScreenSaverSettings::DEFAULT_ANIMATION_SPEED_PERCENT;

    // Bloom/glow parameters (stored as percentages matching ScreenSaverSettings)
    int         glowIntensityPercent  = ScreenSaverSettings::DEFAULT_GLOW_INTENSITY_PERCENT;
    int         glowSizePercent       = ScreenSaverSettings::DEFAULT_GLOW_SIZE_PERCENT;

    // Quality-preset-driven advanced graphics knobs.  The defaults here
    // mirror the High preset (today's hardcoded values) so the snapshot
    // produces the same output as v1.3 until the user changes something.
    int               blurPasses             = 3;
    ResolutionDivisor bloomResolutionDivisor = ResolutionDivisor::Half;
    BlurTaps          blurTaps               = BlurTaps::High;

    // Debug/statistics display
    bool        showStatistics        = false;

    // Pause state (spacebar) — broadcast to every monitor so all displays
    // freeze and resume their rain together.  Does not freeze elapsedTime, so
    // color cycling continues while paused.
    bool        isPaused              = false;

    // Monotonic seconds since start, used to cycle colors.  Advanced by the
    // owning (primary) render thread and read by every monitor so all displays
    // cycle color in sync.
    float       elapsedTime           = 0.0f;


    ////////////////////////////////////////////////////////////////////////////
    //
    //  Snapshot — Lock-free copy of SharedState for render thread consumption
    //
    ////////////////////////////////////////////////////////////////////////////

    struct Snapshot
    {
        int               densityPercent         = ScreenSaverSettings::DEFAULT_DENSITY_PERCENT;
        ColorScheme       colorScheme            = ColorScheme::Green;
        int               animationSpeedPercent  = ScreenSaverSettings::DEFAULT_ANIMATION_SPEED_PERCENT;
        int               glowIntensityPercent   = ScreenSaverSettings::DEFAULT_GLOW_INTENSITY_PERCENT;
        int               glowSizePercent        = ScreenSaverSettings::DEFAULT_GLOW_SIZE_PERCENT;
        int               blurPasses             = 3;
        ResolutionDivisor bloomResolutionDivisor = ResolutionDivisor::Half;
        BlurTaps          blurTaps               = BlurTaps::High;
        bool              showStatistics         = false;
        bool              isPaused               = false;
        float             elapsedTime            = 0.0f;
    };


    // Must be called while holding the mutex
    Snapshot GetSnapshot () const
    {
        return Snapshot
        {
            .densityPercent         = densityPercent,
            .colorScheme            = colorScheme,
            .animationSpeedPercent  = animationSpeedPercent,
            .glowIntensityPercent   = glowIntensityPercent,
            .glowSizePercent        = glowSizePercent,
            .blurPasses             = blurPasses,
            .bloomResolutionDivisor = bloomResolutionDivisor,
            .blurTaps               = blurTaps,
            .showStatistics         = showStatistics,
            .isPaused               = isPaused,
            .elapsedTime            = elapsedTime,
        };
    }
};
