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
    HRESULT                  hr       = S_OK;
    BOOL                     fSuccess = FALSE;
    std::vector<MonitorInfo> monitors;


    // EnumDisplayMonitors invokes EnumProc once per connected display.  On
    // failure we return whatever was collected so far; callers fall back to
    // primary-only placement.
    fSuccess = EnumDisplayMonitors (nullptr, nullptr, &WindowsMonitorProvider::EnumProc,
                                    reinterpret_cast<LPARAM> (&monitors));
    CWRA (fSuccess);

Error:
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
    HRESULT                    hr        = S_OK;
    BOOL                       fSuccess  = FALSE;
    std::vector<MonitorInfo> * pMonitors = reinterpret_cast<std::vector<MonitorInfo> *> (dwData);
    MONITORINFO                info      = { sizeof (MONITORINFO) };
    UINT                       dpiX      = 96;
    UINT                       dpiY      = 96;
    MonitorInfo                descriptor;


    UNREFERENCED_PARAMETER (hdc);
    UNREFERENCED_PARAMETER (lprcMonitor);


    // A missing accumulator is a caller programming error: abort the entire
    // enumeration.  Handled outside EHM because the Error path below
    // intentionally continues enumeration for per-monitor failures.
    if (pMonitors == nullptr)
    {
        return FALSE;
    }

    // Read this monitor's geometry; on failure skip it but keep enumerating.
    // A handle EnumDisplayMonitors just produced should never be unreadable,
    // so assert at the point of failure while still degrading gracefully.
    fSuccess = GetMonitorInfoW (hMonitor, &info);
    CWRA (fSuccess);

    // Per-monitor effective DPI is best-effort; fall back to 96 on failure.
    if (FAILED (GetDpiForMonitor (hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY)))
    {
        dpiX = 96;
    }

    descriptor.m_bounds    = info.rcMonitor;
    descriptor.m_dpi       = dpiX;
    descriptor.m_isPrimary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;
    descriptor.m_handle    = hMonitor;

    pMonitors->push_back (descriptor);

Error:
    // A per-monitor failure skips that monitor; enumeration always continues.
    return TRUE;
}
