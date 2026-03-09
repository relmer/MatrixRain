#pragma once

#include "CharacterConstants.h"
#include "HelpHintOverlay.h"
#include "UsageText.h"





////////////////////////////////////////////////////////////////////////////////
//  HotkeyOverlay — In-app overlay showing runtime hotkey reference
//
//  Rendered directly on the main MatrixRain window via RenderSystem.
//  Shows all runtime hotkeys with descriptions.
//  Uses per-character scramble/resolve animation identical to HelpHintOverlay.
//
//  Phase transition:
//    Hidden → Revealing → Holding → Dissolving → Hidden
////////////////////////////////////////////////////////////////////////////////

class HotkeyOverlay
{
public:
    // Layout base constants (at 96 DPI / 100%)
    static constexpr float BASE_ROW_HEIGHT = 28.0f;
    static constexpr float BASE_PADDING    = 30.0f;
    static constexpr float BASE_GAP        = 16.0f;
    static constexpr int   GLOW_LAYERS     = 4;
    static constexpr int   MARGIN_COLS     = 2;


    HotkeyOverlay();


    // DPI scaling
    void  SetDpiScale  (float dpiScale);
    float GetRowHeight() const { return BASE_ROW_HEIGHT * m_dpiScale; }
    float GetPadding()   const { return BASE_PADDING    * m_dpiScale; }
    float GetGap()       const { return BASE_GAP        * m_dpiScale; }


    // State control
    void Show();
    void Dismiss();
    void Hide();


    // Update (called once per render frame)
    void Update (float deltaTime);


    // Layout
    void SetMeasuredLayout (D2D1_RECT_F boundingRect, float keyColumnWidth);


    // Queries
    bool                                      IsActive ()           const { return m_scramble.IsActive();                               }
    OverlayPhase                              GetPhase ()           const;
    D2D1_RECT_F                               GetBoundingRect ()    const { return m_boundingRect;                                      }
    float                                     GetKeyColumnWidth ()  const { return m_keyColumnWidth;                                    }
    const std::vector<HotkeyEntry>          & GetHotkeys ()         const { return m_hotkeys;                                           }
    int                                       GetRows ()            const { return static_cast<int> (m_hotkeys.size());                 }
    std::span<const HintCharacter>            GetCharacters ()      const { return std::span<const HintCharacter> (m_chars);            }
    std::span<const uint32_t>                 GetAllGlyphs ()       const { return std::span<const uint32_t> (m_allGlyphs);             }
    int                                       GetCols ()            const { return m_cols;                                              }
    int                                       GetCharRows ()        const { return static_cast<int> (m_hotkeys.size());                 }


private:
    void BuildHotkeyList();
    void InitializeCharacters();


    // Scramble-reveal effect (handles all timing / phase transitions)
    ScrambleRevealEffect            m_scramble;

    // Content
    std::vector<HotkeyEntry>        m_hotkeys;

    // Character grid (margin + key column + gap + desc column + margin per row)
    std::vector<HintCharacter>      m_chars;
    int                             m_cols          = 0;
    int                             m_textCols      = 0;
    int                             m_keyColChars   = 0;
    int                             m_gapChars      = 2;

    // Glyph set for scrambling
    std::vector<uint32_t>           m_allGlyphs;

    // Bounding rect for rendering
    D2D1_RECT_F                     m_boundingRect    = {};
    float                           m_keyColumnWidth  = 0.0f;

    // DPI scale factor (1.0 at 96 DPI / 100%)
    float                           m_dpiScale        = 1.0f;
};
