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
    HotkeyOverlay();


    // State control
    void Show();
    void Dismiss();
    void Hide();


    // Update (called once per render frame)
    void Update (float deltaTime);


    // Layout
    void SetMeasuredLayout (D2D1_RECT_F boundingRect, float keyColumnWidth);


    // Queries
    bool                                      IsActive ()          const { return m_phase != OverlayPhase::Hidden;                     }
    OverlayPhase                              GetPhase ()           const { return m_phase;                                             }
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
    void InitializeCharactersForReveal();
    void UpdateRevealing  (float deltaTime);
    void UpdateDissolving (float deltaTime);


    // Overlay state
    OverlayPhase                    m_phase      = OverlayPhase::Hidden;
    float                           m_phaseTimer = 0.0f;

    // Content
    std::vector<HotkeyEntry>        m_hotkeys;

    // Character grid (key column + gap + desc column per row)
    std::vector<HintCharacter>      m_chars;
    int                             m_cols          = 0;
    int                             m_keyColChars   = 0;
    int                             m_gapChars      = 2;

    // Glyph set for scrambling
    std::vector<uint32_t>           m_allGlyphs;

    // RNG
    std::mt19937                    m_rng { std::random_device{}() };

    // Bounding rect for rendering
    D2D1_RECT_F                     m_boundingRect    = {};
    float                           m_keyColumnWidth  = 0.0f;

    // Timing constants
    static constexpr float SCRAMBLE_MIN_INTERVAL   = 0.03f;
    static constexpr float SCRAMBLE_MAX_INTERVAL   = 0.08f;
    static constexpr float REVEAL_STAGGER_RANGE    = 0.8f;
    static constexpr float DISSOLVE_STAGGER_RANGE  = 1.0f;
    static constexpr float DISSOLVE_CYCLE_DURATION = 0.3f;
    static constexpr float DISSOLVE_FADE_DURATION  = 0.4f;
};
