#include "pch.h"

#include "Overlay.h"

#include "CharacterSet.h"
#include "OverlayColor.h"





////////////////////////////////////////////////////////////////////////////////
//
//  Overlay::Overlay
//
//  Builds the character grid from the provided entries.
//  Based on HotkeyOverlay's known-good constructor pattern.
//
////////////////////////////////////////////////////////////////////////////////

Overlay::Overlay (std::vector<OverlayEntry> entries,
                  OverlayTimingConfig       timing,
                  OverlayLayoutConfig       layout) :
    m_scramble (timing.revealDuration, timing.dismissDuration, timing.cycleInterval,
                timing.flashDuration, timing.holdDuration),
    m_entries  (std::move (entries)),
    m_layout   (layout)
{
    m_numRows = static_cast<int> (m_entries.size());

    ComputeColumns();
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
//  Overlay::ComputeColumns
//
//  Determines key column width and total column count from entry content.
//
////////////////////////////////////////////////////////////////////////////////

void Overlay::ComputeColumns ()
{
    m_keyColChars = 0;
    int maxRightChars = 0;



    for (const auto & entry : m_entries)
    {
        // For two-column rows (non-empty right), left is the key column
        if (!entry.right.empty())
        {
            m_keyColChars = std::max (m_keyColChars, static_cast<int> (entry.left.size()));
            maxRightChars = std::max (maxRightChars, static_cast<int> (entry.right.size()));
        }
        else
        {
            // Single-column row — contributes to the right/description column width
            // (but not to the key column width since it has no key)
            maxRightChars = std::max (maxRightChars, static_cast<int> (entry.left.size()));
        }
    }

    // If there are any two-column rows, textCols includes key + gap + desc
    // If all rows are single-column, key=0 and gap=0
    int effectiveGap = (m_keyColChars > 0) ? m_layout.gapChars : 0;

    m_textCols = m_keyColChars + effectiveGap + maxRightChars;
    m_cols     = m_textCols + 2 * m_layout.marginCols;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Overlay::InitializeCharacters
//
//  Creates the flat character array from entries.
//  Directly based on HotkeyOverlay::InitializeCharacters.
//
////////////////////////////////////////////////////////////////////////////////

void Overlay::InitializeCharacters ()
{
    m_chars.clear();
    m_chars.reserve (static_cast<size_t> (m_numRows) * m_cols);

    int effectiveGap   = (m_keyColChars > 0) ? m_layout.gapChars : 0;
    int maxRightChars  = m_textCols - m_keyColChars - effectiveGap;



    for (int row = 0; row < m_numRows; row++)
    {
        const OverlayEntry & entry = m_entries[row];

        // Build the full row string with key + gap + description columns
        std::wstring rowText;

        rowText.reserve (m_textCols);

        if (!entry.right.empty())
        {
            // Two-column row: key (padded) + gap + description (padded)
            std::wstring key = entry.left;
            key.resize (m_keyColChars, L' ');
            rowText += key;

            rowText.append (effectiveGap, L' ');

            std::wstring desc = entry.right;
            desc.resize (maxRightChars, L' ');
            rowText += desc;
        }
        else
        {
            // Single-column row: text starts at key column position, fills full width
            std::wstring text = entry.left;
            text.resize (m_textCols, L' ');
            rowText += text;
        }

        // Left margin columns
        for (int m = 0; m < m_layout.marginCols; m++)
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
            ch.col             = m_layout.marginCols + col;
            ch.isSpace         = (rowText[col] == L' ');
            ch.targetCodepoint = static_cast<uint32_t> (rowText[col]);
            ch.phase           = CharPhase::Hidden;
            ch.opacity         = 0.0f;

            m_chars.push_back (ch);
        }

        // Right margin columns
        for (int m = 0; m < m_layout.marginCols; m++)
        {
            HintCharacter ch;

            ch.row     = row;
            ch.col     = m_layout.marginCols + m_textCols + m;
            ch.isSpace = true;
            ch.phase   = CharPhase::Hidden;
            ch.opacity = 0.0f;

            m_chars.push_back (ch);
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  Overlay::ResolveGlyphIndices
//
//  Maps codepoints to CharacterSet glyph indices.
//  Identical to HotkeyOverlay::ResolveGlyphIndices.
//
////////////////////////////////////////////////////////////////////////////////

void Overlay::ResolveGlyphIndices ()
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
//  Overlay::SetDpiScale
//
////////////////////////////////////////////////////////////////////////////////

void Overlay::SetDpiScale (float dpiScale)
{
    m_dpiScale = dpiScale;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Overlay::GetPhase
//
//  Maps ScramblePhase to OverlayPhase.
//
////////////////////////////////////////////////////////////////////////////////

OverlayPhase Overlay::GetPhase () const
{
    switch (m_scramble.GetPhase())
    {
        case ScramblePhase::Revealing: return OverlayPhase::Revealing;
        case ScramblePhase::Holding:   return OverlayPhase::Holding;
        case ScramblePhase::Dismissing: return OverlayPhase::Dissolving;
        default:                       return OverlayPhase::Hidden;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  Overlay::Show
//
//  Starts or restarts the reveal animation.
//  Identical to HotkeyOverlay::Show.
//
////////////////////////////////////////////////////////////////////////////////

void Overlay::Show ()
{
    ResolveGlyphIndices();

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
//  Overlay::Dismiss
//
////////////////////////////////////////////////////////////////////////////////

void Overlay::Dismiss ()
{
    m_scramble.StartDismiss();
}





////////////////////////////////////////////////////////////////////////////////
//
//  Overlay::Hide
//
//  Immediately hides the overlay with no animation.
//  Identical to HotkeyOverlay::Hide.
//
////////////////////////////////////////////////////////////////////////////////

void Overlay::Hide ()
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
//  Overlay::Update
//
//  Advances the scramble-reveal effect and derives per-character state.
//  Copied directly from HotkeyOverlay::Update (known-good).
//
////////////////////////////////////////////////////////////////////////////////

void Overlay::Update (float deltaTime, float schemeR, float schemeG, float schemeB)
{
    m_scramble.Update (deltaTime);

    CharacterSet & charSet         = CharacterSet::GetInstance();
    float          postRevealTimer = m_scramble.GetPostRevealTimer();
    int            cellCount       = m_scramble.GetCellCount();



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
                    ch.randomGlyphIndex  = charSet.GetRandomGlyphIndex (charSet.GetRainGlyphCount());
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
                    ch.randomGlyphIndex  = charSet.GetRandomGlyphIndex (charSet.GetRainGlyphCount());
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
        OverlayColor color = ComputeScrambleColor (cell.phase, cell.flashTimer, cell.flashDuration, postRevealTimer, schemeR, schemeG, schemeB);

        ch.colorR = color.r;
        ch.colorG = color.g;
        ch.colorB = color.b;
    }
}
