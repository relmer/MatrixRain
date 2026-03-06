#include "pch.h"

#include "HelpHintOverlay.h"





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

    m_rows = static_cast<int>(m_textLines.size());
    m_cols = static_cast<int>(maxLen);

    InitializeCharacters();
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
        for (int col = 0; col < m_cols; col++)
        {
            HintCharacter ch;

            ch.row     = row;
            ch.col     = col;
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
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpHintOverlay::InitializeCharactersForReveal
//
//  Resets all non-space characters to Scrambling with staggered timers.
//
////////////////////////////////////////////////////////////////////////////////

void HelpHintOverlay::InitializeCharactersForReveal ()
{
    std::uniform_real_distribution<float> staggerDist (0.0f, REVEAL_STAGGER_RANGE);
    std::uniform_real_distribution<float> intervalDist (SCRAMBLE_MIN_INTERVAL, SCRAMBLE_MAX_INTERVAL);
    std::uniform_int_distribution<size_t> glyphDist (0, m_allGlyphs.size() - 1);



    for (auto & ch : m_chars)
    {
        if (ch.isSpace)
        {
            ch.phase   = CharPhase::Hidden;
            ch.opacity = 0.0f;
            continue;
        }

        ch.phase            = CharPhase::Scrambling;
        ch.opacity          = 1.0f;
        ch.scrambleTimer    = staggerDist (m_rng);
        ch.scrambleInterval = intervalDist (m_rng);
        ch.currentGlyphIndex = glyphDist (m_rng);
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
    m_phase      = OverlayPhase::Revealing;
    m_phaseTimer = 0.0f;

    InitializeCharactersForReveal();
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpHintOverlay::Dismiss
//
//  Triggers dissolve from Revealing or Holding. No-op from Dissolving/Hidden.
//
////////////////////////////////////////////////////////////////////////////////

void HelpHintOverlay::Dismiss ()
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
                // Characters will transition in UpdateDissolving when their offset expires
                if (ch.phase == CharPhase::Scrambling)
                {
                    // Scrambling characters go straight to dissolve cycling
                    ch.phase = CharPhase::DissolveCycling;
                    ch.scrambleTimer = 0.0f;
                }
            }
        }
    }
    // No-op if already Dissolving or Hidden
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
    m_phase = OverlayPhase::Hidden;

    for (auto & ch : m_chars)
    {
        ch.phase   = CharPhase::Hidden;
        ch.opacity = 0.0f;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpHintOverlay::Update
//
//  Advances animation state based on delta time.
//
////////////////////////////////////////////////////////////////////////////////

void HelpHintOverlay::Update (float deltaTime)
{
    switch (m_phase)
    {
        case OverlayPhase::Revealing:
            UpdateRevealing (deltaTime);
            break;

        case OverlayPhase::Holding:
            UpdateHolding (deltaTime);
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
//  HelpHintOverlay::UpdateRevealing
//
//  Cycles scrambling characters and resolves them when timer expires.
//
////////////////////////////////////////////////////////////////////////////////

void HelpHintOverlay::UpdateRevealing (float deltaTime)
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
                // Resolve this character
                ch.phase             = CharPhase::Resolved;
                ch.currentGlyphIndex = ch.targetGlyphIndex;
                ch.opacity           = 1.0f;
            }
            else
            {
                // Cycle to a random glyph
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
//  HelpHintOverlay::UpdateHolding
//
//  Counts down the hold timer, then transitions to Dissolving.
//
////////////////////////////////////////////////////////////////////////////////

void HelpHintOverlay::UpdateHolding (float deltaTime)
{
    m_phaseTimer += deltaTime;

    if (m_phaseTimer >= m_holdDuration)
    {
        m_phase      = OverlayPhase::Dissolving;
        m_phaseTimer = 0.0f;

        // Assign staggered dissolve offsets
        std::uniform_real_distribution<float> staggerDist (0.0f, DISSOLVE_STAGGER_RANGE);

        for (auto & ch : m_chars)
        {
            if (!ch.isSpace && ch.phase == CharPhase::Resolved)
            {
                ch.dissolveStartOffset = staggerDist (m_rng);
            }
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpHintOverlay::UpdateDissolving
//
//  Manages per-character dissolve: Resolved → DissolveCycling → DissolveFading → Hidden.
//
////////////////////////////////////////////////////////////////////////////////

void HelpHintOverlay::UpdateDissolving (float deltaTime)
{
    std::uniform_int_distribution<size_t> glyphDist (0, m_allGlyphs.size() - 1);
    bool                                  allHidden = true;



    m_phaseTimer += deltaTime;

    for (auto & ch : m_chars)
    {
        if (ch.isSpace || ch.phase == CharPhase::Hidden)
        {
            continue;
        }

        // Allow cascading through multiple phases in a single frame
        float remaining = deltaTime;

        if (ch.phase == CharPhase::Resolved)
        {
            // Wait for staggered offset
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
    allHidden = true;

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
//  HelpHintOverlay::UpdateLayout
//
//  Computes centered bounding rect for given viewport dimensions.
//
////////////////////////////////////////////////////////////////////////////////

void HelpHintOverlay::UpdateLayout (float viewportWidth, float viewportHeight)
{
    //
    // Estimate text block size based on character dimensions.
    // Use approximate character cell size (will be refined by render system).
    //

    static constexpr float CHAR_WIDTH  = 16.0f;
    static constexpr float CHAR_HEIGHT = 28.0f;
    static constexpr float PADDING     = 20.0f;

    float blockWidth  = m_cols * CHAR_WIDTH  + PADDING * 2.0f;
    float blockHeight = m_rows * CHAR_HEIGHT + PADDING * 2.0f;

    m_boundingRect.left   = (viewportWidth  - blockWidth)  / 2.0f;
    m_boundingRect.top    = (viewportHeight - blockHeight) / 2.0f;
    m_boundingRect.right  = m_boundingRect.left + blockWidth;
    m_boundingRect.bottom = m_boundingRect.top  + blockHeight;
}
