#pragma once





class AnimationSystem;
class RenderSystem;
class Viewport;
class Timer;
class DensityController;
class InputSystem;
class ApplicationState;
class FPSCounter;
struct ScreenSaverModeContext;





class Application
{
public:
    Application();
    ~Application();

    // Main application lifecycle
    HRESULT Initialize (HINSTANCE hInstance, int nCmdShow, const ScreenSaverModeContext * pScreenSaverContext);
    int     Run();
    void    Shutdown();

    // Window dimensions
    static constexpr UINT            DEFAULT_WIDTH  = 1280;
    static constexpr UINT            DEFAULT_HEIGHT = 720;
    static constexpr const wchar_t * WINDOW_TITLE   = L"Matrix Rain";

private:
    // Core systems
    std::unique_ptr<Viewport>          m_viewport;
    std::unique_ptr<AnimationSystem>   m_animationSystem;
    std::unique_ptr<RenderSystem>      m_renderSystem;
    std::unique_ptr<Timer>             m_timer;
    std::unique_ptr<DensityController> m_densityController;
    std::unique_ptr<InputSystem>       m_inputSystem;
    std::unique_ptr<ApplicationState>  m_appState;
    std::unique_ptr<FPSCounter>        m_fpsCounter;

    // Win32 window
    HWND      m_hwnd                    { nullptr };
    HINSTANCE m_hInstance               { nullptr };
    bool      m_isRunning               { false   };
    bool      m_isPaused                { false   };
    bool      m_inDisplayModeTransition { false   };
    
    // Screensaver mode
    const ScreenSaverModeContext * m_pScreenSaverContext { nullptr };

    // Internal methods
    void    InitializeApplicationState    (const ScreenSaverModeContext * pScreenSaverContext);
    void    InitializeApplicationWindow();
    HRESULT CreateApplicationWindow       (POINT & position, SIZE & size);
    void    Update                        (float deltaTime);
    void    Render();
    void    GetWindowSizeForCurrentMode   (POINT & position, SIZE & size);
    void    ResizeWindowForCurrentMode();
    bool    ShouldExitScreenSaverOnKey    (WPARAM wParam);
    
    // Message handlers
    void OnKeyDown                  (WPARAM wParam);
    void OnSysKeyDown               (WPARAM wParam);
    void OnMouseMove                (LPARAM lParam);
    void OnSize                     (LPARAM lParam);
    void OnNcHitTest                (LRESULT & result);
    
    // Window procedure
    static LRESULT CALLBACK WindowProc    (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT                 HandleMessage (UINT uMsg, WPARAM wParam, LPARAM lParam);
};


