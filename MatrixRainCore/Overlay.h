#pragma once

#include "ScrambleRevealEffect.h"

#include <span>
#include <string>
#include <vector>





////////////////////////////////////////////////////////////////////////////////
//
//  OverlayPhase — Overlay-level animation phase
//
////////////////////////////////////////////////////////////////////////////////

enum class OverlayPhase
{
    Hidden,
    Revealing,
    Holding,
    Dissolving
};





////////////////////////////////////////////////////////////////////////////////
//
//  CharPhase — Per-character animation phase
//
////////////////////////////////////////////////////////////////////////////////

enum class CharPhase
{
    Hidden,
    Cycling,
    LockFlash,
    Settled,
    Dismissing
};





////////////////////////////////////////////////////////////////////////////////
//
//  HintCharacter — Per-character animation state
//
////////////////////////////////////////////////////////////////////////////////

struct HintCharacter
{
    size_t    targetGlyphIndex  = 0;
    size_t    currentGlyphIndex = 0;
    size_t    randomGlyphIndex  = 0;
    uint32_t  targetCodepoint   = 0x0020;
    CharPhase phase             = CharPhase::Hidden;
    float     opacity           = 0.0f;
    float     glowIntensity     = 0.0f;
    float     colorR            = 0.0f;
    float     colorG            = 0.0f;
    float     colorB            = 0.0f;
    float     xOffset           = 0.0f;
    float     charWidth         = 0.0f;
    int       row               = 0;
    int       col               = 0;
    bool      isSpace           = false;
    bool      isSingleColumnRow = false;
};





////////////////////////////////////////////////////////////////////////////////
//
//  OverlayEntry — One row of overlay text (left column + right column)
//
//  For two-column rows: left = key name, right = description
//  For single-column rows: left = full text, right = empty
//
////////////////////////////////////////////////////////////////////////////////

struct OverlayEntry
{
    std::wstring left;
    std::wstring right;
};





////////////////////////////////////////////////////////////////////////////////
//
//  OverlayTimingConfig — Timing parameters for ScrambleRevealEffect
//
////////////////////////////////////////////////////////////////////////////////

struct OverlayTimingConfig
{
    float revealDuration  = 2.5f;
    float dismissDuration = 1.0f;
    float cycleInterval   = 0.25f;
    float flashDuration   = 1.0f;
    float holdDuration    = -1.0f;  // -1 = hold indefinitely
};





////////////////////////////////////////////////////////////////////////////////
//
//  OverlayLayoutConfig — Layout parameters for overlay rendering
//
////////////////////////////////////////////////////////////////////////////////

struct OverlayLayoutConfig
{
    int   marginCols    = 2;
    int   gapChars      = 6;
    float baseCharWidth = 16.0f;
    float baseRowHeight = 28.0f;
    float basePadding   = 30.0f;
};





////////////////////////////////////////////////////////////////////////////////
//
//  Overlay — GPU-rendered text overlay with scramble-reveal animation
//
//  Unified overlay class that replaces HelpHintOverlay, HotkeyOverlay,
//  and UsageOverlay.  Takes a list of OverlayEntry rows and renders them
//  via the shared GPU instancing + SDF halo + bloom pipeline.
//
//  Two-column layout: entries with non-empty left AND right strings
//  render as right-aligned left column + gap + left-aligned right column.
//  Entries with empty right column render as left-aligned single-column.
//
////////////////////////////////////////////////////////////////////////////////

class Overlay
{
public:
    Overlay (OverlayTimingConfig timing,
             OverlayLayoutConfig layout);

    HRESULT Initialize (std::vector<OverlayEntry> entries);


    // DPI scaling
    void  SetDpiScale    (float dpiScale);
    float GetCharWidth() const { return m_layout.baseCharWidth * m_dpiScale; }
    float GetRowHeight() const { return m_layout.baseRowHeight * m_dpiScale; }
    float GetPadding()   const { return m_layout.basePadding   * m_dpiScale; }


    // State control
    void Show();
    void Dismiss();
    void Hide();


    // Update (called once per render frame)
    void Update (float deltaTime, float schemeR, float schemeG, float schemeB);


    // Queries
    bool                           IsActive()        const { return m_scramble.IsActive();                    }
    bool                           IsRevealComplete() const { return m_scramble.IsRevealComplete();            }
    OverlayPhase                   GetPhase()         const;
    std::span<const HintCharacter> GetCharacters()    const { return std::span<const HintCharacter> (m_chars); }
    int                            GetCharRows()      const { return m_numRows;                                }
    int                            GetCols()          const { return m_cols;                                   }
    int                            GetKeyColChars()   const { return m_keyColChars;                            }
    int                            GetGapChars()      const { return m_layout.gapChars;                        }
    int                            GetMarginCols()    const { return m_layout.marginCols;                      }
    float                          GetCharHeight()    const { return GetRowHeight();                            }


private:
    void ComputeColumns();
    void InitializeCharacters();
    void ResolveGlyphIndices();


    ScrambleRevealEffect            m_scramble;

    // Content
    std::vector<OverlayEntry>       m_entries;

    // Layout config
    OverlayLayoutConfig             m_layout;

    // Character grid
    std::vector<HintCharacter>      m_chars;
    int                             m_numRows     = 0;
    int                             m_cols        = 0;
    int                             m_textCols    = 0;
    int                             m_keyColChars = 0;

    // DPI scale factor (1.0 at 96 DPI / 100%)
    float                           m_dpiScale    = 1.0f;
};
