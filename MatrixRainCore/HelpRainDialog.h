#pragma once

#include "CharacterConstants.h"
#include "UsageText.h"

using Microsoft::WRL::ComPtr;





////////////////////////////////////////////////////////////////////////////////
//  AnimationPhase — Which rain phase is active
////////////////////////////////////////////////////////////////////////////////

enum class AnimationPhase
{
    Revealing,
    Background
};





////////////////////////////////////////////////////////////////////////////////
//  CharPosition — Pre-computed pixel position for a non-space character
////////////////////////////////////////////////////////////////////////////////

struct CharPosition
{
    size_t  charIndex = 0;
    float   x         = 0.0f;
    float   y         = 0.0f;
    wchar_t character = L'\0';
};





////////////////////////////////////////////////////////////////////////////////
//  RevealStreak — A short streak spawned to reveal a specific character
////////////////////////////////////////////////////////////////////////////////

struct RevealStreak
{
    size_t               targetCharIndex = 0;
    float                pixelX          = 0.0f;
    float                headPixelY      = 0.0f;
    float                targetPixelY    = 0.0f;
    float                speed           = 0.0f;
    int                  leadCells       = 4;
    int                  trailCells      = 3;
    std::vector<size_t>  glyphIndices;
    bool                 revealed        = false;
};





////////////////////////////////////////////////////////////////////////////////
//  DecorativeStreak — An ambient rain streak for visual atmosphere
////////////////////////////////////////////////////////////////////////////////

struct DecorativeStreak
{
    float                pixelX      = 0.0f;
    float                headPixelY  = 0.0f;
    int                  trailLength = 7;
    float                speed       = 0.0f;
    std::vector<size_t>  glyphIndices;
};





////////////////////////////////////////////////////////////////////////////////
//  HelpRainDialog — Custom graphical window with matrix rain reveal animation
//
//  Displays command-line switches (Options + Screensaver Options) via a
//  two-phase GPU-rendered matrix rain animation. Phase 1 reveals text through
//  per-character rain streaks. Phase 2 continues ambient decorative rain.
//
//  Used by /? and -? invocation only — the ? key uses an in-app overlay.
////////////////////////////////////////////////////////////////////////////////

class HelpRainDialog
{
public:
    explicit HelpRainDialog (const UsageText & usageText);
    ~HelpRainDialog();


    // Display — blocking call, returns when dismissed
    HRESULT Show();


    // Queries
    bool                                       IsRevealComplete ()       const;
    AnimationPhase                             GetAnimationPhase ()      const { return m_animationPhase;      }
    const std::vector<CharPosition>          & GetCharacterPositions ()  const { return m_characterPositions;  }
    const std::vector<bool>                  & GetRevealedFlags ()       const { return m_revealedFlags;       }
    const std::vector<size_t>                & GetRevealQueue ()         const { return m_revealQueue;         }
    size_t                                     GetRevealQueueIndex ()    const { return m_revealQueueIndex;    }
    const std::vector<RevealStreak>          & GetActiveRevealStreaks () const { return m_activeRevealStreaks;  }
    const std::vector<DecorativeStreak>      & GetDecorativeStreaks ()   const { return m_decorativeStreaks;    }
    float                                      GetRevealSpawnRate ()     const { return m_revealSpawnRate;     }


    // Internal sizing helper (public for testability)
    static SIZE ComputeWindowSize (const UsageText & usageText, float dpi);


    // Animation update (public for testability)
    void Update (float deltaTime);


    // Tunable constants
    static constexpr float kRevealDurationSeconds   = 3.0f;
    static constexpr int   kStreakLeadCells          = 4;
    static constexpr int   kStreakTrailCells         = 3;
    static constexpr float kPhase2DensityMultiplier  = 0.15f;
    static constexpr float kPhase2SpeedMultiplier    = 1.5f;
    static constexpr float kCellHeight               = 24.0f;
    static constexpr float kDefaultStreakSpeed        = 300.0f;


private:
    // Initialization
    HRESULT CreateDialogWindow();
    HRESULT CreateDeviceResources();
    HRESULT InitializeTextLayout();
    void    ComputeCharacterPositions();
    void    BuildRevealQueue();

    // Render loop
    void    RenderFrame();
    void    RenderDecorativeStreaks();
    void    RenderRevealStreaks();
    void    RenderResolvedText();
    void    DrawCharacterGlow (wchar_t ch, float x, float y);
    void    DrawRainGlyph (size_t glyphIndex, float x, float y, float opacity, ID2D1SolidColorBrush * pBrush);

    // Animation update helpers
    void    UpdateRevealPhase (float deltaTime);
    void    UpdateBackgroundPhase (float deltaTime);
    void    SpawnDecorativeStreak();
    void    AdvanceStreaks (float deltaTime);

    // Window procedure
    static LRESULT CALLBACK WndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


    // Text content
    const UsageText              & m_usageText;


    // Animation state
    AnimationPhase                 m_animationPhase    = AnimationPhase::Revealing;
    float                          m_phaseTimer        = 0.0f;
    float                          m_revealSpawnRate   = 0.0f;
    float                          m_spawnAccumulator  = 0.0f;

    std::vector<CharPosition>      m_characterPositions;
    std::vector<size_t>            m_revealQueue;
    size_t                         m_revealQueueIndex  = 0;
    std::vector<bool>              m_revealedFlags;
    std::vector<RevealStreak>      m_activeRevealStreaks;
    std::vector<DecorativeStreak>  m_decorativeStreaks;

    float                          m_decorativeSpawnAccumulator = 0.0f;
    float                          m_decorativeSpawnRate        = 0.0f;


    // Text layout metrics
    D2D1_RECT_F                    m_textBoundingBox  = {};
    float                          m_textOffsetX      = 0.0f;
    float                          m_textOffsetY      = 0.0f;


    // Window
    HWND                           m_hWnd             = nullptr;
    int                            m_windowWidth      = 0;
    int                            m_windowHeight     = 0;
    bool                           m_dismissed        = false;

    static constexpr LPCWSTR       kWindowClassName   = L"MatrixRainHelpDialog";


    // D3D11 / D2D resources
    ComPtr<ID3D11Device>           m_device;
    ComPtr<ID3D11DeviceContext>    m_deviceContext;
    ComPtr<IDXGISwapChain>         m_swapChain;
    ComPtr<ID2D1Factory1>          m_d2dFactory;
    ComPtr<ID2D1RenderTarget>      m_d2dRenderTarget;
    ComPtr<IDWriteFactory>         m_dwriteFactory;
    ComPtr<IDWriteTextFormat>      m_textFormat;
    ComPtr<IDWriteTextLayout>      m_textLayout;

    // Brushes
    ComPtr<ID2D1SolidColorBrush>   m_textBrush;
    ComPtr<ID2D1SolidColorBrush>   m_glowBrush;
    ComPtr<ID2D1SolidColorBrush>   m_headBrush;
    ComPtr<ID2D1SolidColorBrush>   m_trailBrush;


    // Character set for random glyphs
    std::vector<uint32_t>          m_allGlyphs;


    // RNG
    std::mt19937                   m_rng { std::random_device{}() };
};
