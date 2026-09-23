#pragma once

#include "ColorMath.h"

struct IDXGISwapChain;





////////////////////////////////////////////////////////////////////////////////
//
//  IDisplayLuminanceProvider
//
//  The seam between mode selection and the OS (contracts/
//  display-luminance-provider.md). WindowsDisplayLuminanceProvider asks DXGI
//  and DisplayConfig; InMemoryDisplayLuminanceProvider answers from a script
//  so the tracker and the luminance math are tested without a display. Same
//  shape as IAdapterProvider / InMemoryAdapterProvider.
//
////////////////////////////////////////////////////////////////////////////////

class IDisplayLuminanceProvider
{
public:
    virtual ~IDisplayLuminanceProvider() = default;

    // A snapshot for the output that currently contains the swap chain's
    // window. Never throws. On any failure it reports hdrEnabled = false,
    // scRgbSupported = false and sdrWhiteNits = 80, which selects SDR
    // (FR-015). Cheap enough to call at 1 Hz per monitor from the render
    // thread, and it must not wait on the UI thread.
    virtual DisplayLuminance Query (IDXGISwapChain * pSwapChain) noexcept = 0;

    // True when the cached DXGI factory no longer reflects the display
    // configuration; the next Query refreshes it.
    virtual bool IsStale() noexcept = 0;
};
