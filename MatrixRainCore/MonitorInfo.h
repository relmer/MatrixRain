#pragma once




////////////////////////////////////////////////////////////////////////////////
//
//  MonitorInfo
//
//  Describes a single connected display: its bounds in virtual-screen
//  coordinates, per-monitor effective DPI, whether it is the primary display,
//  and the OS monitor handle.  Detection is driven by the Win32 desktop APIs
//  (EnumDisplayMonitors / GetMonitorInfo / GetDpiForMonitor), which are the
//  authoritative source of desktop topology for window placement.
//
////////////////////////////////////////////////////////////////////////////////

struct MonitorInfo
{
    RECT     m_bounds    { 0, 0, 0, 0 };
    UINT     m_dpi       { 96          };
    bool     m_isPrimary { false       };
    HMONITOR m_handle    { nullptr     };

    int Width()  const { return m_bounds.right  - m_bounds.left; }
    int Height() const { return m_bounds.bottom - m_bounds.top;  }
};
