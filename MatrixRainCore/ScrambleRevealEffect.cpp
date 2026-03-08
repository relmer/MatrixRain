#include "pch.h"

#include "ScrambleRevealEffect.h"





////////////////////////////////////////////////////////////////////////////////
//
//  ScrambleRevealEffect::ScrambleRevealEffect
//
////////////////////////////////////////////////////////////////////////////////

ScrambleRevealEffect::ScrambleRevealEffect() = default;





////////////////////////////////////////////////////////////////////////////////
//
//  ScrambleRevealEffect::Initialize
//
//  Configures the effect for a given number of cells with timing
//  parameters.  holdDuration < 0 means hold indefinitely.
//
////////////////////////////////////////////////////////////////////////////////

void ScrambleRevealEffect::Initialize (int   cellCount,
                                       float revealDuration,
                                       float dismissDuration,
                                       float cycleInterval,
                                       float flashDuration,
                                       float holdDuration)
{
    m_cellCount        = cellCount;
    m_revealDuration   = revealDuration;
    m_dismissDuration  = dismissDuration;
    m_cycleInterval    = cycleInterval;
    m_flashDuration    = flashDuration;
    m_holdDuration     = holdDuration;



    Reset();

    m_cells.resize (cellCount);
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScrambleRevealEffect::MarkSpace
//
//  Flags a cell as a space character so it won't cycle during reveal.
//
////////////////////////////////////////////////////////////////////////////////

void ScrambleRevealEffect::MarkSpace (int index)
{
    if (index >= 0 && index < m_cellCount)
    {
        m_cells[index].isSpace = true;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScrambleRevealEffect::StartReveal
//
//  Begins the scramble-reveal.  All non-space cells start cycling
//  immediately.  Each cell gets a random lock time within
//  [0, revealDuration] at which it snaps to its target glyph.
//
////////////////////////////////////////////////////////////////////////////////

void ScrambleRevealEffect::StartReveal ()
{
    m_phase            = ScramblePhase::Revealing;
    m_revealTimer      = 0.0f;
    m_dismissTimer     = 0.0f;
    m_postRevealTimer  = 0.0f;
    m_holdStartTime    = 0.0f;

    std::uniform_real_distribution<float> lockDist (0.0f, m_revealDuration);



    for (int i = 0; i < m_cellCount; i++)
    {
        auto & cell = m_cells[i];

        if (cell.isSpace)
        {
            cell.phase      = CellPhase::Hidden;
            cell.opacity    = 0.0f;
            cell.lockTime   = 0.0f;
            cell.needsCycle = false;
        }
        else
        {
            cell.phase      = CellPhase::Cycling;
            cell.opacity    = 1.0f;
            cell.lockTime   = lockDist (m_rng);
            cell.cycleTimer = 0.0f;
            cell.flashTimer = 0.0f;
            cell.needsCycle = true;      // Consumer should pick initial random glyph
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScrambleRevealEffect::StartDismiss
//
//  Begins the dismiss phase.  Settled cells get random unlock times
//  at which they revert to cycling, then fade out.
//
////////////////////////////////////////////////////////////////////////////////

void ScrambleRevealEffect::StartDismiss ()
{
    if (m_phase != ScramblePhase::Revealing && m_phase != ScramblePhase::Holding)
    {
        return;
    }

    m_phase        = ScramblePhase::Dismissing;
    m_dismissTimer = 0.0f;

    std::uniform_real_distribution<float> unlockDist (0.0f, m_dismissDuration);



    for (int i = 0; i < m_cellCount; i++)
    {
        auto & cell = m_cells[i];

        if (cell.isSpace)
        {
            continue;
        }

        // Cells that haven't locked yet keep cycling and will fade out
        if (cell.phase == CellPhase::Cycling || cell.phase == CellPhase::LockFlash)
        {
            cell.phase    = CellPhase::Dismissing;
            cell.lockTime = 0.0f;     // Immediate start of fade
        }
        else if (cell.phase == CellPhase::Settled)
        {
            cell.lockTime = unlockDist (m_rng);   // Random unlock time
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScrambleRevealEffect::Reset
//
//  Returns to Idle with all timers zeroed.
//
////////////////////////////////////////////////////////////////////////////////

void ScrambleRevealEffect::Reset ()
{
    m_phase           = ScramblePhase::Idle;
    m_revealTimer     = 0.0f;
    m_dismissTimer    = 0.0f;
    m_postRevealTimer = 0.0f;
    m_holdStartTime   = 0.0f;



    for (auto & cell : m_cells)
    {
        cell.phase      = CellPhase::Hidden;
        cell.lockTime   = 0.0f;
        cell.flashTimer = 0.0f;
        cell.cycleTimer = 0.0f;
        cell.opacity    = 0.0f;
        cell.needsCycle = false;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScrambleRevealEffect::Update
//
//  Advances the animation clock and per-cell state machines.
//
////////////////////////////////////////////////////////////////////////////////

void ScrambleRevealEffect::Update (float deltaTime)
{
    switch (m_phase)
    {
        case ScramblePhase::Revealing:
        {
            m_revealTimer += deltaTime;

            for (auto & cell : m_cells)
            {
                if (cell.isSpace)
                {
                    continue;
                }

                switch (cell.phase)
                {
                    case CellPhase::Cycling:
                    {
                        // Advance cycle timer; flag consumer when interval elapses
                        cell.cycleTimer += deltaTime;

                        if (cell.cycleTimer >= m_cycleInterval)
                        {
                            cell.cycleTimer -= m_cycleInterval;
                            cell.needsCycle  = true;
                        }

                        // Check if it's time to lock onto target
                        if (m_revealTimer >= cell.lockTime)
                        {
                            // Per-cell flash duration: cap so all cells finish by revealDuration
                            float cellFlash = std::min (m_flashDuration,
                                                        std::max (0.0f, m_revealDuration - cell.lockTime));
                            float overshoot = m_revealTimer - cell.lockTime;

                            cell.flashDuration = cellFlash;

                            if (overshoot >= cellFlash)
                            {
                                // Flash has already completed — go straight to Settled
                                cell.phase      = CellPhase::Settled;
                                cell.flashTimer = 0.0f;
                            }
                            else
                            {
                                cell.phase      = CellPhase::LockFlash;
                                cell.flashTimer = cellFlash - overshoot;
                            }

                            cell.needsCycle = false;
                        }

                        break;
                    }

                    case CellPhase::LockFlash:
                    {
                        cell.flashTimer -= deltaTime;

                        if (cell.flashTimer <= 0.0f)
                        {
                            cell.flashTimer = 0.0f;
                            cell.phase      = CellPhase::Settled;
                        }

                        break;
                    }

                    default:
                        break;
                }
            }

            // Check if all non-space cells have settled
            if (IsRevealComplete())
            {
                m_phase         = ScramblePhase::Holding;
                m_holdStartTime = m_revealTimer;
            }

            break;
        }

        case ScramblePhase::Holding:
        {
            m_revealTimer     += deltaTime;
            m_postRevealTimer += deltaTime;

            if (m_holdDuration >= 0.0f && (m_revealTimer - m_holdStartTime) >= m_holdDuration)
            {
                StartDismiss();
            }

            break;
        }

        case ScramblePhase::Dismissing:
        {
            m_dismissTimer += deltaTime;

            for (auto & cell : m_cells)
            {
                if (cell.isSpace)
                {
                    continue;
                }

                switch (cell.phase)
                {
                    case CellPhase::Settled:
                    {
                        // Wait for unlock time, then start cycling again
                        if (m_dismissTimer >= cell.lockTime)
                        {
                            cell.phase      = CellPhase::Dismissing;
                            cell.cycleTimer = 0.0f;
                            cell.needsCycle = true;

                            // Account for time already elapsed past unlock point
                            float overshoot    = m_dismissTimer - cell.lockTime;
                            float fadeDuration = m_dismissDuration * 0.5f;

                            cell.opacity -= overshoot / fadeDuration;

                            if (cell.opacity <= 0.0f)
                            {
                                cell.opacity = 0.0f;
                                cell.phase   = CellPhase::Hidden;
                            }
                        }

                        break;
                    }

                    case CellPhase::Dismissing:
                    {
                        // Cycle glyphs while fading out
                        cell.cycleTimer += deltaTime;

                        if (cell.cycleTimer >= m_cycleInterval)
                        {
                            cell.cycleTimer -= m_cycleInterval;
                            cell.needsCycle  = true;
                        }

                        // Fade opacity toward zero
                        float fadeDuration = m_dismissDuration * 0.5f;

                        cell.opacity -= deltaTime / fadeDuration;

                        if (cell.opacity <= 0.0f)
                        {
                            cell.opacity = 0.0f;
                            cell.phase   = CellPhase::Hidden;
                        }

                        break;
                    }

                    default:
                        break;
                }
            }

            if (IsDismissComplete())
            {
                m_phase = ScramblePhase::Done;
            }

            break;
        }

        case ScramblePhase::Idle:
        case ScramblePhase::Done:
        default:
            break;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScrambleRevealEffect::IsActive
//
////////////////////////////////////////////////////////////////////////////////

bool ScrambleRevealEffect::IsActive () const
{
    return m_phase != ScramblePhase::Idle && m_phase != ScramblePhase::Done;
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScrambleRevealEffect::IsRevealComplete
//
//  True when every non-space cell has reached Settled (or later).
//
////////////////////////////////////////////////////////////////////////////////

bool ScrambleRevealEffect::IsRevealComplete () const
{
    if (m_cellCount == 0)
    {
        return false;
    }



    for (const auto & cell : m_cells)
    {
        if (cell.isSpace)
        {
            continue;
        }

        if (cell.phase == CellPhase::Cycling || cell.phase == CellPhase::LockFlash)
        {
            return false;
        }
    }

    return true;
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScrambleRevealEffect::IsDismissComplete
//
//  True when every non-space cell has faded to Hidden.
//
////////////////////////////////////////////////////////////////////////////////

bool ScrambleRevealEffect::IsDismissComplete () const
{
    if (m_phase == ScramblePhase::Done)
    {
        return true;
    }

    if (m_phase != ScramblePhase::Dismissing || m_cellCount == 0)
    {
        return false;
    }



    for (const auto & cell : m_cells)
    {
        if (cell.isSpace)
        {
            continue;
        }

        if (cell.phase != CellPhase::Hidden)
        {
            return false;
        }
    }

    return true;
}
