#pragma once

#include "AnimationSystem.h"
#include "CharacterConstants.h"
#include "ColorScheme.h"
#include "DensityController.h"
#include "RenderSystem.h"
#include "ScreenSaverSettings.h"
#include "UsageText.h"
#include "Viewport.h"

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
//  HelpRainDialog — Custom graphical window with matrix rain reveal animation
//
//  Displays command-line switches via a two-phase GPU-rendered matrix rain
//  animation.  Phase 1 reveals text through dense rain that sweeps downward,
//  revealing characters as the front passes.  Phase 2 continues ambient rain
//  at the user's configured density/speed.
//
//  Owns a full RenderSystem + AnimationSystem for GPU-rendered rain with bloom.
//  D2D text overlay renders the revealed usage text on top of the rain.
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
    bool                                  IsRevealComplete ()       const;
    AnimationPhase                        GetAnimationPhase ()      const { return m_animationPhase;      }
    const std::vector<CharPosition>     & GetCharacterPositions ()  const { return m_characterPositions;  }
    const std::vector<float>            & GetRevealedFlags ()       const { return m_revealedFlags;       }
    float                                 GetRevealFrontY ()        const { return m_revealFrontY;        }
    const AnimationSystem               * GetAnimationSystem ()     const { return m_animationSystem.get(); }


    // Internal sizing helper (public for testability)
    static SIZE ComputeWindowSize (const UsageText & usageText, float dpi);


    // Animation update (public for testability)
    void Update (float deltaTime);


    // Tunable constants — reveal phase
    static constexpr float kRevealSpeed              = 150.0f;
    static constexpr float kRevealFadeInSpeed        = 2.5f;
    static constexpr float kRevealProximityThresholdX = 16.0f;
    static constexpr int   kRevealDensityPercent     = 35;
    static constexpr int   kRevealSpeedPercent       = 100;

    // Tunable constants — background phase
    static constexpr float kPhase2DensityMultiplier  = 0.15f;
    static constexpr float kPhase2SpeedMultiplier    = 1.5f;


private:
    // Initialization
    HRESULT CreateDialogWindow();
    HRESULT CreateRenderPipeline();
    HRESULT InitializeTextLayout();
    void    ComputeCharacterPositions();
    HRESULT LoadUserSettings();

    // Render loop
    void    RenderFrame();
    void    RenderTextOverlay();
    void    DrawCharacterGlow (ID2D1DeviceContext * pContext, wchar_t ch, float x, float y, float opacity);

    // Animation update helpers
    void    UpdateRevealPhase (float deltaTime);
    void    UpdateBackgroundPhase (float deltaTime);

    // Window procedure
    static LRESULT CALLBACK WndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


    // Text content
    const UsageText              & m_usageText;


    // GPU render pipeline (rain + bloom)
    std::unique_ptr<RenderSystem>      m_renderSystem;
    std::unique_ptr<AnimationSystem>   m_animationSystem;
    std::unique_ptr<Viewport>          m_viewport;
    std::unique_ptr<DensityController> m_densityController;


    // User settings
    ScreenSaverSettings            m_settings;
    ColorScheme                    m_colorScheme     = ColorScheme::Green;


    // Animation state
    AnimationPhase                 m_animationPhase  = AnimationPhase::Revealing;
    float                          m_phaseTimer      = 0.0f;
    float                          m_revealFrontY    = -50.0f;
    float                          m_elapsedTime     = 0.0f;
    size_t                         m_spawnCallCounter = 0;

    std::vector<CharPosition>      m_characterPositions;
    std::vector<float>             m_revealedFlags;


    // Text layout (DWrite — independent of RenderSystem D2D)
    ComPtr<IDWriteFactory>         m_dwriteFactory;
    ComPtr<IDWriteTextFormat>      m_textFormat;
    ComPtr<IDWriteTextLayout>      m_textLayout;

    // D2D text overlay brushes (created on RenderSystem's D2D context)
    ComPtr<ID2D1SolidColorBrush>   m_textBrush;
    ComPtr<ID2D1SolidColorBrush>   m_glowBrush;


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
};
