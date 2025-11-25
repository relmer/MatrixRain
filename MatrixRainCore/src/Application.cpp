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

namespace MatrixRain
{
    Application::Application()
        : m_hwnd(nullptr)
        , m_hInstance(nullptr)
        , m_isRunning(false)
        , m_isPaused(false)
        , m_inDisplayModeTransition(false)
    {
    }

    Application::~Application()
    {
        Shutdown();
    }

    bool Application::Initialize(HINSTANCE hInstance, int nCmdShow)
    {
        m_hInstance = hInstance;

        // Create window
        if (!CreateApplicationWindow(nCmdShow))
        {
            MessageBoxW(nullptr, L"Failed to create application window.", L"Error", MB_OK | MB_ICONERROR);
            return false;
        }

        // Initialize CharacterSet (singleton) - must be first
        CharacterSet& charSet = CharacterSet::GetInstance();
        if (!charSet.Initialize())
        {
            MessageBoxW(nullptr, L"Failed to initialize CharacterSet.", L"Error", MB_OK | MB_ICONERROR);
            return false;
        }

        // Initialize viewport with 75% of screen size
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        int windowWidth = static_cast<int>(screenWidth * 0.75f);
        int windowHeight = static_cast<int>(screenHeight * 0.75f);
        
        m_viewport = std::make_unique<Viewport>();
        m_viewport->Resize(static_cast<float>(windowWidth), static_cast<float>(windowHeight));

    // Initialize density controller (before AnimationSystem) with viewport reference
    m_densityController = std::make_unique<DensityController>(*m_viewport, 32.0f); // Character width matches horizontal spacing        // Initialize animation system with density controller
        m_animationSystem = std::make_unique<AnimationSystem>();
        m_animationSystem->Initialize(*m_viewport, *m_densityController);

        // Initialize render system
        m_renderSystem = std::make_unique<RenderSystem>();
        if (!m_renderSystem->Initialize(m_hwnd, windowWidth, windowHeight))
        {
            MessageBoxW(nullptr, L"Failed to initialize RenderSystem.\n\nCheck that DirectX 11 is available.", L"Error", MB_OK | MB_ICONERROR);
            return false;
        }

        // Create texture atlas now that we have a D3D11 device
        if (!charSet.CreateTextureAtlas(m_renderSystem->GetDevice()))
        {
            MessageBoxW(nullptr, L"Failed to create texture atlas.\n\nDirect2D/DirectWrite may not be available.", L"Error", MB_OK | MB_ICONERROR);
            return false;
        }

        // Initialize input system (after density controller)
        m_inputSystem = std::make_unique<InputSystem>();
        m_inputSystem->Initialize(*m_densityController);

        // Initialize application state
        m_appState = std::make_unique<ApplicationState>();
        m_appState->Initialize();

        // Initialize FPS counter
        m_fpsCounter = std::make_unique<FPSCounter>();

        // Initialize timer
        m_timer = std::make_unique<Timer>();

        m_isRunning = true;
        return true;
    }

    bool Application::CreateApplicationWindow(int nCmdShow)
    {
        // Verify HINSTANCE
        if (!m_hInstance)
        {
            MessageBoxW(nullptr, L"Invalid HINSTANCE - application instance handle is null.", 
                        L"Window Creation Error", MB_OK | MB_ICONERROR);
            return false;
        }

        // Register window class
        WNDCLASSEXW wcex = {};
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = Application::WindowProc;
        wcex.hInstance = m_hInstance;
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wcex.lpszClassName = L"MatrixRainWindowClass";

        if (!RegisterClassExW(&wcex))
        {
            // Check if class is already registered (ERROR_CLASS_ALREADY_EXISTS = 1410)
            DWORD error = GetLastError();
            if (error != ERROR_CLASS_ALREADY_EXISTS)
            {
                wchar_t errorMsg[256];
                swprintf_s(errorMsg, L"Failed to register window class. Error code: %lu", error);
                MessageBoxW(nullptr, errorMsg, L"Window Registration Error", MB_OK | MB_ICONERROR);
                return false;
            }
            // Class already registered - continue
        }

        // Center window on primary monitor with 75% of screen size
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        int windowWidth = static_cast<int>(screenWidth * 0.75f);
        int windowHeight = static_cast<int>(screenHeight * 0.75f);
        int posX = (screenWidth - windowWidth) / 2;
        int posY = (screenHeight - windowHeight) / 2;

        // Create window
        m_hwnd = CreateWindowExW(
            0,
            L"MatrixRainWindowClass",
            WINDOW_TITLE,
            WS_POPUP,  // Borderless window
            posX, posY,
            windowWidth,
            windowHeight,
            nullptr,
            nullptr,
            m_hInstance,
            this  // Pass 'this' pointer for retrieval in WindowProc
        );

        if (!m_hwnd)
        {
            DWORD error = GetLastError();
            wchar_t errorMsg[256];
            swprintf_s(errorMsg, L"Failed to create window. Error code: %lu", error);
            MessageBoxW(nullptr, errorMsg, L"Window Creation Error", MB_OK | MB_ICONERROR);
            return false;
        }

        ShowWindow(m_hwnd, nCmdShow);
        UpdateWindow(m_hwnd);

        // Debug: Verify window is visible
        if (IsWindowVisible(m_hwnd))
        {
            OutputDebugStringW(L"Window is visible\n");
        }

        return true;
    }

    int Application::Run()
    {
        MSG msg = {};

        while (m_isRunning)
        {
            // Process all pending messages
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                {
                    m_isRunning = false;
                    break;
                }

                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            if (!m_isRunning)
            {
                break;
            }

            // Calculate delta time
            float deltaTime = static_cast<float>(m_timer->GetElapsedSeconds());
            m_timer->Reset();

            // Skip updates/rendering during display mode transitions
            if (m_inDisplayModeTransition)
            {
                continue;
            }

            // Update FPS counter
            if (m_fpsCounter)
            {
                m_fpsCounter->Update(deltaTime);
            }

            // Update and render
            Update(deltaTime);
            Render();
        }

        return static_cast<int>(msg.wParam);
    }

    void Application::Update(float deltaTime)
    {
        if (m_animationSystem && !m_isPaused)
        {
            m_animationSystem->Update(deltaTime);
        }
    }

    void Application::Render()
    {
        if (m_renderSystem && m_animationSystem && m_viewport && m_appState)
        {
            float fps = m_fpsCounter ? m_fpsCounter->GetFPS() : 0.0f;
            ColorScheme scheme = m_appState->GetColorScheme();
            int rainPercentage = m_densityController ? m_densityController->GetPercentage() : 0;
            int streakCount = static_cast<int>(m_animationSystem->GetActiveStreakCount());
            m_renderSystem->Render(*m_animationSystem, *m_viewport, scheme, fps, rainPercentage, streakCount);
            m_renderSystem->Present();
        }
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
            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
        }

        if (m_hInstance)
        {
            UnregisterClassW(L"MatrixRainWindowClass", m_hInstance);
        }
    }

    LRESULT CALLBACK Application::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        Application* pApp = nullptr;

        if (uMsg == WM_NCCREATE)
        {
            // Retrieve 'this' pointer passed during CreateWindowEx
            CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            pApp = reinterpret_cast<Application*>(pCreate->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pApp));
            
            // Let default processing continue for WM_NCCREATE
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
        else
        {
            pApp = reinterpret_cast<Application*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }

        if (pApp)
        {
            return pApp->HandleMessage(uMsg, wParam, lParam);
        }

        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    LRESULT Application::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE)
            {
                // ESC key pressed - exit application
                PostQuitMessage(0);
                return 0;
            }
            else if (wParam == VK_SPACE)
            {
                // Spacebar pressed - toggle pause
                m_isPaused = !m_isPaused;
                return 0;
            }
            else if (wParam == 'C' && m_appState)
            {
                // C key pressed - cycle color scheme
                m_appState->CycleColorScheme();
                return 0;
            }
            else if (m_inputSystem)
            {
                // Process density control keys (+/- on both numpad and main keyboard)
                m_inputSystem->ProcessKeyDown(static_cast<int>(wParam));
                return 0;
            }
            break;

        case WM_SYSKEYDOWN:
            // Handle Alt+key combinations (Alt causes SYSKEYDOWN instead of KEYDOWN)
            if (m_inputSystem && m_inputSystem->IsAltEnterPressed(static_cast<int>(wParam)))
            {
                // Alt+Enter pressed - toggle display mode
                if (m_appState && m_renderSystem && m_viewport && m_animationSystem)
                {
                    m_inDisplayModeTransition = true;
                    m_appState->ToggleDisplayMode();
                    DisplayMode newMode = m_appState->GetDisplayMode();
                    
                    // Save old viewport width for rescaling streaks
                    float oldWidth = m_viewport->GetWidth();
                    
                    if (newMode == DisplayMode::Fullscreen)
                    {
                        // Switch to borderless fullscreen windowed mode
                        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
                        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
                        
                        SetWindowLongPtr(m_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
                        SetWindowPos(m_hwnd, HWND_TOP, 0, 0, screenWidth, screenHeight, 
                                     SWP_FRAMECHANGED | SWP_SHOWWINDOW);
                        
                        m_viewport->Resize(static_cast<float>(screenWidth), static_cast<float>(screenHeight));
                        m_renderSystem->Resize(screenWidth, screenHeight);
                        
                        // Rescale existing streaks to fill new viewport width
                        // Note: We don't spawn additional streaks here to avoid creating visible "waves"
                        // The density controller will naturally spawn new streaks to fill the space over time
                        m_animationSystem->RescaleStreaksForViewport(oldWidth, static_cast<float>(screenWidth));
                    }
                    else
                    {
                        // Return to windowed mode (75% of screen)
                        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
                        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
                        int windowWidth = static_cast<int>(screenWidth * 0.75f);
                        int windowHeight = static_cast<int>(screenHeight * 0.75f);
                        int posX = (screenWidth - windowWidth) / 2;
                        int posY = (screenHeight - windowHeight) / 2;
                        
                        SetWindowLongPtr(m_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
                        SetWindowPos(m_hwnd, HWND_TOP, posX, posY, windowWidth, windowHeight,
                                     SWP_FRAMECHANGED | SWP_SHOWWINDOW);
                        
                        m_viewport->Resize(static_cast<float>(windowWidth), static_cast<float>(windowHeight));
                        m_renderSystem->Resize(windowWidth, windowHeight);
                        
                        // Rescale existing streaks to fit new viewport width (no new spawning when shrinking)
                        m_animationSystem->RescaleStreaksForViewport(oldWidth, static_cast<float>(windowWidth));
                    }
                    m_inDisplayModeTransition = false;
                    
                    // Reset timer to prevent accumulated deltaTime from causing animation jump
                    if (m_timer)
                    {
                        m_timer->Reset();
                    }
                }
                return 0;  // Handled Alt+Enter
            }
            break;  // Let other Alt+key combinations pass through

        case WM_SIZE:
        {
            // Skip redundant resize if we're in the middle of a display mode transition
            if (m_inDisplayModeTransition)
            {
                return 0;
            }
            
            UINT width = LOWORD(lParam);
            UINT height = HIWORD(lParam);
            
            if (width > 0 && height > 0 && m_renderSystem && m_viewport)
            {
                m_renderSystem->Resize(width, height);
                m_viewport->Resize(static_cast<float>(width), static_cast<float>(height));
            }
            return 0;
        }

        case WM_NCHITTEST:
        {
            // Allow dragging the window by clicking anywhere in the client area
            LRESULT hit = DefWindowProc(m_hwnd, uMsg, wParam, lParam);
            if (hit == HTCLIENT)
            {
                return HTCAPTION;  // Treat client area as title bar for dragging
            }
            return hit;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
        }
        
        return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
    }
}
