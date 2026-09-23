#include "pch.h"

#include "OutputModeSelection.h"





////////////////////////////////////////////////////////////////////////////////
//
//  SelectOutputMode
//
////////////////////////////////////////////////////////////////////////////////

OutputMode SelectOutputMode (bool hdrEnabled, bool scRgbPresentSupported, ScreenSaverMode displayMode) noexcept
{
    if (!hdrEnabled || !scRgbPresentSupported)
    {
        return OutputMode::Sdr;
    }

    if (displayMode == ScreenSaverMode::ScreenSaverPreview)
    {
        return OutputMode::Sdr;
    }

    return OutputMode::Hdr;
}
