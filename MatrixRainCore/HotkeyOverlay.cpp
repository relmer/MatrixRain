#include "pch.h"

#include "HotkeyOverlay.h"





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

    m_cols = m_keyColChars + m_gapChars + maxDescChars;

    InitializeCharacters();
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

    int maxDescChars = m_cols - m_keyColChars - m_gapChars;



    for (int row = 0; row < GetRows(); row++)
    {
        // Build the full row string: key (right-padded) + gap + description (right-padded)
        std::wstring rowText;

        rowText.reserve (m_cols);

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

        for (int col = 0; col < m_cols; col++)
        {
            HintCharacter ch;

            ch.row     = row;
            ch.col     = col;
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
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  HotkeyOverlay::InitializeCharactersForReveal
//
//  Resets all non-space characters to Scrambling with staggered timers.
//
////////////////////////////////////////////////////////////////////////////////

void HotkeyOverlay::InitializeCharactersForReveal ()
{
    std::uniform_real_distribution<float> staggerDist  (0.0f, REVEAL_STAGGER_RANGE);
    std::uniform_real_distribution<float> intervalDist (SCRAMBLE_MIN_INTERVAL, SCRAMBLE_MAX_INTERVAL);
    std::uniform_int_distribution<size_t> glyphDist    (0, m_allGlyphs.size() - 1);



    for (auto & ch : m_chars)
    {
        if (ch.isSpace)
        {
            ch.phase   = CharPhase::Hidden;
            ch.opacity = 0.0f;
            continue;
        }

        ch.phase             = CharPhase::Scrambling;
        ch.opacity           = 1.0f;
        ch.scrambleTimer     = staggerDist (m_rng);
        ch.scrambleInterval  = intervalDist (m_rng);
        ch.currentGlyphIndex = glyphDist (m_rng);
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
    m_phase      = OverlayPhase::Revealing;
    m_phaseTimer = 0.0f;

    InitializeCharactersForReveal();
}





////////////////////////////////////////////////////////////////////////////////
//
//  HotkeyOverlay::Dismiss
//
//  Triggers dissolve from Revealing or Holding. No-op from Dissolving/Hidden.
//
////////////////////////////////////////////////////////////////////////////////

void HotkeyOverlay::Dismiss ()
{
    if (m_phase == OverlayPhase::Revealing || m_phase == OverlayPhase::Holding)
    {
        m_phase      = OverlayPhase::Dissolving;
        m_phaseTimer = 0.0f;

        // Start all resolved characters dissolving with staggered offsets
        std::uniform_real_distribution<float> staggerDist (0.0f, DISSOLVE_STAGGER_RANGE);

        for (auto & ch : m_chars)
        {
            if (ch.isSpace)
            {
                continue;
            }

            if (ch.phase == CharPhase::Resolved || ch.phase == CharPhase::Scrambling)
            {
                ch.dissolveStartOffset = staggerDist (m_rng);

                if (ch.phase == CharPhase::Scrambling)
                {
                    ch.phase         = CharPhase::DissolveCycling;
                    ch.scrambleTimer = 0.0f;
                }
            }
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  HotkeyOverlay::Hide
//
//  Immediately hides the overlay with no fade animation.
//
////////////////////////////////////////////////////////////////////////////////

void HotkeyOverlay::Hide ()
{
    m_phase = OverlayPhase::Hidden;

    for (auto & ch : m_chars)
    {
        ch.phase   = CharPhase::Hidden;
        ch.opacity = 0.0f;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  HotkeyOverlay::Update
//
//  Advances animation state based on delta time.
//
////////////////////////////////////////////////////////////////////////////////

void HotkeyOverlay::Update (float deltaTime)
{
    switch (m_phase)
    {
        case OverlayPhase::Revealing:
            UpdateRevealing (deltaTime);
            break;

        case OverlayPhase::Holding:
            // Hotkey overlay holds indefinitely (dismissed by user)
            break;

        case OverlayPhase::Dissolving:
            UpdateDissolving (deltaTime);
            break;

        case OverlayPhase::Hidden:
        default:
            break;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  HotkeyOverlay::UpdateRevealing
//
//  Cycles scrambling characters and resolves them when timer expires.
//
////////////////////////////////////////////////////////////////////////////////

void HotkeyOverlay::UpdateRevealing (float deltaTime)
{
    std::uniform_int_distribution<size_t> glyphDist (0, m_allGlyphs.size() - 1);
    bool                                  allResolved = true;



    for (auto & ch : m_chars)
    {
        if (ch.isSpace || ch.phase == CharPhase::Resolved)
        {
            continue;
        }

        if (ch.phase == CharPhase::Scrambling)
        {
            ch.scrambleTimer -= deltaTime;

            if (ch.scrambleTimer <= 0.0f)
            {
                ch.phase             = CharPhase::Resolved;
                ch.currentGlyphIndex = ch.targetGlyphIndex;
                ch.opacity           = 1.0f;
            }
            else
            {
                ch.currentGlyphIndex = glyphDist (m_rng);
                allResolved          = false;
            }
        }
        else
        {
            allResolved = false;
        }
    }

    if (allResolved)
    {
        m_phase      = OverlayPhase::Holding;
        m_phaseTimer = 0.0f;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  HotkeyOverlay::UpdateDissolving
//
//  Per-character dissolve: Resolved → DissolveCycling → DissolveFading → Hidden.
//
////////////////////////////////////////////////////////////////////////////////

void HotkeyOverlay::UpdateDissolving (float deltaTime)
{
    std::uniform_int_distribution<size_t> glyphDist (0, m_allGlyphs.size() - 1);



    m_phaseTimer += deltaTime;

    for (auto & ch : m_chars)
    {
        if (ch.isSpace || ch.phase == CharPhase::Hidden)
        {
            continue;
        }

        float remaining = deltaTime;

        if (ch.phase == CharPhase::Resolved)
        {
            if (remaining >= ch.dissolveStartOffset)
            {
                remaining -= ch.dissolveStartOffset;
                ch.dissolveStartOffset = 0.0f;
                ch.phase         = CharPhase::DissolveCycling;
                ch.scrambleTimer = DISSOLVE_CYCLE_DURATION;
            }
            else
            {
                ch.dissolveStartOffset -= remaining;
                remaining = 0.0f;
            }
        }

        if (ch.phase == CharPhase::DissolveCycling)
        {
            ch.currentGlyphIndex = glyphDist (m_rng);

            if (remaining >= ch.scrambleTimer)
            {
                remaining -= ch.scrambleTimer;
                ch.scrambleTimer = 0.0f;
                ch.phase         = CharPhase::DissolveFading;
                ch.scrambleTimer = DISSOLVE_FADE_DURATION;
                ch.opacity       = 1.0f;
            }
            else
            {
                ch.scrambleTimer -= remaining;
                remaining = 0.0f;
            }
        }

        if (ch.phase == CharPhase::DissolveFading)
        {
            ch.currentGlyphIndex = glyphDist (m_rng);

            if (remaining >= ch.scrambleTimer)
            {
                ch.phase   = CharPhase::Hidden;
                ch.opacity = 0.0f;
            }
            else
            {
                ch.scrambleTimer -= remaining;
                ch.opacity = std::max (0.0f, ch.scrambleTimer / DISSOLVE_FADE_DURATION);
            }
        }
    }

    // Check if all non-space characters are now hidden
    bool allHidden = true;

    for (const auto & ch : m_chars)
    {
        if (!ch.isSpace && ch.phase != CharPhase::Hidden)
        {
            allHidden = false;
            break;
        }
    }

    if (allHidden)
    {
        m_phase = OverlayPhase::Hidden;
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
