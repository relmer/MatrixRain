#include "pch.h"

#include "HdrDisplayInventory.h"





////////////////////////////////////////////////////////////////////////////////
//
//  QueryHdrDisplayInventory
//
////////////////////////////////////////////////////////////////////////////////

HdrDisplayInventory QueryHdrDisplayInventory() noexcept
{
    HRESULT                              hr        = S_OK;
    LONG                                 status    = ERROR_SUCCESS;
    UINT32                               pathCount = 0;
    UINT32                               modeCount = 0;
    std::vector<DISPLAYCONFIG_PATH_INFO> paths;
    std::vector<DISPLAYCONFIG_MODE_INFO> modes;
    HdrDisplayInventory                  inventory;



    // The buffer sizes can change between the two calls if a display arrives
    // in the gap; the loop is the documented way to handle that.
    for (int attempt = 0; attempt < 3; ++attempt)
    {
        status = GetDisplayConfigBufferSizes (QDC_ONLY_ACTIVE_PATHS, &pathCount, &modeCount);
        CBREx (status == ERROR_SUCCESS, HRESULT_FROM_WIN32 (status));

        paths.resize (pathCount);
        modes.resize (modeCount);

        status = QueryDisplayConfig (QDC_ONLY_ACTIVE_PATHS, &pathCount, paths.data(), &modeCount, modes.data(), nullptr);

        if (status != ERROR_INSUFFICIENT_BUFFER)
        {
            break;
        }
    }

    CBREx (status == ERROR_SUCCESS, HRESULT_FROM_WIN32 (status));

    paths.resize (pathCount);

    for (const DISPLAYCONFIG_PATH_INFO & path : paths)
    {
        DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO_2 info = {};


        info.header.type      = DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO_2;
        info.header.size      = sizeof (info);
        info.header.adapterId = path.targetInfo.adapterId;
        info.header.id        = path.targetInfo.id;

        // Refused before Windows 11 24H2; the monitor then counts as neither.
        if (DisplayConfigGetDeviceInfo (&info.header) != ERROR_SUCCESS)
        {
            continue;
        }

        if (info.highDynamicRangeSupported && info.highDynamicRangeUserEnabled)
        {
            ++inventory.hdrOnCount;
        }
        else if (info.highDynamicRangeSupported)
        {
            ++inventory.hdrCapableOffCount;
        }
    }


Error:
    (void) hr;
    return inventory;
}





////////////////////////////////////////////////////////////////////////////////
//
//  DescribeHdrControls
//
////////////////////////////////////////////////////////////////////////////////

HdrControlsState DescribeHdrControls (const HdrDisplayInventory & inventory, HdrMode hdrMode) noexcept
{
    HdrControlsState state;



    if (inventory.hdrOnCount > 0)
    {
        state.modeEnabled   = true;
        state.sliderEnabled = (hdrMode == HdrMode::Auto);
    }
    else if (inventory.hdrCapableOffCount > 0)
    {
        state.statusText       = L"A monitor supports HDR, but HDR is turned off in Windows.";
        state.showSettingsLink = true;
    }
    else
    {
        state.statusText = L"No monitor has HDR turned on, so these settings have no effect.";
    }

    return state;
}
