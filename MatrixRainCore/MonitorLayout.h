#pragma once



#include "MonitorInfo.h"




////////////////////////////////////////////////////////////////////////////////
//
//  MonitorPlacement
//
//  One fullscreen window placement derived from a monitor descriptor: where the
//  borderless window goes, how large it is, and whether it is the primary.
//
////////////////////////////////////////////////////////////////////////////////

struct MonitorPlacement
{
    POINT position  = { 0, 0 };
    SIZE  size      = { 0, 0 };
    bool  isPrimary = false;
};




std::vector<MonitorPlacement> PlanFullscreenPlacements (const std::vector<MonitorInfo> & monitors);




////////////////////////////////////////////////////////////////////////////////
//
//  SameMonitorLayout
//
//  True when two enumerations describe the same monitors in the same order
//  with the same bounds, scaling, primary and refresh rate: everything a
//  render context is built from. Windows sends WM_DISPLAYCHANGE for more
//  than layout changes -- turning HDR on or off sends one too -- and a layout
//  that has not changed needs no rebuild; HDR is followed in place by each
//  context's output mode detection. Monitor handles are not compared.
//
////////////////////////////////////////////////////////////////////////////////

bool SameMonitorLayout (const std::vector<MonitorInfo> & a, const std::vector<MonitorInfo> & b);
