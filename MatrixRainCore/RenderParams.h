#pragma once

#include "ColorScheme.h"




////////////////////////////////////////////////////////////////////////////////
//
//  RenderParams
//
//  Per-frame inputs handed to a render system's Render() call.  Promoted to a
//  top-level type (rather than nested in RenderSystem) so the IRenderSystem
//  interface can reference it without depending on the concrete renderer.
//
//  The overlay pointers are non-owning and must be consumed synchronously
//  within Render(); a render system must not retain them past the call.
//
////////////////////////////////////////////////////////////////////////////////

class Overlay;

struct RenderParams
{
    ColorScheme     colorScheme           = ColorScheme::Green;
    float           fps                   = 0.0f;
    int             rainPercentage        = 0;
    int             streakCount           = 0;
    int             activeHeadCount       = 0;
    float           elapsedTime           = 0.0f;
    const Overlay * pHelpOverlay          = nullptr;
    const Overlay * pHotkeyOverlay        = nullptr;
    const Overlay * pUsageOverlay         = nullptr;

    // v1.5 additions (data-model.md §5): per-frame render-thread copy of
    // the v1.5 SharedState atomics, with the int → float conversions
    // performed once on the render thread to keep SharedState writers
    // (dialog thread) simple.
    bool            glowEnabled           = true;
    bool            scanlinesEnabled      = true;
    float           scanlinesIntensity    = 0.30f;     // normalised [0..1] from settings 1..100
    // Scanline density is carried as lines-per-CHARACTER-CELL, not as the
    // shader's lines-per-render-height uniform. Only the render system knows
    // the cell's pixel height (it owns characterScale), so it performs the
    // conversion via ScanlineLineCount() at cbuffer upload time. Keeping the
    // resolution-dependent step there is what makes one slider position look
    // identical on monitors of differing heights.
    float           scanlinesLinesPerCell = 7.75f;     // ScanlineLinesPerCell(style); 7.75 at the default Style 50
    COLORREF        customColor           = RGB (0, 255, 0);
};
