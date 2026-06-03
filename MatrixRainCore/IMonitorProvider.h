#pragma once

#include "MonitorInfo.h"




////////////////////////////////////////////////////////////////////////////////
//
//  IMonitorProvider
//
//  Abstract interface for enumerating connected displays.  Allows production
//  code to query the live desktop topology (Win32) while tests inject a fixed
//  list of monitors without touching real hardware.
//
////////////////////////////////////////////////////////////////////////////////

class IMonitorProvider
{
public:
    virtual ~IMonitorProvider() = default;

    // Returns one descriptor per connected display.  The primary display is
    // flagged via MonitorInfo::m_isPrimary.  Order is OS-defined.
    virtual std::vector<MonitorInfo> GetMonitors() const = 0;
};
