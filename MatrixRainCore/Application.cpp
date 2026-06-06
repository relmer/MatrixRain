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
#include "MonitorRenderContext.h"
#include "MonitorInfo.h"
#include "MonitorLayout.h"
#include "MultiMonitorGate.h"
#include "QualityPresets.h"
#include "RenderThreadInputs.h"
#include "WindowsAdapterProvider.h"
#include "WindowsMonitorProvider.h"
#include "AdapterSelection.h"
#include "ScreenSaverModeContext.h"
#include "UnicodeSymbols.h"
#include "Version.h"




//
//  Overlay timing and layout constants
//

static constexpr OverlayTimingConfig HELP_HINT_TIMING
{
    .revealDuration  = 1.5f,
    .dismissDuration = 1.0f,
    .cycleInterval   = 0.25f,
    .flashDuration   = 1.0f,
    .holdDuration    = 2.7f
};

static constexpr OverlayLayoutConfig HELP_HINT_LAYOUT
{
    .marginCols    = 1,
    .gapChars      = 6,
    .baseCharWidth = 16.0f,
    .baseRowHeight = 28.0f,
    .basePadding   = 20.0f
};

static constexpr OverlayTimingConfig HOTKEY_TIMING
{
    .revealDuration  = 1.5f,
    .dismissDuration = 1.0f,
    .cycleInterval   = 0.25f,
    .flashDuration   = 1.0f,
    .holdDuration    = 5.4f
};

static constexpr OverlayLayoutConfig HOTKEY_LAYOUT
{
    .marginCols    = 2,
    .gapChars      = 6,
    .baseCharWidth = 16.0f,
    .baseRowHeight = 28.0f,
    .basePadding   = 30.0f
};

static constexpr OverlayTimingConfig USAGE_TIMING
{
    .revealDuration  = 1.8f,
    .dismissDuration = 0.0f,
    .cycleInterval   = 0.065f,
    .flashDuration   = 1.0f,
    .holdDuration    = -1.0f
};

static constexpr OverlayLayoutConfig USAGE_LAYOUT
{
    .marginCols    = 2,
    .gapChars      = 6,
    .baseCharWidth = 16.0f,
    .baseRowHeight = 28.0f,
    .basePadding   = 30.0f
};





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


    m_hInstance       = hInstance;
    m_appState        = std::make_unique<ApplicationState> (m_settingsProvider);
    m_monitorProvider = std::make_unique<WindowsMonitorProvider>();
    m_inputSystem     = std::make_unique<InputSystem>();

    // Overlays are only used in Normal mode
    if (!pScreenSaverContext || pScreenSaverContext->m_mode == ScreenSaverMode::Normal)
    {
        m_overlays.helpOverlay = std::make_unique<Overlay> (HELP_HINT_TIMING, HELP_HINT_LAYOUT);

        hr = m_overlays.helpOverlay->Initialize (
            {
                { L"Settings", L"Enter" },
                { L"Help",     L"?" },
                { L"Exit",     L"Esc" }
            });
        CHR (hr);

        m_overlays.hotkeyOverlay = std::make_unique<Overlay> (HOTKEY_TIMING, HOTKEY_LAYOUT);

        hr = m_overlays.hotkeyOverlay->Initialize (
            {
                { L"Space",       L"Pause / Resume" },
                { L"Enter",       L"Settings dialog" },
                { L"C",           L"Cycle color scheme" },
                { L"S",           L"Toggle statistics" },
                { L"?",           L"Help reference" },
                { L"+",           L"Increase rain density" },
                { L"-",           L"Decrease rain density" },
                { L"Alt+Enter",   L"Toggle fullscreen" },
                { L"Esc",         L"Exit" }
            });
        CHR (hr);
    }

    // Usage overlay for /? mode — entries built from UsageText (single source of truth)
    if (pScreenSaverContext && pScreenSaverContext->m_mode == ScreenSaverMode::HelpRequested)
    {
        UsageText usageText (pScreenSaverContext->m_switchPrefix);

        auto overlayPairs = usageText.GetOverlayEntries();

        std::vector<OverlayEntry> entries;
        entries.reserve (overlayPairs.size());
        for (auto & [left, right] : overlayPairs)
        {
            entries.push_back ({ std::move (left), std::move (right) });
        }

        m_overlays.usageOverlay = std::make_unique<Overlay> (USAGE_TIMING, USAGE_LAYOUT);

        hr = m_overlays.usageOverlay->Initialize (std::move (entries));
        CHR (hr);
    }


    InitializeApplicationState (pScreenSaverContext);
    
    hr = CreateRenderContexts();
    CHR (hr);

    WirePrimaryContext();
    

    fInitialized = charSet.Initialize();
    CBR (fInitialized);

    hr = InitializeContextResources();
    CHR (hr);


    m_isRunning = true;

    // Show help hint overlay on startup (null in screensaver mode)
    if (m_overlays.helpOverlay)
    {
        m_overlays.helpOverlay->Show();
    }

    // Start usage overlay in /? mode with background rain
    if (m_overlays.usageOverlay)
    {
        float dpiScale = m_primary->GetDpiScale();

        m_overlays.usageOverlay->SetDpiScale (dpiScale);
        m_overlays.usageOverlay->Show();
        m_primary->Density().SetPercentage (8);

        // Full-size rain characters (same physical size as fullscreen)
        m_primary->Renderer().SetCharacterScaleOverride (dpiScale);
        m_primary->Animation().SetCharacterSpacingOverride (24.0f * dpiScale);

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

    // First-run quality preset heuristic (FR-037).  Runs only on a truly
    // fresh install (no registry key existed); upgrades from earlier
    // versions keep the High preset's defaults so the visual output is
    // identical to what they saw before (FR-022).
    if (m_appState->IsFirstRun())
    {
        std::vector<AdapterInfo>  adapters    = WindowsAdapterProvider{}.EnumerateAdapters();
        uint64_t                  totalPixels = 0;

        if (m_monitorProvider)
        {
            for (const MonitorInfo & monitor : m_monitorProvider->GetMonitors())
            {
                totalPixels += static_cast<uint64_t> (monitor.Width()) *
                               static_cast<uint64_t> (monitor.Height());
            }
        }

        QualityPreset firstRunPreset = PickDefaultQualityPreset (adapters, totalPixels);
        (void) m_appState->ApplyFirstRunQualityPreset (firstRunPreset);

        // Also seed SharedState so the first frame renders at the chosen
        // preset's values (the snapshot path picks up subsequent changes).
        {
            std::lock_guard<std::mutex> lock (m_sharedState.mutex);
            const ScreenSaverSettings & s = m_appState->GetSettings();

            m_sharedState.glowIntensityPercent   = s.m_advancedValues.m_glowIntensityPercent;
            m_sharedState.blurPasses             = s.m_advancedValues.m_blurPasses;
            m_sharedState.bloomResolutionDivisor = s.m_advancedValues.m_bloomResolutionDivisor;
            m_sharedState.blurTaps               = s.m_advancedValues.m_blurTaps;
        }
    }
    else
    {
        // Existing install: seed SharedState from the loaded advanced
        // values so the render thread renders at whatever preset/custom
        // values the user previously saved.
        std::lock_guard<std::mutex> lock (m_sharedState.mutex);
        const ScreenSaverSettings & s = m_appState->GetSettings();

        m_sharedState.blurPasses             = s.m_advancedValues.m_blurPasses;
        m_sharedState.bloomResolutionDivisor = s.m_advancedValues.m_bloomResolutionDivisor;
        m_sharedState.blurTaps               = s.m_advancedValues.m_blurTaps;
    }
    
    // Register for settings change notifications — write to SharedState
    m_appState->RegisterDensityChangeCallback ([this](int densityPercent) {
        std::lock_guard<std::mutex> lock (m_sharedState.mutex);
        m_sharedState.densityPercent = densityPercent;
    });

    m_appState->RegisterAnimationSpeedCallback ([this](int speedPercent) {
        std::lock_guard<std::mutex> lock (m_sharedState.mutex);
        m_sharedState.animationSpeedPercent = speedPercent;
    });
    
    m_appState->RegisterGlowIntensityCallback ([this](int intensityPercent) {
        std::lock_guard<std::mutex> lock (m_sharedState.mutex);
        m_sharedState.glowIntensityPercent = intensityPercent;
    });
    
    m_appState->RegisterGlowSizeCallback ([this](int sizePercent) {
        std::lock_guard<std::mutex> lock (m_sharedState.mutex);
        m_sharedState.glowSizePercent = sizePercent;
    });

    m_appState->RegisterAdvancedGraphicsCallback ([this](const AdvancedGraphicsValues & values) {
        std::lock_guard<std::mutex> lock (m_sharedState.mutex);
        m_sharedState.glowIntensityPercent   = values.m_glowIntensityPercent;
        m_sharedState.blurPasses             = values.m_blurPasses;
        m_sharedState.bloomResolutionDivisor = values.m_bloomResolutionDivisor;
        m_sharedState.blurTaps               = values.m_blurTaps;
    });

    m_appState->RegisterColorSchemeCallback ([this](ColorScheme scheme) {
        std::lock_guard<std::mutex> lock (m_sharedState.mutex);
        m_sharedState.colorScheme = scheme;
    });

    m_appState->RegisterShowStatisticsCallback ([this](bool show) {
        std::lock_guard<std::mutex> lock (m_sharedState.mutex);
        m_sharedState.showStatistics = show;
    });

    // v1.5 (T062, FR-016/021/028/034 live preview): bulk callback fired
    // from ApplicationState::ApplySettings.  Atomic-stores into the
    // SharedState.live* fields the render thread reads via GetSnapshot
    // each frame.  Lock-free path (atomics, not the SharedState mutex).
    m_appState->RegisterV15LiveCallback ([this](const ScreenSaverSettings & settings) {
        m_sharedState.liveGlowEnabled       .store (settings.m_glowEnabled,                                    std::memory_order_relaxed);
        m_sharedState.liveScanlinesEnabled  .store (settings.m_scanlinesEnabled,                               std::memory_order_relaxed);
        m_sharedState.liveScanlinesIntensity.store (settings.m_scanlinesIntensity,                             std::memory_order_relaxed);
        m_sharedState.liveScanlinesStyle    .store (settings.m_scanlinesStyle,                                 std::memory_order_relaxed);
        m_sharedState.liveCustomColor       .store (static_cast<DWORD> (settings.m_customColor),               std::memory_order_relaxed);
    });

    // Initialize SharedState from saved settings
    {
        const auto & settings = m_appState->GetSettings();

        m_sharedState.densityPercent        = settings.m_densityPercent;
        m_sharedState.colorScheme           = m_appState->GetColorScheme();
        m_sharedState.animationSpeedPercent = settings.m_animationSpeedPercent;
        m_sharedState.glowIntensityPercent  = settings.m_glowIntensityPercent;
        m_sharedState.glowSizePercent       = settings.m_glowSizePercent;
        m_sharedState.showStatistics        = m_appState->GetShowStatistics();

        // v1.5 (T062 bootstrap): seed the live atomics from persisted
        // settings so the very first frame uses the user's saved values
        // (otherwise the atomics keep their in-struct defaults until the
        // first ApplySettings call from the dialog).
        m_sharedState.liveGlowEnabled       .store (settings.m_glowEnabled,                              std::memory_order_relaxed);
        m_sharedState.liveScanlinesEnabled  .store (settings.m_scanlinesEnabled,                         std::memory_order_relaxed);
        m_sharedState.liveScanlinesIntensity.store (settings.m_scanlinesIntensity,                       std::memory_order_relaxed);
        m_sharedState.liveScanlinesStyle    .store (settings.m_scanlinesStyle,                           std::memory_order_relaxed);
        m_sharedState.liveCustomColor       .store (static_cast<DWORD> (settings.m_customColor),         std::memory_order_relaxed);
    }

    // Apply settings to subsystems and bind input once the primary render
    // context exists — see Application::WirePrimaryContext.
    
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
//  Application::CreateRenderContexts
//
//  Creates the window(s) and render context(s) for the current display mode.
//  Fullscreen (normal Alt+Enter or /s screensaver, excluding preview and /?)
//  fans out to one window per monitor; every other mode is a single window.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT Application::CreateRenderContexts()
{
    HRESULT hr = S_OK;


    // Resolve the user's preferred GPU adapter (description -> LUID) once
    // per (re)build so every per-monitor context is created on the same
    // device.  A saved adapter that is no longer present silently falls
    // back to the system default (FR-014); the resolved value is consumed
    // by AddContext when it constructs each MonitorRenderContext.
    {
        WindowsAdapterProvider   provider;
        std::vector<AdapterInfo> adapters = provider.EnumerateAdapters();
        std::wstring             saved    = m_appState ? m_appState->GetSettings().m_gpuAdapter : std::wstring();

        m_resolvedAdapter = ResolveAdapter (adapters, saved);
    }


    if (ShouldSpanAllMonitors())
    {
        hr = CreateFullscreenContexts();
        CHR (hr);
    }
    else
    {
        hr = CreateSingleContext();
        CHR (hr);
    }



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::ShouldSpanAllMonitors
//
//  Multimon applies only in the Fullscreen display mode.  Preview (/p) is a
//  single child window and the /? usage screen is a single windowed view, so
//  both are always forced single regardless of the saved display mode.
//
////////////////////////////////////////////////////////////////////////////////

bool Application::ShouldSpanAllMonitors() const
{
    std::optional<ScreenSaverMode> saverMode;

    if (m_pScreenSaverContext)
    {
        saverMode = m_pScreenSaverContext->m_mode;
    }


    bool         multiMonEnabled = m_appState && m_appState->GetSettings().m_multiMonitorEnabled;
    DisplayMode  displayMode     = m_appState ? m_appState->GetDisplayMode() : DisplayMode::Windowed;

    return ::ShouldSpanAllMonitors (multiMonEnabled, displayMode, saverMode);
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::CreateSingleContext
//
//  Creates a single window + render context: a child window for preview, or a
//  borderless popup sized by GetWindowSizeForCurrentMode for every other mode.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT Application::CreateSingleContext()
{
    HRESULT hr         = S_OK;
    POINT   position   = {};
    SIZE    size       = {};
    DWORD   dwStyle    = WS_POPUP | WS_VISIBLE;
    HWND    hwndParent = nullptr;



    // For preview mode, create a single child window sized to the parent
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

        dwStyle    = WS_CHILD | WS_VISIBLE;
        hwndParent = m_pScreenSaverContext->m_previewParentHwnd;
    }
    else
    {
        GetWindowSizeForCurrentMode (position, size);
    }

    hr = AddContext (position, size, dwStyle, hwndParent, true);
    CHR (hr);



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::CreateFullscreenContexts
//
//  Creates one borderless popup window + render context per connected monitor,
//  each placed at the monitor's virtual-screen bounds.  Degenerate monitors are
//  skipped; the primary is the first monitor flagged primary (else the first).
//  If no usable monitors are reported, falls back to a single primary window.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT Application::CreateFullscreenContexts()
{
    HRESULT                       hr         = S_OK;
    std::vector<MonitorPlacement> placements = PlanFullscreenPlacements (m_monitorProvider->GetMonitors());


    if (placements.empty())
    {
        // No usable topology reported — degrade gracefully to one window
        hr = CreateSingleContext();
        CHR (hr);
    }
    else
    {
        for (const MonitorPlacement & placement : placements)
        {
            hr = AddContext (placement.position, placement.size, WS_POPUP | WS_VISIBLE, nullptr, placement.isPrimary);
            CHR (hr);
        }
    }



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::AddContext
//
//  Creates one window, builds the matching render context, initializes its D3D
//  device/swap chain, and appends it to m_contexts.  The context is pushed only
//  AFTER it is fully initialized, so a stray early WM_SIZE/WM_DPICHANGED routed
//  by HWND finds either a ready context or none at all (never a half-built one).
//
////////////////////////////////////////////////////////////////////////////////

HRESULT Application::AddContext (const POINT & position, const SIZE & size, DWORD dwStyle, HWND hwndParent, bool isPrimary)
{
    HRESULT                               hr   = S_OK;
    HWND                                  hwnd = nullptr;
    std::unique_ptr<MonitorRenderContext> context;



    hr = CreateWindowAtBounds (position, size, dwStyle, hwndParent, hwnd);
    CHR (hr);

    context = std::make_unique<MonitorRenderContext> (isPrimary);

    hr = context->Initialize (hwnd, static_cast<UINT> (size.cx), static_cast<UINT> (size.cy), m_resolvedAdapter);
    CHR (hr);

    if (isPrimary)
    {
        m_primary = context.get();
        m_hwnd    = hwnd;
    }

    m_contexts.push_back (std::move (context));



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::CreateWindowAtBounds
//
//  Registers the (idempotent) window class and creates a single window at the
//  supplied bounds and style.  Every monitor window shares one WindowProc with
//  the Application* in GWLP_USERDATA.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT Application::CreateWindowAtBounds (const POINT & position, const SIZE & size, DWORD dwStyle, HWND hwndParent, HWND & hwndOut)
{
    HRESULT      hr        = S_OK;
    WNDCLASSEXW  wcex      = { };
    DWORD        error     = 0;
    ATOM         classAtom = 0;
    BOOL         fSuccess  = FALSE;
    HWND         hwnd      = nullptr;

    // Verify HINSTANCE
    CBRA (m_hInstance != nullptr);

    // Register window class (idempotent — shared by every monitor window)
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

    hwnd = CreateWindowExW (0,
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
    
    CWRA (hwnd != nullptr);

    // ShowWindow returns the previous visibility state (not success/failure).
    // A return of FALSE simply means the window was previously hidden, which is
    // expected for a newly created window.  Do not check with CWRA.
    ShowWindow (hwnd, SW_SHOW);
    
    fSuccess = UpdateWindow (hwnd);
    CWRA (fSuccess);

    hwndOut = hwnd;

    

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::WirePrimaryContext
//
//  Seeds the primary context's subsystems from saved settings, propagates the
//  primary monitor's DPI scale to the overlays, and binds the input system to
//  the primary context's density controller.  Must run after the contexts are
//  created and before any render thread starts (the subsystem accessors assert
//  the render thread is not yet running).
//
////////////////////////////////////////////////////////////////////////////////

void Application::WirePrimaryContext()
{
    const auto & settings = m_appState->GetSettings();


    m_primary->Density().SetPercentage       (settings.m_densityPercent);
    m_primary->Animation().SetAnimationSpeed (settings.m_animationSpeedPercent);
    m_primary->Renderer().SetGlowIntensity   (settings.m_glowIntensityPercent);
    m_primary->Renderer().SetGlowSize        (settings.m_glowSizePercent);

    // Propagate the initial DPI scale to overlay classes so layout computations
    // use the correct scaling from the very first frame.
    {
        float dpiScale = m_primary->GetDpiScale();

        if (m_overlays.helpOverlay)   m_overlays.helpOverlay->SetDpiScale (dpiScale);
        if (m_overlays.hotkeyOverlay) m_overlays.hotkeyOverlay->SetDpiScale (dpiScale);
    }

    m_inputSystem->Initialize (m_primary->Density(), *m_appState);
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::InitializeContextResources
//
//  Builds the CharacterSet-dependent per-context resources (animation wiring +
//  per-device glyph atlas).  Must run after CharacterSet::Initialize.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT Application::InitializeContextResources()
{
    HRESULT hr = S_OK;


    for (auto & context : m_contexts)
    {
        context->InitializeAnimation();

        hr = context->BuildGlyphAtlas();
        CHR (hr);
    }

    // Each device above rebuilt the shared CharacterSet overlay metrics (last
    // write wins).  Overlays render on the primary only, so rebuild the primary's
    // overlay atlas last to pin those shared metrics to the primary's DPI.
    if (m_primary != nullptr)
    {
        hr = m_primary->Renderer().RebuildOverlayAtlas();
        CHR (hr);
    }



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::ContextForHwnd
//
////////////////////////////////////////////////////////////////////////////////

MonitorRenderContext * Application::ContextForHwnd (HWND hwnd) const
{
    for (const auto & context : m_contexts)
    {
        if (context->Hwnd() == hwnd)
        {
            return context.get();
        }
    }

    return nullptr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::StartRenderThreads
//
//  Starts one render thread per context.  Only the primary context renders
//  overlays/statistics and advances the shared color-cycle clock; secondaries
//  receive a null OverlayState and a null primary-clock pointer.
//
////////////////////////////////////////////////////////////////////////////////

void Application::StartRenderThreads()
{
    for (auto & context : m_contexts)
    {
        RenderThreadInputs inputs = MakeRenderThreadInputs (context->IsPrimary(),
                                                            &m_sharedState,
                                                            &m_overlays,
                                                            m_appState.get(),
                                                            &m_inDisplayModeTransition);

        context->StartRenderThread (*inputs.sharedState, inputs.overlays, inputs.primaryClock, *inputs.inTransition);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::StopRenderThreads
//
//  Signals every render thread to stop, then joins them.  Stopping all before
//  joining lets the threads quiesce in parallel.
//
////////////////////////////////////////////////////////////////////////////////

void Application::StopRenderThreads()
{
    for (auto & context : m_contexts)
    {
        context->RequestStop();
    }

    for (auto & context : m_contexts)
    {
        context->Join();
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::Run
//
////////////////////////////////////////////////////////////////////////////////

int Application::Run()
{
    MSG msg = {};

    // Start one render thread per monitor context
    StartRenderThreads();

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
//  Application::RebuildContextsForCurrentMode
//
//  Barrier transition between windowed and per-monitor fullscreen (and back).
//  Quiesces every render thread, releases each context's D3D resources, destroys
//  the old windows, re-enumerates the desktop, recreates the window/context set
//  for the new mode, rebinds the primary, and restarts the render threads.  A
//  failed rebuild leaves no usable window, so it fails closed by posting quit.
//
////////////////////////////////////////////////////////////////////////////////

void Application::RebuildContextsForCurrentMode()
{
    HRESULT           hr = S_OK;
    std::vector<HWND> hwnds;


    // Close any live config dialog first — the rebuild destroys the primary
    // window that owns it, which would otherwise orphan the dialog.
    if (m_hConfigDialog)
    {
        DestroyWindow (m_hConfigDialog);
        m_hConfigDialog = nullptr;
    }


    m_inDisplayModeTransition = true;

    StopRenderThreads();

    for (auto & context : m_contexts)
    {
        hwnds.push_back (context->Hwnd());
    }

    m_contexts.clear();
    m_primary = nullptr;
    m_hwnd    = nullptr;

    for (HWND hwnd : hwnds)
    {
        if (hwnd)
        {
            DestroyWindow (hwnd);
        }
    }


    hr = CreateRenderContexts();

    if (SUCCEEDED (hr))
    {
        WirePrimaryContext();

        hr = InitializeContextResources();
    }

    if (SUCCEEDED (hr))
    {
        StartRenderThreads();
    }

    m_inDisplayModeTransition = false;

    if (FAILED (hr))
    {
        PostQuitMessage (1);
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
    // Stop and join every render thread, then release each context's D3D
    // resources BEFORE destroying the windows they observe.
    StopRenderThreads();

    std::vector<HWND> hwnds;

    for (auto & context : m_contexts)
    {
        hwnds.push_back (context->Hwnd());
    }

    m_inputSystem.reset();
    m_contexts.clear();
    m_primary = nullptr;

    for (HWND hwnd : hwnds)
    {
        if (hwnd)
        {
            DestroyWindow (hwnd);
        }
    }

    m_hwnd = nullptr;

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
            return pApp->HandleMessage (hwnd, uMsg, wParam, lParam);
        }
    }

    return DefWindowProc (hwnd, uMsg, wParam, lParam);
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::HandleMessage
//
////////////////////////////////////////////////////////////////////////////////

LRESULT Application::HandleMessage (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
            OnSize (hwnd, lParam);
            return 0;

        case WM_DPICHANGED:
            OnDpiChanged (hwnd, wParam, lParam);
            return 0;

        case WM_NCHITTEST:
        {
            LRESULT result = DefWindowProc (hwnd, uMsg, wParam, lParam);
            OnNcHitTest (result);
            return result;
        }

        case WM_DESTROY:
            // During a display-mode transition windows are destroyed and
            // rebuilt; only a genuine shutdown (outside a transition) quits.
            if (!m_inDisplayModeTransition)
            {
                PostQuitMessage (0);
            }
            return 0;

        case WM_APP_REBUILD_CONTEXTS:
            // Coalesced burst handler — clear the latch so any further
            // topology / device-loss notifications that arrive while we
            // are rebuilding can request a follow-up rebuild.
            m_rebuildCoalescer.Consume();
            RebuildContextsForCurrentMode();
            return 0;

        case WM_DISPLAYCHANGE:
            // Monitor topology changed (add, remove, resolution, primary
            // reassignment).  Windows broadcasts WM_DISPLAYCHANGE to every
            // top-level window we own, so coalesce to a single rebuild via
            // the latch; the rebuild itself is then driven by the
            // WM_APP_REBUILD_CONTEXTS case above.
            if (m_rebuildCoalescer.RequestRebuild() && m_hwnd)
            {
                PostMessageW (m_hwnd, WM_APP_REBUILD_CONTEXTS, 0, 0);
            }
            return 0;

        default:
            return DefWindowProc (hwnd, uMsg, wParam, lParam);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  Application::OnKeyDown
//
////////////////////////////////////////////////////////////////////////////////

void Application::OnKeyDown (WPARAM wParam)
{
    bool isRecognized     = false;
    bool isQuestionKey    = false;
    bool shouldExitSaver  = ShouldExitScreenSaverOnKey (wParam);


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
    if (m_hConfigDialog && (wParam == VK_ESCAPE || shouldExitSaver))
    {
        return;
    }

    // ShouldExitScreenSaverOnKey matches any key in screensaver mode, so it
    // can't be a switch case — check it first before the per-key dispatch.
    if (shouldExitSaver)
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
            // Spacebar pressed — toggle pause for every monitor via shared state
            {
                std::lock_guard<std::mutex> lock (m_sharedState.mutex);
                m_sharedState.isPaused = !m_sharedState.isPaused;
            }
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
            if (m_overlays.hotkeyOverlay && (GetKeyState (VK_SHIFT) & 0x8000))
            {
                isQuestionKey = true;
                isRecognized  = true;

                std::lock_guard<std::mutex> lock (m_overlays.mutex);

                if (m_overlays.hotkeyOverlay->GetPhase() == OverlayPhase::Holding ||
                    m_overlays.hotkeyOverlay->GetPhase() == OverlayPhase::Revealing)
                {
                    m_overlays.hotkeyOverlay->Dismiss();
                }
                else
                {
                    m_overlays.hotkeyOverlay->Show();
                }
            }
            break;

        default:
            // All other keys — delegate to InputSystem
            if (m_inputSystem)
            {
                isRecognized = m_inputSystem->ProcessKeyDown (static_cast<int> (wParam));

                // Sync SharedState with any changes made by ProcessKeyDown
                // (e.g., CycleColorScheme, ToggleStatistics, density +/-)
                if (isRecognized && m_appState)
                {
                    std::lock_guard<std::mutex> lock (m_sharedState.mutex);

                    m_sharedState.colorScheme      = m_appState->GetColorScheme();
                    m_sharedState.showStatistics   = m_appState->GetShowStatistics();
                }
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

    if (m_overlays.helpOverlay)
    {
        std::lock_guard<std::mutex> lock (m_overlays.mutex);

        if (m_overlays.helpOverlay->GetPhase() == OverlayPhase::Holding ||
            m_overlays.helpOverlay->GetPhase() == OverlayPhase::Revealing)
        {
            m_overlays.helpOverlay->Dismiss();
        }
        else if (!isRecognized)
        {
            // Unrecognized key with overlay hidden or dissolving — (re)show
            m_overlays.helpOverlay->Show();
        }
    }

    if (m_overlays.hotkeyOverlay && m_overlays.hotkeyOverlay->IsActive() && !isQuestionKey)
    {
        std::lock_guard<std::mutex> lock (m_overlays.mutex);
        m_overlays.hotkeyOverlay->Dismiss();
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
        if (m_appState && m_primary)
        {
            m_appState->ToggleDisplayMode();
            RebuildContextsForCurrentMode();
        }

        // Immediately hide overlays on Alt+Enter (no fade — viewport is changing)
        {
            std::lock_guard<std::mutex> lock (m_overlays.mutex);

            if (m_overlays.helpOverlay && m_overlays.helpOverlay->IsActive())
            {
                m_overlays.helpOverlay->Hide();
            }

            if (m_overlays.hotkeyOverlay && m_overlays.hotkeyOverlay->IsActive())
            {
                m_overlays.hotkeyOverlay->Hide();
            }
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

void Application::OnSize (HWND hwnd, LPARAM lParam)
{
    // Skip redundant resize if we're in the middle of a display mode transition
    if (m_inDisplayModeTransition)
    {
        return;
    }

    UINT                   width   = LOWORD (lParam);
    UINT                   height  = HIWORD (lParam);
    MonitorRenderContext * context = ContextForHwnd (hwnd);

    if (width > 0 && height > 0 && context)
    {
        context->Resize (width, height, false);
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

void Application::OnDpiChanged (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    UINT                   newDpi   = HIWORD (wParam);
    float                  dpiScale = static_cast<float> (newDpi) / 96.0f;
    const RECT *           pRect    = reinterpret_cast<const RECT *> (lParam);
    MonitorRenderContext * context  = ContextForHwnd (hwnd);



    // Resize window to the rect Windows suggests for the new DPI.  Only honored
    // in windowed mode — fullscreen windows stay pinned to their monitor bounds.
    if (pRect && m_appState && m_appState->GetDisplayMode() == DisplayMode::Windowed)
    {
        SetWindowPos (hwnd,
                      nullptr,
                      pRect->left,
                      pRect->top,
                      pRect->right  - pRect->left,
                      pRect->bottom - pRect->top,
                      SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // Propagate the new DPI to this monitor's render pipeline (render system,
    // animation system, and density controller) under its render lock.
    if (context)
    {
        context->OnDpiChanged (newDpi);
    }

    // Overlays live only on the primary window
    if (hwnd == m_hwnd)
    {
        if (m_overlays.helpOverlay)
        {
            m_overlays.helpOverlay->SetDpiScale (dpiScale);
        }

        if (m_overlays.hotkeyOverlay)
        {
            m_overlays.hotkeyOverlay->SetDpiScale (dpiScale);
        }
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