#pragma once

#include "ColorScheme.h"
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

    // Debug/statistics display
    bool        showStatistics        = false;
    bool        showDebugFadeTimes    = false;


    ////////////////////////////////////////////////////////////////////////////
    //
    //  Snapshot — Lock-free copy of SharedState for render thread consumption
    //
    ////////////////////////////////////////////////////////////////////////////

    struct Snapshot
    {
        int         densityPercent        = ScreenSaverSettings::DEFAULT_DENSITY_PERCENT;
        ColorScheme colorScheme           = ColorScheme::Green;
        int         animationSpeedPercent = ScreenSaverSettings::DEFAULT_ANIMATION_SPEED_PERCENT;
        int         glowIntensityPercent  = ScreenSaverSettings::DEFAULT_GLOW_INTENSITY_PERCENT;
        int         glowSizePercent       = ScreenSaverSettings::DEFAULT_GLOW_SIZE_PERCENT;
        bool        showStatistics        = false;
        bool        showDebugFadeTimes    = false;
    };


    // Must be called while holding the mutex
    Snapshot GetSnapshot () const
    {
        return Snapshot
        {
            .densityPercent        = densityPercent,
            .colorScheme           = colorScheme,
            .animationSpeedPercent = animationSpeedPercent,
            .glowIntensityPercent  = glowIntensityPercent,
            .glowSizePercent       = glowSizePercent,
            .showStatistics        = showStatistics,
            .showDebugFadeTimes    = showDebugFadeTimes,
        };
    }
};
