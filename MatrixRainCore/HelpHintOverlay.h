#pragma once

#include "CharacterConstants.h"





////////////////////////////////////////////////////////////////////////////////
//
//  OverlayPhase — Overlay-level animation phase
//
////////////////////////////////////////////////////////////////////////////////

enum class OverlayPhase
{
    Hidden,
    Revealing,
    Holding,
    Dissolving
};





////////////////////////////////////////////////////////////////////////////////
//
//  CharPhase — Per-character animation phase
//
////////////////////////////////////////////////////////////////////////////////

enum class CharPhase
{
    Scrambling,
    Resolved,
    DissolveCycling,
    DissolveFading,
    Hidden
};





////////////////////////////////////////////////////////////////////////////////
//
//  HintCharacter — Per-character animation state
//
////////////////////////////////////////////////////////////////////////////////

struct HintCharacter
{
    size_t   targetGlyphIndex   = 0;
    size_t   currentGlyphIndex  = 0;
    CharPhase phase             = CharPhase::Hidden;
    float    opacity            = 0.0f;
    float    scrambleTimer      = 0.0f;
    float    scrambleInterval   = 0.05f;
    float    dissolveStartOffset = 0.0f;
    int      row                = 0;
    int      col                = 0;
    bool     isSpace            = false;
};





////////////////////////////////////////////////////////////////////////////////
//
//  HelpHintOverlay — State machine for the three-line help hint
//
//  Text layout:
//      Settings      Enter
//          Help      ?
//          Exit      Esc
//
////////////////////////////////////////////////////////////////////////////////

class HelpHintOverlay
{
public:
    HelpHintOverlay();


    // State control
    void Show();
    void Dismiss();
    void Hide();


    // Update (called once per render frame)
    void Update (float deltaTime);


    // Layout
    void UpdateLayout (float viewportWidth, float viewportHeight);


    // Queries
    bool                             IsActive()        const { return m_phase != OverlayPhase::Hidden;          }
    OverlayPhase                     GetPhase()         const { return m_phase;                                  }
    D2D1_RECT_F                      GetBoundingRect()  const { return m_boundingRect;                           }
    std::span<const HintCharacter>   GetCharacters()    const { return std::span<const HintCharacter>(m_chars);  }
    int                              GetRows()          const { return m_rows;                                   }
    int                              GetCols()          const { return m_cols;                                   }


private:
    void InitializeCharacters();
    void InitializeCharactersForReveal();
    void UpdateRevealing  (float deltaTime);
    void UpdateHolding    (float deltaTime);
    void UpdateDissolving (float deltaTime);


    // Overlay state
    OverlayPhase                 m_phase        = OverlayPhase::Hidden;
    float                        m_phaseTimer   = 0.0f;
    float                        m_holdDuration = 3.0f;

    // Character grid
    std::vector<HintCharacter>   m_chars;
    int                          m_rows = 0;
    int                          m_cols = 0;

    // Text content
    std::vector<std::wstring>    m_textLines;

    // Bounding rect for rendering / occlusion
    D2D1_RECT_F                  m_boundingRect = {};

    // Glyph set for scrambling
    std::vector<uint32_t>        m_allGlyphs;

    // RNG
    std::mt19937                 m_rng { std::random_device{}() };

    // Timing constants
    static constexpr float SCRAMBLE_MIN_INTERVAL = 0.03f;
    static constexpr float SCRAMBLE_MAX_INTERVAL = 0.08f;
    static constexpr float REVEAL_STAGGER_RANGE  = 0.8f;
    static constexpr float DISSOLVE_STAGGER_RANGE = 1.0f;
    static constexpr float DISSOLVE_CYCLE_DURATION = 0.3f;
    static constexpr float DISSOLVE_FADE_DURATION  = 0.4f;
};
