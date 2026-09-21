#pragma once





/// <summary>
/// Maps the Scanlines "Style" slider value (1..100) to the number of scanlines
/// that fall across one rain character cell.
///
/// "Cell" means the ROW PITCH -- AnimationSystem's BASE_SPACING, 24px at
/// characterScale 1.0 -- which is the rain's true text cell, exactly like the
/// 16-row cell of VGA text mode. It is NOT the 36px glyph quad: the quad is
/// drawn oversized so neighbours overlap, and the glyph ink inside it measures
/// only ~18px. Anchoring to the quad over-counts the scanlines a viewer
/// actually sees crossing a character by roughly 2x.
///
/// Lines-per-CELL rather than lines-per-screen-height is what makes the
/// setting mean the same thing on every display. A real CRT's raster and its
/// character cell are locked together, so the ratio that reads as "CRT" is the
/// one between the raster and the glyph, not between the raster and the panel.
/// A lines-per-height figure instead divides an absolute quantity by a
/// relative one and lands on a different ratio for every viewport height: a
/// portrait 3840-tall monitor got 3.6 lines per cell where a landscape
/// 2160-tall one got 6.5 from the same slider position.
///
/// The mapping is geometric so the slider feels evenly weighted:
///     10 * (6/10)^(style / 100)
/// Endpoints:
///   style =   1  -> ~9.97 lines/cell (~7.6 across the glyph ink)
///   style =  50  -> ~7.75 lines/cell (~5.9 across the glyph ink, default)
///   style = 100  ->  6.00 lines/cell (~4.6 across the glyph ink)
///
/// The dense end is bounded by the pitch the smallest cell can take, not by
/// taste: below ~3px a fractional pitch beats against the pixel grid and some
/// scanline gaps visibly go missing. 10 lines keeps a 30px cell (125% DPI) at
/// 3px. The floor of 6 is legibility: below it the pattern reads as a blind
/// laid over the glyph rather than as a raster.
///
/// Note the pitch in PIXELS differs between monitors of differing DPI while
/// the lines-per-cell stays fixed. That is the intent -- the raster-to-glyph
/// ratio is what the eye judges, so holding it constant is what makes one
/// slider position look the same on every display.
///
/// Inputs are clamped to [1, 100].
/// </summary>
/// <param name="style">Slider value in the range [1, 100]</param>
/// <returns>Number of scanlines spanning one character cell (row pitch)</returns>
float ScanlineLinesPerCell (int style) noexcept;





/// <summary>
/// Converts a lines-per-cell density (from ScanlineLinesPerCell) into the
/// g_linesPerHeight uniform the scanline pixel shader expects -- the number of
/// scanline cycles spanning the full render-target height -- by anchoring it to the rain character cell.
///
/// Because the result scales with viewportHeightPx, two monitors of different
/// heights (or one rotated into portrait) resolve to different line counts
/// that produce the SAME scanlines-per-character-cell, which is the ratio the
/// eye actually judges.
///
/// Guards against a zero or negative cell height by falling back to the
/// caller's viewport height, which yields a 1px pitch that the shader's
/// roll-off renders as flat -- degrading to "no visible scanlines" rather than
/// to a division by zero.
/// </summary>
/// <param name="linesPerCell">Scanlines per character cell</param>
/// <param name="viewportHeightPx">Render target height in physical pixels</param>
/// <param name="cellHeightPx">Rain row pitch in physical pixels</param>
/// <returns>Scanline cycles spanning the render-target height</returns>
float ScanlineLineCount (float linesPerCell, float viewportHeightPx, float cellHeightPx) noexcept;
