#pragma once

#include "ColorMath.h"
#include "OutputModeSelection.h"
#include "ScreenSaverMode.h"





/// <summary>
/// What a monitor context should do after handing the tracker a fresh
/// luminance snapshot.
/// </summary>
enum class OutputModeAction
{
    None,          // Keep presenting as before
    Reconfigure    // Rebuild the swap chain's buffers and color space for the mode below
};





/// <summary>
/// The tracker's answer to one observation.
/// </summary>
struct OutputModeDecision
{
    OutputModeAction action { OutputModeAction::None };
    OutputMode       mode   { OutputMode::Sdr };       // The mode to present in after acting
};





////////////////////////////////////////////////////////////////////////////////
//
//  OutputModeTracker
//
//  The per-monitor state machine of data-model §7, with the OS kept out of it
//  so it can be unit tested. A monitor context feeds it a DisplayLuminance
//  snapshot whenever detection runs (at 1 Hz, and when the DXGI factory goes
//  stale); it says whether the swap chain must be reconfigured and into which
//  mode, and it keeps the SDR white scale and headroom the output transform
//  needs. The caller reports whether a reconfiguration succeeded, because a
//  swap chain that refuses HDR must leave the tracker, and the constant
//  buffer, saying SDR: the mode the tracker holds always matches the back
//  buffer format actually in use.
//
//  A failed switch is not retried while the display keeps reporting the same
//  facts. Windows sends nothing when a mode switch fails, so without this a
//  monitor whose driver refuses scRGB would be asked again every second.
//
////////////////////////////////////////////////////////////////////////////////

class OutputModeTracker
{
public:
    explicit OutputModeTracker (ScreenSaverMode displayMode) noexcept;

    // Feed a fresh snapshot. Updates the white scale and headroom on every
    // call; asks for a reconfiguration only when the selected mode differs
    // from the one in use.
    OutputModeDecision Observe (const DisplayLuminance & luminance) noexcept;

    // The caller's answer to a Reconfigure: succeeded means the swap chain
    // now presents in the decision's mode; failed means it presents in SDR,
    // and that mode is not asked for again until the display's facts change.
    void ReportReconfigureResult (OutputMode attempted, bool succeeded) noexcept;

    OutputMode CurrentMode()   const noexcept { return m_currentMode;   }
    float      SdrWhiteScale() const noexcept { return m_sdrWhiteScale; }
    float      Headroom()      const noexcept { return m_headroom;      }

private:
    ScreenSaverMode m_displayMode;
    OutputMode      m_currentMode    { OutputMode::Sdr };
    float           m_sdrWhiteScale  { 1.0f };
    float           m_headroom       { 1.0f };

    // The facts from the last observation, so a refusal can remember what it
    // was selected from.
    bool            m_lastHdrEnabled     { false };
    bool            m_lastScRgbSupported { false };

    // Set after a failed switch: the mode that was refused, and the facts it
    // was selected from. Cleared when either fact changes.
    bool            m_hasRefusal         { false };
    OutputMode      m_refusedMode        { OutputMode::Sdr };
    bool            m_refusedHdr         { false };
    bool            m_refusedScRgb       { false };
};
