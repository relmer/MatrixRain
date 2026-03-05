#include "pch.h"

#include "HelpRainDialog.h"

#include "CharacterSet.h"
#include "RegistrySettingsProvider.h"
#include "UnicodeSymbols.h"
#include "Version.h"





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::HelpRainDialog
//
//  Pre-computes character positions for the reveal mechanism.
//  Does NOT create the window or start animation — call Show() to begin.
//
////////////////////////////////////////////////////////////////////////////////

HelpRainDialog::HelpRainDialog (const UsageText & usageText) :
    m_usageText (usageText)
{
    // Pre-compute character positions using a temporary text layout
    InitializeTextLayout();
    ComputeCharacterPositions();
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::~HelpRainDialog
//
////////////////////////////////////////////////////////////////////////////////

HelpRainDialog::~HelpRainDialog()
{
    // Release D2D brushes before RenderSystem shutdown
    m_textBrush.Reset();
    m_glowBrush.Reset();

    // Shutdown render pipeline
    if (m_renderSystem)
    {
        m_renderSystem->Shutdown();
    }

    CharacterSet::GetInstance().Shutdown();

    if (m_hWnd)
    {
        DestroyWindow (m_hWnd);
        m_hWnd = nullptr;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::IsRevealComplete
//
//  Returns true when all characters in the reveal queue have been locked in.
//
////////////////////////////////////////////////////////////////////////////////

bool HelpRainDialog::IsRevealComplete () const
{
    for (float opacity : m_revealedFlags)
    {
        if (opacity <= 0.0f)
        {
            return false;
        }
    }

    return !m_revealedFlags.empty();
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::ComputeWindowSize
//
//  Measures text content bounding box using IDWriteTextLayout::GetMetrics().
//  Multiplies by 2x, caps at 80% of primary monitor work area per dimension.
//
////////////////////////////////////////////////////////////////////////////////

SIZE HelpRainDialog::ComputeWindowSize (const UsageText & usageText, float dpi)
{
    HRESULT                    hr      = S_OK;
    ComPtr<IDWriteFactory>     factory;
    ComPtr<IDWriteTextFormat>  format;
    ComPtr<IDWriteTextLayout>  layout;
    DWRITE_TEXT_METRICS        metrics = {};
    float                      fontSize;
    SIZE                       result  = {};



    fontSize = 15.0f * (dpi / 96.0f);

    hr = DWriteCreateFactory (DWRITE_FACTORY_TYPE_SHARED,
                              __uuidof (IDWriteFactory),
                              reinterpret_cast<IUnknown **> (factory.GetAddressOf()));
    CHRA (hr);

    hr = factory->CreateTextFormat (L"Segoe UI",
                                    nullptr,
                                    DWRITE_FONT_WEIGHT_NORMAL,
                                    DWRITE_FONT_STYLE_NORMAL,
                                    DWRITE_FONT_STRETCH_NORMAL,
                                    fontSize,
                                    L"en-us",
                                    &format);
    CHRA (hr);

    {
        const std::wstring & text = usageText.GetFormattedText();

        hr = factory->CreateTextLayout (text.c_str(),
                                        static_cast<UINT32> (text.length()),
                                        format.Get(),
                                        10000.0f,
                                        10000.0f,
                                        &layout);
        CHRA (hr);
    }

    hr = layout->GetMetrics (&metrics);
    CHRA (hr);

    // 2x text bounding box
    result.cx = static_cast<LONG> (metrics.width  * 2.0f);
    result.cy = static_cast<LONG> (metrics.height * 2.0f);

    // Cap at 80% of primary monitor work area
    {
        RECT workArea;
        SystemParametersInfoW (SPI_GETWORKAREA, 0, &workArea, 0);

        int maxWidth  = static_cast<int> ((workArea.right  - workArea.left) * 0.8f);
        int maxHeight = static_cast<int> ((workArea.bottom - workArea.top)  * 0.8f);

        if (result.cx > maxWidth)
        {
            result.cx = maxWidth;
        }

        if (result.cy > maxHeight)
        {
            result.cy = maxHeight;
        }
    }

    // Ensure minimum size
    if (result.cx < 400)
    {
        result.cx = 400;
    }

    if (result.cy < 300)
    {
        result.cy = 300;
    }

Error:
    return result;
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::Show
//
//  Creates the dialog window, initializes the GPU render pipeline, and runs
//  the message loop.  Blocking call — returns when dismissed.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT HelpRainDialog::Show ()
{
    HRESULT hr     = S_OK;
    MSG     msg    = {};
    auto    lastTime = std::chrono::high_resolution_clock::now();



    hr = CreateDialogWindow();
    CHR (hr);

    hr = CreateRenderPipeline();
    CHR (hr);

    // Re-initialize text layout with actual window DPI
    m_textFormat.Reset();
    m_textLayout.Reset();
    InitializeTextLayout();
    ComputeCharacterPositions();

    ShowWindow (m_hWnd, SW_SHOW);
    UpdateWindow (m_hWnd);

    // PeekMessage-based message pump with render-on-idle
    while (!m_dismissed)
    {
        while (PeekMessageW (&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                m_dismissed = true;
                break;
            }

            TranslateMessage (&msg);
            DispatchMessageW (&msg);
        }

        if (!m_dismissed)
        {
            auto now       = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float> (now - lastTime).count();
            lastTime        = now;

            // Cap delta time to avoid huge jumps
            if (deltaTime > 0.1f)
            {
                deltaTime = 0.1f;
            }

            Update (deltaTime);
            RenderFrame();
        }
    }

Error:
    if (m_hWnd)
    {
        DestroyWindow (m_hWnd);
        m_hWnd = nullptr;
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::Update
//
//  Advances the animation state by deltaTime seconds.
//  Delegates rain spawning/updating to AnimationSystem.
//
////////////////////////////////////////////////////////////////////////////////

void HelpRainDialog::Update (float deltaTime)
{
    m_phaseTimer   += deltaTime;
    m_elapsedTime  += deltaTime;

    if (m_animationPhase == AnimationPhase::Revealing)
    {
        UpdateRevealPhase (deltaTime);
    }
    else
    {
        UpdateBackgroundPhase (deltaTime);
    }

    // Ramp up opacity on all revealed characters
    for (float & opacity : m_revealedFlags)
    {
        if (opacity > 0.0f && opacity < 1.0f)
        {
            opacity = std::min (1.0f, opacity + kRevealFadeInSpeed * deltaTime);
        }
    }

    // Density tracks reveal progress: full reveal density when all hidden,
    // drops smoothly to background density as characters are revealed.
    if (m_densityController && !m_characterPositions.empty())
    {
        size_t unrevealed = 0;

        for (float flag : m_revealedFlags)
        {
            if (flag <= 0.0f)
            {
                unrevealed++;
            }
        }

        float hiddenRatio = static_cast<float> (unrevealed) / static_cast<float> (m_characterPositions.size());
        float bgDensity   = std::max (1.0f, m_settings.m_densityPercent * kPhase2DensityMultiplier);
        float density     = bgDensity + hiddenRatio * (static_cast<float> (kRevealDensityPercent) - bgDensity);

        m_densityController->SetPercentage (std::max (1, static_cast<int> (density)));
    }

    // Let AnimationSystem handle streak spawning, updating, and despawning
    if (m_animationSystem)
    {
        m_animationSystem->Update (deltaTime);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::UpdateRevealPhase
//
//  Reveals text characters when rain streaks cross their positions.
//  Each streak's head Y is compared against unrevealed characters:
//  if a streak's column is close enough in X and its head has passed
//  the character's Y, the character begins fading in.
//
//  When no AnimationSystem is available (unit-test path), falls back to
//  a simple time-based sweep front at kRevealSpeed.
//
////////////////////////////////////////////////////////////////////////////////

void HelpRainDialog::UpdateRevealPhase (float deltaTime)
{
    // Always advance the reveal front (used by sweep-front fallback
    // and exposed via GetRevealFrontY for test observability).
    m_revealFrontY += kRevealSpeed * deltaTime;

    if (m_animationSystem)
    {
        // Production path: reveal characters when rain streaks cross them
        for (const auto & streak : m_animationSystem->GetStreaks())
        {
            float streakX    = streak.GetPosition().x;
            float streakHeadY = streak.GetPosition().y;

            for (size_t i = 0; i < m_characterPositions.size(); i++)
            {
                if (m_revealedFlags[i] > 0.0f)
                {
                    continue;
                }

                float charWorldX = m_characterPositions[i].x + m_textOffsetX;
                float charWorldY = m_characterPositions[i].y + m_textOffsetY;

                float dx = std::abs (streakX - charWorldX);

                // Streak column must be close in X and head must have
                // reached or passed the character's Y position.
                if (dx <= kRevealProximityThresholdX && streakHeadY >= charWorldY)
                {
                    m_revealedFlags[i] = 0.01f;  // Start fade-in
                }
            }
        }
    }
    else
    {
        // Test fallback: sweep-front reveals characters by Y position
        for (size_t i = 0; i < m_characterPositions.size(); i++)
        {
            if (m_revealedFlags[i] > 0.0f)
            {
                continue;
            }

            float charWorldY = m_characterPositions[i].y + m_textOffsetY;

            if (charWorldY <= m_revealFrontY)
            {
                m_revealedFlags[i] = 0.01f;  // Start fade-in
            }
        }
    }

    // Check if reveal is complete
    if (IsRevealComplete())
    {
        m_animationPhase = AnimationPhase::Background;
        m_phaseTimer     = 0.0f;

        // Transition to background speed and clear targeted spawning
        if (m_animationSystem)
        {
            int bgSpeed = static_cast<int> (m_settings.m_animationSpeedPercent * kPhase2SpeedMultiplier);
            m_animationSystem->SetAnimationSpeed (std::min (100, bgSpeed));
            m_animationSystem->SetSpawnPositionCallback (nullptr);
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::UpdateBackgroundPhase
//
//  Ambient rain continues at reduced density — AnimationSystem handles
//  all streak lifecycle.  Nothing additional to do here.
//
////////////////////////////////////////////////////////////////////////////////

void HelpRainDialog::UpdateBackgroundPhase (float /* deltaTime */)
{
    // Density is now continuously driven by the hidden-ratio formula
    // in Update() — no phase-specific density logic needed here.
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::InitializeTextLayout
//
//  Creates an IDWriteTextLayout from the formatted text for measurement
//  and character position computation.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT HelpRainDialog::InitializeTextLayout ()
{
    HRESULT hr       = S_OK;
    float   fontSize = 15.0f;



    // If we don't have a DWrite factory yet, create a temporary one
    if (!m_dwriteFactory)
    {
        hr = DWriteCreateFactory (DWRITE_FACTORY_TYPE_SHARED,
                                  __uuidof (IDWriteFactory),
                                  reinterpret_cast<IUnknown **> (m_dwriteFactory.GetAddressOf()));
        CHRA (hr);
    }

    if (!m_textFormat)
    {
        // DPI-scale the font size
        if (m_hWnd)
        {
            UINT dpi = GetDpiForWindow (m_hWnd);
            fontSize = 15.0f * (static_cast<float> (dpi) / 96.0f);
        }

        hr = m_dwriteFactory->CreateTextFormat (L"Segoe UI",
                                                 nullptr,
                                                 DWRITE_FONT_WEIGHT_NORMAL,
                                                 DWRITE_FONT_STYLE_NORMAL,
                                                 DWRITE_FONT_STRETCH_NORMAL,
                                                 fontSize,
                                                 L"en-us",
                                                 &m_textFormat);
        CHRA (hr);
    }

    {
        const std::wstring & text = m_usageText.GetFormattedText();

        float maxWidth  = (m_windowWidth  > 0) ? static_cast<float> (m_windowWidth)  : 10000.0f;
        float maxHeight = (m_windowHeight > 0) ? static_cast<float> (m_windowHeight) : 10000.0f;

        m_textLayout.Reset();

        hr = m_dwriteFactory->CreateTextLayout (text.c_str(),
                                                 static_cast<UINT32> (text.length()),
                                                 m_textFormat.Get(),
                                                 maxWidth,
                                                 maxHeight,
                                                 &m_textLayout);
        CHRA (hr);

        // Get text metrics for bounding box and centering
        DWRITE_TEXT_METRICS metrics = {};

        hr = m_textLayout->GetMetrics (&metrics);
        CHRA (hr);

        m_textBoundingBox.left   = 0;
        m_textBoundingBox.top    = 0;
        m_textBoundingBox.right  = metrics.width;
        m_textBoundingBox.bottom = metrics.height;

        // Center text in window
        float windowW = (m_windowWidth  > 0) ? static_cast<float> (m_windowWidth)  : metrics.width  * 2.0f;
        float windowH = (m_windowHeight > 0) ? static_cast<float> (m_windowHeight) : metrics.height * 2.0f;

        m_textOffsetX = (windowW - metrics.width)  / 2.0f;
        m_textOffsetY = (windowH - metrics.height) / 2.0f;
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::ComputeCharacterPositions
//
//  Iterates every non-space character in the formatted text and calls
//  IDWriteTextLayout::HitTestTextPosition to get (x, y) pixel coordinates.
//
////////////////////////////////////////////////////////////////////////////////

void HelpRainDialog::ComputeCharacterPositions ()
{
    m_characterPositions.clear();

    if (!m_textLayout)
    {
        return;
    }

    const std::wstring & text = m_usageText.GetFormattedText();

    for (size_t i = 0; i < text.length(); i++)
    {
        wchar_t ch = text[i];

        if (ch == L' ' || ch == L'\n' || ch == L'\r')
        {
            continue;
        }

        DWRITE_HIT_TEST_METRICS hitMetrics = {};
        float                   pointX     = 0.0f;
        float                   pointY     = 0.0f;

        HRESULT hr = m_textLayout->HitTestTextPosition (static_cast<UINT32> (i),
                                                         FALSE,
                                                         &pointX,
                                                         &pointY,
                                                         &hitMetrics);

        if (SUCCEEDED (hr))
        {
            CharPosition pos;
            pos.charIndex = i;
            pos.x         = pointX;
            pos.y         = pointY;
            pos.character = ch;
            m_characterPositions.push_back (pos);
        }
    }

    // Initialize revealed flags
    m_revealedFlags.assign (m_characterPositions.size(), 0.0f);

    // Initialize the reveal front above the window
    m_revealFrontY = -50.0f;
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::CreateDialogWindow
//
//  Registers window class and creates the popup window.
//  Non-resizable with close button, centered on primary monitor.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT HelpRainDialog::CreateDialogWindow ()
{
    HRESULT    hr        = S_OK;
    WNDCLASSW  wc        = {};
    SIZE       clientSize;
    RECT       windowRect;
    RECT       workArea;
    int        windowX;
    int        windowY;
    int        windowW;
    int        windowH;



    // Register window class
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = sizeof (LONG_PTR);
    wc.hInstance     = GetModuleHandleW (nullptr);
    wc.hIcon         = LoadIconW (nullptr, IDI_APPLICATION);
    wc.hCursor       = LoadCursorW (nullptr, IDC_ARROW);
    wc.hbrBackground = static_cast<HBRUSH> (GetStockObject (BLACK_BRUSH));
    wc.lpszMenuName  = nullptr;
    wc.lpszClassName = kWindowClassName;

    RegisterClassW (&wc);

    // Compute window size using actual system DPI (must match rendering DPI)
    {
        UINT dpi = GetDpiForSystem();

        clientSize = ComputeWindowSize (m_usageText, static_cast<float> (dpi));
    }

    // Adjust for window chrome (DPI-aware)
    {
        UINT dpi = GetDpiForSystem();

        windowRect.left   = 0;
        windowRect.top    = 0;
        windowRect.right  = clientSize.cx;
        windowRect.bottom = clientSize.cy;

        AdjustWindowRectExForDpi (&windowRect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE, 0, dpi);
    }

    windowW = windowRect.right  - windowRect.left;
    windowH = windowRect.bottom - windowRect.top;

    // Center on primary monitor work area
    SystemParametersInfoW (SPI_GETWORKAREA, 0, &workArea, 0);

    windowX = workArea.left + ((workArea.right  - workArea.left) - windowW) / 2;
    windowY = workArea.top  + ((workArea.bottom - workArea.top)  - windowH) / 2;

    // Build title with em dash: "MatrixRain — Help"
    std::wstring title = std::format (L"MatrixRain {} Help", UnicodeSymbols::EmDash);

    m_hWnd = CreateWindowExW (0,
                              kWindowClassName,
                              title.c_str(),
                              WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                              windowX, windowY,
                              windowW, windowH,
                              nullptr,
                              nullptr,
                              GetModuleHandleW (nullptr),
                              this);

    CWRA (m_hWnd != nullptr);

    m_windowWidth  = clientSize.cx;
    m_windowHeight = clientSize.cy;

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::CreateDeviceResources
//
//  Creates the GPU render pipeline: RenderSystem, CharacterSet, AnimationSystem,
//  Viewport, DensityController, and D2D text overlay brushes.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT HelpRainDialog::CreateRenderPipeline ()
{
    HRESULT hr = S_OK;



    // Load user's settings from registry (color scheme, glow, etc.)
    hr = LoadUserSettings();
    CHR (hr);

    // Create RenderSystem — creates D3D11 device, swap chain, shaders, and bloom
    m_renderSystem = std::make_unique<RenderSystem>();

    hr = m_renderSystem->Initialize (m_hWnd,
                                     static_cast<UINT> (m_windowWidth),
                                     static_cast<UINT> (m_windowHeight));
    CHR (hr);

    // Apply user's glow settings
    m_renderSystem->SetGlowIntensity (m_settings.m_glowIntensityPercent);
    m_renderSystem->SetGlowSize (m_settings.m_glowSizePercent);

    // Initialize CharacterSet atlas on RenderSystem's device
    {
        CharacterSet & charSet = CharacterSet::GetInstance();

        if (!charSet.Initialize())
        {
            hr = E_FAIL;
            CHR (hr);
        }

        hr = charSet.CreateTextureAtlas (m_renderSystem->GetDevice());
        CHR (hr);
    }

    // Create Viewport and DensityController
    m_viewport = std::make_unique<Viewport>();
    m_viewport->Resize (static_cast<float> (m_windowWidth),
                        static_cast<float> (m_windowHeight));

    m_densityController = std::make_unique<DensityController> (*m_viewport, 32.0f);
    m_densityController->SetPercentage (kRevealDensityPercent);

    // Create AnimationSystem — max density + max speed for reveal phase
    m_animationSystem = std::make_unique<AnimationSystem>();
    m_animationSystem->Initialize (*m_viewport, *m_densityController);
    m_animationSystem->SetAnimationSpeed (kRevealSpeedPercent);

    // Force full-size rain characters regardless of viewport size.
    // Without overrides the pipeline scales characters down proportionally
    // for viewports smaller than 1080px — fine for the settings preview
    // but wrong for the help dialog where we want full-impact rain.
    m_animationSystem->SetCharacterSpacingOverride (32.0f);
    m_renderSystem->SetCharacterScaleOverride (1.0f);

    // Install targeted spawning callback — biases new streak positions
    // toward unrevealed character columns during the reveal phase.
    // ~70% of spawns target unrevealed characters; ~30% are fully random
    // to maintain natural rain coverage across the entire viewport.
    m_animationSystem->SetSpawnPositionCallback (
        [this] (const SpawnRange & /* range */) -> std::optional<float>
        {
            m_spawnCallCounter++;

            // 30% of spawns are random for visual variety
            if (m_spawnCallCounter % 10 >= 7)
            {
                return std::nullopt;
            }

            // Find the next unrevealed character, cycling from the counter
            // position so spawns distribute across all unrevealed columns
            size_t count = m_characterPositions.size();

            if (count == 0)
            {
                return std::nullopt;
            }

            for (size_t j = 0; j < count; j++)
            {
                size_t idx = (m_spawnCallCounter + j) % count;

                if (m_revealedFlags[idx] <= 0.0f)
                {
                    return m_characterPositions[idx].x + m_textOffsetX;
                }
            }

            return std::nullopt;  // All revealed
        }
    );

    // Create D2D text overlay brushes on RenderSystem's D2D context
    {
        ID2D1DeviceContext * d2dContext = m_renderSystem->GetD2DContext();
        CBRA (d2dContext != nullptr);

        // White text brush for resolved usage text
        hr = d2dContext->CreateSolidColorBrush (D2D1::ColorF (D2D1::ColorF::White), &m_textBrush);
        CHRA (hr);

        // Black glow brush (variable opacity)
        hr = d2dContext->CreateSolidColorBrush (D2D1::ColorF (D2D1::ColorF::Black, 0.0f), &m_glowBrush);
        CHRA (hr);

        // Keep D2D context at 96 DPI — the font size and all text layout
        // coordinates are already manually DPI-scaled to physical pixel
        // space, so 1 D2D unit = 1 physical pixel is the correct mapping.
        d2dContext->SetDpi (96.0f, 96.0f);
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::LoadUserSettings
//
//  Loads the user's screensaver settings from the registry to pick up
//  color scheme, glow intensity, and glow size.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT HelpRainDialog::LoadUserSettings ()
{
    // Load settings from registry — ignore failure (defaults are fine)
    RegistrySettingsProvider::Load (m_settings);
    m_settings.Clamp();

    m_colorScheme = ParseColorSchemeKey (m_settings.m_colorSchemeKey);

    return S_OK;
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::RenderFrame
//
//  Renders one frame: GPU rain (via RenderSystem) → D2D text overlay → present.
//
////////////////////////////////////////////////////////////////////////////////

void HelpRainDialog::RenderFrame ()
{
    if (!m_renderSystem || !m_animationSystem || !m_viewport)
    {
        return;
    }

    // Render GPU rain with bloom
    m_renderSystem->Render (*m_animationSystem,
                            *m_viewport,
                            m_colorScheme,
                            0.0f,     // fps (hidden)
                            0,        // rainPercentage
                            0,        // streakCount
                            0,        // activeHeadCount
                            false,    // showDebugFadeTimes
                            m_elapsedTime);

    // D2D text overlay on top of rain
    RenderTextOverlay();

    m_renderSystem->Present();
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::RenderTextOverlay
//
//  Draws revealed usage text on top of the GPU-rendered rain using D2D.
//  Two passes: dark glow halos first, then white foreground text.
//  Uses RenderSystem's D2D context for rendering.
//
////////////////////////////////////////////////////////////////////////////////

void HelpRainDialog::RenderTextOverlay ()
{
    if (!m_renderSystem || !m_textBrush || !m_glowBrush || !m_textFormat)
    {
        return;
    }

    ID2D1DeviceContext * d2dContext = m_renderSystem->GetD2DContext();

    if (!d2dContext)
    {
        return;
    }

    d2dContext->BeginDraw();

    static constexpr float kCharWidth  = 20.0f;
    static constexpr float kCharHeight = 24.0f;

    // Pass 1: Draw all feathered dark glows
    for (size_t i = 0; i < m_characterPositions.size(); i++)
    {
        float opacity = m_revealedFlags[i];

        if (opacity <= 0.0f)
        {
            continue;
        }

        const CharPosition & pos = m_characterPositions[i];
        float drawX              = pos.x + m_textOffsetX;
        float drawY              = pos.y + m_textOffsetY;

        DrawCharacterGlow (d2dContext, pos.character, drawX, drawY, opacity);
    }

    // Pass 2: Draw all foreground text on top
    for (size_t i = 0; i < m_characterPositions.size(); i++)
    {
        float opacity = m_revealedFlags[i];

        if (opacity <= 0.0f)
        {
            continue;
        }

        const CharPosition & pos = m_characterPositions[i];
        float drawX              = pos.x + m_textOffsetX;
        float drawY              = pos.y + m_textOffsetY;

        wchar_t     str[2]   = { pos.character, L'\0' };
        D2D1_RECT_F charRect = D2D1::RectF (drawX, drawY, drawX + kCharWidth, drawY + kCharHeight);

        m_textBrush->SetOpacity (opacity);
        d2dContext->DrawText (str, 1, m_textFormat.Get(), charRect, m_textBrush.Get());
        m_textBrush->SetOpacity (1.0f);
    }

    d2dContext->EndDraw();
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::DrawCharacterGlow
//
//  Renders only the feathered dark glow halo for a single character.
//  The foreground text is drawn separately in a second pass so that
//  adjacent characters' glow layers cannot dim already-drawn text.
//
////////////////////////////////////////////////////////////////////////////////

void HelpRainDialog::DrawCharacterGlow (ID2D1DeviceContext * pContext, wchar_t ch, float x, float y, float opacity)
{
    if (!pContext || !m_textFormat || !m_glowBrush)
    {
        return;
    }

    static constexpr float kCharWidth  = 20.0f;
    static constexpr float kCharHeight = 24.0f;

    wchar_t str[2] = { ch, L'\0' };

    D2D1_RECT_F charRect = D2D1::RectF (x, y, x + kCharWidth, y + kCharHeight);

    // Draw feathered dark glow
    const int glowLayers = 8;

    for (int i = glowLayers; i > 0; --i)
    {
        float offset    = static_cast<float> (i);
        float baseAlpha = 0.9f - (static_cast<float> (i) / static_cast<float> (glowLayers));

        m_glowBrush->SetColor (D2D1::ColorF (D2D1::ColorF::Black, baseAlpha * opacity));

        for (int dx = -1; dx <= 1; ++dx)
        {
            for (int dy = -1; dy <= 1; ++dy)
            {
                if (dx == 0 && dy == 0)
                {
                    continue;
                }

                D2D1_RECT_F glowRect = D2D1::RectF (charRect.left   + offset * dx,
                                                     charRect.top    + offset * dy,
                                                     charRect.right  + offset * dx,
                                                     charRect.bottom + offset * dy);

                pContext->DrawText (str, 1, m_textFormat.Get(), glowRect, m_glowBrush.Get());
            }
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::WndProc
//
//  Window procedure — handles WM_KEYDOWN (Enter/Escape to dismiss),
//  WM_CLOSE, WM_DESTROY, close button (X).
//
////////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK HelpRainDialog::WndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HelpRainDialog * pDialog = nullptr;

    if (msg == WM_CREATE)
    {
        CREATESTRUCTW * pCreate = reinterpret_cast<CREATESTRUCTW *> (lParam);
        pDialog = static_cast<HelpRainDialog *> (pCreate->lpCreateParams);
        SetWindowLongPtrW (hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR> (pDialog));

        return 0;
    }

    pDialog = reinterpret_cast<HelpRainDialog *> (GetWindowLongPtrW (hwnd, GWLP_USERDATA));

    switch (msg)
    {
        case WM_KEYDOWN:
        {
            if (wParam == VK_RETURN || wParam == VK_ESCAPE)
            {
                if (pDialog)
                {
                    pDialog->m_dismissed = true;
                }

                return 0;
            }

            // DEBUG: Space resets animation to initial state
            if (wParam == VK_SPACE)
            {
                if (pDialog)
                {
                    pDialog->m_animationPhase = AnimationPhase::Revealing;
                    pDialog->m_phaseTimer     = 0.0f;
                    pDialog->m_revealFrontY   = -50.0f;
                    pDialog->m_elapsedTime    = 0.0f;
                    pDialog->m_revealedFlags.assign (pDialog->m_characterPositions.size(), 0.0f);

                    // Reset AnimationSystem to reveal-phase density/speed
                    if (pDialog->m_animationSystem)
                    {
                        pDialog->m_animationSystem->ClearAllStreaks();
                        pDialog->m_animationSystem->SetAnimationSpeed (kRevealSpeedPercent);
                    }

                    if (pDialog->m_densityController)
                    {
                        pDialog->m_densityController->SetPercentage (kRevealDensityPercent);
                    }
                }

                return 0;
            }

            break;
        }

        case WM_CLOSE:
        {
            if (pDialog)
            {
                pDialog->m_dismissed = true;
            }

            return 0;
        }

        case WM_DESTROY:
        {
            PostQuitMessage (0);
            return 0;
        }
    }

    return DefWindowProcW (hwnd, msg, wParam, lParam);
}
