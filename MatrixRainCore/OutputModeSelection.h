#pragma once

#include "ScreenSaverMode.h"





/// <summary>
/// How one monitor's swap chain presents: 8-bit sRGB, or 16-bit float scRGB
/// on a monitor with Windows HDR turned on (data-model §1).
/// </summary>
enum class OutputMode
{
    Sdr,
    Hdr
};





/// <summary>
/// The user's HDR setting (data-model §3). Off does not force SDR output; it
/// only removes the highlight gain of Phase 3. A monitor with Windows HDR on
/// still presents natively, at SDR white.
/// </summary>
enum class HdrMode
{
    Auto,
    Off
};





/// <summary>
/// Chooses the output mode for one monitor (contracts/color-math.md).
///
///     hdrEnabled | scRgbPresentSupported | displayMode        | result
///     -----------|-----------------------|--------------------|-------
///     false      | any                   | any                | Sdr
///     true       | false                 | any                | Sdr
///     true       | true                  | ScreenSaverPreview | Sdr (FR-017)
///     true       | true                  | any other          | Hdr
///
/// The preview window is the small one inside the Windows screen saver
/// dialog. It is a child of a window the app does not own, on a swap chain
/// the desktop composes as SDR, so it never presents in HDR.
/// </summary>
/// <param name="hdrEnabled">Windows HDR is on for the monitor the window sits on</param>
/// <param name="scRgbPresentSupported">The swap chain reports present support for scRGB</param>
/// <param name="displayMode">How the app was launched</param>
/// <returns>The mode the swap chain should present in</returns>
OutputMode SelectOutputMode (bool hdrEnabled, bool scRgbPresentSupported, ScreenSaverMode displayMode) noexcept;
