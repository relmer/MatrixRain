#include "pch.h"

#include "MatrixRain/Application.h"
#include "MatrixRain/AnimationSystem.h"
#include "MatrixRain/ApplicationState.h"
#include "MatrixRain/RenderSystem.h"
#include "MatrixRain/Viewport.h"
#include "MatrixRain/Timer.h"
#include "MatrixRain/CharacterSet.h"
#include "MatrixRain/DensityController.h"
#include "MatrixRain/InputSystem.h"
#include "MatrixRain/FPSCounter.h"
#include "MatrixRain/ScreenSaverModeContext.h"





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
    int            screenWidth  = 0;
    int            screenHeight = 0;


    m_hInstance           = hInstance;
    m_pScreenSaverContext = pScreenSaverContext;

    // Create window
    hr = CreateApplicationWindow (nCmdShow);
    CHR (hr);

    // Initialize CharacterSet (singleton) - must be first
    fInitialized = charSet.Initialize();
    CBR (fInitialized);

    // Initialize viewport with full screen size (starting in fullscreen)
    screenWidth  = GetSystemMetrics (SM_CXSCREEN);
    screenHeight = GetSystemMetrics (SM_CYSCREEN);
    
    m_viewport = std::make_unique<Viewport>();
    m_viewport->Resize (static_cast<float> (screenWidth), static_cast<float> (screenHeight));

    // Initialize density controller (before AnimationSystem) with viewport reference
    m_densityController = std::make_unique<DensityController> (*m_viewport, 32.0f);  // Character width matches horizontal spacing
    
    // Initialize animation system with density controller
    m_animationSystem = std::make_unique<AnimationSystem>();
    m_animationSystem->Initialize (*m_viewport, *m_densityController);

    // Initialize render system
    m_renderSystem = std::make_unique<RenderSystem>();
    hr = m_renderSystem->Initialize (m_hwnd, screenWidth, screenHeight);
    CHR (hr);

    // Create texture atlas now that we have a D3D11 device
    hr = charSet.CreateTextureAtlas (m_renderSystem->GetDevice());
    CHR (hr);

    // Initialize input system (after density controller and app state)
    m_inputSystem = std::make_unique<InputSystem>();

    // Initialize application state
    m_appState = std::make_unique<ApplicationState>();
    m_appState->Initialize (pScreenSaverContext);

    // Now initialize input system with both dependencies
    m_inputSystem->Initialize (*m_densityController, *m_appState);

    // Initialize FPS counter
    m_fpsCounter = std::make_unique<FPSCounter>();

    // Initialize timer
    m_timer = std::make_unique<Timer>();
    
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

    m_isRunning = true;


    
Error:
    return hr;
}



HRESULT Application::CreateApplicationWindow (int nCmdShow)
{
    HRESULT      hr            = S_OK;
    WNDCLASSEXW  wcex          = {};
    DWORD        error         = 0;
    int          screenWidth   = 0;
    int          screenHeight  = 0;
    ATOM         classAtom     = 0;
    BOOL         fSuccess      = FALSE;
    
    
    UNREFERENCED_PARAMETER (nCmdShow);  // Starting in fullscreen, so nCmdShow is ignored
    
    
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
    wcex.hbrBackground  = (HBRUSH)GetStockObject (BLACK_BRUSH);
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

    // Start in fullscreen mode by default
    screenWidth  = GetSystemMetrics (SM_CXSCREEN);
    screenHeight = GetSystemMetrics (SM_CYSCREEN);

    // Create window (fullscreen borderless)
    m_hwnd = CreateWindowExW (0,
                              L"MatrixRainWindowClass",
                              WINDOW_TITLE,
                              WS_POPUP | WS_VISIBLE,  // Borderless window, visible
                              0, 0,
                              screenWidth,
                              screenHeight,
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

    while (m_isRunning)
    {
        // Process all pending messages
        while (PeekMessage (&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                m_isRunning = false;
                break;
            }

            TranslateMessage (&msg);
            DispatchMessage (&msg);
        }

        if (!m_isRunning)
        {
            break;
        }

        // Calculate delta time
        float deltaTime = static_cast<float> (m_timer->GetElapsedSeconds());
        m_timer->Reset();

        // Skip updates/rendering during display mode transitions
        if (m_inDisplayModeTransition)
        {
            continue;
        }

        // Update FPS counter
        if (m_fpsCounter)
        {
            m_fpsCounter->Update (deltaTime);
        }

        // Update and render
        Update (deltaTime);
        Render();
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
            DisplayMode newMode = m_appState->GetDisplayMode();
            
            // Save old viewport dimensions for rescaling streaks
            float oldWidth  = m_viewport->GetWidth();
            float oldHeight = m_viewport->GetHeight();
            
            if (newMode == DisplayMode::Fullscreen)
            {
                // Switch to borderless fullscreen windowed mode
                int screenWidth  = GetSystemMetrics (SM_CXSCREEN);
                int screenHeight = GetSystemMetrics (SM_CYSCREEN);
                
                SetWindowLongPtr (m_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
                SetWindowPos (m_hwnd, HWND_TOP, 0, 0, screenWidth, screenHeight, 
                                SWP_FRAMECHANGED | SWP_SHOWWINDOW);
                
                m_viewport->Resize (static_cast<float> (screenWidth), static_cast<float> (screenHeight));
                m_renderSystem->Resize (screenWidth, screenHeight);
                
                // Rescale existing streaks to fill new viewport dimensions
                m_animationSystem->RescaleStreaksForViewport (oldWidth, static_cast<float> (screenWidth), 
                                                                oldHeight, static_cast<float> (screenHeight));
            }
            else
            {
                // Return to windowed mode (75% of screen)
                int screenWidth  = GetSystemMetrics (SM_CXSCREEN);
                int screenHeight = GetSystemMetrics (SM_CYSCREEN);
                int windowWidth  = static_cast<int> (screenWidth * 0.75f);
                int windowHeight = static_cast<int> (screenHeight * 0.75f);
                int posX         = (screenWidth - windowWidth) / 2;
                int posY         = (screenHeight - windowHeight) / 2;
                
                SetWindowLongPtr (m_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
                SetWindowPos (m_hwnd, HWND_TOP, posX, posY, windowWidth, windowHeight,
                                SWP_FRAMECHANGED | SWP_SHOWWINDOW);
                
                m_viewport->Resize (static_cast<float> (windowWidth), static_cast<float> (windowHeight));
                m_renderSystem->Resize (windowWidth, windowHeight);
                
                // Rescale existing streaks to fit new viewport dimensions
                m_animationSystem->RescaleStreaksForViewport (oldWidth, static_cast<float> (windowWidth),
                                                                oldHeight, static_cast<float> (windowHeight));
            }
            m_inDisplayModeTransition = false;
            
            // Reset timer to prevent accumulated deltaTime from causing animation jump
            if (m_timer)
            {
                m_timer->Reset();
            }
        }
    }
}





void Application::OnMouseMove (LPARAM lParam)
{
    HRESULT hr         = S_OK;
    POINT   currentPos;



    // In screensaver mode with exit-on-input, check for meaningful mouse movement
    BAIL_OUT_IF (!m_pScreenSaverContext || !m_pScreenSaverContext->m_exitOnInput, S_OK);
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
