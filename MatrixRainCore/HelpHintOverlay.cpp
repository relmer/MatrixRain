#include "pch.h"

#include "HelpHintOverlay.h"

#include "SweepCommon.h"





////////////////////////////////////////////////////////////////////////////////
//
//  HelpHintOverlay::HelpHintOverlay
//
//  Initializes the three-line hint text and glyph set.
//
////////////////////////////////////////////////////////////////////////////////

HelpHintOverlay::HelpHintOverlay()
{
    m_allGlyphs = CharacterConstants::GetAllCodepoints();

    //
    // Build three-line hint layout:
    //   Settings      Enter
    //       Help      ?
    //       Exit      Esc
    //
    // Right-justified left column, left-justified right column,
    // separated by 6 spaces.
    //

    m_textLines =
    {
        L"Settings      Enter",
        L"    Help      ?",
        L"    Exit      Esc"
    };

    // Ensure text characters (like '?') that aren't in the
    // katakana/Latin/numeral glyph set get appended so they
    // resolve to the correct codepoint during scramble→reveal.
    for (const auto & line : m_textLines)
    {
        for (wchar_t wc : line)
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

    // Ensure uniform width
    size_t maxLen = 0;
    for (const auto & line : m_textLines)
    {
        maxLen = std::max (maxLen, line.size());
    }

    for (auto & line : m_textLines)
    {
        line.resize (maxLen, L' ');
    }

    m_rows     = static_cast<int>(m_textLines.size());
    m_textCols = static_cast<int>(maxLen);
    m_cols     = m_textCols + 2 * MARGIN_COLS;

    InitializeCharacters();

    // holdDuration = 3.0 → auto-dismiss after 3 seconds of holding
    m_sweep.Initialize (m_rows, 1.2f, 1.0f, 2.0f, 2.5f, 3.0f, 1.5f);
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpHintOverlay::InitializeCharacters
//
//  Creates the flat character array from the text lines.
//  All characters start in Hidden phase.
//
////////////////////////////////////////////////////////////////////////////////

void HelpHintOverlay::InitializeCharacters ()
{
    m_chars.clear();
    m_chars.reserve (m_rows * m_cols);



    for (int row = 0; row < m_rows; row++)
    {
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

        // Text columns
        for (int col = 0; col < m_textCols; col++)
        {
            HintCharacter ch;

            ch.row     = row;
            ch.col     = MARGIN_COLS + col;
            ch.isSpace = (m_textLines[row][col] == L' ');
            ch.phase   = CharPhase::Hidden;
            ch.opacity = 0.0f;

            // Find the target glyph index for the character
            if (!ch.isSpace)
            {
                uint32_t codepoint = static_cast<uint32_t>(m_textLines[row][col]);

                auto it = std::find (m_allGlyphs.begin(), m_allGlyphs.end(), codepoint);
                if (it != m_allGlyphs.end())
                {
                    ch.targetGlyphIndex = static_cast<size_t>(it - m_allGlyphs.begin());
                }
                else
                {
                    // Character not in glyph set — use index 0 as fallback
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
//  HelpHintOverlay::Show
//
//  Starts or restarts the reveal animation.
//
////////////////////////////////////////////////////////////////////////////////

void HelpHintOverlay::Show ()
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
//  HelpHintOverlay::Dismiss
//
//  Triggers dismiss sweep from Revealing or Holding.
//  No-op from Dismissing/Idle/Done.
//
////////////////////////////////////////////////////////////////////////////////

void HelpHintOverlay::Dismiss ()
{
    m_sweep.StartDismiss();
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpHintOverlay::Hide
//
//  Immediately hides the overlay.
//
////////////////////////////////////////////////////////////////////////////////

void HelpHintOverlay::Hide ()
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
//  HelpHintOverlay::Update
//
//  Advances the sweep effect and derives per-character state from it.
//
////////////////////////////////////////////////////////////////////////////////

void HelpHintOverlay::Update (float deltaTime)
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
//  HelpHintOverlay::GetPhase
//
//  Maps TextSweepEffect's SweepPhase to the public OverlayPhase.
//
////////////////////////////////////////////////////////////////////////////////

OverlayPhase HelpHintOverlay::GetPhase () const
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
//  HelpHintOverlay::UpdateLayout
//
//  Computes centered bounding rect for given viewport dimensions.
//
////////////////////////////////////////////////////////////////////////////////

void HelpHintOverlay::UpdateLayout (float viewportWidth, float viewportHeight)
{
    //
    // Estimate text block size based on character dimensions.
    // Margin columns fill part of the padding area; any remainder
    // is kept as edge padding.
    //

    float edgePadding = std::max (0.0f, PADDING - MARGIN_COLS * CHAR_WIDTH);
    float blockWidth  = m_cols * CHAR_WIDTH  + edgePadding * 2.0f;
    float blockHeight = m_rows * CHAR_HEIGHT + PADDING * 2.0f;

    m_boundingRect.left   = (viewportWidth  - blockWidth)  / 2.0f;
    m_boundingRect.top    = (viewportHeight - blockHeight) / 2.0f;
    m_boundingRect.right  = m_boundingRect.left + blockWidth;
    m_boundingRect.bottom = m_boundingRect.top  + blockHeight;
}
