#include "pch.h"

#include "DeviceLost.h"


// D3DDDIERR_DEVICEREMOVED lives in d3dukmdt.h (kernel-mode DDI header),
// which we don't include from user-mode TUs.  Define inline — the value
// is documented and stable.  See Microsoft device-lost recovery samples.
#ifndef D3DDDIERR_DEVICEREMOVED
#define D3DDDIERR_DEVICEREMOVED ((HRESULT) 0x88760870L)
#endif




////////////////////////////////////////////////////////////////////////////////
//
//  IsDeviceLost
//
////////////////////////////////////////////////////////////////////////////////

bool IsDeviceLost (HRESULT hr)
{
    switch (hr)
    {
        case DXGI_ERROR_DEVICE_REMOVED:
        case DXGI_ERROR_DEVICE_RESET:
        case DXGI_ERROR_DEVICE_HUNG:
        case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
        case D3DDDIERR_DEVICEREMOVED:
            return true;

        default:
            return false;
    }
}
