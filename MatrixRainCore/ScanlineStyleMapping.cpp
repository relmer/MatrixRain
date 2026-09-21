#include "pch.h"

#include "ScanlineStyleMapping.h"





//  Endpoints of the lines-per-cell range, where "cell" is the row pitch
//  (24px base, scaled by DPI -- 30px at 125%, 36px at 150%).
//
//  The ratio is kept continuous and identical on every monitor, so the only
//  thing bounding the dense end is the pitch the smallest cell can take. At a
//  fractional pitch the troughs land at drifting offsets within their pixels
//  and alternate gaps darken unevenly -- a beat with period 1/frac(pitch).
//  The beat is severe below ~3px (per-cycle gap spread 0.42 at 2.27px) and
//  mild at or above it (<= ~0.15), so 10 lines/cell keeps a 30px cell at 3px.
//  Snapping the pitch to whole pixels removes the beat entirely, but was
//  rejected: it makes monitors with differently sized cells land on different
//  lines-per-cell, and pitch 2 is degenerate (samples hit the cosine's zero
//  crossings, contrast 0).
static constexpr float kLinesPerCellDensest  = 10.0f;   // style = 1
static constexpr float kLinesPerCellSparsest =  6.0f;   // style = 100





////////////////////////////////////////////////////////////////////////////////
//
//  ScanlineLinesPerCell
//
////////////////////////////////////////////////////////////////////////////////

float ScanlineLinesPerCell (int style) noexcept
{
    const int   clamped = std::clamp (style, 1, 100);
    const float ratio   = kLinesPerCellSparsest / kLinesPerCellDensest;



    return kLinesPerCellDensest * std::pow (ratio, static_cast<float> (clamped) / 100.0f);
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScanlineLineCount
//
////////////////////////////////////////////////////////////////////////////////

float ScanlineLineCount (float linesPerCell, float viewportHeightPx, float cellHeightPx) noexcept
{
    //  A non-positive cell height means the caller has no glyph metrics to
    //  anchor to; fall back to a 1px pitch, which the shader's sinc roll-off
    //  flattens out.
    if (!(cellHeightPx > 0.0f))
    {
        return viewportHeightPx;
    }

    return viewportHeightPx * linesPerCell / cellHeightPx;
}
