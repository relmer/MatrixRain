#include "pch.h"

#include "OutputModeTracker.h"





////////////////////////////////////////////////////////////////////////////////
//
//  OutputModeTracker::OutputModeTracker
//
////////////////////////////////////////////////////////////////////////////////

OutputModeTracker::OutputModeTracker (ScreenSaverMode displayMode) noexcept :
    m_displayMode (displayMode)
{
}





////////////////////////////////////////////////////////////////////////////////
//
//  OutputModeTracker::Observe
//
////////////////////////////////////////////////////////////////////////////////

OutputModeDecision OutputModeTracker::Observe (const DisplayLuminance & luminance) noexcept
{
    OutputModeDecision decision;
    const OutputMode   selected = SelectOutputMode (luminance.hdrEnabled,
                                                    luminance.scRgbSupported,
                                                    m_displayMode);



    // The white level and peak follow the display on every observation,
    // whether or not the mode moves: the Windows SDR brightness slider
    // changes the white level with no notification, and FR-012 wants the
    // rain to follow it live.
    m_sdrWhiteScale = ::SdrWhiteScale (luminance.sdrWhiteNits);
    m_headroom      = ::Headroom (EffectivePeakNits (luminance.reportedPeakNits), luminance.sdrWhiteNits);

    // A refusal stands only while the display keeps saying what it said
    // when the switch failed.
    if (m_hasRefusal
        && (luminance.hdrEnabled != m_refusedHdr || luminance.scRgbSupported != m_refusedScRgb))
    {
        m_hasRefusal = false;
    }

    m_lastHdrEnabled     = luminance.hdrEnabled;
    m_lastScRgbSupported = luminance.scRgbSupported;

    decision.mode = m_currentMode;

    if (selected == m_currentMode)
    {
        return decision;
    }

    if (m_hasRefusal && selected == m_refusedMode)
    {
        return decision;
    }

    decision.action = OutputModeAction::Reconfigure;
    decision.mode   = selected;

    return decision;
}





////////////////////////////////////////////////////////////////////////////////
//
//  OutputModeTracker::ReportReconfigureResult
//
////////////////////////////////////////////////////////////////////////////////

void OutputModeTracker::ReportReconfigureResult (OutputMode attempted, bool succeeded) noexcept
{
    if (succeeded)
    {
        m_currentMode = attempted;
        m_hasRefusal  = false;

        return;
    }

    // RenderSystem falls back to SDR when a switch fails (data-model §7), so
    // that is what the back buffer holds now, whatever was attempted.
    m_currentMode  = OutputMode::Sdr;
    m_hasRefusal   = true;
    m_refusedMode  = attempted;

    // Remember the facts the refused selection came from: the ones from the
    // last Observe, which is what selected the attempted mode.
    m_refusedHdr   = m_lastHdrEnabled;
    m_refusedScRgb = m_lastScRgbSupported;
}
