#include "pch.h"

#include "WindowsMonitorProvider.h"





////////////////////////////////////////////////////////////////////////////////
//
//  WindowsMonitorProvider::GetMonitors
//
//  Enumerates every connected display via EnumDisplayMonitors and returns a
//  descriptor for each.  The accumulating vector is threaded through the
//  callback's LPARAM.
//
////////////////////////////////////////////////////////////////////////////////

std::vector<MonitorInfo> WindowsMonitorProvider::GetMonitors() const
{
    std::vector<MonitorInfo> monitors;


    EnumDisplayMonitors (nullptr, nullptr, &WindowsMonitorProvider::EnumProc, reinterpret_cast<LPARAM> (&monitors));

    return monitors;
}





////////////////////////////////////////////////////////////////////////////////
//
//  WindowsMonitorProvider::EnumProc
//
//  EnumDisplayMonitors callback.  Reads the monitor's bounds and primary flag
//  via GetMonitorInfo and its effective DPI via GetDpiForMonitor, then appends
//  a descriptor to the vector referenced by dwData.
//
////////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK WindowsMonitorProvider::EnumProc (HMONITOR hMonitor, HDC hdc, LPRECT lprcMonitor, LPARAM dwData)
{
    UNREFERENCED_PARAMETER (hdc);
    UNREFERENCED_PARAMETER (lprcMonitor);


    std::vector<MonitorInfo> * pMonitors = reinterpret_cast<std::vector<MonitorInfo> *> (dwData);
    MONITORINFO                info       = { sizeof (MONITORINFO) };
    UINT                       dpiX       = 96;
    UINT                       dpiY       = 96;


    if (pMonitors == nullptr)
    {
        return FALSE;
    }

    if (!GetMonitorInfoW (hMonitor, &info))
    {
        // Skip a monitor we cannot describe rather than abort the whole enum.
        return TRUE;
    }

    if (FAILED (GetDpiForMonitor (hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY)))
    {
        dpiX = 96;
    }

    MonitorInfo descriptor;
    descriptor.m_bounds    = info.rcMonitor;
    descriptor.m_dpi       = dpiX;
    descriptor.m_isPrimary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;
    descriptor.m_handle    = hMonitor;

    pMonitors->push_back (descriptor);

    return TRUE;
}
