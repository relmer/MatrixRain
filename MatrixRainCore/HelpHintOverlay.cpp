#include "pch.h"

#include "HelpHintOverlay.h"

#include "OverlayColor.h"





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

    // Initialize the scramble-reveal effect
    int cellCount = static_cast<int>(m_chars.size());

    m_scramble.Initialize (cellCount, 1.2f, 1.0f, 0.065f, 1.0f, 3.0f);

    for (int i = 0; i < cellCount; i++)
    {
        if (m_chars[i].isSpace)
        {
            m_scramble.MarkSpace (i);
        }
    }
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
    // Reset all characters to Hidden so the new reveal starts clean
    for (auto & ch : m_chars)
    {
        ch.phase         = CharPhase::Hidden;
        ch.opacity       = 0.0f;
        ch.glowIntensity = 0.0f;
    }

    m_scramble.StartReveal();
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpHintOverlay::Dismiss
//
//  Triggers dismiss from Revealing or Holding.
//  No-op from Dismissing/Idle/Done.
//
////////////////////////////////////////////////////////////////////////////////

void HelpHintOverlay::Dismiss ()
{
    m_scramble.StartDismiss();
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
    m_scramble.Reset();

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
//  Advances the scramble-reveal effect and derives per-character state from it.
//
////////////////////////////////////////////////////////////////////////////////

void HelpHintOverlay::Update (float deltaTime)
{
    m_scramble.Update (deltaTime);

    float postRevealTimer = m_scramble.GetPostRevealTimer();
    int   cellCount       = m_scramble.GetCellCount();



    for (int i = 0; i < cellCount; i++)
    {
        auto       & ch   = m_chars[i];
        const auto & cell = m_scramble.GetCell (i);

        ch.opacity = cell.opacity;

        // Map CellPhase to CharPhase and update display glyph
        switch (cell.phase)
        {
            case CellPhase::Cycling:
            {
                ch.phase = CharPhase::Cycling;

                if (cell.needsCycle)
                {
                    ch.randomGlyphIndex  = (ch.randomGlyphIndex + 7) % m_allGlyphs.size();
                    ch.currentGlyphIndex = ch.randomGlyphIndex;
                }

                ch.glowIntensity = 0.3f;
                break;
            }

            case CellPhase::LockFlash:
            {
                ch.phase             = CharPhase::LockFlash;
                ch.currentGlyphIndex = ch.targetGlyphIndex;
                ch.glowIntensity     = 1.0f;
                break;
            }

            case CellPhase::Settled:
            {
                ch.phase             = CharPhase::Settled;
                ch.currentGlyphIndex = ch.targetGlyphIndex;
                ch.glowIntensity     = 0.0f;
                break;
            }

            case CellPhase::Dismissing:
            {
                ch.phase = CharPhase::Dismissing;

                if (cell.needsCycle)
                {
                    ch.randomGlyphIndex  = (ch.randomGlyphIndex + 7) % m_allGlyphs.size();
                    ch.currentGlyphIndex = ch.randomGlyphIndex;
                }
                else
                {
                    ch.currentGlyphIndex = ch.randomGlyphIndex;
                }

                ch.glowIntensity = 0.0f;
                break;
            }

            case CellPhase::Hidden:
            default:
            {
                ch.phase         = CharPhase::Hidden;
                ch.glowIntensity = 0.0f;
                break;
            }
        }

        // Pre-compute color for rendering
        OverlayColor color = ComputeScrambleColor (cell.phase, cell.flashTimer, cell.flashDuration, postRevealTimer);

        ch.colorR = color.r;
        ch.colorG = color.g;
        ch.colorB = color.b;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpHintOverlay::GetPhase
//
//  Maps ScrambleRevealEffect's ScramblePhase to the public OverlayPhase.
//
////////////////////////////////////////////////////////////////////////////////

OverlayPhase HelpHintOverlay::GetPhase () const
{
    switch (m_scramble.GetPhase())
    {
        case ScramblePhase::Revealing:  return OverlayPhase::Revealing;
        case ScramblePhase::Holding:    return OverlayPhase::Holding;
        case ScramblePhase::Dismissing: return OverlayPhase::Dissolving;

        case ScramblePhase::Idle:
        case ScramblePhase::Done:
        default:                        return OverlayPhase::Hidden;
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

    float charWidth   = GetCharWidth();
    float charHeight  = GetCharHeight();
    float padding     = GetPadding();
    float edgePadding = std::max (0.0f, padding - MARGIN_COLS * charWidth);
    float blockWidth  = m_cols * charWidth  + edgePadding * 2.0f;
    float blockHeight = m_rows * charHeight + padding * 2.0f;

    m_boundingRect.left   = (viewportWidth  - blockWidth)  / 2.0f;
    m_boundingRect.top    = (viewportHeight - blockHeight) / 2.0f;
    m_boundingRect.right  = m_boundingRect.left + blockWidth;
    m_boundingRect.bottom = m_boundingRect.top  + blockHeight;
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpHintOverlay::SetDpiScale
//
//  Updates the DPI scale factor used by the scaled layout accessors.
//
////////////////////////////////////////////////////////////////////////////////

void HelpHintOverlay::SetDpiScale (float dpiScale)
{
    m_dpiScale = dpiScale;
}
