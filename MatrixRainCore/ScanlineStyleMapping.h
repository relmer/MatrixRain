#pragma once





/// <summary>
/// Maps the v1.5 Scanlines "Style" slider value (1..100) to the number of
/// scanlines that span the full output height. The mapping is logarithmic
/// so the slider feels evenly weighted across the visible density range.
///
/// Formula: 1000 * 0.15^(style / 100)
/// Endpoints:
///   style =   1  -> ~981 lines (densest, near-imperceptible)
///   style =  25  -> ~622 lines
///   style =  50  -> ~387 lines (default)
///   style =  75  -> ~241 lines
///   style = 100  -> ~150 lines (sparsest, retro CRT look)
///
/// Inputs are clamped to [1, 100].
/// </summary>
/// <param name="style">Slider value in the range [1, 100]</param>
/// <returns>Number of scanlines across the render-target height</returns>
float ScanlineLineCount (int style) noexcept;
