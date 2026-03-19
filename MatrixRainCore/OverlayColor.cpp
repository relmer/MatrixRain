#include "pch.h"

#include "OverlayColor.h"





////////////////////////////////////////////////////////////////////////////////
//
//  ComputeScrambleColor
//
//  Scramble-reveal color model:
//    Cycling:      Medium green — glyphs are still scrambling
//    LockFlash:    Bright yellow-green decaying to bright green
//    Settled:      Bright green, with a brief white pulse after all cells lock
//    Dismissing:   Medium green (opacity handles the fade-out)
//
////////////////////////////////////////////////////////////////////////////////

OverlayColor ComputeScrambleColor (CellPhase cellPhase,
                                 float     flashTimer,
                                 float     flashDuration,
                                 float     postRevealTimer,
                                 float     schemeR,
                                 float     schemeG,
                                 float     schemeB)
{
    OverlayColor color;

    static constexpr float kPulseRampUp   = 0.33f;   // Smooth ramp to white duration
    static constexpr float kPulseHold     = 0.75f;   // Hold at white duration
    static constexpr float kPulseRampDown = 0.5f;    // Smooth ramp back to green duration



    switch (cellPhase)
    {
        case CellPhase::Cycling:
        {
            // Medium intensity of scheme color
            color.r = schemeR * 0.5f;
            color.g = schemeG * 0.5f;
            color.b = schemeB * 0.5f;
            break;
        }

        case CellPhase::LockFlash:
        {
            // Bright flash decaying to medium scheme color over flashDuration
            float t = (flashDuration > 0.0f)
                    ? std::clamp (flashTimer / flashDuration, 0.0f, 1.0f)
                    : 0.0f;

            // t=1 at flash start (bright white-ish), t=0 at end (full scheme color)
            color.r = schemeR + (1.0f - schemeR) * t;
            color.g = schemeG + (1.0f - schemeG) * t;
            color.b = schemeB + (1.0f - schemeB) * t;
            break;
        }

        case CellPhase::Settled:
        {
            // Pre-pulse: scheme color.  Pulse: ramp to white, hold, ramp down.
            if (postRevealTimer <= 0.0f)
            {
                // All locked but pulse hasn't started — full scheme color
                color.r = schemeR;
                color.g = schemeG;
                color.b = schemeB;
            }
            else
            {
                float elapsed = postRevealTimer;

                if (elapsed < kPulseRampUp)
                {
                    float t = elapsed / kPulseRampUp;
                    float s = t * t;

                    color.r = schemeR + (1.0f - schemeR) * s;
                    color.g = schemeG + (1.0f - schemeG) * s;
                    color.b = schemeB + (1.0f - schemeB) * s;
                }
                else if (elapsed < kPulseRampUp + kPulseHold)
                {
                    color.r = 1.0f;
                    color.g = 1.0f;
                    color.b = 1.0f;
                }
                else
                {
                    float t = std::clamp ((elapsed - kPulseRampUp - kPulseHold) / kPulseRampDown, 0.0f, 1.0f);
                    float s = (1.0f - t) * (1.0f - t);

                    color.r = 0.75f + 0.25f * s;
                    color.g = 0.75f + 0.25f * s;
                    color.b = 0.75f + 0.25f * s;
                }
            }

            break;
        }

        case CellPhase::Dismissing:
        {
            color.r = schemeR * 0.5f;
            color.g = schemeG * 0.5f;
            color.b = schemeB * 0.5f;
            break;
        }

        case CellPhase::Hidden:
        default:
            break;
    }

    return color;
}
