#include "pch.h"

#include "SweepCommon.h"





////////////////////////////////////////////////////////////////////////////////
//
//  ComputeSweepColor
//
//  Rain-streak color model shared by all sweep renderers:
//    Head:           White (1, 1, 1)
//    Streak trail:   Pure green scaled by streakIntensity
//    Settled text:   Phase 1: dark green → bright green (1s)
//                    Phase 2: bright green → white (1s)
//
////////////////////////////////////////////////////////////////////////////////

SweepColor ComputeSweepColor (bool  isHead,
                              bool  inStreakZone,
                              float streakIntensity,
                              float brightenTimer)
{
    SweepColor color;



    if (isHead)
    {
        color.r = 1.0f;
        color.g = 1.0f;
        color.b = 1.0f;
    }
    else if (inStreakZone)
    {
        color.g = SweepConstants::MIN_GREEN + (1.0f - SweepConstants::MIN_GREEN) * streakIntensity;
    }
    else
    {
        float t = std::clamp (brightenTimer * SweepConstants::BRIGHTEN_SPEED, 0.0f, 2.0f);

        if (t <= 1.0f)
        {
            // Phase 1: dark green → bright green
            color.g = SweepConstants::MIN_GREEN + (1.0f - SweepConstants::MIN_GREEN) * t;
        }
        else
        {
            // Phase 2: bright green → white
            float w = t - 1.0f;

            color.r = w;
            color.g = 1.0f;
            color.b = w;
        }
    }

    return color;
}





////////////////////////////////////////////////////////////////////////////////
//
//  UpdateSweepStreaks
//
//  Shared Pass 2 logic for overlay Update() functions.
//  During Revealing: a streak of random glyphs sweeps left-to-right.
//  During Dismissing: a streak sweeps right-to-left.
//  Otherwise: all non-space characters show their target glyph.
//
////////////////////////////////////////////////////////////////////////////////

void UpdateSweepStreaks (std::span<HintCharacter>   chars,
                        int                        cols,
                        const TextSweepEffect    & sweep,
                        size_t                     glyphCount,
                        float                      deltaTime)
{
    SweepPhase sweepPhase = sweep.GetPhase();
    float      streakLen  = static_cast<float>(cols);



    for (auto & ch : chars)
    {
        bool  localInStreak = false;
        float dist          = 0.0f;
        float headPos       = 0.0f;

        if (sweepPhase == SweepPhase::Revealing)
        {
            float revealProg = sweep.GetRevealProgress (ch.row);

            if (revealProg > 0.0f)
            {
                headPos       = revealProg * (static_cast<float>(cols - 1) + streakLen);
                dist          = headPos - static_cast<float>(ch.col);
                localInStreak = (dist >= 0.0f && dist < streakLen);
            }
        }
        else if (sweepPhase == SweepPhase::Dismissing)
        {
            float dismissProg = sweep.GetDismissProgress (ch.row);

            if (dismissProg > 0.0f)
            {
                // Right-to-left: head starts at rightmost column and moves left
                headPos       = static_cast<float>(cols - 1) - dismissProg * (static_cast<float>(cols - 1) + streakLen);
                dist          = static_cast<float>(ch.col) - headPos;
                localInStreak = (dist >= 0.0f && dist < streakLen);
            }
        }

        if (localInStreak)
        {
            // Promote Hidden chars to Resolved so the streak is always visible
            if (ch.phase == CharPhase::Hidden)
            {
                size_t charIndex    = static_cast<size_t>(ch.row * cols + ch.col);
                ch.randomGlyphIndex = (charIndex * 7 + 13) % glyphCount;
                ch.phase            = CharPhase::Resolved;
            }

            ch.inStreakZone   = true;
            ch.brightenTimer = 0.0f;
            ch.opacity       = 1.0f;

            // Rain model: front half of streak = full brightness,
            // back half fades from 1.0 → 0.0.
            float halfLen = streakLen * 0.5f;

            if (dist < halfLen)
            {
                ch.streakIntensity = 1.0f;
            }
            else
            {
                ch.streakIntensity = std::clamp (1.0f - (dist - halfLen) / halfLen, 0.0f, 1.0f);
            }

            // Mark the visible leading-edge column as head, but only
            // when the actual head position is within the grid.
            int rawHeadCol = 0;

            if (sweepPhase == SweepPhase::Revealing)
            {
                rawHeadCol = static_cast<int>(headPos);
            }
            else
            {
                rawHeadCol = static_cast<int>(std::ceil (headPos));
            }

            ch.isHead = (ch.col == rawHeadCol && rawHeadCol >= 0 && rawHeadCol < cols);

            // At the tail end, show target glyph for smooth transition
            if (dist >= streakLen - 1.0f && !ch.isSpace)
            {
                ch.currentGlyphIndex = ch.targetGlyphIndex;
            }
            else
            {
                ch.currentGlyphIndex = ch.randomGlyphIndex;
            }
        }
        else if (ch.phase != CharPhase::Resolved)
        {
            continue;
        }
        else if (ch.isSpace)
        {
            ch.currentGlyphIndex = SIZE_MAX;
        }
        else
        {
            // Not in streak — advance brighten timer
            ch.brightenTimer += deltaTime;
            ch.currentGlyphIndex = ch.targetGlyphIndex;
        }
    }
}
