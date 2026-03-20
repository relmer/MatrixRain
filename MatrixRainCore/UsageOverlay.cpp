#include "pch.h"

#include "UsageOverlay.h"

#include "CharacterSet.h"
#include "OverlayColor.h"





////////////////////////////////////////////////////////////////////////////////
//
//  UsageOverlay::UsageOverlay
//
////////////////////////////////////////////////////////////////////////////////

UsageOverlay::UsageOverlay (wchar_t switchPrefix)
{
    BuildLines (switchPrefix);
    InitializeCharacters();

    int cellCount = static_cast<int> (m_chars.size());

    m_scramble.SetCellCount (cellCount);

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
//  UsageOverlay::BuildLines
//
//  Builds the formatted text lines from UsageText.
//
////////////////////////////////////////////////////////////////////////////////

void UsageOverlay::BuildLines (wchar_t switchPrefix)
{
    UsageText usage (switchPrefix);

    m_lines   = usage.GetFormattedLines();
    m_numRows = static_cast<int> (m_lines.size());

    // Find the longest line to determine column count
    m_textCols = 0;

    for (const auto & line : m_lines)
    {
        m_textCols = std::max (m_textCols, static_cast<int> (line.size()));
    }

    m_cols = m_textCols + 2 * MARGIN_COLS;
}





////////////////////////////////////////////////////////////////////////////////
//
//  UsageOverlay::InitializeCharacters
//
//  Builds the flat HintCharacter array from formatted text lines.
//  Each line is padded to m_textCols with spaces for uniform grid.
//
////////////////////////////////////////////////////////////////////////////////

void UsageOverlay::InitializeCharacters ()
{
    m_chars.clear();
    m_chars.reserve (static_cast<size_t> (m_numRows) * m_cols);

    for (int row = 0; row < m_numRows; row++)
    {
        // Pad line to m_textCols
        std::wstring line = m_lines[row];
        line.resize (m_textCols, L' ');

        // Left margin columns
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

            ch.row             = row;
            ch.col             = MARGIN_COLS + col;
            ch.isSpace         = (line[col] == L' ');
            ch.targetCodepoint = static_cast<uint32_t> (line[col]);
            ch.phase           = CharPhase::Hidden;
            ch.opacity         = 0.0f;

            m_chars.push_back (ch);
        }

        // Right margin columns
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
//  UsageOverlay::ResolveGlyphIndices
//
//  Maps codepoints to glyph indices in the CharacterSet atlas.
//
////////////////////////////////////////////////////////////////////////////////

void UsageOverlay::ResolveGlyphIndices ()
{
    CharacterSet & charSet = CharacterSet::GetInstance();



    for (auto & ch : m_chars)
    {
        size_t idx = charSet.FindGlyphByCodepoint (ch.targetCodepoint);

        ch.targetGlyphIndex  = (idx != SIZE_MAX) ? idx : 0;
        ch.randomGlyphIndex  = charSet.GetRandomGlyphIndex (charSet.GetRainGlyphCount());
        ch.currentGlyphIndex = ch.targetGlyphIndex;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  UsageOverlay::SetDpiScale
//
////////////////////////////////////////////////////////////////////////////////

void UsageOverlay::SetDpiScale (float dpiScale)
{
    m_dpiScale = dpiScale;
}





////////////////////////////////////////////////////////////////////////////////
//
//  UsageOverlay::Show
//
////////////////////////////////////////////////////////////////////////////////

void UsageOverlay::Show ()
{
    ResolveGlyphIndices();

    for (auto & ch : m_chars)
    {
        ch.phase            = CharPhase::Hidden;
        ch.opacity          = 0.0f;
        ch.glowIntensity    = 0.0f;
        ch.currentGlyphIndex = ch.isSpace ? SIZE_MAX : ch.targetGlyphIndex;
    }

    m_scramble.StartReveal();
}





////////////////////////////////////////////////////////////////////////////////
//
//  UsageOverlay::Update
//
////////////////////////////////////////////////////////////////////////////////

void UsageOverlay::Update (float deltaTime, float schemeR, float schemeG, float schemeB)
{
    if (!m_scramble.IsActive())
    {
        return;
    }

    m_scramble.Update (deltaTime);

    CharacterSet & charSet        = CharacterSet::GetInstance();
    float          postRevealTime = m_scramble.GetPostRevealTimer();
    int            cellCount      = m_scramble.GetCellCount();



    for (int i = 0; i < cellCount; i++)
    {
        HintCharacter  & ch   = m_chars[i];
        const CellState & cell = m_scramble.GetCell (i);



        ch.opacity = cell.opacity;

        if (ch.isSpace)
        {
            ch.phase = CharPhase::Hidden;
            continue;
        }

        // Map CellPhase → CharPhase and pick display glyph
        switch (cell.phase)
        {
            case CellPhase::Hidden:
            default:
                ch.phase = CharPhase::Hidden;
                break;

            case CellPhase::Cycling:
                ch.phase = CharPhase::Cycling;

                if (cell.needsCycle)
                {
                    ch.randomGlyphIndex  = charSet.GetRandomGlyphIndex (charSet.GetRainGlyphCount());
                }

                ch.currentGlyphIndex = ch.randomGlyphIndex;
                break;

            case CellPhase::LockFlash:
                ch.phase             = CharPhase::LockFlash;
                ch.currentGlyphIndex = ch.targetGlyphIndex;
                break;

            case CellPhase::Settled:
                ch.phase             = CharPhase::Settled;
                ch.currentGlyphIndex = ch.targetGlyphIndex;
                break;

            case CellPhase::Dismissing:
                ch.phase = CharPhase::Dismissing;

                if (cell.needsCycle)
                {
                    ch.randomGlyphIndex  = charSet.GetRandomGlyphIndex (charSet.GetRainGlyphCount());
                }

                ch.currentGlyphIndex = ch.randomGlyphIndex;
                break;
        }

        // Compute color from scramble phase
        OverlayColor color = ComputeScrambleColor (cell.phase, cell.flashTimer, cell.flashDuration,
                                                    postRevealTime, schemeR, schemeG, schemeB);

        ch.colorR = color.r;
        ch.colorG = color.g;
        ch.colorB = color.b;
    }
}
