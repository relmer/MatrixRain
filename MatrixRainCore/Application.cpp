#include "pch.h"

#include "Application.h"
#include "AnimationSystem.h"
#include "ApplicationState.h"
#include "ColorScheme.h"
#include "Overlay.h"
#include "RenderSystem.h"
#include "UsageText.h"
#include "Viewport.h"
#include "Timer.h"
#include "CharacterSet.h"
#include "DensityController.h"
#include "InputSystem.h"
#include "FPSCounter.h"
#include "ScreenSaverModeContext.h"
#include "UnicodeSymbols.h"
#include "Version.h"





////////////////////////////////////////////////////////////////////////////////
//
//  Application::Application
//
//  Constructor must be defined in .cpp (not just declared in header) because
//  Application contains unique_ptr members to forward-declared types (Pimpl
//  idiom).  The unique_ptr destructor requires complete type definitions, which
//  are only available here in the .cpp where all headers are included.
//
////////////////////////////////////////////////////////////////////////////////

Application::Application() = default;





////////////////////////////////////////////////////////////////////////////////
//
//  Application::~Application
//
////////////////////////////////////////////////////////////////////////////////

Application::~Application()
{
    Shutdown();
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::Initialize
//
////////////////////////////////////////////////////////////////////////////////

HRESULT Application::Initialize (HINSTANCE hInstance, int nCmdShow, const ScreenSaverModeContext * pScreenSaverContext)
{
    HRESULT        hr           = S_OK;
    CharacterSet & charSet      = CharacterSet::GetInstance();
    BOOL           fInitialized = FALSE;
    
    
    UNREFERENCED_PARAMETER (nCmdShow);


    m_hInstance         = hInstance;
    m_appState          = std::make_unique<ApplicationState> (m_settingsProvider);
    m_viewport          = std::make_unique<Viewport>();
    m_densityController = std::make_unique<DensityController> (*m_viewport, 24.0f);  // Character width matches horizontal spacing
    m_animationSystem   = std::make_unique<AnimationSystem>();
    m_renderSystem      = std::make_unique<RenderSystem>();
    m_inputSystem       = std::make_unique<InputSystem>();
    m_fpsCounter        = std::make_unique<FPSCounter>();
    m_timer             = std::make_unique<Timer>();

    // Overlays are only used in Normal mode
    if (!pScreenSaverContext || pScreenSaverContext->m_mode == ScreenSaverMode::Normal)
    {
        m_helpOverlay = std::make_unique<Overlay> (
            std::vector<OverlayEntry> {
                { L"Settings", L"Enter" },
                { L"Help",     L"?" },
                { L"Exit",     L"Esc" }
            },
            OverlayTimingConfig { .revealDuration = 1.5f, .dismissDuration = 1.0f, .cycleInterval = 0.25f, .flashDuration = 1.0f, .holdDuration = 2.7f },
            OverlayLayoutConfig { .marginCols = 1, .gapChars = 6, .baseCharWidth = 16.0f, .baseRowHeight = 28.0f, .basePadding = 20.0f }
        );

        m_hotkeyOverlay = std::make_unique<Overlay> (
            std::vector<OverlayEntry> {
                { L"Space",       L"Pause / Resume" },
                { L"Enter",       L"Settings dialog" },
                { L"C",           L"Cycle color scheme" },
                { L"S",           L"Toggle statistics" },
                { L"?",           L"Help reference" },
                { L"+",           L"Increase rain density" },
                { L"-",           L"Decrease rain density" },
                { L"Alt+Enter",   L"Toggle fullscreen" },
                { L"Esc",         L"Exit" }
            },
            OverlayTimingConfig { .revealDuration = 1.5f, .dismissDuration = 1.0f, .cycleInterval = 0.25f, .flashDuration = 1.0f, .holdDuration = 5.4f },
            OverlayLayoutConfig { .marginCols = 2, .gapChars = 6, .baseCharWidth = 16.0f, .baseRowHeight = 28.0f, .basePadding = 30.0f }
        );
    }

    // Usage overlay for /? mode — two-column layout for switches, single-column for headers
    if (pScreenSaverContext && pScreenSaverContext->m_mode == ScreenSaverMode::HelpRequested)
    {
        wchar_t prefix = pScreenSaverContext->m_switchPrefix;

        std::vector<OverlayEntry> entries =
        {
            { std::format (L"MatrixRain v{} {} ({})", VERSION_WSTRING, VERSION_ARCH_WSTRING, std::wstring (VERSION_BUILD_TIMESTAMP)), L"" },
            { std::format (L"Copyright {} 2024-{} by Robert Elmer", UnicodeSymbols::Copyright, VERSION_YEAR_WSTRING), L"" },
            { L"", L"" },
            { std::format (L"Usage: MatrixRain.exe [{}option]", prefix), L"" },
            { L"", L"" },
            { L"Options:", L"" },
            { std::format (L"  {}c", prefix),  L"Show settings dialog" },
            { std::format (L"  {}?", prefix),  L"Display this help message" },
        };

        m_usageOverlay = std::make_unique<Overlay> (
            std::move (entries),
            OverlayTimingConfig { .revealDuration = 1.8f, .dismissDuration = 0.0f, .cycleInterval = 0.065f, .flashDuration = 1.0f, .holdDuration = -1.0f },
            OverlayLayoutConfig { .marginCols = 2, .gapChars = 6, .baseCharWidth = 16.0f, .baseRowHeight = 28.0f, .basePadding = 30.0f }
        );
    }


    InitializeApplicationState (pScreenSaverContext);
    
    hr = InitializeApplicationWindow();
    CHR (hr);
    

    fInitialized = charSet.Initialize();
    CBR (fInitialized);

    m_animationSystem->Initialize (*m_viewport, *m_densityController);

    hr = charSet.CreateTextureAtlas (m_renderSystem->GetDevice(), m_renderSystem->GetDpiScale());
    CHR (hr);


    m_isRunning = true;

    // Show help hint overlay on startup (null in screensaver mode)
    if (m_helpOverlay)
    {
        m_helpOverlay->Show();
    }

    // Start usage overlay in /? mode (no rain until reveal completes)
    if (m_usageOverlay)
    {
        float dpiScale = m_renderSystem->GetDpiScale();

        m_usageOverlay->SetDpiScale (dpiScale);
        m_usageOverlay->Show();
        m_densityController->SetPercentage (0);
        m_animationSystem->ClearAllStreaks();

        // Full-size rain characters (same physical size as fullscreen)
        m_renderSystem->SetCharacterScaleOverride (dpiScale);
        m_animationSystem->SetCharacterSpacingOverride (24.0f * dpiScale);

        // Force windowed mode (enables click-to-drag) and suppress stats
        m_appState->SetDisplayMode (DisplayMode::Windowed);
        m_appState->SetShowStatistics (false);
    }


    
Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::InitializeApplicationState
//
////////////////////////////////////////////////////////////////////////////////

void Application::InitializeApplicationState (const ScreenSaverModeContext * pScreenSaverContext)
{
    m_pScreenSaverContext = pScreenSaverContext;

    m_appState->Initialize (m_pScreenSaverContext);
    
    // Register for settings change notifications
    m_appState->RegisterDensityChangeCallback    ([this](int densityPercent)   { m_densityController->SetPercentage   (densityPercent);   });
    m_appState->RegisterAnimationSpeedCallback   ([this](int speedPercent)     { m_animationSystem->SetAnimationSpeed (speedPercent);     });
    m_appState->RegisterGlowIntensityCallback    ([this](int intensityPercent) { m_renderSystem->SetGlowIntensity     (intensityPercent); });
    m_appState->RegisterGlowSizeCallback         ([this](int sizePercent)      { m_renderSystem->SetGlowSize          (sizePercent);      });
    
    // Apply settings to controller
    m_densityController->SetPercentage   (m_appState->GetSettings().m_densityPercent);
    m_animationSystem->SetAnimationSpeed (m_appState->GetSettings().m_animationSpeedPercent);
    m_renderSystem->SetGlowIntensity     (m_appState->GetSettings().m_glowIntensityPercent);
    m_renderSystem->SetGlowSize          (m_appState->GetSettings().m_glowSizePercent);
    
    // Now initialize input system with both dependencies
    m_inputSystem->Initialize (*m_densityController, *m_appState);
    
    if (m_pScreenSaverContext)
    {
        // Hide cursor if in screensaver mode
        if (m_pScreenSaverContext->m_hideCursor)
        {
            ShowCursor (FALSE);
        }
    
        // Initialize exit state if in screensaver mode with exit-on-input
        if (m_pScreenSaverContext->m_exitOnInput)
        {
            m_inputSystem->InitializeExitState();
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::InitializeApplicationWindow
//
////////////////////////////////////////////////////////////////////////////////

HRESULT Application::InitializeApplicationWindow()
{
    HRESULT hr       = S_OK;
    POINT   position = {};
    SIZE    size     = {};



    // For preview mode, get dimensions from parent window
    if (m_pScreenSaverContext && 
        m_pScreenSaverContext->m_mode == ScreenSaverMode::ScreenSaverPreview &&
        m_pScreenSaverContext->m_previewParentHwnd != nullptr)
    {
        RECT rcParent { 0 };
        
        GetClientRect (m_pScreenSaverContext->m_previewParentHwnd, &rcParent);
     
        position.x = 0;
        position.y = 0;
        size.cx    = rcParent.right  - rcParent.left;
        size.cy    = rcParent.bottom - rcParent.top;
    }
    else
    {
        GetWindowSizeForCurrentMode (position, size);
    }

    hr = CreateApplicationWindow (position, size);
    CHR (hr);

    hr = m_renderSystem->Initialize (m_hwnd, size.cx, size.cy);
    CHR (hr);

    // Propagate the initial DPI scale to overlay classes so layout
    // computations use the correct scaling from the very first frame.
    {
        float dpiScale = m_renderSystem->GetDpiScale();

        if (m_helpOverlay) m_helpOverlay->SetDpiScale (dpiScale);
        if (m_hotkeyOverlay)   m_hotkeyOverlay->SetDpiScale (dpiScale);

        m_animationSystem->SetDpiScale (dpiScale);
    }



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::CreateApplicationWindow
//
////////////////////////////////////////////////////////////////////////////////

HRESULT Application::CreateApplicationWindow (POINT & position, SIZE & size)
{
    HRESULT      hr         = S_OK;
    WNDCLASSEXW  wcex       = { };
    DWORD        error      = 0;
    ATOM         classAtom  = 0;
    BOOL         fSuccess   = FALSE;
    DWORD        dwStyle    = WS_POPUP | WS_VISIBLE;
    HWND         hwndParent = nullptr;

    // Verify HINSTANCE
    CBRA (m_hInstance != nullptr);

    // Register window class
    wcex.cbSize         = sizeof (WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = Application::WindowProc;
    wcex.hInstance      = m_hInstance;
    wcex.hIcon          = LoadIconW (m_hInstance, MAKEINTRESOURCEW (101));  // IDI_MATRIXRAIN
    wcex.hIconSm        = LoadIconW (m_hInstance, MAKEINTRESOURCEW (101));  // IDI_MATRIXRAIN
    wcex.hCursor        = LoadCursor (nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH) GetStockObject (BLACK_BRUSH);
    wcex.lpszClassName  = L"MatrixRainWindowClass";

    classAtom = RegisterClassExW (&wcex);
    if (!classAtom)
    {
        // Check if class is already registered (ERROR_CLASS_ALREADY_EXISTS = 1410)
        error = GetLastError();
        if (error != ERROR_CLASS_ALREADY_EXISTS)
        {
            CWRA (FALSE);
        }
        // Class already registered - continue
    }

    // For preview mode, create as child window; otherwise create borderless popup
    if ((m_pScreenSaverContext && m_pScreenSaverContext->m_mode == ScreenSaverMode::ScreenSaverPreview))
    {
        dwStyle    = WS_CHILD | WS_VISIBLE;
        hwndParent = m_pScreenSaverContext->m_previewParentHwnd;
    }
    
    m_hwnd = CreateWindowExW (0,
                              L"MatrixRainWindowClass",
                              WINDOW_TITLE,
                              dwStyle,
                              position.x, 
                              position.y,
                              size.cx,
                              size.cy,
                              hwndParent,
                              nullptr,
                              m_hInstance,
                              this);  // Pass 'this' pointer for retrieval in WindowProc
    
    CWRA (m_hwnd != nullptr);

    // ShowWindow returns the previous visibility state (not success/failure).
    // A return of FALSE simply means the window was previously hidden, which is
    // expected for a newly created window.  Do not check with CWRA.
    ShowWindow (m_hwnd, SW_SHOW);
    
    fSuccess = UpdateWindow (m_hwnd);
    CWRA (fSuccess);

    

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::Run
//
////////////////////////////////////////////////////////////////////////////////

int Application::Run()
{
    MSG msg = {};

    // Start the render thread
    m_renderThread = std::thread (&Application::RenderThreadProc, this);

    // Main thread only processes messages
    while (m_isRunning)
    {
        // Wait for messages (no busy-wait)
        BOOL hasMessage = GetMessage (&msg, nullptr, 0, 0);
        
        if (hasMessage == 0)  // WM_QUIT
        {
            m_isRunning = false;
            break;
        }
        
        if (hasMessage == -1)  // Error
        {
            break;
        }

        // If live overlay dialog is active, let it handle dialog messages
        if (m_hConfigDialog && IsDialogMessage (m_hConfigDialog, &msg))
        {
            continue;
        }

        TranslateMessage (&msg);
        DispatchMessage (&msg);
    }

    return static_cast<int> (msg.wParam);
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::Update
//
////////////////////////////////////////////////////////////////////////////////

void Application::Update (float deltaTime)
{
    if (m_appState)
    {
        m_appState->Update (deltaTime);
    }
    
    if (m_animationSystem && !m_isPaused)
    {
        m_animationSystem->Update (deltaTime);
    }

    if ((m_helpOverlay && m_helpOverlay->IsActive()) ||
        (m_hotkeyOverlay  && m_hotkeyOverlay->IsActive())  ||
        (m_usageOverlay   && m_usageOverlay->IsActive()))
    {
        Color4 scheme = GetColorRGB (m_appState->GetColorScheme(), m_appState->GetElapsedTime());

        if (m_helpOverlay && m_helpOverlay->IsActive())
        {
            m_helpOverlay->Update (deltaTime, scheme.r, scheme.g, scheme.b);
        }

        if (m_hotkeyOverlay && m_hotkeyOverlay->IsActive())
        {
            m_hotkeyOverlay->Update (deltaTime, scheme.r, scheme.g, scheme.b);
        }

        if (m_usageOverlay && m_usageOverlay->IsActive())
        {
            bool wasRevealing = !m_usageOverlay->IsRevealComplete();

            m_usageOverlay->Update (deltaTime, scheme.r, scheme.g, scheme.b);

            // Start background rain once reveal completes
            if (wasRevealing && m_usageOverlay->IsRevealComplete() && m_densityController)
            {
                m_densityController->SetPercentage (8);
            }
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::GetWindowSizeForCurrentMode
//
////////////////////////////////////////////////////////////////////////////////

void Application::GetWindowSizeForCurrentMode (POINT & position, SIZE & size)
{
    HRESULT hr           = S_OK;
    int     screenWidth  = GetSystemMetrics (SM_CXSCREEN);
    int     screenHeight = GetSystemMetrics (SM_CYSCREEN);



    CBRAEx (m_appState != nullptr, E_UNEXPECTED);

    if (GetScreenSaverMode() == ScreenSaverMode::HelpRequested)
    {
        // /? mode: 60% x 50% of screen, centered
        int windowWidth  = screenWidth  * 60 / 100;
        int windowHeight = screenHeight * 50 / 100;

        position.x = (screenWidth  - windowWidth)  / 2;
        position.y = (screenHeight - windowHeight) / 2;
        size       = { windowWidth, windowHeight };
    }
    else if (m_appState->GetDisplayMode() == DisplayMode::Fullscreen)
    {
        // Fullscreen mode
        position = { 0, 0 };
        size     = { screenWidth, screenHeight };
    }
    else
    {
        // Windowed mode (75% of screen, centered)
        int windowWidth  = screenWidth  * 3 / 4;
        int windowHeight = screenHeight * 3 / 4;

        position.x = (screenWidth  - windowWidth)  / 2;
        position.y = (screenHeight - windowHeight) / 2;
        size       = { windowWidth, windowHeight };
    }



Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::ResizeWindowForCurrentMode
//
////////////////////////////////////////////////////////////////////////////////

void Application::ResizeWindowForCurrentMode()
{
    HRESULT hr         = S_OK;
    float   oldWidth   = { 0 };
    float   oldHeight  = { 0 };
    POINT   position   = { 0 };
    SIZE    windowSize = { 0 };
    

    
    CBRAEx (m_appState && m_viewport && m_renderSystem, E_UNEXPECTED);


    oldWidth  = m_viewport->GetWidth();
    oldHeight = m_viewport->GetHeight();

    GetWindowSizeForCurrentMode (position, windowSize);

    SetWindowLongPtr (m_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);


    SetWindowPos (m_hwnd, 
                  HWND_TOP, 
                  position.x, 
                  position.y, 
                  windowSize.cx, 
                  windowSize.cy, 
                  SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    
    m_viewport->Resize     (static_cast<float> (windowSize.cx), static_cast<float> (windowSize.cy));
    m_renderSystem->Resize (windowSize.cx, windowSize.cy);
                    
    // Rescale existing streaks to fill new viewport dimensions
    if (oldWidth > 0 && oldHeight > 0)
    {
        m_animationSystem->RescaleStreaksForViewport (oldWidth, 
                                                      oldHeight, 
                                                      static_cast<float> (windowSize.cx), 
                                                      static_cast<float> (windowSize.cy));
    }


    // Reset timer to prevent accumulated deltaTime from causing animation jump
    if (m_timer)
    {
        m_timer->Reset();
    }



Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::Render
//
////////////////////////////////////////////////////////////////////////////////

void Application::Render()
{
    if (m_renderSystem && m_animationSystem && m_viewport && m_appState)
    {
        // Only pass fps value if statistics are enabled
        float       fps                = (m_appState->GetShowStatistics() && m_fpsCounter) ? m_fpsCounter->GetFPS() : 0.0f;
        ColorScheme scheme             = m_appState->GetColorScheme();
        int         rainPercentage     = m_densityController ? m_densityController->GetPercentage() : 0;
        int         streakCount        = static_cast<int> (m_animationSystem->GetActiveStreakCount());
        int         activeHeadCount    = static_cast<int> (m_animationSystem->GetActiveHeadCount());
        bool        showDebugFadeTimes = m_appState->GetShowDebugFadeTimes();
        float       elapsedTime        = m_appState->GetElapsedTime();
        
        // Pass overlay pointers to render system for rendering
        const Overlay * pHelpOverlay    = (m_helpOverlay && m_helpOverlay->IsActive())       ? m_helpOverlay.get()    : nullptr;
        const Overlay * pHotkeyOverlay  = (m_hotkeyOverlay && m_hotkeyOverlay->IsActive())   ? m_hotkeyOverlay.get()  : nullptr;
        const Overlay * pUsageOverlay   = (m_usageOverlay && m_usageOverlay->IsActive())     ? m_usageOverlay.get()   : nullptr;

        RenderSystem::RenderParams renderParams =
        {
            .colorScheme        = scheme,
            .fps                = fps,
            .rainPercentage     = rainPercentage,
            .streakCount        = streakCount,
            .activeHeadCount    = activeHeadCount,
            .showDebugFadeTimes = showDebugFadeTimes,
            .elapsedTime        = elapsedTime,
            .pHelpOverlay       = pHelpOverlay,
            .pHotkeyOverlay     = pHotkeyOverlay,
            .pUsageOverlay      = pUsageOverlay
        };

        m_renderSystem->Render (*m_animationSystem, *m_viewport, renderParams);
        m_renderSystem->Present();
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::ShouldExitScreenSaverOnKey
//
////////////////////////////////////////////////////////////////////////////////

bool Application::ShouldExitScreenSaverOnKey (WPARAM wParam)
{
    HRESULT hr         = S_OK;
    bool    shouldExit = false;



    // In screensaver mode with exit-on-input, any key triggers exit

    BAIL_OUT_IF (!m_pScreenSaverContext || !m_pScreenSaverContext->m_exitOnInput, S_OK);
    BAIL_OUT_IF (!m_inputSystem, S_OK);

    m_inputSystem->ProcessKeyDown (static_cast<int> (wParam));

    shouldExit = m_inputSystem->ShouldExit();
        

Error:
    return shouldExit;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::Shutdown
//
////////////////////////////////////////////////////////////////////////////////

void Application::Shutdown()
{
    // Signal render thread to stop and wait for it
    if (m_renderThread.joinable())
    {
        m_renderThreadShouldStop = true;
        m_renderThread.join();
    }

    m_timer.reset();
    m_renderSystem.reset();
    m_animationSystem.reset();
    m_viewport.reset();
    m_inputSystem.reset();
    m_densityController.reset();

    if (m_hwnd)
    {
        DestroyWindow (m_hwnd);
        m_hwnd = nullptr;
    }

    if (m_hInstance)
    {
        UnregisterClassW (L"MatrixRainWindowClass", m_hInstance);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::WindowProc
//
////////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK Application::WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Application * pApp = nullptr;



    if (uMsg == WM_NCCREATE)
    {
        // Retrieve 'this' pointer passed during CreateWindowEx
        CREATESTRUCT * pCreate = reinterpret_cast<CREATESTRUCT *> (lParam);



        pApp = reinterpret_cast<Application *> (pCreate->lpCreateParams);
        SetWindowLongPtr (hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR> (pApp));
    }
    else
    {
        pApp = reinterpret_cast<Application *> (GetWindowLongPtr (hwnd, GWLP_USERDATA));

        if (pApp)
        {
            return pApp->HandleMessage (uMsg, wParam, lParam);
        }
    }

    return DefWindowProc (hwnd, uMsg, wParam, lParam);
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::HandleMessage
//
////////////////////////////////////////////////////////////////////////////////

LRESULT Application::HandleMessage (UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_KEYDOWN:
            OnKeyDown (wParam);
            return 0;

        case WM_SYSKEYDOWN:
            OnSysKeyDown (wParam);
            return 0;

        case WM_MOUSEMOVE:
            OnMouseMove (lParam);
            return 0;

        case WM_SIZE:
            OnSize (lParam);
            return 0;

        case WM_DPICHANGED:
            OnDpiChanged (wParam, lParam);
            return 0;

        case WM_NCHITTEST:
        {
            LRESULT result = DefWindowProc (m_hwnd, uMsg, wParam, lParam);
            OnNcHitTest (result);
            return result;
        }

        case WM_DESTROY:
            PostQuitMessage (0);
            return 0;

        default:
            return DefWindowProc (m_hwnd, uMsg, wParam, lParam);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::OnKeyDown
//
////////////////////////////////////////////////////////////////////////////////

void Application::OnKeyDown (WPARAM wParam)
{
    bool isRecognized  = false;
    bool isQuestionKey = false;


    // In /? help mode, only Enter and Escape exit the application
    if (GetScreenSaverMode() == ScreenSaverMode::HelpRequested)
    {
        if (wParam == VK_RETURN || wParam == VK_ESCAPE)
        {
            PostQuitMessage (0);
        }

        return;
    }



    // If live overlay dialog is active, don't process exit keys
    if (m_hConfigDialog && (wParam == VK_ESCAPE || ShouldExitScreenSaverOnKey (wParam)))
    {
        return;
    }

    // ShouldExitScreenSaverOnKey matches any key in screensaver mode, so it
    // can't be a switch case — check it first before the per-key dispatch.
    if (ShouldExitScreenSaverOnKey (wParam))
    {
        PostQuitMessage (0);
        return;
    }
    
    //
    // Determine if this key is a recognized hotkey
    //

    switch (wParam)
    {
        case VK_ESCAPE:
            // ESC key pressed - exit application
            PostQuitMessage (0);
            return;

        case VK_SPACE:
            // Spacebar pressed - toggle pause
            m_isPaused = !m_isPaused;
            isRecognized = true;
            break;

        case VK_RETURN:
            // Enter key — open config dialog as live overlay (guard against opening twice)
            if (!m_hConfigDialog && m_openConfigDialogCallback)
            {
                m_openConfigDialogCallback();
            }
            isRecognized = true;
            break;

        case VK_OEM_2:
            // ? key (Shift + /) — toggle hotkey overlay
            if (m_hotkeyOverlay && (GetKeyState (VK_SHIFT) & 0x8000))
            {
                isQuestionKey = true;
                isRecognized  = true;

                if (m_hotkeyOverlay->GetPhase() == OverlayPhase::Holding ||
                    m_hotkeyOverlay->GetPhase() == OverlayPhase::Revealing)
                {
                    m_hotkeyOverlay->Dismiss();
                }
                else
                {
                    m_hotkeyOverlay->Show();
                }
            }
            break;

        default:
            // All other keys — delegate to InputSystem
            if (m_inputSystem)
            {
                isRecognized = m_inputSystem->ProcessKeyDown (static_cast<int> (wParam));
            }
            break;
    }

    //
    // Overlay management: dismiss active overlays on any non-modifier key,
    // or show the help hint if no overlay is active and the key is unrecognized.
    //

    // Ignore standalone modifier keys entirely
    if (wParam == VK_SHIFT   || wParam == VK_LSHIFT   || wParam == VK_RSHIFT   ||
        wParam == VK_CONTROL || wParam == VK_LCONTROL || wParam == VK_RCONTROL ||
        wParam == VK_MENU    || wParam == VK_LMENU    || wParam == VK_RMENU    ||
        wParam == VK_LWIN    || wParam == VK_RWIN)
    {
        return;
    }

    if (m_helpOverlay)
    {
        if (m_helpOverlay->GetPhase() == OverlayPhase::Holding ||
            m_helpOverlay->GetPhase() == OverlayPhase::Revealing)
        {
            m_helpOverlay->Dismiss();
        }
        else if (!isRecognized)
        {
            // Unrecognized key with overlay hidden or dissolving — (re)show
            m_helpOverlay->Show();
        }
    }

    if (m_hotkeyOverlay && m_hotkeyOverlay->IsActive() && !isQuestionKey)
    {
        m_hotkeyOverlay->Dismiss();
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::OnSysKeyDown
//
////////////////////////////////////////////////////////////////////////////////

void Application::OnSysKeyDown (WPARAM wParam)
{
    // In /? help mode, suppress all system keys
    if (GetScreenSaverMode() == ScreenSaverMode::HelpRequested)
    {
        return;
    }

    // In screensaver mode with exit-on-input, suppress all hotkeys including Alt+Enter
    if (ShouldExitScreenSaverOnKey (wParam))
    {
        PostQuitMessage (0);
        return;  // Suppress Alt+Enter and other system keys when exit-on-input is enabled
    }
    
    // Handle Alt+key combinations (Alt causes SYSKEYDOWN instead of KEYDOWN)
    // WM_SYSKEYDOWN is only sent when Alt is held, so VK_RETURN here is Alt+Enter.
    if (wParam == VK_RETURN)
    {
        // Alt+Enter pressed - toggle display mode
        if (m_appState && m_renderSystem && m_viewport && m_animationSystem)
        {
            m_inDisplayModeTransition = true;
            m_appState->ToggleDisplayMode();
            ResizeWindowForCurrentMode();
            m_inDisplayModeTransition = false;
        }

        // Immediately hide help hint on Alt+Enter (no fade — viewport is changing)
        if (m_helpOverlay && m_helpOverlay->IsActive())
        {
            m_helpOverlay->Hide();
        }

        // Immediately hide hotkey overlay on Alt+Enter
        if (m_hotkeyOverlay && m_hotkeyOverlay->IsActive())
        {
            m_hotkeyOverlay->Hide();
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::OnMouseMove
//
////////////////////////////////////////////////////////////////////////////////

void Application::OnMouseMove (LPARAM lParam)
{
    HRESULT hr         = S_OK;
    POINT   currentPos;



    // In screensaver mode with exit-on-input, check for meaningful mouse movement
    // Skip exit detection if live overlay config dialog is active
    BAIL_OUT_IF (!m_pScreenSaverContext || !m_pScreenSaverContext->m_exitOnInput, S_OK);
    BAIL_OUT_IF (m_hConfigDialog != nullptr, S_OK);  // Dialog active - don't exit on mouse
    BAIL_OUT_IF (!m_inputSystem, S_OK);

    currentPos.x = static_cast<LONG> (static_cast<short> (LOWORD (lParam)));
    currentPos.y = static_cast<LONG> (static_cast<short> (HIWORD (lParam)));

    m_inputSystem->ProcessMouseMove (currentPos);
    if (m_inputSystem->ShouldExit())
    {
        PostQuitMessage (0);
    }


Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::OnSize
//
////////////////////////////////////////////////////////////////////////////////

void Application::OnSize (LPARAM lParam)
{
    // Skip redundant resize if we're in the middle of a display mode transition
    if (m_inDisplayModeTransition)
    {
        return;
    }

    UINT width  = LOWORD (lParam);
    UINT height = HIWORD (lParam);

    if (width > 0 && height > 0 && m_renderSystem && m_viewport)
    {
        m_renderSystem->Resize (width, height);
        m_viewport->Resize (static_cast<float> (width), static_cast<float> (height));


    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::OnDpiChanged
//
//  Handles WM_DPICHANGED — triggered when the window moves to a monitor
//  with a different DPI.  Resizes the window per the Windows-suggested rect
//  and propagates the new DPI to the render system and overlays.
//
////////////////////////////////////////////////////////////////////////////////

void Application::OnDpiChanged (WPARAM wParam, LPARAM lParam)
{
    UINT              newDpi   = HIWORD (wParam);
    float             dpiScale = static_cast<float> (newDpi) / 96.0f;
    const RECT      * pRect   = reinterpret_cast<const RECT *> (lParam);


    // Resize window to the rect Windows suggests for the new DPI
    if (pRect)
    {
        SetWindowPos (m_hwnd,
                      nullptr,
                      pRect->left,
                      pRect->top,
                      pRect->right  - pRect->left,
                      pRect->bottom - pRect->top,
                      SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // Propagate new DPI scale to the render system
    if (m_renderSystem)
    {
        m_renderSystem->OnDpiChanged (newDpi);
    }

    // Propagate new DPI scale to overlays
    if (m_helpOverlay)
    {
        m_helpOverlay->SetDpiScale (dpiScale);


    }

    if (m_hotkeyOverlay)
    {
        m_hotkeyOverlay->SetDpiScale (dpiScale);


    }

    // Propagate new DPI scale to animation system for character spacing
    if (m_animationSystem)
    {
        m_animationSystem->SetDpiScale (dpiScale);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::OnNcHitTest
//
////////////////////////////////////////////////////////////////////////////////

void Application::OnNcHitTest (LRESULT & result)
{
    // Allow dragging the window by clicking anywhere in the client area
    // but only in windowed mode (not in fullscreen)
    if (result == HTCLIENT && m_appState && m_appState->GetDisplayMode() == DisplayMode::Windowed)
    {
        result = HTCAPTION;  // Treat client area as title bar for dragging
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::RenderThreadProc
//
//  Dedicated render thread that runs independently of the message loop.
//  Maintains fixed 60 FPS animation even during modal operations (dialog drag,
//  resize, menus, etc.).
//
////////////////////////////////////////////////////////////////////////////////

void Application::RenderThreadProc()
{
    using namespace std::chrono;
    
    auto lastFrameTime = steady_clock::now();
    
    while (!m_renderThreadShouldStop)
    {
        auto currentTime = steady_clock::now();
        auto deltaTime   = duration_cast<duration<float>> (currentTime - lastFrameTime).count();
        lastFrameTime    = currentTime;
        
        // Skip updates/rendering during display mode transitions
        if (!m_inDisplayModeTransition)
        {
            // Update FPS counter
            if (m_fpsCounter)
            {
                m_fpsCounter->Update (deltaTime);
            }
            
            // Update and render (with mutex protection for config changes)
            // Mutex must cover both Update and Render to prevent the UI thread from
            // modifying shared state (e.g., streak vectors) while Render reads them.
            {
                std::lock_guard<std::mutex> lock (m_renderMutex);
                Update (deltaTime);
                Render();
            }
        }
        
        // Sleep to maintain target framerate
        std::this_thread::sleep_for (RENDER_FRAME_TIME);
    }
}
