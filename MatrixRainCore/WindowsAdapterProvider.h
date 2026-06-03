#pragma once

#include "IAdapterProvider.h"




////////////////////////////////////////////////////////////////////////////////
//
//  WindowsAdapterProvider
//
//  Concrete IAdapterProvider that enumerates GPU adapters via DXGI.  Drives
//  the GPU dropdown in the configuration dialog and the adapter-resolution
//  path used by Application::Initialize / RebuildContextsForCurrentMode.
//
//  Implementation uses IDXGIFactory1::EnumAdapters1 for the basic list and
//  IDXGIFactory6::EnumAdapterByGpuPreference (UNSPECIFIED) to identify the
//  system default rendering adapter.  Software adapters
//  (DXGI_ADAPTER_FLAG_SOFTWARE - Microsoft Basic Render Driver / WARP) are
//  filtered out before being returned.
//
//  On any DXGI failure the provider returns an empty vector; callers then
//  use the default-adapter path (D3D11CreateDevice(nullptr, HARDWARE)).
//
////////////////////////////////////////////////////////////////////////////////

class WindowsAdapterProvider : public IAdapterProvider
{
public:
    std::vector<AdapterInfo> EnumerateAdapters() const override;
};
