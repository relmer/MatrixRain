#include "pch.h"

#include "HotkeyOverlay.h"





////////////////////////////////////////////////////////////////////////////////
//
//  HotkeyOverlay::HotkeyOverlay
//
//  Builds the hotkey reference list.
//
////////////////////////////////////////////////////////////////////////////////

HotkeyOverlay::HotkeyOverlay()
{
    BuildHotkeyList();
}





////////////////////////////////////////////////////////////////////////////////
//
//  HotkeyOverlay::BuildHotkeyList
//
//  Populates the hotkey table with all runtime hotkeys.
//
////////////////////////////////////////////////////////////////////////////////

void HotkeyOverlay::BuildHotkeyList ()
{
    m_hotkeys =
    {
        { L"Space",       L"Pause / Resume" },
        { L"Enter",       L"Settings dialog" },
        { L"C",           L"Settings dialog" },
        { L"?",           L"Help reference" },
        { L"+",           L"Increase rain density" },
        { L"-",           L"Decrease rain density" },
        { L"`",           L"Toggle FPS counter" },
        { L"Alt+Enter",   L"Toggle fullscreen" },
        { L"Esc",         L"Exit" },
    };
}





////////////////////////////////////////////////////////////////////////////////
//
//  HotkeyOverlay::Show
//
//  Transitions to Visible. No-op if already Visible.
//
////////////////////////////////////////////////////////////////////////////////

void HotkeyOverlay::Show ()
{
    if (m_phase == HotkeyOverlayPhase::Visible)
    {
        return;
    }

    m_phase   = HotkeyOverlayPhase::Visible;
    m_opacity = 1.0f;
}





////////////////////////////////////////////////////////////////////////////////
//
//  HotkeyOverlay::Dismiss
//
//  Transitions to Dissolving. No-op if already Hidden.
//
////////////////////////////////////////////////////////////////////////////////

void HotkeyOverlay::Dismiss ()
{
    if (m_phase == HotkeyOverlayPhase::Hidden)
    {
        return;
    }

    if (m_phase == HotkeyOverlayPhase::Dissolving)
    {
        return;
    }

    m_phase = HotkeyOverlayPhase::Dissolving;
}





////////////////////////////////////////////////////////////////////////////////
//
//  HotkeyOverlay::Update
//
//  Advances dissolve animation — opacity decreases, transitions to Hidden
//  when fully faded.
//
////////////////////////////////////////////////////////////////////////////////

void HotkeyOverlay::Update (float deltaTime)
{
    if (m_phase != HotkeyOverlayPhase::Dissolving)
    {
        return;
    }

    m_opacity -= deltaTime / DISSOLVE_DURATION;

    if (m_opacity <= 0.0f)
    {
        m_opacity = 0.0f;
        m_phase   = HotkeyOverlayPhase::Hidden;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  HotkeyOverlay::UpdateLayout
//
//  Computes centered bounding rect for the hotkey overlay.
//
////////////////////////////////////////////////////////////////////////////////

void HotkeyOverlay::UpdateLayout (float viewportWidth, float viewportHeight)
{
    // Estimate size based on content
    static constexpr float ROW_HEIGHT = 28.0f;
    static constexpr float PADDING    = 30.0f;
    static constexpr float COL_WIDTH  = 300.0f;

    float contentHeight = static_cast<float> (m_hotkeys.size()) * ROW_HEIGHT + 2.0f * PADDING;
    float contentWidth  = COL_WIDTH + 2.0f * PADDING;

    float left = (viewportWidth  - contentWidth)  / 2.0f;
    float top  = (viewportHeight - contentHeight) / 2.0f;

    m_boundingRect.left   = left;
    m_boundingRect.top    = top;
    m_boundingRect.right  = left + contentWidth;
    m_boundingRect.bottom = top  + contentHeight;
}
