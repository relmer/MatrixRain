#pragma once

#include "ApplicationState.h"
#include "ScreenSaverMode.h"

#include <optional>




////////////////////////////////////////////////////////////////////////////////
//
//  ShouldSpanAllMonitors
//
//  Pure decision: should MatrixRain create a render context per connected
//  monitor (true), or use a single primary-only window (false)?
//
//  Truth table:
//   - Preview (/p) and the /? usage view are always single (forced false,
//     regardless of the user's multi-monitor setting).
//   - Otherwise: spans all monitors iff the user has multi-monitor enabled
//     AND the application is currently in Fullscreen display mode.
//     (Windowed mode is always single.)
//
//  saverMode may be std::nullopt when no screensaver-mode context is
//  attached (the normal /c launch from Explorer-as-screensaver still goes
//  through a context, but unit tests can pass nullopt to model the
//  "non-screensaver invocation" case).
//
////////////////////////////////////////////////////////////////////////////////

bool ShouldSpanAllMonitors (bool                            multiMonEnabled,
                            DisplayMode                     displayMode,
                            std::optional<ScreenSaverMode>  saverMode);
