#pragma once

#include "IMonitorProvider.h"




////////////////////////////////////////////////////////////////////////////////
//
//  WindowsMonitorProvider
//
//  Production IMonitorProvider backed by the Win32 desktop APIs.  Enumerates
//  displays with EnumDisplayMonitors, reads each monitor's bounds and primary
//  flag via GetMonitorInfo, and queries per-monitor effective DPI with
//  GetDpiForMonitor.  This is the authoritative topology used for window
//  placement; DXGI output enumeration is deliberately avoided here because it
//  undercounts on hybrid-GPU laptops.
//
////////////////////////////////////////////////////////////////////////////////

class WindowsMonitorProvider : public IMonitorProvider
{
public:
    std::vector<MonitorInfo> GetMonitors() const override;

private:
    static BOOL CALLBACK EnumProc (HMONITOR hMonitor, HDC hdc, LPRECT lprcMonitor, LPARAM dwData);
};
