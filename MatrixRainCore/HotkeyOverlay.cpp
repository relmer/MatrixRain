#include "pch.h"

#include "HotkeyOverlay.h"

#include "CharacterSet.h"
#include "OverlayColor.h"





////////////////////////////////////////////////////////////////////////////////
//
//  HotkeyOverlay::HotkeyOverlay
//
//  Builds the hotkey reference list and character grid.
//
////////////////////////////////////////////////////////////////////////////////

HotkeyOverlay::HotkeyOverlay()
{
    BuildHotkeyList();

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

    // Initialize the scramble-reveal effect (holdDuration = -1 → hold indefinitely)
    int cellCount = static_cast<int>(m_chars.size());

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

            ch.row             = row;
            ch.col             = MARGIN_COLS + col;
            ch.isSpace         = (rowText[col] == L' ');
            ch.targetCodepoint = static_cast<uint32_t> (rowText[col]);
            ch.phase           = CharPhase::Hidden;
            ch.opacity         = 0.0f;

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
//  HotkeyOverlay::ResolveGlyphIndices
//
//  Resolves targetCodepoint to CharacterSet glyph indices.
//  Called from Show() when CharacterSet is guaranteed to be initialized.
//
////////////////////////////////////////////////////////////////////////////////

void HotkeyOverlay::ResolveGlyphIndices ()
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
//  HotkeyOverlay::Show
//
//  Starts or restarts the reveal animation.
//
////////////////////////////////////////////////////////////////////////////////

void HotkeyOverlay::Show ()
{
    ResolveGlyphIndices();

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
//  HotkeyOverlay::Dismiss
//
//  Triggers dismiss from Revealing or Holding.
//  No-op from Dismissing/Idle/Done.
//
////////////////////////////////////////////////////////////////////////////////

void HotkeyOverlay::Dismiss ()
{
    m_scramble.StartDismiss();
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
//  HotkeyOverlay::Update
//
//  Advances the scramble-reveal effect and derives per-character state from it.
//
////////////////////////////////////////////////////////////////////////////////

void HotkeyOverlay::Update (float deltaTime)
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
        OverlayColor color = ComputeScrambleColor (cell.phase, cell.flashTimer, cell.flashDuration, postRevealTimer);

        ch.colorR = color.r;
        ch.colorG = color.g;
        ch.colorB = color.b;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  HotkeyOverlay::GetPhase
//
//  Maps ScrambleRevealEffect's ScramblePhase to the public OverlayPhase.
//
////////////////////////////////////////////////////////////////////////////////

OverlayPhase HotkeyOverlay::GetPhase () const
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





////////////////////////////////////////////////////////////////////////////////
//
//  HotkeyOverlay::SetDpiScale
//
//  Updates the DPI scale factor used by the scaled layout accessors.
//
////////////////////////////////////////////////////////////////////////////////

void HotkeyOverlay::SetDpiScale (float dpiScale)
{
    m_dpiScale = dpiScale;
}
