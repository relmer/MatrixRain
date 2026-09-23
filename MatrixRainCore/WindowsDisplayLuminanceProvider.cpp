#include "pch.h"

#include "WindowsDisplayLuminanceProvider.h"





////////////////////////////////////////////////////////////////////////////////
//
//  QuerySdrWhiteNits
//
//  The SDR white level for the DisplayConfig source whose GDI device name is
//  deviceName (the name DXGI gives the same output), in nits. 80 when the
//  source is not found or any call fails: that is scRGB's reference white,
//  and what a display with no HDR reports anyway.
//
//  DisplayConfig identifies a display by (adapter LUID, id) pairs, one for
//  the source (the GDI view, whose name we can match) and one for the target
//  (the physical output, which is what the white level belongs to). Both sit
//  on the same active path, so the source finds the target.
//
////////////////////////////////////////////////////////////////////////////////

static float QuerySdrWhiteNits (const std::wstring & deviceName) noexcept
{
    HRESULT                             hr        = S_OK;
    LONG                                status    = ERROR_SUCCESS;
    UINT32                              pathCount = 0;
    UINT32                              modeCount = 0;
    std::vector<DISPLAYCONFIG_PATH_INFO> paths;
    std::vector<DISPLAYCONFIG_MODE_INFO> modes;
    float                               nits      = DisplayLuminanceConstants::kScRgbWhiteNits;



    BAIL_OUT_IF (deviceName.empty(), S_OK);

    // The buffer sizes can change between the two calls if a display arrives
    // in the gap; the loop is the documented way to handle that.
    for (int attempt = 0; attempt < 3; ++attempt)
    {
        status = GetDisplayConfigBufferSizes (QDC_ONLY_ACTIVE_PATHS, &pathCount, &modeCount);
        CBRAEx (status == ERROR_SUCCESS, HRESULT_FROM_WIN32 (status));

        paths.resize (pathCount);
        modes.resize (modeCount);

        status = QueryDisplayConfig (QDC_ONLY_ACTIVE_PATHS, &pathCount, paths.data(), &modeCount, modes.data(), nullptr);

        if (status != ERROR_INSUFFICIENT_BUFFER)
        {
            break;
        }
    }

    CBRAEx (status == ERROR_SUCCESS, HRESULT_FROM_WIN32 (status));

    paths.resize (pathCount);

    for (const DISPLAYCONFIG_PATH_INFO & path : paths)
    {
        DISPLAYCONFIG_SOURCE_DEVICE_NAME sourceName = {};
        DISPLAYCONFIG_SDR_WHITE_LEVEL    whiteLevel = {};


        sourceName.header.type      = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
        sourceName.header.size      = sizeof (sourceName);
        sourceName.header.adapterId = path.sourceInfo.adapterId;
        sourceName.header.id        = path.sourceInfo.id;

        if (DisplayConfigGetDeviceInfo (&sourceName.header) != ERROR_SUCCESS)
        {
            continue;
        }

        if (deviceName != sourceName.viewGdiDeviceName)
        {
            continue;
        }

        whiteLevel.header.type      = DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL;
        whiteLevel.header.size      = sizeof (whiteLevel);
        whiteLevel.header.adapterId = path.targetInfo.adapterId;
        whiteLevel.header.id        = path.targetInfo.id;

        if (DisplayConfigGetDeviceInfo (&whiteLevel.header) != ERROR_SUCCESS)
        {
            break;
        }

        nits = SdrWhiteNitsFromDisplayConfig (whiteLevel.SDRWhiteLevel);
        break;
    }


Error:
    return nits;
}





////////////////////////////////////////////////////////////////////////////////
//
//  WindowsDisplayLuminanceProvider::EnsureFactory
//
////////////////////////////////////////////////////////////////////////////////

HRESULT WindowsDisplayLuminanceProvider::EnsureFactory() noexcept
{
    HRESULT hr = S_OK;



    if (m_factory && !m_factory->IsCurrent())
    {
        m_factory.Reset();
    }

    if (!m_factory)
    {
        hr = CreateDXGIFactory1 (IID_PPV_ARGS (&m_factory));
        CHRA (hr);
    }


Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  WindowsDisplayLuminanceProvider::IsStale
//
////////////////////////////////////////////////////////////////////////////////

bool WindowsDisplayLuminanceProvider::IsStale() noexcept
{
    if (!m_factory)
    {
        return true;
    }

    return !m_factory->IsCurrent();
}





////////////////////////////////////////////////////////////////////////////////
//
//  WindowsDisplayLuminanceProvider::Query
//
////////////////////////////////////////////////////////////////////////////////

DisplayLuminance WindowsDisplayLuminanceProvider::Query (IDXGISwapChain * pSwapChain) noexcept
{
    HRESULT                                hr           = S_OK;
    DisplayLuminance                       luminance;
    Microsoft::WRL::ComPtr<IDXGIOutput>    output;
    Microsoft::WRL::ComPtr<IDXGIOutput6>   output6;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain3;
    DXGI_OUTPUT_DESC1                      desc          = {};
    DXGI_SWAP_CHAIN_DESC                   swapChainDesc = {};
    UINT                                   supportFlags  = 0;



    CBRAEx (pSwapChain != nullptr, E_POINTER);

    // A stale factory is the sign the display configuration moved under us;
    // refresh it so the next IsStale answers about the new configuration.
    // The outputs below come from the swap chain, which DXGI keeps current.
    (void) EnsureFactory();

    hr = pSwapChain->GetContainingOutput (&output);
    CHRA (hr);

    hr = output.As (&output6);
    CHRA (hr);

    hr = output6->GetDesc1 (&desc);
    CHRA (hr);

    luminance.deviceName       = desc.DeviceName;
    luminance.hdrEnabled       = (desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
    luminance.reportedPeakNits = desc.MaxLuminance;

    hr = pSwapChain->QueryInterface (IID_PPV_ARGS (&swapChain3));
    CHRA (hr);

    // CheckColorSpaceSupport answers for the swap chain's CURRENT back buffer
    // format, and scRGB needs 16-bit float; asked while the buffers are still
    // 8-bit it always says no. So the answer here is definitive only once the
    // buffers are float. Before that, having IDXGISwapChain3 at all is the
    // test that DXGI can present scRGB, and RenderSystem::ResizeBackBuffer
    // makes the format-specific check after resizing, falling back to SDR if
    // it fails. That fallback is what the tracker records as a refusal.
    hr = pSwapChain->GetDesc (&swapChainDesc);
    CHRA (hr);

    if (swapChainDesc.BufferDesc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT)
    {
        hr = swapChain3->CheckColorSpaceSupport (DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709, &supportFlags);
        CHRA (hr);

        luminance.scRgbSupported = (supportFlags & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) != 0;
    }
    else
    {
        luminance.scRgbSupported = true;
    }

    luminance.sdrWhiteNits = QuerySdrWhiteNits (luminance.deviceName);


Error:
    if (FAILED (hr))
    {
        // FR-015: any failure is the SDR answer, never an error.
        luminance.hdrEnabled     = false;
        luminance.scRgbSupported = false;
        luminance.sdrWhiteNits   = DisplayLuminanceConstants::kScRgbWhiteNits;
    }

    return luminance;
}
