#pragma once





////////////////////////////////////////////////////////////////////////////////
//
//  SweepPhase — Overall animation phase
//
////////////////////////////////////////////////////////////////////////////////

enum class SweepPhase
{
    Idle,
    Revealing,
    Holding,
    Dismissing,
    Done
};





////////////////////////////////////////////////////////////////////////////////
//
//  TextSweepEffect — Per-row horizontal sweep timing oracle
//
//  Manages a left-to-right sweep that reveals text row by row with
//  staggered random delays and durations.  Supports an optional hold
//  phase followed by a right-to-left dismiss sweep.
//
//  Does NOT own character data — consumers query IsPositionRevealed()
//  and GetOpacity() / GetGlowIntensity() for each character's
//  normalized column position (0.0 = leftmost, 1.0 = rightmost).
//
////////////////////////////////////////////////////////////////////////////////

class TextSweepEffect
{
public:
    TextSweepEffect();


    // Configuration
    void Initialize (int   rowCount,
                     float staggerRange  = 0.8f,
                     float durationMin   = 0.5f,
                     float durationMax   = 1.0f,
                     float fadeInSpeed   = 2.5f,
                     float holdDuration  = -1.0f,
                     float glowDecaySpeed = 3.0f);


    // Phase transitions
    void StartReveal();
    void StartDismiss();
    void Reset();


    // Advance animation
    void Update (float deltaTime);


    // Queries
    SweepPhase GetPhase()        const { return m_phase;       }
    float      GetRevealTimer()  const { return m_revealTimer; }
    bool       IsActive()        const;
    bool       IsRevealComplete()  const;
    bool       IsDismissComplete() const;


    // Per-position queries (normalizedCol in 0..1)
    float GetRevealProgress    (int row)                        const;
    bool  IsPositionRevealed   (int row, float normalizedCol)   const;
    float GetOpacity           (int row, float normalizedCol)   const;
    float GetGlowIntensity     (int row, float normalizedCol)   const;
    float GetDismissProgress   (int row)                        const;
    bool  IsPositionDismissed  (int row, float normalizedCol)   const;


private:
    SweepPhase            m_phase          = SweepPhase::Idle;
    float                 m_revealTimer    = 0.0f;
    float                 m_holdStartTime  = 0.0f;
    float                 m_dismissTimer   = 0.0f;
    float                 m_fadeInSpeed    = 2.5f;
    float                 m_holdDuration   = -1.0f;
    float                 m_glowDecaySpeed = 3.0f;
    int                   m_rowCount       = 0;

    std::vector<float>    m_rowRevealDelays;
    std::vector<float>    m_rowRevealDurations;
    std::vector<float>    m_rowDismissDelays;
    std::vector<float>    m_rowDismissDurations;

    float                 m_staggerRange   = 0.8f;
    float                 m_durationMin    = 0.5f;
    float                 m_durationMax    = 1.0f;

    std::mt19937          m_rng { std::random_device{}() };
};
