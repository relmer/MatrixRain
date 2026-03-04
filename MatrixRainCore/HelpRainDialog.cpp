#include "pch.h"

#include "HelpRainDialog.h"

#include "UnicodeSymbols.h"
#include "Version.h"





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::HelpRainDialog
//
//  Pre-computes character positions and builds the randomized reveal queue.
//  Does NOT create the window or start animation — call Show() to begin.
//
////////////////////////////////////////////////////////////////////////////////

HelpRainDialog::HelpRainDialog (const UsageText & usageText) :
    m_usageText (usageText)
{
    // Cache the glyph set for random rain characters
    m_allGlyphs = CharacterConstants::GetAllCodepoints();

    // Pre-compute character positions using a temporary text layout
    InitializeTextLayout();
    ComputeCharacterPositions();
    BuildRevealQueue();

    // Compute reveal spawn rate: N characters over kRevealDurationSeconds
    if (!m_characterPositions.empty())
    {
        m_revealSpawnRate = static_cast<float> (m_characterPositions.size()) / kRevealDurationSeconds;
    }

    // Set decorative spawn rate to match reveal rate initially
    m_decorativeSpawnRate = m_revealSpawnRate;
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::~HelpRainDialog
//
////////////////////////////////////////////////////////////////////////////////

HelpRainDialog::~HelpRainDialog()
{
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
    for (bool flag : m_revealedFlags)
    {
        if (!flag)
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
//  Creates the dialog window, initializes D3D/D2D, and runs the message loop.
//  Blocking call — returns when dismissed.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT HelpRainDialog::Show ()
{
    HRESULT hr     = S_OK;
    MSG     msg    = {};
    auto    lastTime = std::chrono::high_resolution_clock::now();



    hr = CreateDialogWindow();
    CHR (hr);

    hr = CreateDeviceResources();
    CHR (hr);

    // Re-initialize text layout with the actual D2D resources
    InitializeTextLayout();
    ComputeCharacterPositions();
    BuildRevealQueue();

    // Recalculate spawn rate with final character count
    if (!m_characterPositions.empty())
    {
        m_revealSpawnRate   = static_cast<float> (m_characterPositions.size()) / kRevealDurationSeconds;
        m_decorativeSpawnRate = m_revealSpawnRate;
    }

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
//
////////////////////////////////////////////////////////////////////////////////

void HelpRainDialog::Update (float deltaTime)
{
    m_phaseTimer += deltaTime;

    if (m_animationPhase == AnimationPhase::Revealing)
    {
        UpdateRevealPhase (deltaTime);
    }
    else
    {
        UpdateBackgroundPhase (deltaTime);
    }

    AdvanceStreaks (deltaTime);
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::UpdateRevealPhase
//
//  Drains the reveal queue at the calibrated spawn rate, spawning reveal
//  streaks at each dequeued character's position.
//
////////////////////////////////////////////////////////////////////////////////

void HelpRainDialog::UpdateRevealPhase (float deltaTime)
{
    // Drain reveal queue at the calibrated rate
    m_spawnAccumulator += deltaTime * m_revealSpawnRate;

    while (m_spawnAccumulator >= 1.0f && m_revealQueueIndex < m_revealQueue.size())
    {
        size_t charIdx = m_revealQueue[m_revealQueueIndex];
        m_revealQueueIndex++;
        m_spawnAccumulator -= 1.0f;

        const CharPosition & pos = m_characterPositions[charIdx];

        RevealStreak streak;
        streak.targetCharIndex = charIdx;
        streak.pixelX          = pos.x + m_textOffsetX;
        streak.targetPixelY    = pos.y + m_textOffsetY;
        streak.headPixelY      = streak.targetPixelY - (kStreakLeadCells * kCellHeight);
        streak.speed           = kDefaultStreakSpeed;
        streak.leadCells       = kStreakLeadCells;
        streak.trailCells      = kStreakTrailCells;
        streak.revealed        = false;

        // Random glyphs for head and trail
        std::uniform_int_distribution<size_t> glyphDist (0, m_allGlyphs.size() - 1);

        for (int i = 0; i < streak.leadCells + streak.trailCells + 1; i++)
        {
            streak.glyphIndices.push_back (glyphDist (m_rng));
        }

        m_activeRevealStreaks.push_back (std::move (streak));
    }

    // Spawn decorative streaks
    m_decorativeSpawnAccumulator += deltaTime * m_decorativeSpawnRate;

    while (m_decorativeSpawnAccumulator >= 1.0f)
    {
        SpawnDecorativeStreak();
        m_decorativeSpawnAccumulator -= 1.0f;
    }

    // Check if reveal is complete (all flags true AND all reveal streaks done)
    if (m_revealQueueIndex >= m_revealQueue.size() && IsRevealComplete() && m_activeRevealStreaks.empty())
    {
        m_animationPhase = AnimationPhase::Background;
        m_phaseTimer     = 0.0f;

        // Reduce decorative density for Phase 2
        m_decorativeSpawnRate = m_revealSpawnRate * kPhase2DensityMultiplier;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::UpdateBackgroundPhase
//
//  Continues ambient decorative rain at reduced density/speed.
//
////////////////////////////////////////////////////////////////////////////////

void HelpRainDialog::UpdateBackgroundPhase (float deltaTime)
{
    // Continue spawning decorative streaks at reduced rate
    m_decorativeSpawnAccumulator += deltaTime * m_decorativeSpawnRate;

    while (m_decorativeSpawnAccumulator >= 1.0f)
    {
        SpawnDecorativeStreak();
        m_decorativeSpawnAccumulator -= 1.0f;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::SpawnDecorativeStreak
//
//  Creates a new decorative streak at a random pixel x-position.
//
////////////////////////////////////////////////////////////////////////////////

void HelpRainDialog::SpawnDecorativeStreak ()
{
    int width = (m_windowWidth > 0) ? m_windowWidth : 800;

    std::uniform_real_distribution<float> xDist (0.0f, static_cast<float> (width));
    std::uniform_int_distribution<int>    trailDist (5, 10);
    std::uniform_int_distribution<size_t> glyphDist (0, m_allGlyphs.empty() ? 0 : m_allGlyphs.size() - 1);

    DecorativeStreak streak;
    streak.pixelX      = xDist (m_rng);
    streak.headPixelY  = -kCellHeight * 5.0f;
    streak.trailLength = trailDist (m_rng);
    streak.speed       = kDefaultStreakSpeed;

    if (m_animationPhase == AnimationPhase::Background)
    {
        streak.speed *= kPhase2SpeedMultiplier;
    }

    for (int i = 0; i < streak.trailLength + 1; i++)
    {
        streak.glyphIndices.push_back (glyphDist (m_rng));
    }

    m_decorativeStreaks.push_back (std::move (streak));
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::AdvanceStreaks
//
//  Moves all streaks downward, locks in revealed characters, removes
//  off-screen streaks.
//
////////////////////////////////////////////////////////////////////////////////

void HelpRainDialog::AdvanceStreaks (float deltaTime)
{
    int height = (m_windowHeight > 0) ? m_windowHeight : 600;

    // Advance reveal streaks
    for (auto & streak : m_activeRevealStreaks)
    {
        streak.headPixelY += streak.speed * deltaTime;

        // Lock in character when head reaches target
        if (!streak.revealed && streak.headPixelY >= streak.targetPixelY)
        {
            streak.revealed = true;
            m_revealedFlags[streak.targetCharIndex] = true;
        }

        // Cycle random glyphs
        if (!m_allGlyphs.empty())
        {
            std::uniform_int_distribution<size_t> glyphDist (0, m_allGlyphs.size() - 1);

            for (auto & idx : streak.glyphIndices)
            {
                idx = glyphDist (m_rng);
            }
        }
    }

    // Remove reveal streaks that have passed below the window
    std::erase_if (m_activeRevealStreaks, [height] (const RevealStreak & s)
    {
        return s.revealed && s.headPixelY > static_cast<float> (height) + s.trailCells * 24.0f;
    });

    // Advance decorative streaks
    for (auto & streak : m_decorativeStreaks)
    {
        streak.headPixelY += streak.speed * deltaTime;

        // Cycle random glyphs
        if (!m_allGlyphs.empty())
        {
            std::uniform_int_distribution<size_t> glyphDist (0, m_allGlyphs.size() - 1);

            for (auto & idx : streak.glyphIndices)
            {
                idx = glyphDist (m_rng);
            }
        }
    }

    // Remove decorative streaks that have passed below the window
    std::erase_if (m_decorativeStreaks, [height] (const DecorativeStreak & s)
    {
        return s.headPixelY > static_cast<float> (height) + s.trailLength * 24.0f;
    });
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
    m_revealedFlags.assign (m_characterPositions.size(), false);
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::BuildRevealQueue
//
//  Creates a randomized permutation of indices into characterPositions.
//
////////////////////////////////////////////////////////////////////////////////

void HelpRainDialog::BuildRevealQueue ()
{
    m_revealQueue.resize (m_characterPositions.size());

    for (size_t i = 0; i < m_revealQueue.size(); i++)
    {
        m_revealQueue[i] = i;
    }

    std::ranges::shuffle (m_revealQueue, m_rng);
    m_revealQueueIndex = 0;
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

    // Compute window size
    {
        UINT dpi = 96;

        clientSize = ComputeWindowSize (m_usageText, static_cast<float> (dpi));
    }

    // Adjust for window chrome
    windowRect.left   = 0;
    windowRect.top    = 0;
    windowRect.right  = clientSize.cx;
    windowRect.bottom = clientSize.cy;

    AdjustWindowRect (&windowRect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE);

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
//  Creates D3D11 device, swap chain, and D2D render target.
//  Minimal setup — no 3D geometry, no bloom.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT HelpRainDialog::CreateDeviceResources ()
{
    HRESULT              hr                = S_OK;
    UINT                 createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    D3D_FEATURE_LEVEL    featureLevels[]   = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL    featureLevel;
    ComPtr<IDXGIDevice>  dxgiDevice;
    ComPtr<IDXGIAdapter> dxgiAdapter;
    ComPtr<IDXGIFactory> dxgiFactory;
    ComPtr<IDXGISurface> dxgiSurface;



#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    // Create D3D11 device
    hr = D3D11CreateDevice (nullptr,
                            D3D_DRIVER_TYPE_HARDWARE,
                            nullptr,
                            createDeviceFlags,
                            featureLevels,
                            _countof (featureLevels),
                            D3D11_SDK_VERSION,
                            &m_device,
                            &featureLevel,
                            &m_deviceContext);

#ifdef _DEBUG
    if (hr == DXGI_ERROR_SDK_COMPONENT_MISSING)
    {
        // Retry without debug layer
        createDeviceFlags &= ~D3D11_CREATE_DEVICE_DEBUG;

        hr = D3D11CreateDevice (nullptr,
                                D3D_DRIVER_TYPE_HARDWARE,
                                nullptr,
                                createDeviceFlags,
                                featureLevels,
                                _countof (featureLevels),
                                D3D11_SDK_VERSION,
                                &m_device,
                                &featureLevel,
                                &m_deviceContext);
    }
#endif

    CHRA (hr);

    // Create swap chain
    {
        DXGI_SWAP_CHAIN_DESC swapChainDesc                  = {};
        swapChainDesc.BufferCount                            = 2;
        swapChainDesc.BufferDesc.Width                       = static_cast<UINT> (m_windowWidth);
        swapChainDesc.BufferDesc.Height                      = static_cast<UINT> (m_windowHeight);
        swapChainDesc.BufferDesc.Format                      = DXGI_FORMAT_B8G8R8A8_UNORM;
        swapChainDesc.BufferDesc.RefreshRate.Numerator        = 60;
        swapChainDesc.BufferDesc.RefreshRate.Denominator      = 1;
        swapChainDesc.BufferUsage                             = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.OutputWindow                            = m_hWnd;
        swapChainDesc.SampleDesc.Count                        = 1;
        swapChainDesc.SampleDesc.Quality                      = 0;
        swapChainDesc.Windowed                                = TRUE;
        swapChainDesc.SwapEffect                              = DXGI_SWAP_EFFECT_DISCARD;

        hr = m_device.As (&dxgiDevice);
        CHRA (hr);

        hr = dxgiDevice->GetAdapter (&dxgiAdapter);
        CHRA (hr);

        hr = dxgiAdapter->GetParent (__uuidof (IDXGIFactory), reinterpret_cast<void **> (dxgiFactory.GetAddressOf()));
        CHRA (hr);

        hr = dxgiFactory->CreateSwapChain (m_device.Get(), &swapChainDesc, &m_swapChain);
        CHRA (hr);
    }

    // Create D2D factory
    {
        D2D1_FACTORY_OPTIONS d2dOptions = {};

#ifdef _DEBUG
        d2dOptions.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

        hr = D2D1CreateFactory (D2D1_FACTORY_TYPE_SINGLE_THREADED,
                                __uuidof (ID2D1Factory1),
                                &d2dOptions,
                                reinterpret_cast<void **> (m_d2dFactory.GetAddressOf()));
        CHRA (hr);
    }

    // Create D2D render target from swap chain surface
    {
        hr = m_swapChain->GetBuffer (0, __uuidof (IDXGISurface), reinterpret_cast<void **> (dxgiSurface.GetAddressOf()));
        CHRA (hr);

        D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties (
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat (DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
        );

        hr = m_d2dFactory->CreateDxgiSurfaceRenderTarget (dxgiSurface.Get(), &props, &m_d2dRenderTarget);
        CHRA (hr);
    }

    // Create DirectWrite factory and text format
    {
        m_dwriteFactory.Reset();
        m_textFormat.Reset();

        hr = DWriteCreateFactory (DWRITE_FACTORY_TYPE_SHARED,
                                  __uuidof (IDWriteFactory),
                                  reinterpret_cast<IUnknown **> (m_dwriteFactory.GetAddressOf()));
        CHRA (hr);

        float fontSize = 15.0f;

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

    // Create brushes
    {
        // White text brush for resolved usage text
        hr = m_d2dRenderTarget->CreateSolidColorBrush (D2D1::ColorF (D2D1::ColorF::White), &m_textBrush);
        CHRA (hr);

        // Black glow brush (variable opacity)
        hr = m_d2dRenderTarget->CreateSolidColorBrush (D2D1::ColorF (D2D1::ColorF::Black, 0.0f), &m_glowBrush);
        CHRA (hr);

        // Bright green head brush
        hr = m_d2dRenderTarget->CreateSolidColorBrush (D2D1::ColorF (0.5f, 1.0f, 0.5f, 1.0f), &m_headBrush);
        CHRA (hr);

        // Dim green trail brush
        hr = m_d2dRenderTarget->CreateSolidColorBrush (D2D1::ColorF (0.0f, 0.7f, 0.0f, 0.8f), &m_trailBrush);
        CHRA (hr);
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::RenderFrame
//
//  Renders one frame: decorative rain → reveal streaks → resolved text.
//
////////////////////////////////////////////////////////////////////////////////

void HelpRainDialog::RenderFrame ()
{
    if (!m_d2dRenderTarget)
    {
        return;
    }

    m_d2dRenderTarget->BeginDraw();
    m_d2dRenderTarget->Clear (D2D1::ColorF (D2D1::ColorF::Black));

    RenderDecorativeStreaks();
    RenderRevealStreaks();
    RenderResolvedText();

    m_d2dRenderTarget->EndDraw();

    if (m_swapChain)
    {
        m_swapChain->Present (1, 0);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::RenderDecorativeStreaks
//
//  Draws all active decorative rain streaks.
//
////////////////////////////////////////////////////////////////////////////////

void HelpRainDialog::RenderDecorativeStreaks ()
{
    if (!m_trailBrush || !m_headBrush)
    {
        return;
    }

    for (const auto & streak : m_decorativeStreaks)
    {
        // Render trail characters
        for (int t = 0; t < streak.trailLength; t++)
        {
            float trailY   = streak.headPixelY - (t + 1) * kCellHeight;
            float opacity  = 1.0f - (static_cast<float> (t + 1) / static_cast<float> (streak.trailLength + 1));

            if (t < static_cast<int> (streak.glyphIndices.size()))
            {
                DrawRainGlyph (streak.glyphIndices[t], streak.pixelX, trailY, opacity * 0.6f, m_trailBrush.Get());
            }
        }

        // Render head character (bright)
        if (!streak.glyphIndices.empty())
        {
            DrawRainGlyph (streak.glyphIndices.back(), streak.pixelX, streak.headPixelY, 1.0f, m_headBrush.Get());
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::RenderRevealStreaks
//
//  Draws all active reveal streaks (Phase 1 only).
//
////////////////////////////////////////////////////////////////////////////////

void HelpRainDialog::RenderRevealStreaks ()
{
    if (!m_trailBrush || !m_headBrush)
    {
        return;
    }

    for (const auto & streak : m_activeRevealStreaks)
    {
        // Render lead characters above head
        for (int l = 1; l <= streak.leadCells; l++)
        {
            float leadY   = streak.headPixelY - l * kCellHeight;
            float opacity = 1.0f - (static_cast<float> (l) / static_cast<float> (streak.leadCells + 1));

            if (l < static_cast<int> (streak.glyphIndices.size()))
            {
                DrawRainGlyph (streak.glyphIndices[l], streak.pixelX, leadY, opacity * 0.5f, m_trailBrush.Get());
            }
        }

        // Render head (bright green/white)
        if (!streak.glyphIndices.empty())
        {
            DrawRainGlyph (streak.glyphIndices[0], streak.pixelX, streak.headPixelY, 1.0f, m_headBrush.Get());
        }

        // Render trail below head
        for (int t = 1; t <= streak.trailCells; t++)
        {
            float trailY  = streak.headPixelY + t * kCellHeight;
            float opacity = 1.0f - (static_cast<float> (t) / static_cast<float> (streak.trailCells + 1));

            size_t idx = static_cast<size_t> (streak.leadCells + t);

            if (idx < streak.glyphIndices.size())
            {
                DrawRainGlyph (streak.glyphIndices[idx], streak.pixelX, trailY, opacity * 0.4f, m_trailBrush.Get());
            }
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::RenderResolvedText
//
//  Draws all revealed characters with feathered dark glow for readability.
//
////////////////////////////////////////////////////////////////////////////////

void HelpRainDialog::RenderResolvedText ()
{
    if (!m_textBrush || !m_glowBrush)
    {
        return;
    }

    for (size_t i = 0; i < m_characterPositions.size(); i++)
    {
        if (!m_revealedFlags[i])
        {
            continue;
        }

        const CharPosition & pos = m_characterPositions[i];
        float drawX              = pos.x + m_textOffsetX;
        float drawY              = pos.y + m_textOffsetY;

        DrawCharacterWithGlow (pos.character, drawX, drawY, m_textBrush.Get());
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::DrawCharacterWithGlow
//
//  Renders a single character with a feathered dark glow effect for
//  readability against the rain background.
//
////////////////////////////////////////////////////////////////////////////////

void HelpRainDialog::DrawCharacterWithGlow (wchar_t ch, float x, float y, ID2D1SolidColorBrush * pBrush)
{
    if (!m_d2dRenderTarget || !m_textFormat || !m_glowBrush || !pBrush)
    {
        return;
    }

    wchar_t str[2] = { ch, L'\0' };

    D2D1_RECT_F charRect = D2D1::RectF (x, y, x + 20.0f, y + kCellHeight);

    // Draw feathered dark glow (same technique as DrawFeatheredGlow)
    const int glowLayers = 8;

    for (int i = glowLayers; i > 0; --i)
    {
        float offset  = static_cast<float> (i);
        float opacity = 0.9f - (static_cast<float> (i) / static_cast<float> (glowLayers));

        m_glowBrush->SetColor (D2D1::ColorF (D2D1::ColorF::Black, opacity));

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

                m_d2dRenderTarget->DrawText (str, 1, m_textFormat.Get(), glowRect, m_glowBrush.Get());
            }
        }
    }

    // Draw the actual character
    m_d2dRenderTarget->DrawText (str, 1, m_textFormat.Get(), charRect, pBrush);
}





////////////////////////////////////////////////////////////////////////////////
//
//  HelpRainDialog::DrawRainGlyph
//
//  Renders a single rain glyph at (x, y) with specified opacity.
//
////////////////////////////////////////////////////////////////////////////////

void HelpRainDialog::DrawRainGlyph (size_t glyphIndex, float x, float y, float opacity, ID2D1SolidColorBrush * pBrush)
{
    if (!m_d2dRenderTarget || !m_textFormat || !pBrush || m_allGlyphs.empty())
    {
        return;
    }

    if (glyphIndex >= m_allGlyphs.size())
    {
        return;
    }

    uint32_t codepoint = m_allGlyphs[glyphIndex];

    // Convert codepoint to wchar_t string
    wchar_t glyphStr[3] = {};
    int     glyphLen    = 0;

    if (codepoint <= 0xFFFF)
    {
        glyphStr[0] = static_cast<wchar_t> (codepoint);
        glyphLen    = 1;
    }
    else
    {
        glyphStr[0] = static_cast<wchar_t> (0xD800 + ((codepoint - 0x10000) >> 10));
        glyphStr[1] = static_cast<wchar_t> (0xDC00 + ((codepoint - 0x10000) & 0x3FF));
        glyphLen    = 2;
    }

    D2D1_RECT_F charRect = D2D1::RectF (x, y, x + 20.0f, y + kCellHeight);

    D2D1_COLOR_F origColor = pBrush->GetColor();
    D2D1_COLOR_F newColor  = origColor;
    newColor.a             = opacity;

    pBrush->SetColor (newColor);
    m_d2dRenderTarget->DrawText (glyphStr, static_cast<UINT32> (glyphLen), m_textFormat.Get(), charRect, pBrush);
    pBrush->SetColor (origColor);
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
