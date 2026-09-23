#pragma once

#include "IDisplayLuminanceProvider.h"





////////////////////////////////////////////////////////////////////////////////
//
//  WindowsDisplayLuminanceProvider
//
//  The production IDisplayLuminanceProvider (research R5, R6; contracts/
//  display-luminance-provider.md). For the output that contains the swap
//  chain's window it reads:
//
//    - whether Windows HDR is on, and the peak luminance and device name,
//      from IDXGIOutput6::GetDesc1;
//    - whether the swap chain can present scRGB, from
//      IDXGISwapChain3::CheckColorSpaceSupport;
//    - the SDR white level the user set with the Windows brightness slider,
//      from DisplayConfig, pairing the DXGI output with the DisplayConfig
//      source by GDI device name.
//
//  Every failure along the way yields the SDR answer (hdrEnabled and
//  scRgbSupported false, 80 nits) rather than an error: FR-015 says an HDR
//  problem must never take the rain down with it. Nothing here throws.
//
//  IsStale asks a DXGI factory of its own whether the display configuration
//  has changed since it was created; a stale factory keeps describing the
//  old configuration, so Query makes a fresh one whenever that says so.
//
////////////////////////////////////////////////////////////////////////////////

class WindowsDisplayLuminanceProvider : public IDisplayLuminanceProvider
{
public:
    DisplayLuminance Query   (IDXGISwapChain * pSwapChain) noexcept override;
    bool             IsStale () noexcept override;

private:
    HRESULT EnsureFactory() noexcept;

    Microsoft::WRL::ComPtr<IDXGIFactory1> m_factory;
};
