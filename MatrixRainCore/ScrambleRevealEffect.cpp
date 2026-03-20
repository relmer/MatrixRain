#include "pch.h"

#include "ScrambleRevealEffect.h"





////////////////////////////////////////////////////////////////////////////////
//
//  ScrambleRevealEffect::ScrambleRevealEffect
//
////////////////////////////////////////////////////////////////////////////////

ScrambleRevealEffect::ScrambleRevealEffect (float revealDuration,
                                            float dismissDuration,
                                            float cycleInterval,
                                            float flashDuration,
                                            float holdDuration) :
    m_revealDuration  (revealDuration),
    m_dismissDuration (dismissDuration),
    m_cycleInterval   (cycleInterval),
    m_flashDuration   (flashDuration),
    m_holdDuration    (holdDuration)
{
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScrambleRevealEffect::SetCellCount
//
////////////////////////////////////////////////////////////////////////////////

void ScrambleRevealEffect::SetCellCount (int cellCount)
{
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
    if (index >= 0 && index < static_cast<int> (m_cells.size()))
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

    std::uniform_real_distribution<float> lockDist  (0.0f, m_revealDuration);
    std::uniform_real_distribution<float> cycleDist (0.0f, m_cycleInterval);



    for (auto & cell : m_cells)
    {
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
            cell.opacity    = 0.0f;
            cell.lockTime   = lockDist (m_rng);
            cell.cycleTimer = cycleDist (m_rng);  // Stagger so cells don't all cycle on the same frame
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



    for (auto & cell : m_cells)
    {
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
//  ScrambleRevealEffect::AdvanceCycleTimer
//
////////////////////////////////////////////////////////////////////////////////

void ScrambleRevealEffect::AdvanceCycleTimer (CellState & cell, float deltaTime)
{
    cell.cycleTimer += deltaTime;

    if (cell.cycleTimer >= m_cycleInterval)
    {
        cell.cycleTimer -= m_cycleInterval;
        cell.needsCycle  = true;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScrambleRevealEffect::FadeOpacity
//
////////////////////////////////////////////////////////////////////////////////

void ScrambleRevealEffect::FadeOpacity (CellState & cell, float deltaTime, float fadeDuration, float target)
{
    if (target > cell.opacity)
    {
        cell.opacity += deltaTime / fadeDuration;

        if (cell.opacity >= target)
        {
            cell.opacity = target;
        }
    }
    else
    {
        cell.opacity -= deltaTime / fadeDuration;

        if (cell.opacity <= target)
        {
            cell.opacity = target;
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScrambleRevealEffect::UpdateRevealingCell
//
////////////////////////////////////////////////////////////////////////////////

void ScrambleRevealEffect::UpdateRevealingCell (CellState & cell, float deltaTime, float fadeDuration)
{
    FadeOpacity (cell, deltaTime, fadeDuration, 1.0f);
    AdvanceCycleTimer (cell, deltaTime);

    if (m_revealTimer >= cell.lockTime)
    {
        cell.opacity = 1.0f;

        float cellFlash = std::min (m_flashDuration,
                                    std::max (0.0f, m_revealDuration - cell.lockTime));
        float overshoot = m_revealTimer - cell.lockTime;

        cell.flashDuration = cellFlash;
        cell.needsCycle    = false;

        if (overshoot >= cellFlash)
        {
            cell.phase      = CellPhase::Settled;
            cell.flashTimer = 0.0f;
        }
        else
        {
            cell.phase      = CellPhase::LockFlash;
            cell.flashTimer = cellFlash - overshoot;
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScrambleRevealEffect::UpdateLockFlash
//
////////////////////////////////////////////////////////////////////////////////

void ScrambleRevealEffect::UpdateLockFlash (CellState & cell, float deltaTime)
{
    cell.flashTimer -= deltaTime;

    if (cell.flashTimer <= 0.0f)
    {
        cell.flashTimer = 0.0f;
        cell.phase      = CellPhase::Settled;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScrambleRevealEffect::UpdateRevealing
//
////////////////////////////////////////////////////////////////////////////////

void ScrambleRevealEffect::UpdateRevealing (float deltaTime)
{
    float fadeDuration = m_revealDuration * 0.5f;


    m_revealTimer += deltaTime;

    for (auto & cell : m_cells)
    {
        if (cell.isSpace)
        {
            continue;
        }

        cell.needsCycle = false;

        switch (cell.phase)
        {
            case CellPhase::Cycling:   UpdateRevealingCell (cell, deltaTime, fadeDuration); break;
            case CellPhase::LockFlash: UpdateLockFlash (cell, deltaTime);                   break;
            default:                                                                        break;
        }
    }

    if (IsRevealComplete())
    {
        m_phase         = ScramblePhase::Holding;
        m_holdStartTime = m_revealTimer;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScrambleRevealEffect::UpdateHolding
//
////////////////////////////////////////////////////////////////////////////////

void ScrambleRevealEffect::UpdateHolding (float deltaTime)
{
    m_revealTimer     += deltaTime;
    m_postRevealTimer += deltaTime;

    if (m_holdDuration >= 0.0f && (m_revealTimer - m_holdStartTime) >= m_holdDuration)
    {
        StartDismiss();
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScrambleRevealEffect::UpdateDismissing
//
////////////////////////////////////////////////////////////////////////////////

void ScrambleRevealEffect::UpdateDismissing (float deltaTime)
{
    float fadeDuration = m_dismissDuration * 0.5f;


    m_dismissTimer += deltaTime;

    for (auto & cell : m_cells)
    {
        if (cell.isSpace)
        {
            continue;
        }

        cell.needsCycle = false;

        switch (cell.phase)
        {
            case CellPhase::Settled:
            {
                if (m_dismissTimer >= cell.lockTime)
                {
                    cell.phase      = CellPhase::Dismissing;
                    cell.cycleTimer = std::uniform_real_distribution<float> (0.0f, m_cycleInterval) (m_rng);
                    cell.needsCycle = true;

                    float overshoot = m_dismissTimer - cell.lockTime;

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
                AdvanceCycleTimer (cell, deltaTime);
                FadeOpacity (cell, deltaTime, fadeDuration, 0.0f);

                if (cell.opacity <= 0.0f)
                {
                    cell.phase = CellPhase::Hidden;
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
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScrambleRevealEffect::Update
//
////////////////////////////////////////////////////////////////////////////////

void ScrambleRevealEffect::Update (float deltaTime)
{
    switch (m_phase)
    {
        case ScramblePhase::Revealing:  UpdateRevealing  (deltaTime); break;
        case ScramblePhase::Holding:    UpdateHolding    (deltaTime); break;
        case ScramblePhase::Dismissing: UpdateDismissing (deltaTime); break;
        default:                                                      break;
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
    if (m_cells.empty())
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

    if (m_phase != ScramblePhase::Dismissing || m_cells.empty())
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
