#pragma once

#include "ScrambleRevealEffect.h"





////////////////////////////////////////////////////////////////////////////////
//
//  OverlayColor — RGB color result from overlay color computation
//
////////////////////////////////////////////////////////////////////////////////

struct OverlayColor
{
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
};





////////////////////////////////////////////////////////////////////////////////
//
//  ComputeScrambleColor
//
//  Computes the RGB color for a character based on its scramble-reveal
//  cell phase:
//
//    Cycling:      Medium green (0, 0.5, 0)
//    LockFlash:    Bright yellow-green → bright green (flashTimer decay)
//    Settled:      Bright green, with optional white pulse (postRevealTimer)
//    Dismissing:   Medium green fading to dark green (via opacity)
//
////////////////////////////////////////////////////////////////////////////////

OverlayColor ComputeScrambleColor (CellPhase cellPhase,
                                 float     flashTimer,
                                 float     flashDuration,
                                 float     postRevealTimer,
                                 float     schemeR,
                                 float     schemeG,
                                 float     schemeB);
