#pragma once

#include "HelpHintOverlay.h"
#include "ScrambleRevealEffect.h"
#include "TextSweepEffect.h"





////////////////////////////////////////////////////////////////////////////////
//
//  SweepColor — RGB color result from sweep color computation
//
////////////////////////////////////////////////////////////////////////////////

struct SweepColor
{
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
};





////////////////////////////////////////////////////////////////////////////////
//
//  SweepConstants — Shared constants for sweep rendering
//
////////////////////////////////////////////////////////////////////////////////

namespace SweepConstants
{
    constexpr float BRIGHTEN_SPEED = 1.0f;     // Phase 1 + Phase 2 each take 1s
    constexpr float MIN_GREEN      = 0.15f;    // Minimum green channel for dim state
}





////////////////////////////////////////////////////////////////////////////////
//
//  ComputeSweepColor
//
//  Computes the RGB color for a character based on its sweep state.
//  Encodes the rain-streak color model shared by all sweep renderers:
//
//    Head:           White (1, 1, 1)
//    Streak trail:   Green scaled by streakIntensity (bright zone → fade zone)
//    Settled text:   Dark green → bright green → white via brightenTimer
//
////////////////////////////////////////////////////////////////////////////////

SweepColor ComputeSweepColor (bool  isHead,
                              bool  inStreakZone,
                              float streakIntensity,
                              float brightenTimer);





////////////////////////////////////////////////////////////////////////////////
//
//  UpdateSweepStreaks
//
//  Shared Pass 2 logic for overlay Update() functions.
//  Determines which characters are in the sweep streak, computes
//  streakIntensity, isHead, and currentGlyphIndex for each character.
//
//  Used by HelpHintOverlay::Update and HotkeyOverlay::Update.
//
////////////////////////////////////////////////////////////////////////////////

void UpdateSweepStreaks (std::span<HintCharacter>  chars,
                        int                        cols,
                        const TextSweepEffect    & sweep,
                        size_t                     glyphCount,
                        float                      deltaTime);





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

SweepColor ComputeScrambleColor (CellPhase cellPhase,
                                 float     flashTimer,
                                 float     flashDuration,
                                 float     postRevealTimer);
