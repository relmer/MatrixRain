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
//  Displays command-line switches via GPU-rendered matrix rain with a
//  horizontal tracer reveal.  Short rain-like streaks sweep left to right
//  along each text line, revealing characters in their wake.  Background
//  rain runs at the user's configured steady-state density throughout.
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
    static constexpr float kTracerDurationMin        = 2.0f;
    static constexpr float kTracerDurationMax        = 4.0f;
    static constexpr int   kRevealSpeedPercent       = 100;
    static constexpr float kTracerTrailMin           = 250.0f;
    static constexpr float kTracerTrailMax           = 550.0f;
    static constexpr float kTracerStaggerRange       = 0.8f;

    // Tunable constants — background phase
    static constexpr float kBgDensityMultiplier     = 0.15f;
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
    void    ComputeLineYPositions();
    size_t  FindLineIndex (float charY) const;
    float   ComputeLineTracerX (size_t lineIndex) const;

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
    float                          m_tracerProgress  = 0.0f;

    std::vector<CharPosition>      m_characterPositions;
    std::vector<float>             m_revealedFlags;
    std::vector<float>             m_revealTimes;
    std::vector<float>             m_lineYPositions;
    std::vector<float>             m_lineTracerDelays;
    std::vector<float>             m_lineTracerDurations;
    std::vector<float>             m_lineTrailPixels;
    std::vector<float>             m_lineFadeDurations;
    float                          m_textMinX        = 0.0f;
    float                          m_textMaxX        = 0.0f;


    // Text layout (DWrite — independent of RenderSystem D2D)
    ComPtr<IDWriteFactory>         m_dwriteFactory;
    ComPtr<IDWriteTextFormat>      m_textFormat;
    ComPtr<IDWriteTextLayout>      m_textLayout;

    // D2D text overlay brushes (created on RenderSystem's D2D context)
    ComPtr<ID2D1SolidColorBrush>   m_textBrush;
    ComPtr<ID2D1SolidColorBrush>   m_glowBrush;
    ComPtr<ID2D1SolidColorBrush>   m_tracerBrush;


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
