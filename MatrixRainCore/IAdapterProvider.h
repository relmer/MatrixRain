#pragma once




////////////////////////////////////////////////////////////////////////////////
//
//  AdapterInfo
//
//  Describes one rendering-capable GPU adapter discovered at runtime.  Mirrors
//  the existing MonitorInfo / IMonitorProvider pattern.  Software adapters
//  (Microsoft Basic Render Driver / WARP) are filtered out by the concrete
//  provider before the list reaches consumers, so callers never see them and
//  do not need to re-filter.
//
////////////////////////////////////////////////////////////////////////////////

struct AdapterInfo
{
    std::wstring  m_description;     // DXGI_ADAPTER_DESC1::Description (UTF-16)
    LUID          m_luid             { 0, 0 };  // DXGI_ADAPTER_DESC1::AdapterLuid
    unsigned int  m_dedicatedVramMb  { 0     };
    bool          m_isSoftware       { false };
    bool          m_isDefault        { false };  // System default rendering adapter
};




////////////////////////////////////////////////////////////////////////////////
//
//  IAdapterProvider
//
//  Abstract enumeration of GPU adapters.  WindowsAdapterProvider drives this
//  via DXGI in production; InMemoryAdapterProvider drives it from a
//  test-supplied vector for unit tests.
//
////////////////////////////////////////////////////////////////////////////////

class IAdapterProvider
{
public:
    virtual ~IAdapterProvider() = default;

    // Enumerate all rendering-capable GPU adapters currently present on the
    // system.  Software adapters MUST be excluded.  Returns an empty vector
    // if no non-software adapters exist; callers then use the system default-
    // adapter path (D3D11CreateDevice(nullptr, HARDWARE)).
    virtual std::vector<AdapterInfo> EnumerateAdapters() const = 0;
};
