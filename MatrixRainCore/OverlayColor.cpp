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
                                 float     postRevealTimer)
{
    OverlayColor color;

    static constexpr float kPulseRampUp   = 0.8f;    // Smooth ramp to white duration
    static constexpr float kPulseHold     = 1.5f;    // Hold at white duration
    static constexpr float kPulseRampDown = 1.0f;    // Smooth ramp back to green duration



    switch (cellPhase)
    {
        case CellPhase::Cycling:
        {
            color.g = 0.33f;
            break;
        }

        case CellPhase::LockFlash:
        {
            // Yellow decaying to mid green over flashDuration
            float t = (flashDuration > 0.0f)
                    ? std::clamp (flashTimer / flashDuration, 0.0f, 1.0f)
                    : 0.0f;

            // t=1 at flash start (yellow), t=0 at end (mid green)
            color.r = 0.8f * t;
            color.g = 0.6f + 0.4f * t;
            color.b = 0.3f * t;
            break;
        }

        case CellPhase::Settled:
        {
            // Pre-pulse: mid green.  Pulse: ramp to white, hold, ramp down to grey.
            if (postRevealTimer <= 0.0f)
            {
                // All locked but pulse hasn't started — mid green
                color.g = 0.6f;
            }
            else
            {
                float elapsed = postRevealTimer;

                if (elapsed < kPulseRampUp)
                {
                    // Smooth ramp from mid green (0, 0.6, 0) to white (1, 1, 1)
                    float t = elapsed / kPulseRampUp;
                    float s = t * t;

                    color.r = s;
                    color.g = 0.6f + 0.4f * s;
                    color.b = s;
                }
                else if (elapsed < kPulseRampUp + kPulseHold)
                {
                    // Hold at white
                    color.r = 1.0f;
                    color.g = 1.0f;
                    color.b = 1.0f;
                }
                else
                {
                    // Smooth ramp from white (1, 1, 1) to grey (0.75, 0.75, 0.75)
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
            color.g = 0.33f;
            break;
        }

        case CellPhase::Hidden:
        default:
            break;
    }

    return color;
}
