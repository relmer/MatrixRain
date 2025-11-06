#include "pch.h"
#include "matrixrain/Application.h"
#include "matrixrain/AnimationSystem.h"
#include "matrixrain/RenderSystem.h"
#include "matrixrain/Viewport.h"
#include "matrixrain/Timer.h"
#include "matrixrain/CharacterSet.h"

namespace MatrixRain
{
    Application::Application()
        : m_hwnd(nullptr)
        , m_hInstance(nullptr)
        , m_isRunning(false)
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
            return false;
        }

        // Initialize CharacterSet (singleton) - must be first
        CharacterSet& charSet = CharacterSet::GetInstance();
        if (!charSet.Initialize())
        {
            return false;
        }

        // Initialize viewport
        m_viewport = std::make_unique<Viewport>();
        m_viewport->Resize(static_cast<float>(DEFAULT_WIDTH), static_cast<float>(DEFAULT_HEIGHT));

        // Initialize animation system
        m_animationSystem = std::make_unique<AnimationSystem>();
        m_animationSystem->Initialize(*m_viewport);

        // Initialize render system
        m_renderSystem = std::make_unique<RenderSystem>();
        if (!m_renderSystem->Initialize(m_hwnd, DEFAULT_WIDTH, DEFAULT_HEIGHT))
        {
            return false;
        }

        // Initialize timer
        m_timer = std::make_unique<Timer>();

        m_isRunning = true;
        return true;
    }

    bool Application::CreateApplicationWindow(int nCmdShow)
    {
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
            return false;
        }

        // Calculate window size to get desired client area
        RECT rc = { 0, 0, static_cast<LONG>(DEFAULT_WIDTH), static_cast<LONG>(DEFAULT_HEIGHT) };
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

        // Create window
        m_hwnd = CreateWindowExW(
            0,
            L"MatrixRainWindowClass",
            WINDOW_TITLE,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            rc.right - rc.left,
            rc.bottom - rc.top,
            nullptr,
            nullptr,
            m_hInstance,
            this  // Pass 'this' pointer for retrieval in WindowProc
        );

        if (!m_hwnd)
        {
            return false;
        }

        ShowWindow(m_hwnd, nCmdShow);
        UpdateWindow(m_hwnd);

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

            // Update and render
            Update(deltaTime);
            Render();
        }

        return static_cast<int>(msg.wParam);
    }

    void Application::Update(float deltaTime)
    {
        if (m_animationSystem)
        {
            m_animationSystem->Update(deltaTime);
        }
    }

    void Application::Render()
    {
        if (m_renderSystem && m_animationSystem && m_viewport)
        {
            m_renderSystem->Render(*m_animationSystem, *m_viewport);
            m_renderSystem->Present();
        }
    }

    void Application::Shutdown()
    {
        m_timer.reset();
        m_renderSystem.reset();
        m_animationSystem.reset();
        m_viewport.reset();

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
        case WM_SIZE:
        {
            UINT width = LOWORD(lParam);
            UINT height = HIWORD(lParam);
            
            if (width > 0 && height > 0 && m_renderSystem && m_viewport)
            {
                m_renderSystem->Resize(width, height);
                m_viewport->Resize(static_cast<float>(width), static_cast<float>(height));
            }
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
        }
    }
}
