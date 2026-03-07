#include "pch.h"

#include "TextSweepEffect.h"





////////////////////////////////////////////////////////////////////////////////
//
//  TextSweepEffect::TextSweepEffect
//
////////////////////////////////////////////////////////////////////////////////

TextSweepEffect::TextSweepEffect() = default;





////////////////////////////////////////////////////////////////////////////////
//
//  TextSweepEffect::Initialize
//
//  Configures the sweep for a given number of rows with random stagger
//  and duration ranges.  holdDuration < 0 means hold indefinitely.
//
////////////////////////////////////////////////////////////////////////////////

void TextSweepEffect::Initialize (int   rowCount,
                                  float staggerRange,
                                  float durationMin,
                                  float durationMax,
                                  float fadeInSpeed,
                                  float holdDuration,
                                  float glowDecaySpeed)
{
    m_rowCount       = rowCount;
    m_staggerRange   = staggerRange;
    m_durationMin    = durationMin;
    m_durationMax    = durationMax;
    m_fadeInSpeed    = fadeInSpeed;
    m_holdDuration   = holdDuration;
    m_glowDecaySpeed = glowDecaySpeed;



    Reset();

    m_rowRevealDelays.resize (rowCount, 0.0f);
    m_rowRevealDurations.resize (rowCount, durationMax);
    m_rowDismissDelays.resize (rowCount, 0.0f);
    m_rowDismissDurations.resize (rowCount, durationMax);
}





////////////////////////////////////////////////////////////////////////////////
//
//  TextSweepEffect::StartReveal
//
//  Begins the left-to-right reveal sweep.  Generates random per-row
//  delays and durations.
//
////////////////////////////////////////////////////////////////////////////////

void TextSweepEffect::StartReveal ()
{
    m_phase         = SweepPhase::Revealing;
    m_revealTimer   = 0.0f;
    m_holdStartTime = 0.0f;
    m_dismissTimer  = 0.0f;

    std::uniform_real_distribution<float> delayDist    (0.0f, m_staggerRange);
    std::uniform_real_distribution<float> durationDist (m_durationMin, m_durationMax);



    for (int row = 0; row < m_rowCount; row++)
    {
        m_rowRevealDelays[row]    = delayDist (m_rng);
        m_rowRevealDurations[row] = durationDist (m_rng);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  TextSweepEffect::StartDismiss
//
//  Begins a right-to-left dismiss sweep.  Only valid from Revealing or
//  Holding — no-op from other phases.
//
////////////////////////////////////////////////////////////////////////////////

void TextSweepEffect::StartDismiss ()
{
    if (m_phase != SweepPhase::Revealing && m_phase != SweepPhase::Holding)
    {
        return;
    }

    m_phase        = SweepPhase::Dismissing;
    m_dismissTimer = 0.0f;

    std::uniform_real_distribution<float> delayDist    (0.0f, m_staggerRange);
    std::uniform_real_distribution<float> durationDist (m_durationMin, m_durationMax);



    for (int row = 0; row < m_rowCount; row++)
    {
        m_rowDismissDelays[row]    = delayDist (m_rng);
        m_rowDismissDurations[row] = durationDist (m_rng);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  TextSweepEffect::Reset
//
//  Returns to Idle with all timers zeroed.
//
////////////////////////////////////////////////////////////////////////////////

void TextSweepEffect::Reset ()
{
    m_phase         = SweepPhase::Idle;
    m_revealTimer   = 0.0f;
    m_holdStartTime = 0.0f;
    m_dismissTimer  = 0.0f;
}





////////////////////////////////////////////////////////////////////////////////
//
//  TextSweepEffect::Update
//
//  Advances the animation clock and handles phase transitions.
//
////////////////////////////////////////////////////////////////////////////////

void TextSweepEffect::Update (float deltaTime)
{
    switch (m_phase)
    {
        case SweepPhase::Revealing:
        {
            m_revealTimer += deltaTime;

            if (IsRevealComplete())
            {
                m_phase         = SweepPhase::Holding;
                m_holdStartTime = m_revealTimer;
            }

            break;
        }

        case SweepPhase::Holding:
        {
            m_revealTimer += deltaTime;

            if (m_holdDuration >= 0.0f && (m_revealTimer - m_holdStartTime) >= m_holdDuration)
            {
                StartDismiss();
            }

            break;
        }

        case SweepPhase::Dismissing:
        {
            m_revealTimer  += deltaTime;
            m_dismissTimer += deltaTime;

            if (IsDismissComplete())
            {
                m_phase = SweepPhase::Done;
            }

            break;
        }

        case SweepPhase::Idle:
        case SweepPhase::Done:
        default:
            break;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  TextSweepEffect::IsActive
//
////////////////////////////////////////////////////////////////////////////////

bool TextSweepEffect::IsActive () const
{
    return m_phase != SweepPhase::Idle && m_phase != SweepPhase::Done;
}





////////////////////////////////////////////////////////////////////////////////
//
//  TextSweepEffect::IsRevealComplete
//
//  True when every row's sweep front has reached the right edge.
//
////////////////////////////////////////////////////////////////////////////////

bool TextSweepEffect::IsRevealComplete () const
{
    if (m_rowCount == 0)
    {
        return false;
    }



    for (int row = 0; row < m_rowCount; row++)
    {
        if (GetRevealProgress (row) < 1.0f)
        {
            return false;
        }
    }

    return true;
}





////////////////////////////////////////////////////////////////////////////////
//
//  TextSweepEffect::IsDismissComplete
//
//  True when every row's dismiss sweep has fully faded all positions.
//
////////////////////////////////////////////////////////////////////////////////

bool TextSweepEffect::IsDismissComplete () const
{
    if (m_phase == SweepPhase::Done)
    {
        return true;
    }

    if (m_phase != SweepPhase::Dismissing || m_rowCount == 0)
    {
        return false;
    }



    // Check that the leftmost position (normalizedCol=0) on every row
    // has been fully dismissed
    for (int row = 0; row < m_rowCount; row++)
    {
        if (!IsPositionDismissed (row, 0.0f))
        {
            return false;
        }
    }

    return true;
}





////////////////////////////////////////////////////////////////////////////////
//
//  TextSweepEffect::GetRevealProgress
//
//  Returns 0..1 sweep front position for the given row.
//
////////////////////////////////////////////////////////////////////////////////

float TextSweepEffect::GetRevealProgress (int row) const
{
    if (row < 0 || row >= m_rowCount || m_phase == SweepPhase::Idle)
    {
        return 0.0f;
    }

    float delay    = m_rowRevealDelays[row];
    float duration = m_rowRevealDurations[row];



    return std::clamp ((m_revealTimer - delay) / duration, 0.0f, 1.0f);
}





////////////////////////////////////////////////////////////////////////////////
//
//  TextSweepEffect::IsPositionRevealed
//
//  True when the sweep front has passed the given normalized column.
//
////////////////////////////////////////////////////////////////////////////////

bool TextSweepEffect::IsPositionRevealed (int row, float normalizedCol) const
{
    if (m_phase == SweepPhase::Idle)
    {
        return false;
    }

    return normalizedCol <= GetRevealProgress (row);
}





////////////////////////////////////////////////////////////////////////////////
//
//  TextSweepEffect::GetOpacity
//
//  Returns the current opacity for a position.  Combines the reveal
//  fade-in with an optional dismiss fade-out.
//
////////////////////////////////////////////////////////////////////////////////

float TextSweepEffect::GetOpacity (int row, float normalizedCol) const
{
    if (m_phase == SweepPhase::Idle || m_phase == SweepPhase::Done)
    {
        return 0.0f;
    }

    if (row < 0 || row >= m_rowCount)
    {
        return 0.0f;
    }

    if (!IsPositionRevealed (row, normalizedCol))
    {
        return 0.0f;
    }

    // Compute reveal fade-in
    float delay           = m_rowRevealDelays[row];
    float duration        = m_rowRevealDurations[row];
    float revealTime      = delay + duration * normalizedCol;
    float timeSinceReveal = m_revealTimer - revealTime;
    float revealOpacity   = std::clamp (timeSinceReveal * m_fadeInSpeed, 0.0f, 1.0f);



    // During dismiss, apply fade-out
    if (m_phase == SweepPhase::Dismissing)
    {
        float dDelay          = m_rowDismissDelays[row];
        float dDuration       = m_rowDismissDurations[row];
        // Right-to-left: rightmost positions dismissed first
        float dismissTime     = dDelay + dDuration * (1.0f - normalizedCol);
        float timeSinceDismiss = m_dismissTimer - dismissTime;

        if (timeSinceDismiss > 0.0f)
        {
            float dismissOpacity = std::clamp (1.0f - timeSinceDismiss * m_fadeInSpeed, 0.0f, 1.0f);
            return revealOpacity * dismissOpacity;
        }
    }

    return revealOpacity;
}





////////////////////////////////////////////////////////////////////////////////
//
//  TextSweepEffect::GetGlowIntensity
//
//  Returns 1.0 at the moment of reveal, decaying to 0.0 over time.
//  Used to drive the green→white color transition.
//
////////////////////////////////////////////////////////////////////////////////

float TextSweepEffect::GetGlowIntensity (int row, float normalizedCol) const
{
    if (!IsPositionRevealed (row, normalizedCol))
    {
        return 0.0f;
    }

    if (row < 0 || row >= m_rowCount)
    {
        return 0.0f;
    }

    float delay           = m_rowRevealDelays[row];
    float duration        = m_rowRevealDurations[row];
    float revealTime      = delay + duration * normalizedCol;
    float timeSinceReveal = m_revealTimer - revealTime;



    return std::clamp (1.0f - timeSinceReveal * m_glowDecaySpeed, 0.0f, 1.0f);
}





////////////////////////////////////////////////////////////////////////////////
//
//  TextSweepEffect::GetDismissProgress
//
//  Returns 0..1 dismiss front position for the given row.
//
////////////////////////////////////////////////////////////////////////////////

float TextSweepEffect::GetDismissProgress (int row) const
{
    if (m_phase != SweepPhase::Dismissing && m_phase != SweepPhase::Done)
    {
        return 0.0f;
    }

    if (m_phase == SweepPhase::Done)
    {
        return 1.0f;
    }

    if (row < 0 || row >= m_rowCount)
    {
        return 0.0f;
    }

    float delay    = m_rowDismissDelays[row];
    float duration = m_rowDismissDurations[row];



    return std::clamp ((m_dismissTimer - delay) / duration, 0.0f, 1.0f);
}





////////////////////////////////////////////////////////////////////////////////
//
//  TextSweepEffect::IsPositionDismissed
//
//  True when the dismiss sweep has fully faded the given position.
//  The dismiss sweeps right-to-left (rightmost characters disappear first).
//
////////////////////////////////////////////////////////////////////////////////

bool TextSweepEffect::IsPositionDismissed (int row, float normalizedCol) const
{
    if (m_phase == SweepPhase::Done)
    {
        return true;
    }

    if (m_phase != SweepPhase::Dismissing)
    {
        return false;
    }

    if (row < 0 || row >= m_rowCount)
    {
        return false;
    }

    float dDelay           = m_rowDismissDelays[row];
    float dDuration        = m_rowDismissDurations[row];
    // Right-to-left: dismiss time proportional to (1 - normalizedCol)
    float dismissTime      = dDelay + dDuration * (1.0f - normalizedCol);
    float timeSinceDismiss = m_dismissTimer - dismissTime;



    // Considered dismissed when fade-out would reach zero
    return timeSinceDismiss >= (1.0f / m_fadeInSpeed);
}
