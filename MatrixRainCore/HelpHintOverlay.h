#pragma once

#include "CharacterConstants.h"
#include "TextSweepEffect.h"





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
    Resolved,
    Hidden
};





////////////////////////////////////////////////////////////////////////////////
//
//  HintCharacter — Per-character animation state
//
////////////////////////////////////////////////////////////////////////////////

struct HintCharacter
{
    size_t    targetGlyphIndex  = 0;
    size_t    currentGlyphIndex = 0;
    size_t    randomGlyphIndex  = 0;
    CharPhase phase             = CharPhase::Hidden;
    float     opacity           = 0.0f;
    float     glowIntensity     = 0.0f;
    float     brightenTimer     = 0.0f;
    float     streakIntensity   = 0.0f;
    int       row               = 0;
    int       col               = 0;
    bool      isSpace           = false;
    bool      inStreakZone      = false;
    bool      isHead            = false;
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
    // Layout constants
    static constexpr float CHAR_WIDTH   = 16.0f;
    static constexpr float CHAR_HEIGHT  = 28.0f;
    static constexpr float PADDING      = 20.0f;
    static constexpr int   GLOW_LAYERS  = 4;
    static constexpr int   MARGIN_COLS  = 1;


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
    bool                             IsActive()        const { return m_sweep.IsActive();                       }
    OverlayPhase                     GetPhase()         const;
    D2D1_RECT_F                      GetBoundingRect()  const { return m_boundingRect;                           }
    std::span<const HintCharacter>   GetCharacters()    const { return std::span<const HintCharacter>(m_chars);  }
    std::span<const uint32_t>        GetAllGlyphs()     const { return std::span<const uint32_t>(m_allGlyphs);   }
    int                              GetRows()          const { return m_rows;                                   }
    int                              GetCols()          const { return m_cols;                                   }


private:
    void InitializeCharacters();


    // Sweep effect (handles all timing / phase transitions)
    TextSweepEffect                  m_sweep;

    // Character grid
    std::vector<HintCharacter>       m_chars;
    int                              m_rows     = 0;
    int                              m_cols     = 0;
    int                              m_textCols = 0;

    // Text content
    std::vector<std::wstring>        m_textLines;

    // Bounding rect for rendering / occlusion
    D2D1_RECT_F                      m_boundingRect = {};

    // Glyph set for scrambling
    std::vector<uint32_t>            m_allGlyphs;
};
