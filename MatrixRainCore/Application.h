#pragma once

#include "RebuildCoalescer.h"
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
class IMonitorProvider;



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
    void                ApplyDisplayModeChange()                   { PostMessageW (m_hwnd, WM_APP_REBUILD_CONTEXTS, 0, 0); }

    // v1.5 (T032, FR-010): the property-sheet 1 Hz title timer needs the
    // primary monitor's MonitorRenderContext to read its published FPS.
    // Returns nullptr during the brief window in which a multimon rebuild
    // is in flight (the timer falls back to "0 fps" for that tick — see
    // contracts/fps-publisher.md).
    MonitorRenderContext * GetPrimaryRenderContext() const { return m_primary; }

    // Window dimensions
    static constexpr UINT            DEFAULT_WIDTH  = 1280;
    static constexpr UINT            DEFAULT_HEIGHT = 720;
    static constexpr const wchar_t * WINDOW_TITLE   = L"Matrix Rain";

    // Posted to the primary window to rebuild contexts outside any dialog proc
    static constexpr UINT            WM_APP_REBUILD_CONTEXTS = WM_APP + 1;

    // Posted by per-monitor render threads when Present returns a device-lost
    // HRESULT.  The HandleMessage handler routes the request through
    // m_rebuildCoalescer so an N-monitor burst (driver reset, sleep/resume)
    // collapses to a single subsequent WM_APP_REBUILD_CONTEXTS.
    static constexpr UINT            WM_APP_DEVICE_LOST      = WM_APP + 2;

private:
    // Core systems
    RegistrySettingsProvider                           m_settingsProvider;
    std::unique_ptr<IMonitorProvider>                  m_monitorProvider;
    std::vector<std::unique_ptr<MonitorRenderContext>> m_contexts;
    MonitorRenderContext *                             m_primary { nullptr };
    std::unique_ptr<InputSystem>                       m_inputSystem;
    std::unique_ptr<ApplicationState>                  m_appState;
    OverlayState                                       m_overlays;
    RebuildCoalescer                                   m_rebuildCoalescer;
    std::optional<LUID>                                m_resolvedAdapter;

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
    HRESULT CreateRenderContexts();
    HRESULT CreateSingleContext();
    HRESULT CreateFullscreenContexts();
    bool    ShouldSpanAllMonitors()       const;
    HRESULT AddContext                    (const POINT & position, const SIZE & size, DWORD dwStyle, HWND hwndParent, bool isPrimary);
    HRESULT CreateWindowAtBounds          (const POINT & position, const SIZE & size, DWORD dwStyle, HWND hwndParent, HWND & hwndOut);
    void    WirePrimaryContext();
    HRESULT InitializeContextResources();
    void    RebuildContextsForCurrentMode();
    void    StartRenderThreads();
    void    StopRenderThreads();
    MonitorRenderContext * ContextForHwnd (HWND hwnd) const;
    void    GetWindowSizeForCurrentMode   (POINT & position, SIZE & size);
    bool    ShouldExitScreenSaverOnKey    (WPARAM wParam);
    
    // Message handlers
    void OnKeyDown                  (WPARAM wParam);
    void OnSysKeyDown               (WPARAM wParam);
    void OnMouseMove                (LPARAM lParam);
    void OnSize                     (HWND hwnd, LPARAM lParam);
    void OnDpiChanged               (HWND hwnd, WPARAM wParam, LPARAM lParam);
    void OnNcHitTest                (LRESULT & result);
    
    // Window procedure
    static LRESULT CALLBACK WindowProc    (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT                 HandleMessage (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};


