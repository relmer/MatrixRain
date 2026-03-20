#pragma once

#include "HelpHintOverlay.h"
#include "UsageText.h"





////////////////////////////////////////////////////////////////////////////////
//  UsageOverlay — In-app overlay showing command-line usage text
//
//  Rendered via the same GPU instancing + SDF halo + bloom pipeline as
//  HelpHintOverlay and HotkeyOverlay.  Displays formatted usage text
//  (version header, switches) with scramble-reveal animation.
//
//  Used by /? invocation — Application runs in HelpRequested mode with
//  a smaller window.  The overlay reveals once, then holds indefinitely.
//  Enter or Esc exits the application.
////////////////////////////////////////////////////////////////////////////////

class UsageOverlay
{
public:
    // Layout base constants (at 96 DPI / 100%)
    static constexpr float BASE_CHAR_WIDTH = 16.0f;
    static constexpr float BASE_ROW_HEIGHT = 28.0f;
    static constexpr float BASE_PADDING    = 30.0f;
    static constexpr int   MARGIN_COLS     = 2;


    explicit UsageOverlay (wchar_t switchPrefix);


    // DPI scaling
    void  SetDpiScale    (float dpiScale);
    float GetCharWidth() const { return BASE_CHAR_WIDTH * m_dpiScale; }
    float GetRowHeight() const { return BASE_ROW_HEIGHT * m_dpiScale; }
    float GetPadding()   const { return BASE_PADDING    * m_dpiScale; }


    // State control
    void Show();


    // Update (called once per render frame)
    void Update (float deltaTime, float schemeR, float schemeG, float schemeB);


    // Queries
    bool                           IsActive()        const { return m_scramble.IsActive();                    }
    bool                           IsRevealComplete() const { return m_scramble.IsRevealComplete();            }
    std::span<const HintCharacter> GetCharacters()    const { return std::span<const HintCharacter> (m_chars); }
    int                            GetCharRows()      const { return m_numRows;                                }
    int                            GetKeyColChars()   const { return 0;                                        }
    int                            GetGapChars()      const { return 0;                                        }
    float                          GetCharHeight()    const { return GetRowHeight();                            }


private:
    void BuildLines (wchar_t switchPrefix);
    void InitializeCharacters();
    void ResolveGlyphIndices();


    // Scramble-reveal effect: 1.8s reveal, no dismiss, 0.065s cycle, 1.0s flash, hold forever
    ScrambleRevealEffect            m_scramble { 1.8f, 0.0f, 0.065f, 1.0f, -1.0f };

    // Text lines from UsageText
    std::vector<std::wstring>       m_lines;

    // Character grid
    std::vector<HintCharacter>      m_chars;
    int                             m_numRows   = 0;
    int                             m_cols      = 0;
    int                             m_textCols  = 0;

    // DPI scale factor (1.0 at 96 DPI / 100%)
    float                           m_dpiScale  = 1.0f;
};
