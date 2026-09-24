#include "pch.h"

#include "MonitorLayout.h"




////////////////////////////////////////////////////////////////////////////////
//
//  PlanFullscreenPlacements
//
//  Filters out degenerate (zero-or-negative size) monitors, selects the primary
//  (the first valid monitor flagged primary, else the first valid monitor), and
//  produces one placement per surviving monitor at its virtual-screen bounds.
//  Returns empty when no usable monitor is reported, so the caller can fall back
//  to a single window.
//
////////////////////////////////////////////////////////////////////////////////

std::vector<MonitorPlacement> PlanFullscreenPlacements (const std::vector<MonitorInfo> & monitors)
{
    std::vector<MonitorPlacement>    placements;
    std::vector<const MonitorInfo *> valid;
    size_t                           primaryIndex = 0;



    for (const MonitorInfo & monitor : monitors)
    {
        if (monitor.Width() > 0 && monitor.Height() > 0)
        {
            valid.push_back (&monitor);
        }
    }

    if (valid.empty())
    {
        return placements;
    }

    // Primary is the first VALID monitor flagged primary; if a flagged monitor
    // was dropped as degenerate, the first surviving monitor becomes primary.
    for (size_t i = 0; i < valid.size(); ++i)
    {
        if (valid[i]->m_isPrimary)
        {
            primaryIndex = i;
            break;
        }
    }

    for (size_t i = 0; i < valid.size(); ++i)
    {
        const MonitorInfo & monitor = *valid[i];

        MonitorPlacement placement;
        placement.position  = { monitor.m_bounds.left, monitor.m_bounds.top };
        placement.size      = { monitor.Width(), monitor.Height() };
        placement.isPrimary = (i == primaryIndex);

        placements.push_back (placement);
    }

    return placements;
}




////////////////////////////////////////////////////////////////////////////////
//
//  SameMonitorLayout
//
////////////////////////////////////////////////////////////////////////////////

bool SameMonitorLayout (const std::vector<MonitorInfo> & a, const std::vector<MonitorInfo> & b)
{
    if (a.size() != b.size())
    {
        return false;
    }

    for (size_t i = 0; i < a.size(); ++i)
    {
        const MonitorInfo & x = a[i];
        const MonitorInfo & y = b[i];


        if (x.m_bounds.left   != y.m_bounds.left   || x.m_bounds.top    != y.m_bounds.top
         || x.m_bounds.right  != y.m_bounds.right  || x.m_bounds.bottom != y.m_bounds.bottom
         || x.m_dpi           != y.m_dpi           || x.m_isPrimary     != y.m_isPrimary
         || x.m_refreshHz     != y.m_refreshHz)
        {
            return false;
        }
    }

    return true;
}
