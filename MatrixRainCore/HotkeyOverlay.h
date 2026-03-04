#pragma once

#include "UsageText.h"





////////////////////////////////////////////////////////////////////////////////
//  HotkeyOverlayPhase — Overlay-level phase
////////////////////////////////////////////////////////////////////////////////

enum class HotkeyOverlayPhase
{
    Hidden,
    Visible,
    Dissolving
};





////////////////////////////////////////////////////////////////////////////////
//  HotkeyOverlay — In-app overlay showing runtime hotkey reference
//
//  Rendered directly on the main MatrixRain window via RenderSystem.
//  Shows all runtime hotkeys with descriptions. Phase transition:
//    Hidden → Visible → Dissolving → Hidden
////////////////////////////////////////////////////////////////////////////////

class HotkeyOverlay
{
public:
    HotkeyOverlay();


    // State control
    void Show();
    void Dismiss();


    // Update (called once per render frame)
    void Update (float deltaTime);


    // Layout
    void UpdateLayout (float viewportWidth, float viewportHeight);


    // Queries
    bool                                      IsActive ()       const { return m_phase != HotkeyOverlayPhase::Hidden;   }
    HotkeyOverlayPhase                        GetPhase ()        const { return m_phase;                                 }
    float                                     GetOpacity ()      const { return m_opacity;                               }
    D2D1_RECT_F                               GetBoundingRect () const { return m_boundingRect;                          }
    const std::vector<HotkeyEntry>          & GetHotkeys ()      const { return m_hotkeys;                               }
    int                                       GetRows ()         const { return static_cast<int> (m_hotkeys.size());     }


private:
    void BuildHotkeyList();


    // Overlay state
    HotkeyOverlayPhase              m_phase   = HotkeyOverlayPhase::Hidden;
    float                           m_opacity = 0.0f;

    // Content
    std::vector<HotkeyEntry>        m_hotkeys;

    // Bounding rect for rendering
    D2D1_RECT_F                     m_boundingRect = {};

    // Timing constants
    static constexpr float DISSOLVE_DURATION = 0.5f;
};
