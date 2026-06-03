#include "pch.h"

#include "DeviceLost.h"




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
            return true;

        default:
            return false;
    }
}
