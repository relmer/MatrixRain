#pragma once

#include "Overlay.h"





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
    // Layout base constants (at 96 DPI / 100%)
    static constexpr float BASE_CHAR_WIDTH  = 16.0f;
    static constexpr float BASE_CHAR_HEIGHT = 28.0f;
    static constexpr float BASE_PADDING     = 20.0f;
    static constexpr float BASE_GAP         = 40.0f;
    static constexpr int   GLOW_LAYERS      = 8;
    static constexpr int   MARGIN_COLS      = 1;


    HelpHintOverlay();


    // DPI scaling
    void  SetDpiScale (float dpiScale);
    float GetCharWidth()  const { return BASE_CHAR_WIDTH  * m_dpiScale; }
    float GetCharHeight() const { return BASE_CHAR_HEIGHT * m_dpiScale; }
    float GetPadding()    const { return BASE_PADDING     * m_dpiScale; }
    float GetGap()        const { return BASE_GAP         * m_dpiScale; }


    // State control
    void Show();
    void Dismiss();
    void Hide();


    // Update (called once per render frame)
    void Update (float deltaTime, float schemeR = 0.0f, float schemeG = 1.0f, float schemeB = 0.0f);


    // Layout
    void UpdateLayout (float viewportWidth, float viewportHeight);


    // Queries
    bool                             IsActive()        const { return m_scramble.IsActive();                    }
    OverlayPhase                     GetPhase()         const;
    D2D1_RECT_F                      GetBoundingRect()  const { return m_boundingRect;                           }
    std::span<const HintCharacter>   GetCharacters()    const { return std::span<const HintCharacter>(m_chars);  }
    std::span<HintCharacter>         GetMutableCharacters()   { return std::span<HintCharacter>(m_chars);        }
    int                              GetRows()          const { return m_rows;                                   }
    int                              GetCols()          const { return m_cols;                                   }
    int                              GetTextCols()      const { return m_textCols;                               }
    int                              GetLeftColChars()  const { return m_leftColChars;                            }
    int                              GetGapChars()      const { return m_gapChars;                                }
    const std::vector<std::wstring> & GetTextLines()    const { return m_textLines;                              }
    const std::vector<std::wstring> & GetLeftTexts()    const { return m_leftTexts;                              }
    const std::vector<std::wstring> & GetRightTexts()   const { return m_rightTexts;                             }
    void                             SetBoundingRect (D2D1_RECT_F rect) { m_boundingRect = rect;                 }


private:
    void InitializeCharacters();
    void ResolveGlyphIndices();


    // Scramble-reveal effect (handles all timing / phase transitions)
    ScrambleRevealEffect                 m_scramble { 2.5f, 1.0f, 0.25f, 1.0f, 2.7f };

    // Character grid
    std::vector<HintCharacter>       m_chars;
    int                              m_rows     = 0;
    int                              m_cols     = 0;
    int                              m_textCols = 0;

    // Text content
    std::vector<std::wstring>        m_textLines;
    std::vector<std::wstring>        m_leftTexts;
    std::vector<std::wstring>        m_rightTexts;
    int                              m_leftColChars = 0;
    int                              m_gapChars     = 0;

    // Bounding rect for rendering / occlusion
    D2D1_RECT_F                      m_boundingRect = {};

    // DPI scale factor (1.0 at 96 DPI / 100%)
    float                            m_dpiScale = 1.0f;
};
