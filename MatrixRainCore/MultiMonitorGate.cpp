#include "pch.h"

#include "MultiMonitorGate.h"




////////////////////////////////////////////////////////////////////////////////
//
//  ShouldSpanAllMonitors
//
////////////////////////////////////////////////////////////////////////////////

bool ShouldSpanAllMonitors (bool                            multiMonEnabled,
                            DisplayMode                     displayMode,
                            std::optional<ScreenSaverMode>  saverMode)
{
    // Preview and Help views are explicitly single-window paths and the
    // user's multimon preference never applies to them.
    if (saverMode.has_value() &&
        (*saverMode == ScreenSaverMode::ScreenSaverPreview ||
         *saverMode == ScreenSaverMode::HelpRequested))
    {
        return false;
    }

    return multiMonEnabled && displayMode == DisplayMode::Fullscreen;
}
