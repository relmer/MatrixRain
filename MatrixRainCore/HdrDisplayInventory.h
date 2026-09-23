#pragma once

#include "OutputModeSelection.h"





////////////////////////////////////////////////////////////////////////////////
//
//  HdrDisplayInventory
//
//  How many active monitors have Windows HDR turned on, and how many could
//  but have it off. Read by the settings dialog to decide whether the HDR
//  controls apply, and to tell a user whose monitor can do HDR where to turn
//  it on (T043).
//
//  Queried from DisplayConfig (DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_
//  INFO_2, Windows 11 24H2 and later), which reports HDR support and the
//  user's HDR switch separately. Needs no swap chain and no render thread, so
//  it answers the same in the Control Panel dialog as in the live one.
//
////////////////////////////////////////////////////////////////////////////////

struct HdrDisplayInventory
{
    int hdrOnCount         { 0 };   // Monitors with Windows HDR turned on
    int hdrCapableOffCount { 0 };   // Monitors that support HDR with it turned off
};





/// <summary>
/// Counts the active monitors by HDR state. Never fails: on an older Windows,
/// or if DisplayConfig refuses, it reports no monitors of either kind, and
/// the dialog shows its no-HDR state.
/// </summary>
HdrDisplayInventory QueryHdrDisplayInventory() noexcept;





/// <summary>
/// What the settings dialog's HDR row should show (contracts/settings-ui.md).
/// </summary>
struct HdrControlsState
{
    bool            rowEnabled     { false };     // Mode combo and the rest of the row
    bool            sliderEnabled  { false };     // Highlight brightness; also needs HDR mode Auto
    const wchar_t * disabledReason { nullptr };   // Tooltip over the grayed row, or nullptr when enabled
};





/// <summary>
/// Decides the HDR row's state from the inventory and the HDR mode. Pure, so
/// it is unit tested.
/// </summary>
HdrControlsState DescribeHdrControls (const HdrDisplayInventory & inventory, HdrMode hdrMode) noexcept;
