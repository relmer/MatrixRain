#pragma once





////////////////////////////////////////////////////////////////////////////////
//
//  ScramblePhase — Overall animation phase
//
////////////////////////////////////////////////////////////////////////////////

enum class ScramblePhase
{
    Idle,
    Revealing,
    Holding,
    Dismissing,
    Done
};





////////////////////////////////////////////////////////////////////////////////
//
//  CellPhase — Per-cell animation phase
//
////////////////////////////////////////////////////////////////////////////////

enum class CellPhase
{
    Hidden,
    Cycling,
    LockFlash,
    Settled,
    Dismissing
};





////////////////////////////////////////////////////////////////////////////////
//
//  CellState — Per-cell animation state (internal to ScrambleRevealEffect)
//
////////////////////////////////////////////////////////////////////////////////

struct CellState
{
    CellPhase phase         = CellPhase::Hidden;
    float     lockTime      = 0.0f;     // Time at which cell locks (reveal) or unlocks (dismiss)
    float     flashTimer    = 0.0f;     // Countdown from lock flash to settled
    float     flashDuration = 0.0f;     // Per-cell flash duration (capped for tail sync)
    float     cycleTimer    = 0.0f;     // Accumulator for glyph cycling interval
    float     opacity       = 0.0f;     // 0 = invisible, 1 = fully visible
    bool      needsCycle    = false;    // True when consumer should pick a new random glyph
    bool      isSpace       = false;    // Spaces don't cycle
};





////////////////////////////////////////////////////////////////////////////////
//
//  ScrambleRevealEffect — Per-cell scramble/lock timing oracle
//
//  All non-space cells simultaneously cycle through random glyphs,
//  then each independently "locks" onto its target character at a
//  random time within the reveal duration.  Dismissal reverses: cells
//  randomly unlock back into cycling, then fade out.
//
//  Does NOT own glyph data — consumers query cell state and pick
//  glyphs accordingly.
//
////////////////////////////////////////////////////////////////////////////////

class ScrambleRevealEffect
{
public:
    ScrambleRevealEffect (float revealDuration,
                          float dismissDuration,
                          float cycleInterval,
                          float flashDuration,
                          float holdDuration);


    void SetCellCount (int cellCount);
    void MarkSpace    (int index);


    // Phase transitions
    void StartReveal();
    void StartDismiss();
    void Reset();


    // Advance animation
    void Update (float deltaTime);


    // Global queries
    ScramblePhase GetPhase()            const { return m_phase;            }
    bool          IsActive()            const;
    bool          IsRevealComplete()    const;
    bool          IsDismissComplete()   const;
    float         GetPostRevealTimer()  const { return m_postRevealTimer;  }
    float         GetRevealTimer()      const { return m_revealTimer;      }


    // Per-cell queries
    const CellState & GetCell (int index) const { return m_cells[index];                    }
    int               GetCellCount()      const { return static_cast<int> (m_cells.size()); }


private:
    void UpdateRevealing    (float deltaTime);
    void UpdateHolding      (float deltaTime);
    void UpdateDismissing   (float deltaTime);
    void UpdateRevealingCell (CellState & cell, float deltaTime, float fadeDuration);
    void UpdateLockFlash     (CellState & cell, float deltaTime);
    void AdvanceCycleTimer   (CellState & cell, float deltaTime);
    void FadeOpacity         (CellState & cell, float deltaTime, float fadeDuration, float target);

    ScramblePhase          m_phase            = ScramblePhase::Idle;
    float                  m_revealTimer      = 0.0f;
    float                  m_dismissTimer     = 0.0f;
    float                  m_postRevealTimer  = 0.0f;
    float                  m_holdStartTime    = 0.0f;

    // Configuration (immutable after construction)
    const float            m_revealDuration;
    const float            m_dismissDuration;
    const float            m_cycleInterval;
    const float            m_flashDuration;
    const float            m_holdDuration;

    // Per-cell state
    std::vector<CellState> m_cells;

    // RNG
    std::mt19937           m_rng { std::random_device{}() };
};
