#pragma once

#include "RegistrySettingsProvider.h"
#include "ScreenSaverModeContext.h"
#include "SharedState.h"





class AnimationSystem;
class RenderSystem;
class Viewport;
class Timer;
class DensityController;
class InputSystem;
class ApplicationState;
class FPSCounter;
class Overlay;



////////////////////////////////////////////////////////////////////////////////
//
//  OverlayState — Thread-safe container for overlay objects
//
//  UI thread locks to call Show()/Dismiss()/Hide().
//  Render thread locks to call Update() and read characters for rendering.
//
////////////////////////////////////////////////////////////////////////////////

struct OverlayState
{
    mutable std::mutex mutex;

    std::unique_ptr<Overlay> helpOverlay;
    std::unique_ptr<Overlay> hotkeyOverlay;
    std::unique_ptr<Overlay> usageOverlay;
};





class Application
{
public:
    Application();
    ~Application();

    // Main application lifecycle
    HRESULT Initialize (HINSTANCE hInstance, int nCmdShow, const ScreenSaverModeContext * pScreenSaverContext);
    int     Run();
    void    Shutdown();

    // Accessors for live overlay configuration dialog
    HWND                GetMainWindowHwnd()   const                { return m_hwnd;                   }
    HINSTANCE           GetInstance()         const                { return m_hInstance;              }
    ApplicationState  * GetApplicationState() const                { return m_appState.get();         }
    ISettingsProvider & GetSettingsProvider()                      { return m_settingsProvider;       }
    void                SetConfigDialog       (HWND hConfigDialog) { m_hConfigDialog = hConfigDialog; }
    ScreenSaverMode     GetScreenSaverMode()  const                { return m_pScreenSaverContext ? m_pScreenSaverContext->m_mode : ScreenSaverMode::Normal; }
    
    void                SetOpenConfigDialogCallback (std::function<void()> callback) { m_openConfigDialogCallback = callback; }
    void                SetShowUsageDialogCallback (std::function<void()> callback)  { m_showUsageDialogCallback = callback; }
    void                ApplyDisplayModeChange()                   { ResizeWindowForCurrentMode();    }

    // Window dimensions
    static constexpr UINT            DEFAULT_WIDTH  = 1280;
    static constexpr UINT            DEFAULT_HEIGHT = 720;
    static constexpr const wchar_t * WINDOW_TITLE   = L"Matrix Rain";

private:
    // Core systems
    RegistrySettingsProvider           m_settingsProvider;
    std::unique_ptr<Viewport>          m_viewport;
    std::unique_ptr<AnimationSystem>   m_animationSystem;
    std::unique_ptr<RenderSystem>      m_renderSystem;
    std::unique_ptr<Timer>             m_timer;
    std::unique_ptr<DensityController> m_densityController;
    std::unique_ptr<InputSystem>       m_inputSystem;
    std::unique_ptr<ApplicationState>  m_appState;
    std::unique_ptr<FPSCounter>        m_fpsCounter;
    OverlayState                       m_overlays;

    // Win32 window
    HWND      m_hwnd                    { nullptr };
    HINSTANCE m_hInstance               { nullptr };
    bool      m_isRunning               { false   };
    std::atomic<bool> m_isPaused                { false };
    std::atomic<bool> m_inDisplayModeTransition { false };
    
    HWND      m_hConfigDialog           { nullptr };
    
    std::function<void()> m_openConfigDialogCallback;
    std::function<void()> m_showUsageDialogCallback;
    
    const ScreenSaverModeContext * m_pScreenSaverContext { nullptr };
    
    // Render thread
    static constexpr int RENDER_FPS = 60;
    static constexpr std::chrono::milliseconds RENDER_FRAME_TIME { 1000 / RENDER_FPS };
    
    std::thread       m_renderThread;
    std::atomic<bool> m_renderThreadShouldStop { false };
    SharedState       m_sharedState;

    // Internal methods
    void    InitializeApplicationState    (const ScreenSaverModeContext * pScreenSaverContext);
    HRESULT InitializeApplicationWindow();
    HRESULT CreateApplicationWindow       (POINT & position, SIZE & size);
    void    Update                        (float deltaTime);
    void    Render                        (const SharedState::Snapshot & snapshot);
    void    GetWindowSizeForCurrentMode   (POINT & position, SIZE & size);
    void    ResizeWindowForCurrentMode();
    bool    ShouldExitScreenSaverOnKey    (WPARAM wParam);
    
    void    RenderThreadProc();
    
    // Message handlers
    void OnKeyDown                  (WPARAM wParam);
    void OnSysKeyDown               (WPARAM wParam);
    void OnMouseMove                (LPARAM lParam);
    void OnSize                     (LPARAM lParam);
    void OnDpiChanged               (WPARAM wParam, LPARAM lParam);
    void OnNcHitTest                (LRESULT & result);
    
    // Window procedure
    static LRESULT CALLBACK WindowProc    (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT                 HandleMessage (UINT uMsg, WPARAM wParam, LPARAM lParam);
};


