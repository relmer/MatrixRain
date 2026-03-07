#include "pch.h"

#include "UsageDialog.h"

#include "CharacterSet.h"
#include "UnicodeSymbols.h"
#include "Version.h"





////////////////////////////////////////////////////////////////////////////////
//
//  UsageDialog::UsageDialog
//
//  Pre-computes character positions for the reveal mechanism.
//  Does NOT create the window or start animation — call Show() to begin.
//
////////////////////////////////////////////////////////////////////////////////

UsageDialog::UsageDialog (const UsageText & usageText, ISettingsProvider & settingsProvider) :
    m_usageText        (usageText),
    m_settingsProvider (settingsProvider)
{
    // Pre-compute character positions using a temporary text layout
    InitializeTextLayout();
    ComputeCharacterPositions();
    ComputeLineYPositions();
}





////////////////////////////////////////////////////////////////////////////////
//
//  UsageDialog::~UsageDialog
//
////////////////////////////////////////////////////////////////////////////////

UsageDialog::~UsageDialog()
{
    // Release D2D brushes before RenderSystem shutdown
    m_textBrush.Reset();
    m_tracerBrush.Reset();
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
//  UsageDialog::IsRevealComplete
//
//  Returns true when all characters have been revealed AND all per-line
//  tracer trails have fully swept past the text.  This prevents the
//  trail animation from being cut short when the head passes the last
//  character but the trail is still visible.
//
////////////////////////////////////////////////////////////////////////////////

bool UsageDialog::IsRevealComplete () const
{
    for (float opacity : m_revealedFlags)
    {
        if (opacity <= 0.0f)
        {
            return false;
        }
    }

    if (m_revealedFlags.empty())
    {
        return false;
    }

    // All characters revealed — now ensure every line's trail has
    // finished sweeping (lineProgress == 1.0 for all lines).
    for (size_t j = 0; j < m_lineTracerDelays.size(); j++)
    {
        float delay    = m_lineTracerDelays[j];
        float duration = (j < m_lineTracerDurations.size()) ? m_lineTracerDurations[j] : kTracerDurationMax;
        float progress = (m_phaseTimer - delay) / duration;

        if (progress < 1.0f)
        {
            return false;
        }
    }

    return true;
}





////////////////////////////////////////////////////////////////////////////////
//
//  UsageDialog::ComputeWindowSize
//
//  Measures text content bounding box using IDWriteTextLayout::GetMetrics().
//  Multiplies by 2x, caps at 80% of primary monitor work area per dimension.
//
////////////////////////////////////////////////////////////////////////////////

SIZE UsageDialog::ComputeWindowSize (const UsageText & usageText, float dpi)
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
//  UsageDialog::Show
//
//  Creates the dialog window, initializes the GPU render pipeline, and runs
//  the message loop.  Blocking call — returns when dismissed.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT UsageDialog::Show ()
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
    ComputeLineYPositions();

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
//  UsageDialog::Update
//
//  Advances the animation state by deltaTime seconds.
//  Delegates rain spawning/updating to AnimationSystem.
//
////////////////////////////////////////////////////////////////////////////////

void UsageDialog::Update (float deltaTime)
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

    // Let AnimationSystem handle streak spawning, updating, and despawning
    // (only during background phase — no rain during reveal)
    if (m_animationSystem && m_animationPhase == AnimationPhase::Background)
    {
        m_animationSystem->Update (deltaTime);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  UsageDialog::UpdateRevealPhase
//
//  Advances horizontal tracer streaks left to right across each text line.
//  Characters are revealed as the tracer head passes their X position.
//  When no AnimationSystem is available (unit-test path), falls back to
//  a simple time-based sweep front at kRevealSpeed.
//
////////////////////////////////////////////////////////////////////////////////

void UsageDialog::UpdateRevealPhase (float deltaTime)
{
    // Always advance the reveal front (used by sweep-front fallback
    // and exposed via GetRevealFrontY for test observability).
    m_revealFrontY += kRevealSpeed * deltaTime;

    // Advance tracer progress (0 = left edge, 1 = right edge)
    m_tracerProgress = std::min (1.0f, m_phaseTimer / kTracerDurationMax);

    if (m_animationSystem)
    {
        // Production path: tracer reveals characters as it sweeps right.
        // Each line has its own staggered tracer position.
        for (size_t i = 0; i < m_characterPositions.size(); i++)
        {
            if (m_revealedFlags[i] > 0.0f)
            {
                continue;
            }

            float  charWorldX = m_characterPositions[i].x + m_textOffsetX;
            size_t lineIdx    = FindLineIndex (m_characterPositions[i].y);
            float  lineTracerX = ComputeLineTracerX (lineIdx);

            if (charWorldX <= lineTracerX)
            {
                m_revealedFlags[i] = 0.01f;  // Start fade-in
                m_revealTimes[i]   = m_elapsedTime;

                // Assign a random katakana glyph once, on first reveal
                size_t katIndex = (i * 7 + 13) % CharacterConstants::HALFWIDTH_KATAKANA_COUNT;
                m_characterPositions[i].randomCharacter = static_cast<wchar_t> (CharacterConstants::HALFWIDTH_KATAKANA_CODEPOINTS[katIndex]);
            }
        }

        // Tracer coloring is handled in RenderTextOverlay via D2D
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
                m_revealTimes[i]   = m_elapsedTime;

                // Assign a random katakana glyph once, on first reveal
                size_t katIndex = (i * 7 + 13) % CharacterConstants::HALFWIDTH_KATAKANA_COUNT;
                m_characterPositions[i].randomCharacter = static_cast<wchar_t> (CharacterConstants::HALFWIDTH_KATAKANA_CODEPOINTS[katIndex]);
            }
        }
    }

    // Check if reveal is complete
    if (IsRevealComplete())
    {
        m_animationPhase = AnimationPhase::Background;
        m_phaseTimer     = 0.0f;

        // Start background rain now that text is fully revealed
        if (m_animationSystem)
        {
            int bgDensity = std::max (1, static_cast<int> (m_settings.m_densityPercent * kBgDensityMultiplier));
            m_densityController->SetPercentage (bgDensity);

            int bgSpeed = static_cast<int> (m_settings.m_animationSpeedPercent * kPhase2SpeedMultiplier);
            m_animationSystem->SetAnimationSpeed (std::min (100, bgSpeed));
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  UsageDialog::UpdateBackgroundPhase
//
//  Ambient rain continues at reduced density — AnimationSystem handles
//  all streak lifecycle.  Nothing additional to do here.
//
////////////////////////////////////////////////////////////////////////////////

void UsageDialog::UpdateBackgroundPhase (float /* deltaTime */)
{
    // Background rain runs at steady-state — AnimationSystem handles
    // all streak lifecycle.  Nothing additional to do here.
}








////////////////////////////////////////////////////////////////////////////////
//
//  UsageDialog::ComputeLineYPositions
//
//  Extracts the unique Y coordinates from character positions to know
//  which text lines exist.  Used to spawn one tracer per line.
//
////////////////////////////////////////////////////////////////////////////////

void UsageDialog::ComputeLineYPositions()
{
    m_lineYPositions.clear();
    m_textMinX =  std::numeric_limits<float>::max();
    m_textMaxX = -std::numeric_limits<float>::max();



    for (const auto & pos : m_characterPositions)
    {
        // Track overall X bounds
        float worldX = pos.x + m_textOffsetX;

        if (worldX < m_textMinX) m_textMinX = worldX;
        if (worldX > m_textMaxX) m_textMaxX = worldX;

        // Collect unique Y values (within 1px tolerance for float rounding)
        bool found = false;

        for (float existing : m_lineYPositions)
        {
            if (std::abs (existing - pos.y) < 1.0f)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            m_lineYPositions.push_back (pos.y);
        }
    }

    // Sort top to bottom for visual consistency
    std::sort (m_lineYPositions.begin(), m_lineYPositions.end());

    // Generate random per-line parameters so each streak has its own
    // start delay, sweep speed, and trail length.
    size_t lineCount = m_lineYPositions.size();

    std::mt19937 rng (42);
    std::uniform_real_distribution<float> delayDist    (0.0f, kTracerStaggerRange);
    std::uniform_real_distribution<float> durationDist (kTracerDurationMin, kTracerDurationMax);
    std::uniform_real_distribution<float> trailDist    (kTracerTrailMin, kTracerTrailMax);

    m_lineTracerDelays.resize    (lineCount);
    m_lineTracerDurations.resize (lineCount);
    m_lineTrailPixels.resize     (lineCount);
    m_lineFadeDurations.resize   (lineCount);

    float textWidth = m_textMaxX - m_textMinX;

    for (size_t j = 0; j < lineCount; j++)
    {
        m_lineTracerDelays[j]    = delayDist (rng);
        m_lineTracerDurations[j] = durationDist (rng);
        m_lineTrailPixels[j]     = trailDist (rng);

        // Fade-to-white duration matches the time a character dwells in the trail
        float sweepSpan          = textWidth + m_lineTrailPixels[j];
        m_lineFadeDurations[j]   = (sweepSpan > 0.0f)
                                   ? m_lineTrailPixels[j] * m_lineTracerDurations[j] / sweepSpan
                                   : 1.0f;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  UsageDialog::FindLineIndex
//
//  Returns the index into m_lineYPositions that matches the given
//  character Y position (within 1px tolerance).  Returns 0 if no match.
//
////////////////////////////////////////////////////////////////////////////////

size_t UsageDialog::FindLineIndex (float charY) const
{
    for (size_t j = 0; j < m_lineYPositions.size(); j++)
    {
        if (std::abs (m_lineYPositions[j] - charY) < 1.0f)
        {
            return j;
        }
    }

    return 0;
}





////////////////////////////////////////////////////////////////////////////////
//
//  UsageDialog::ComputeLineTracerX
//
//  Computes the current tracer X position for a given line, accounting
//  for that line's random stagger delay.
//
////////////////////////////////////////////////////////////////////////////////

float UsageDialog::ComputeLineTracerX (size_t lineIndex) const
{
    float delay    = (lineIndex < m_lineTracerDelays.size())    ? m_lineTracerDelays[lineIndex]    : 0.0f;
    float duration = (lineIndex < m_lineTracerDurations.size()) ? m_lineTracerDurations[lineIndex] : kTracerDurationMax;
    float trail    = (lineIndex < m_lineTrailPixels.size())     ? m_lineTrailPixels[lineIndex]     : kTracerTrailMax;

    // Tracer hasn't started for this line yet — return a position
    // far enough left that no characters will match.
    if (m_phaseTimer < delay)
    {
        return m_textMinX - trail - 1.0f;
    }

    float lineProgress = std::clamp ((m_phaseTimer - delay) / duration, 0.0f, 1.0f);
    float textWidth    = m_textMaxX - m_textMinX;

    return m_textMinX + lineProgress * (textWidth + trail);
}





////////////////////////////////////////////////////////////////////////////////
//
//  UsageDialog::InitializeTextLayout
//
//  Creates an IDWriteTextLayout from the formatted text for measurement
//  and character position computation.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT UsageDialog::InitializeTextLayout ()
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
//  UsageDialog::ComputeCharacterPositions
//
//  Iterates every non-space character in the formatted text and calls
//  IDWriteTextLayout::HitTestTextPosition to get (x, y) pixel coordinates.
//
////////////////////////////////////////////////////////////////////////////////

void UsageDialog::ComputeCharacterPositions ()
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

    // Initialize revealed flags and reveal times
    m_revealedFlags.assign (m_characterPositions.size(), 0.0f);
    m_revealTimes.assign   (m_characterPositions.size(), 0.0f);

    // Initialize the reveal front above the window
    m_revealFrontY = -50.0f;
}





////////////////////////////////////////////////////////////////////////////////
//
//  UsageDialog::CreateDialogWindow
//
//  Registers window class and creates the popup window.
//  Non-resizable with close button, centered on primary monitor.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT UsageDialog::CreateDialogWindow ()
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
    wc.hIcon         = LoadIconW (GetModuleHandleW (nullptr), MAKEINTRESOURCEW (101));  // IDI_MATRIXRAIN
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
//  UsageDialog::CreateDeviceResources
//
//  Creates the GPU render pipeline: RenderSystem, CharacterSet, AnimationSystem,
//  Viewport, DensityController, and D2D text overlay brushes.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT UsageDialog::CreateRenderPipeline ()
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
    m_densityController->SetPercentage (0);  // No rain during reveal; starts after text is fully revealed

    // Create AnimationSystem — steady-state density and speed from the start.
    // The horizontal tracers handle reveal; background rain is purely ambient.
    m_animationSystem = std::make_unique<AnimationSystem>();
    m_animationSystem->Initialize (*m_viewport, *m_densityController);
    m_animationSystem->ClearAllStreaks();  // Remove the MIN_STREAKS streak spawned during init
    m_animationSystem->SetAnimationSpeed (std::min (100, static_cast<int> (m_settings.m_animationSpeedPercent)));

    // Force full-size rain characters regardless of viewport size.
    // Without overrides the pipeline scales characters down proportionally
    // for viewports smaller than 1080px — fine for the settings preview
    // but wrong for the help dialog where we want full-impact rain.
    m_animationSystem->SetCharacterSpacingOverride (32.0f);
    m_renderSystem->SetCharacterScaleOverride (1.0f);

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

        // Green brush for tracer trail (variable color/opacity)
        hr = d2dContext->CreateSolidColorBrush (D2D1::ColorF (0.0f, 1.0f, 0.0f, 1.0f), &m_tracerBrush);
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
//  UsageDialog::LoadUserSettings
//
//  Loads the user's screensaver settings from the registry to pick up
//  color scheme, glow intensity, and glow size.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT UsageDialog::LoadUserSettings ()
{
    // Load settings from provider — ignore failure (defaults are fine)
    m_settingsProvider.Load (m_settings);
    m_settings.Clamp();

    m_colorScheme = ParseColorSchemeKey (m_settings.m_colorSchemeKey);

    return S_OK;
}





////////////////////////////////////////////////////////////////////////////////
//
//  UsageDialog::RenderFrame
//
//  Renders one frame: GPU rain (via RenderSystem) → D2D text overlay → present.
//
////////////////////////////////////////////////////////////////////////////////

void UsageDialog::RenderFrame ()
{
    if (!m_renderSystem || !m_animationSystem || !m_viewport)
    {
        return;
    }

    // Render GPU rain with bloom (no occlusion — D2D glow halos keep text readable)
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
//  UsageDialog::RenderTextOverlay
//
//  Draws revealed usage text on top of the GPU-rendered rain using D2D.
//  Two passes: dark glow halos first, then white foreground text.
//  Uses RenderSystem's D2D context for rendering.
//
////////////////////////////////////////////////////////////////////////////////

void UsageDialog::RenderTextOverlay ()
{
    if (!m_renderSystem || !m_textBrush || !m_glowBrush || !m_tracerBrush || !m_textFormat)
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

    bool  isTracing = (m_animationPhase == AnimationPhase::Revealing && m_animationSystem != nullptr);

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
    // Characters in the tracer zone show random matrix rain characters (katakana)
    // that transition to the actual text character halfway through the trail.
    // This creates the iconic "decoding" effect:
    //   Head        — bright white random char
    //   Early trail — green random chars, brightness fading
    //   Mid trail   — crossfade from random to actual char
    //   Late trail  — actual text char in green, settling to white
    //   Behind      — normal white text (fully revealed)
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

        D2D1_RECT_F charRect = D2D1::RectF (drawX, drawY, drawX + kCharWidth, drawY + kCharHeight);

        // Compute per-line tracer X to support staggered line reveals
        size_t lineIdx       = FindLineIndex (pos.y);
        float  lineTracerX   = ComputeLineTracerX (lineIdx);
        float  lineTrail     = (lineIdx < m_lineTrailPixels.size())     ? m_lineTrailPixels[lineIdx]     : kTracerTrailMax;

        // Determine character color based on distance from tracer head
        float distFromTracer = lineTracerX - drawX;

        if (isTracing && distFromTracer >= 0.0f && distFromTracer < lineTrail)
        {
            // Character is in the tracer trail zone
            float t = distFromTracer / lineTrail;  // 0 = head, 1 = tail end

            // Use the one-time random katakana assigned at reveal.
            wchar_t randomStr[2] = { pos.randomCharacter, L'\0' };
            wchar_t actualStr[2] = { pos.character, L'\0' };

            if (t < 0.5f)
            {
                // First half: random matrix character (white head → fading green trail)
                if (distFromTracer < kCharWidth)
                {
                    // Head: bright white random char (exactly one character)
                    m_textBrush->SetOpacity (1.0f);
                    d2dContext->DrawText (randomStr, 1, m_textFormat.Get(), charRect, m_textBrush.Get());
                }
                else
                {
                    // Early trail: green random char, brightness proportional to position
                    float greenBrightness = 1.0f - (t - 0.10f) / 0.40f;  // 1.0 → 0.0 over 10%–50%

                    m_tracerBrush->SetColor (D2D1::ColorF (0.0f, 0.3f + 0.7f * greenBrightness, 0.0f, 1.0f));
                    m_tracerBrush->SetOpacity (1.0f);
                    d2dContext->DrawText (randomStr, 1, m_textFormat.Get(), charRect, m_tracerBrush.Get());
                }
            }
            else
            {
                // Second half: actual text character stays green
                // (matching the dim green at the end of the first half)
                // and brightens slightly toward the tail.
                float settleT = (t - 0.5f) / 0.5f;  // 0 → 1 over the second half
                float g       = 0.3f + 0.2f * settleT;   // 0.3 → 0.5

                m_tracerBrush->SetColor (D2D1::ColorF (0.0f, g, 0.0f, 1.0f));
                m_tracerBrush->SetOpacity (1.0f);
                d2dContext->DrawText (actualStr, 1, m_textFormat.Get(), charRect, m_tracerBrush.Get());
            }
        }
        else
        {
            // Revealed text behind the trail — fade from green to white
            // using time since reveal, so the fade continues even after
            // the tracer phase ends.
            wchar_t str[2] = { pos.character, L'\0' };

            float timeSinceReveal = m_elapsedTime - m_revealTimes[i];
            float lineFade        = (lineIdx < m_lineFadeDurations.size()) ? m_lineFadeDurations[lineIdx] : 1.0f;
            float timeSinceExit   = timeSinceReveal - lineFade;  // Offset by trail dwell time
            float fadeT           = std::clamp (timeSinceExit / 0.3f, 0.0f, 1.0f);

            if (fadeT < 1.0f)
            {
                // Lerp from dim green (matching trail tail) to white
                float r = fadeT;
                float g = 0.5f + 0.5f * fadeT;
                float b = fadeT;

                m_tracerBrush->SetColor (D2D1::ColorF (r, g, b, 1.0f));
                m_tracerBrush->SetOpacity (opacity);
                d2dContext->DrawText (str, 1, m_textFormat.Get(), charRect, m_tracerBrush.Get());
            }
            else
            {
                // Fully settled: white
                m_textBrush->SetOpacity (opacity);
                d2dContext->DrawText (str, 1, m_textFormat.Get(), charRect, m_textBrush.Get());
                m_textBrush->SetOpacity (1.0f);
            }
        }
    }

    d2dContext->EndDraw();
}





////////////////////////////////////////////////////////////////////////////////
//
//  UsageDialog::DrawCharacterGlow
//
//  Renders only the feathered dark glow halo for a single character.
//  The foreground text is drawn separately in a second pass so that
//  adjacent characters' glow layers cannot dim already-drawn text.
//
////////////////////////////////////////////////////////////////////////////////

void UsageDialog::DrawCharacterGlow (ID2D1DeviceContext * pContext, wchar_t ch, float x, float y, float opacity)
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
//  UsageDialog::WndProc
//
//  Window procedure — handles WM_KEYDOWN (Enter/Escape to dismiss),
//  WM_CLOSE, WM_DESTROY, close button (X).
//
////////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK UsageDialog::WndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    UsageDialog * pDialog = nullptr;

    if (msg == WM_CREATE)
    {
        CREATESTRUCTW * pCreate = reinterpret_cast<CREATESTRUCTW *> (lParam);
        pDialog = static_cast<UsageDialog *> (pCreate->lpCreateParams);
        SetWindowLongPtrW (hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR> (pDialog));

        return 0;
    }

    pDialog = reinterpret_cast<UsageDialog *> (GetWindowLongPtrW (hwnd, GWLP_USERDATA));

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
                    pDialog->m_tracerProgress = 0.0f;
                    pDialog->m_revealedFlags.assign (pDialog->m_characterPositions.size(), 0.0f);
                    pDialog->m_revealTimes.assign   (pDialog->m_characterPositions.size(), 0.0f);

                    // Regenerate per-line stagger delays for variety on each replay
                    std::mt19937 rng (static_cast<unsigned> (std::chrono::steady_clock::now().time_since_epoch().count()));
                    std::uniform_real_distribution<float> delayDist    (0.0f, kTracerStaggerRange);
                    std::uniform_real_distribution<float> durationDist (kTracerDurationMin, kTracerDurationMax);
                    std::uniform_real_distribution<float> trailDist    (kTracerTrailMin, kTracerTrailMax);

                    for (size_t j = 0; j < pDialog->m_lineTracerDelays.size(); j++)
                    {
                        pDialog->m_lineTracerDelays[j]    = delayDist (rng);
                        pDialog->m_lineTracerDurations[j] = durationDist (rng);
                        pDialog->m_lineTrailPixels[j]     = trailDist (rng);

                        float sweepSpan                   = (pDialog->m_textMaxX - pDialog->m_textMinX) + pDialog->m_lineTrailPixels[j];
                        pDialog->m_lineFadeDurations[j]   = (sweepSpan > 0.0f)
                                                            ? pDialog->m_lineTrailPixels[j] * pDialog->m_lineTracerDurations[j] / sweepSpan
                                                            : 1.0f;
                    }

                    // Reset AnimationSystem — no rain during reveal
                    if (pDialog->m_animationSystem)
                    {
                        pDialog->m_animationSystem->ClearAllStreaks();
                        pDialog->m_densityController->SetPercentage (0);
                        pDialog->m_animationSystem->SetAnimationSpeed (
                            std::min (100, static_cast<int> (pDialog->m_settings.m_animationSpeedPercent)));
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
