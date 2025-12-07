#include "pch.h"

#include "Application.h"
#include "AnimationSystem.h"
#include "ApplicationState.h"
#include "RenderSystem.h"
#include "Viewport.h"
#include "Timer.h"
#include "CharacterSet.h"
#include "DensityController.h"
#include "InputSystem.h"
#include "FPSCounter.h"
#include "ScreenSaverModeContext.h"





// Constructor must be defined in .cpp (not just declared in header) because Application
// contains unique_ptr members to forward-declared types (Pimpl idiom). The unique_ptr
// destructor requires complete type definitions, which are only available here in the .cpp
// where all headers are included. Using = default still gets compiler-generated behavior.
Application::Application() = default;





Application::~Application()
{
    Shutdown();
}





HRESULT Application::Initialize (HINSTANCE hInstance, int nCmdShow, const ScreenSaverModeContext * pScreenSaverContext)
{
    HRESULT        hr           = S_OK;
    CharacterSet & charSet      = CharacterSet::GetInstance();
    BOOL           fInitialized = FALSE;
    
    
    UNREFERENCED_PARAMETER (nCmdShow);


    m_hInstance         = hInstance;
    m_appState          = std::make_unique<ApplicationState>();
    m_viewport          = std::make_unique<Viewport>();
    m_densityController = std::make_unique<DensityController> (*m_viewport, 32.0f);  // Character width matches horizontal spacing
    m_animationSystem   = std::make_unique<AnimationSystem>();
    m_renderSystem      = std::make_unique<RenderSystem>();
    m_inputSystem       = std::make_unique<InputSystem>();
    m_fpsCounter        = std::make_unique<FPSCounter>();
    m_timer             = std::make_unique<Timer>();


    InitializeApplicationState (pScreenSaverContext);
    InitializeApplicationWindow();
    

    fInitialized = charSet.Initialize();
    CBR (fInitialized);

    m_animationSystem->Initialize (*m_viewport, *m_densityController);

    hr = charSet.CreateTextureAtlas (m_renderSystem->GetDevice());
    CHR (hr);


    m_isRunning = true;


    
Error:
    return hr;
}





void Application::InitializeApplicationState (const ScreenSaverModeContext * pScreenSaverContext)
{
    m_pScreenSaverContext = pScreenSaverContext;

    m_appState->Initialize (m_pScreenSaverContext);
    
    // Register for settings change notifications
    m_appState->RegisterDensityChangeCallback  ([this](int densityPercent) { m_densityController->SetPercentage   (densityPercent); });
    m_appState->RegisterAnimationSpeedCallback ([this](int speedPercent)   { m_animationSystem->SetAnimationSpeed (speedPercent);   });
    
    // Apply settings to controller
    m_densityController->SetPercentage   (m_appState->GetSettings().m_densityPercent);
    m_animationSystem->SetAnimationSpeed (m_appState->GetSettings().m_animationSpeedPercent);
    
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





void Application::InitializeApplicationWindow()
{
    HRESULT hr       = S_OK;
    POINT   position = { 0 };
    SIZE    size     = { 0 };



    GetWindowSizeForCurrentMode (position, size);

    hr = CreateApplicationWindow (position, size);
    CHR (hr);

    hr = m_renderSystem->Initialize (m_hwnd, size.cx, size.cy);
    CHR (hr);



Error:
    return;
}





HRESULT Application::CreateApplicationWindow (POINT & position, SIZE & size)
{
    HRESULT      hr        = S_OK;
    WNDCLASSEXW  wcex      = {};
    DWORD        error     = 0;
    ATOM         classAtom = 0;
    BOOL         fSuccess  = FALSE;
    
    
    
    // Verify HINSTANCE
    CBRLA (m_hInstance != nullptr, L"Invalid HINSTANCE - application instance handle is null.");

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

    // Create window (fullscreen borderless)
    m_hwnd = CreateWindowExW (0,
                              L"MatrixRainWindowClass",
                              WINDOW_TITLE,
                              WS_POPUP | WS_VISIBLE,  // Borderless window, visible
                              position.x, 
                              position.y,
                              size.cx,
                              size.cy,
                              nullptr,
                              nullptr,
                              m_hInstance,
                              this);  // Pass 'this' pointer for retrieval in WindowProc
    
    CWRA (m_hwnd != nullptr);

    fSuccess = ShowWindow (m_hwnd, SW_SHOW);
    CWRA (fSuccess);
    
    fSuccess = UpdateWindow (m_hwnd);
    CWRA (fSuccess);

    

Error:
    return hr;
}





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
}





void Application::GetWindowSizeForCurrentMode (POINT & position, SIZE & size)
{
    HRESULT hr           = S_OK;
    int     screenWidth  = GetSystemMetrics (SM_CXSCREEN);
    int     screenHeight = GetSystemMetrics (SM_CYSCREEN);



    CBRAEx (m_appState != nullptr, E_UNEXPECTED);

    if (m_appState->GetDisplayMode() == DisplayMode::Fullscreen)
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
        
        m_renderSystem->Render (*m_animationSystem, *m_viewport, scheme, fps, rainPercentage, streakCount, activeHeadCount, showDebugFadeTimes, elapsedTime);
        m_renderSystem->Present();
    }
}





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





void Application::OnKeyDown (WPARAM wParam)
{
    // If live overlay dialog is active, don't process exit keys
    if (m_hConfigDialog && (wParam == VK_ESCAPE || ShouldExitScreenSaverOnKey (wParam)))
    {
        return;
    }

    if (wParam == VK_ESCAPE || ShouldExitScreenSaverOnKey (wParam))
    {
        // ESC key pressed - exit application
        PostQuitMessage (0);
    }
    else if (wParam == VK_SPACE)
    {
        // Spacebar pressed - toggle pause
        m_isPaused = !m_isPaused;
    }
    else if (wParam == 'C' && m_appState)
    {
        // C key pressed - cycle color scheme
        m_appState->CycleColorScheme();
    }
    else if (wParam == 'S' && m_appState)
    {
        // S key pressed - toggle statistics display
        m_appState->ToggleStatistics();
    }
    else if (m_inputSystem)
    {
        // Process density control keys (+/- on both numpad and main keyboard)
        m_inputSystem->ProcessKeyDown (static_cast<int> (wParam));
    }
}





void Application::OnSysKeyDown (WPARAM wParam)
{
    // In screensaver mode with exit-on-input, suppress all hotkeys including Alt+Enter
    if (ShouldExitScreenSaverOnKey (wParam))
    {
        PostQuitMessage (0);
        return;  // Suppress Alt+Enter and other system keys when exit-on-input is enabled
    }
    
    // Handle Alt+key combinations (Alt causes SYSKEYDOWN instead of KEYDOWN)
    if (m_inputSystem && m_inputSystem->IsAltEnterPressed (static_cast<int> (wParam)))
    {
        // Alt+Enter pressed - toggle display mode
        if (m_appState && m_renderSystem && m_viewport && m_animationSystem)
        {
            m_inDisplayModeTransition = true;
            m_appState->ToggleDisplayMode();
            ResizeWindowForCurrentMode();
            m_inDisplayModeTransition = false;
        }
    }
}





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
            {
                std::lock_guard<std::mutex> lock (m_renderMutex);
                Update (deltaTime);
            }
            
            Render();
        }
        
        // Sleep to maintain target framerate
        std::this_thread::sleep_for (RENDER_FRAME_TIME);
    }
}
