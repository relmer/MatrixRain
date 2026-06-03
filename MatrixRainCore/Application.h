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
class MonitorRenderContext;



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
    void                SetShowUsageDialogCallback  (std::function<void()> callback) { m_showUsageDialogCallback  = callback; }
    void                ApplyDisplayModeChange()                   { ResizeWindowForCurrentMode();    }

    // Window dimensions
    static constexpr UINT            DEFAULT_WIDTH  = 1280;
    static constexpr UINT            DEFAULT_HEIGHT = 720;
    static constexpr const wchar_t * WINDOW_TITLE   = L"Matrix Rain";

private:
    // Core systems
    RegistrySettingsProvider              m_settingsProvider;
    std::unique_ptr<MonitorRenderContext> m_primaryContext;
    std::unique_ptr<InputSystem>          m_inputSystem;
    std::unique_ptr<ApplicationState>     m_appState;
    OverlayState                          m_overlays;

    // Win32 window`
    HWND              m_hwnd                    { nullptr };
    HINSTANCE         m_hInstance               { nullptr };
    bool              m_isRunning               { false   };
    std::atomic<bool> m_inDisplayModeTransition { false   };
    
    HWND              m_hConfigDialog           { nullptr };
    
    std::function<void()> m_openConfigDialogCallback;
    std::function<void()> m_showUsageDialogCallback;
    
    const ScreenSaverModeContext * m_pScreenSaverContext { nullptr };
    
    SharedState       m_sharedState;

    // Internal methods
    void    InitializeApplicationState    (const ScreenSaverModeContext * pScreenSaverContext);
    HRESULT InitializeApplicationWindow();
    HRESULT CreateApplicationWindow       (POINT & position, SIZE & size);
    void    GetWindowSizeForCurrentMode   (POINT & position, SIZE & size);
    void    ResizeWindowForCurrentMode();
    bool    ShouldExitScreenSaverOnKey    (WPARAM wParam);
    
    // Message handlers
    void OnKeyDown                  (WPARAM wParam);
    void OnSysKeyDown               (WPARAM wParam);
    void OnMouseMove                (LPARAM lParam);
    void OnSize                     (LPARAM lParam);
    void OnDpiChanged               (WPARAM wParam, LPARAM lParam);
    void OnNcHitTest                (LRESULT & result);
    
    // Window procedure
    static LRESULT CALLBACK WindowProc    (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT                 HandleMessage (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};


