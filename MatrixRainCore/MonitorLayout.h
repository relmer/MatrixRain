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
