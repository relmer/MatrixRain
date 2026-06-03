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
    ColorScheme     colorScheme        = ColorScheme::Green;
    float           fps                = 0.0f;
    int             rainPercentage     = 0;
    int             streakCount        = 0;
    int             activeHeadCount    = 0;
    bool            showDebugFadeTimes = false;
    float           elapsedTime        = 0.0f;
    const Overlay * pHelpOverlay       = nullptr;
    const Overlay * pHotkeyOverlay     = nullptr;
    const Overlay * pUsageOverlay      = nullptr;
};
