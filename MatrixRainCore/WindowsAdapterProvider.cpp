#include "pch.h"

#include "WindowsAdapterProvider.h"




////////////////////////////////////////////////////////////////////////////////
//
//  GetDefaultAdapterLuid
//
//  Returns the LUID of the system default rendering adapter (the one Windows
//  picks via the UNSPECIFIED GPU preference), or std::nullopt if it cannot
//  be determined.  Used to set AdapterInfo::m_isDefault.
//
////////////////////////////////////////////////////////////////////////////////

static std::optional<LUID> GetDefaultAdapterLuid (IDXGIFactory1 * pFactory)
{
    HRESULT                              hr            = S_OK;
    Microsoft::WRL::ComPtr<IDXGIFactory6> pFactory6;
    Microsoft::WRL::ComPtr<IDXGIAdapter1> pDefaultAdapter;
    DXGI_ADAPTER_DESC1                   desc          = {};
    std::optional<LUID>                  result;


    hr = pFactory->QueryInterface (IID_PPV_ARGS (&pFactory6));
    CBREx (SUCCEEDED (hr), S_OK);

    hr = pFactory6->EnumAdapterByGpuPreference (0,
                                                DXGI_GPU_PREFERENCE_UNSPECIFIED,
                                                IID_PPV_ARGS (&pDefaultAdapter));
    CBREx (SUCCEEDED (hr), S_OK);

    hr = pDefaultAdapter->GetDesc1 (&desc);
    CBREx (SUCCEEDED (hr), S_OK);

    result = desc.AdapterLuid;


Error:
    return result;
}




////////////////////////////////////////////////////////////////////////////////
//
//  WindowsAdapterProvider::EnumerateAdapters
//
////////////////////////////////////////////////////////////////////////////////

std::vector<AdapterInfo> WindowsAdapterProvider::EnumerateAdapters() const
{
    HRESULT                               hr             = S_OK;
    Microsoft::WRL::ComPtr<IDXGIFactory1> pFactory;
    std::vector<AdapterInfo>              adapters;
    std::optional<LUID>                   defaultLuid;
    UINT                                  index          = 0;
    bool                                  dxgiFailure    = false;


    hr = CreateDXGIFactory1 (IID_PPV_ARGS (&pFactory));
    CBREx (SUCCEEDED (hr), S_OK);

    defaultLuid = GetDefaultAdapterLuid (pFactory.Get());


    for (;;)
    {
        Microsoft::WRL::ComPtr<IDXGIAdapter1> pAdapter;
        DXGI_ADAPTER_DESC1                   desc       = {};


        hr = pFactory->EnumAdapters1 (index, &pAdapter);

        if (hr == DXGI_ERROR_NOT_FOUND)
        {
            // End of enumeration is the normal termination, not a failure.
            hr = S_OK;
            break;
        }

        if (FAILED (hr))
        {
            // Per contracts/adapter-provider.md: on any DXGI failure return
            // an empty vector (callers then use the system default-adapter
            // path).  Latch the failure and bail out of the loop; the
            // cleared `adapters` happens at the Error: label below.
            dxgiFailure = true;
            break;
        }

        hr = pAdapter->GetDesc1 (&desc);

        if (SUCCEEDED (hr) && (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0)
        {
            AdapterInfo info;
            info.m_description     = desc.Description;
            info.m_luid            = desc.AdapterLuid;
            info.m_dedicatedVramMb = static_cast<unsigned int> (desc.DedicatedVideoMemory / (1024ull * 1024ull));
            info.m_isSoftware      = false;
            info.m_isDefault       = defaultLuid.has_value() &&
                                     defaultLuid->LowPart  == desc.AdapterLuid.LowPart &&
                                     defaultLuid->HighPart == desc.AdapterLuid.HighPart;

            adapters.push_back (std::move (info));
        }

        ++index;
    }


Error:
    if (dxgiFailure)
    {
        adapters.clear();
    }

    return adapters;
}
