#include "pch.h"

#include "HotkeyOverlay.h"

#include "SweepCommon.h"





////////////////////////////////////////////////////////////////////////////////
//
//  HotkeyOverlay::HotkeyOverlay
//
//  Builds the hotkey reference list and character grid.
//
////////////////////////////////////////////////////////////////////////////////

HotkeyOverlay::HotkeyOverlay()
{
    m_allGlyphs = CharacterConstants::GetAllCodepoints();

    BuildHotkeyList();

    // Ensure text characters (like '+', '-', '`', '?') that aren't in
    // the katakana/Latin/numeral glyph set get appended so they
    // resolve to the correct codepoint during scramble→reveal.
    for (const auto & entry : m_hotkeys)
    {
        for (wchar_t wc : entry.keyName)
        {
            if (wc == L' ')
            {
                continue;
            }

            uint32_t cp = static_cast<uint32_t> (wc);

            if (std::find (m_allGlyphs.begin(), m_allGlyphs.end(), cp) == m_allGlyphs.end())
            {
                m_allGlyphs.push_back (cp);
            }
        }

        for (wchar_t wc : entry.description)
        {
            if (wc == L' ')
            {
                continue;
            }

            uint32_t cp = static_cast<uint32_t> (wc);

            if (std::find (m_allGlyphs.begin(), m_allGlyphs.end(), cp) == m_allGlyphs.end())
            {
                m_allGlyphs.push_back (cp);
            }
        }
    }

    // Compute key column width (max key name length) and total columns
    m_keyColChars = 0;

    int maxDescChars = 0;

    for (const auto & entry : m_hotkeys)
    {
        m_keyColChars = std::max (m_keyColChars, static_cast<int> (entry.keyName.size()));
        maxDescChars  = std::max (maxDescChars,  static_cast<int> (entry.description.size()));
    }

    m_cols     = m_keyColChars + m_gapChars + maxDescChars;
    m_textCols = m_cols;
    m_cols     = m_textCols + 2 * MARGIN_COLS;

    InitializeCharacters();

    // holdDuration = -1 → hold indefinitely (dismissed by user)
    m_sweep.Initialize (GetRows(), 1.2f, 1.0f, 2.0f, 2.5f, -1.0f, 1.5f);
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
        { L"C",           L"Cycle color scheme" },
        { L"S",           L"Toggle statistics" },
        { L"?",           L"Help reference" },
        { L"+",           L"Increase rain density" },
        { L"-",           L"Decrease rain density" },
        { L"Alt+Enter",   L"Toggle fullscreen" },
        { L"Esc",         L"Exit" },
    };
}





////////////////////////////////////////////////////////////////////////////////
//
//  HotkeyOverlay::InitializeCharacters
//
//  Creates the flat character array from the hotkey entries.
//  Each row is: [key name padded to m_keyColChars] [gap] [description padded]
//  All characters start in Hidden phase.
//
////////////////////////////////////////////////////////////////////////////////

void HotkeyOverlay::InitializeCharacters ()
{
    m_chars.clear();
    m_chars.reserve (static_cast<size_t> (GetRows()) * m_cols);

    int maxDescChars = m_textCols - m_keyColChars - m_gapChars;



    for (int row = 0; row < GetRows(); row++)
    {
        // Build the full row string: key (right-padded) + gap + description (right-padded)
        std::wstring rowText;

        rowText.reserve (m_textCols);

        // Key name, padded to m_keyColChars
        std::wstring key = m_hotkeys[row].keyName;

        key.resize (m_keyColChars, L' ');
        rowText += key;

        // Gap (spaces)
        rowText.append (m_gapChars, L' ');

        // Description, padded to maxDescChars
        std::wstring desc = m_hotkeys[row].description;

        desc.resize (maxDescChars, L' ');
        rowText += desc;

        // Left margin columns (fill PADDING area)
        for (int m = 0; m < MARGIN_COLS; m++)
        {
            HintCharacter ch;

            ch.row     = row;
            ch.col     = m;
            ch.isSpace = true;
            ch.phase   = CharPhase::Hidden;
            ch.opacity = 0.0f;

            m_chars.push_back (ch);
        }

        // Text columns (key + gap + desc)
        for (int col = 0; col < m_textCols; col++)
        {
            HintCharacter ch;

            ch.row     = row;
            ch.col     = MARGIN_COLS + col;
            ch.isSpace = (rowText[col] == L' ');
            ch.phase   = CharPhase::Hidden;
            ch.opacity = 0.0f;

            // Find the target glyph index for the character
            if (!ch.isSpace)
            {
                uint32_t codepoint = static_cast<uint32_t> (rowText[col]);

                auto it = std::find (m_allGlyphs.begin(), m_allGlyphs.end(), codepoint);
                if (it != m_allGlyphs.end())
                {
                    ch.targetGlyphIndex = static_cast<size_t> (it - m_allGlyphs.begin());
                }
                else
                {
                    ch.targetGlyphIndex = 0;
                }
            }

            ch.currentGlyphIndex = ch.targetGlyphIndex;

            m_chars.push_back (ch);
        }

        // Right margin columns (fill PADDING area)
        for (int m = 0; m < MARGIN_COLS; m++)
        {
            HintCharacter ch;

            ch.row     = row;
            ch.col     = MARGIN_COLS + m_textCols + m;
            ch.isSpace = true;
            ch.phase   = CharPhase::Hidden;
            ch.opacity = 0.0f;

            m_chars.push_back (ch);
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  HotkeyOverlay::Show
//
//  Starts or restarts the reveal animation.
//
////////////////////////////////////////////////////////////////////////////////

void HotkeyOverlay::Show ()
{
    // Reset all characters to Hidden so the new sweep starts clean
    for (auto & ch : m_chars)
    {
        ch.phase         = CharPhase::Hidden;
        ch.opacity       = 0.0f;
        ch.glowIntensity = 0.0f;
    }

    m_sweep.StartReveal();
}





////////////////////////////////////////////////////////////////////////////////
//
//  HotkeyOverlay::Dismiss
//
//  Triggers dismiss sweep from Revealing or Holding.
//  No-op from Dismissing/Idle/Done.
//
////////////////////////////////////////////////////////////////////////////////

void HotkeyOverlay::Dismiss ()
{
    m_sweep.StartDismiss();
}





////////////////////////////////////////////////////////////////////////////////
//
//  HotkeyOverlay::Hide
//
//  Immediately hides the overlay with no animation.
//
////////////////////////////////////////////////////////////////////////////////

void HotkeyOverlay::Hide ()
{
    m_sweep.Reset();

    for (auto & ch : m_chars)
    {
        ch.phase         = CharPhase::Hidden;
        ch.opacity       = 0.0f;
        ch.glowIntensity = 0.0f;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  HotkeyOverlay::Update
//
//  Advances the sweep effect and derives per-character state from it.
//
////////////////////////////////////////////////////////////////////////////////

void HotkeyOverlay::Update (float deltaTime)
{
    m_sweep.Update (deltaTime);

    // Pass 1: update phase, opacity, glow for each character.
    for (auto & ch : m_chars)
    {
        float normCol = (m_cols > 1)
                       ? static_cast<float>(ch.col) / static_cast<float>(m_cols - 1)
                       : 0.0f;

        float opacity = m_sweep.GetOpacity (ch.row, normCol);

        // Reset streak fields each frame
        ch.inStreakZone    = false;
        ch.streakIntensity = 0.0f;
        ch.isHead          = false;

        if (opacity > 0.0f)
        {
            // Assign a random glyph once on first reveal (Hidden → Resolved)
            if (ch.phase == CharPhase::Hidden)
            {
                size_t charIndex    = static_cast<size_t>(ch.row * m_cols + ch.col);
                ch.randomGlyphIndex = (charIndex * 7 + 13) % m_allGlyphs.size();
            }

            ch.phase         = CharPhase::Resolved;
            ch.opacity       = opacity;
            ch.glowIntensity = m_sweep.GetGlowIntensity (ch.row, normCol);
        }
        else
        {
            ch.phase         = CharPhase::Hidden;
            ch.opacity       = 0.0f;
            ch.glowIntensity = 0.0f;
        }
    }

    // Pass 2: streak detection, glyph selection, brightness computation.
    UpdateSweepStreaks (std::span<HintCharacter>(m_chars), m_cols, m_sweep, m_allGlyphs.size(), deltaTime);
}





////////////////////////////////////////////////////////////////////////////////
//
//  HotkeyOverlay::GetPhase
//
//  Maps TextSweepEffect's SweepPhase to the public OverlayPhase.
//
////////////////////////////////////////////////////////////////////////////////

OverlayPhase HotkeyOverlay::GetPhase () const
{
    switch (m_sweep.GetPhase())
    {
        case SweepPhase::Revealing:  return OverlayPhase::Revealing;
        case SweepPhase::Holding:    return OverlayPhase::Holding;
        case SweepPhase::Dismissing: return OverlayPhase::Dissolving;

        case SweepPhase::Idle:
        case SweepPhase::Done:
        default:                     return OverlayPhase::Hidden;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  HotkeyOverlay::SetMeasuredLayout
//
//  Stores bounding rect and key column width computed by RenderSystem
//  using DWrite text metrics.
//
////////////////////////////////////////////////////////////////////////////////

void HotkeyOverlay::SetMeasuredLayout (D2D1_RECT_F boundingRect, float keyColumnWidth)
{
    m_boundingRect   = boundingRect;
    m_keyColumnWidth = keyColumnWidth;
}
