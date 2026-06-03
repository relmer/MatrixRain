#include "pch.h"

#include "MonitorRenderContext.h"

#include "AnimationSystem.h"
#include "Application.h"
#include "ApplicationState.h"
#include "ColorScheme.h"
#include "DensityController.h"
#include "FPSCounter.h"
#include "Overlay.h"
#include "RenderParams.h"
#include "RenderSystem.h"
#include "Viewport.h"




////////////////////////////////////////////////////////////////////////////////
//
//  MonitorRenderContext::MonitorRenderContext
//
////////////////////////////////////////////////////////////////////////////////

MonitorRenderContext::MonitorRenderContext (bool isPrimary) :
    m_isPrimary (isPrimary)
{
    m_viewport          = std::make_unique<Viewport>();
    m_densityController = std::make_unique<DensityController> (*m_viewport, 16.0f);  // Base spacing; scaled by monitor DPI
    m_animationSystem   = std::make_unique<AnimationSystem>();
    m_renderSystem      = std::make_unique<RenderSystem>();
    m_fpsCounter        = std::make_unique<FPSCounter>();
}




////////////////////////////////////////////////////////////////////////////////
//
//  MonitorRenderContext::~MonitorRenderContext
//
////////////////////////////////////////////////////////////////////////////////

MonitorRenderContext::~MonitorRenderContext()
{
    RequestStop();
    Join();
}




////////////////////////////////////////////////////////////////////////////////
//
//  MonitorRenderContext::Initialize
//
//  Creates the D3D11 device + swap chain on the supplied HWND and seeds the
//  per-monitor DPI scale.  Runs on the UI thread before the render thread
//  starts, so the render thread becomes the exclusive owner of fully-built
//  subsystems.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT MonitorRenderContext::Initialize (HWND hwnd, UINT width, UINT height)
{
    HRESULT hr = S_OK;


    m_hwnd = hwnd;

    hr = m_renderSystem->Initialize (hwnd, width, height);
    CHR (hr);

    // Size the viewport to match the swap chain.  The WM_SIZE fired during
    // window creation is dropped (this context isn't registered yet), so the
    // initial dimensions must be seeded here or the animation spawns no streaks.
    m_viewport->Resize (static_cast<float> (width), static_cast<float> (height));

    {
        float dpiScale = m_renderSystem->GetDpiScale();

        m_animationSystem->SetDpiScale   (dpiScale);
        m_densityController->SetDpiScale (dpiScale);
    }


Error:
    return hr;
}




////////////////////////////////////////////////////////////////////////////////
//
//  MonitorRenderContext::InitializeAnimation
//
//  Wires the animation system to this context's viewport and density
//  controller.  Must run after CharacterSet::Initialize so the glyph layout is
//  available.
//
////////////////////////////////////////////////////////////////////////////////

void MonitorRenderContext::InitializeAnimation()
{
    m_animationSystem->Initialize (*m_viewport, *m_densityController);
}




////////////////////////////////////////////////////////////////////////////////
//
//  MonitorRenderContext::BuildGlyphAtlas
//
////////////////////////////////////////////////////////////////////////////////

HRESULT MonitorRenderContext::BuildGlyphAtlas()
{
    return m_renderSystem->BuildGlyphAtlas();
}




////////////////////////////////////////////////////////////////////////////////
//
//  MonitorRenderContext::StartRenderThread
//
////////////////////////////////////////////////////////////////////////////////

void MonitorRenderContext::StartRenderThread (SharedState      & sharedState,
                                              OverlayState     * overlays,
                                              ApplicationState * primaryClock,
                                              std::atomic<bool> & inTransition)
{
    m_sharedState  = &sharedState;
    m_overlays     = overlays;
    m_primaryClock = primaryClock;
    m_inTransition = &inTransition;
    m_shouldStop   = false;

    m_renderThread = std::thread (&MonitorRenderContext::RenderThreadProc, this);
}




////////////////////////////////////////////////////////////////////////////////
//
//  MonitorRenderContext::RequestStop
//
////////////////////////////////////////////////////////////////////////////////

void MonitorRenderContext::RequestStop()
{
    m_shouldStop = true;
}




////////////////////////////////////////////////////////////////////////////////
//
//  MonitorRenderContext::Join
//
////////////////////////////////////////////////////////////////////////////////

void MonitorRenderContext::Join()
{
    if (m_renderThread.joinable())
    {
        m_renderThread.join();
    }
}




////////////////////////////////////////////////////////////////////////////////
//
//  MonitorRenderContext::Resize
//
//  Resizes the viewport and swap chain to new client dimensions.  Serialized
//  against the render thread via the render mutex so swap-chain recreation
//  never races an in-flight frame.
//
////////////////////////////////////////////////////////////////////////////////

void MonitorRenderContext::Resize (UINT width, UINT height, bool rescaleStreaks)
{
    std::lock_guard<std::mutex> lock (m_renderMutex);

    float oldWidth  = m_viewport->GetWidth();
    float oldHeight = m_viewport->GetHeight();


    m_viewport->Resize     (static_cast<float> (width), static_cast<float> (height));
    m_renderSystem->Resize (width, height);

    // Rescale existing streaks to fill the new viewport dimensions
    if (rescaleStreaks && oldWidth > 0 && oldHeight > 0)
    {
        m_animationSystem->RescaleStreaksForViewport (oldWidth,
                                                      oldHeight,
                                                      static_cast<float> (width),
                                                      static_cast<float> (height));
    }
}




////////////////////////////////////////////////////////////////////////////////
//
//  MonitorRenderContext::OnDpiChanged
//
//  Propagates a new monitor DPI to the render system, animation system, and
//  density controller.  Serialized against the render thread via the render
//  mutex.
//
////////////////////////////////////////////////////////////////////////////////

void MonitorRenderContext::OnDpiChanged (UINT dpi)
{
    std::lock_guard<std::mutex> lock (m_renderMutex);

    float dpiScale = static_cast<float> (dpi) / 96.0f;


    m_renderSystem->OnDpiChanged     (dpi);
    m_animationSystem->SetDpiScale   (dpiScale);
    m_densityController->SetDpiScale (dpiScale);
}




////////////////////////////////////////////////////////////////////////////////
//
//  MonitorRenderContext::Renderer
//
////////////////////////////////////////////////////////////////////////////////

RenderSystem & MonitorRenderContext::Renderer()
{
    assert (!m_renderThread.joinable());

    return *m_renderSystem;
}




////////////////////////////////////////////////////////////////////////////////
//
//  MonitorRenderContext::Animation
//
////////////////////////////////////////////////////////////////////////////////

AnimationSystem & MonitorRenderContext::Animation()
{
    assert (!m_renderThread.joinable());

    return *m_animationSystem;
}




////////////////////////////////////////////////////////////////////////////////
//
//  MonitorRenderContext::Density
//
////////////////////////////////////////////////////////////////////////////////

DensityController & MonitorRenderContext::Density()
{
    assert (!m_renderThread.joinable());

    return *m_densityController;
}




////////////////////////////////////////////////////////////////////////////////
//
//  MonitorRenderContext::ViewportRef
//
////////////////////////////////////////////////////////////////////////////////

Viewport & MonitorRenderContext::ViewportRef()
{
    assert (!m_renderThread.joinable());

    return *m_viewport;
}




////////////////////////////////////////////////////////////////////////////////
//
//  MonitorRenderContext::GetDpiScale
//
////////////////////////////////////////////////////////////////////////////////

float MonitorRenderContext::GetDpiScale() const
{
    return m_renderSystem->GetDpiScale();
}




////////////////////////////////////////////////////////////////////////////////
//
//  MonitorRenderContext::RenderThreadProc
//
//  Dedicated render thread for this monitor.  Runs independently of the message
//  loop so animation stays smooth during modal operations (dialog drag, resize,
//  menus).  Paced by Present() VSync on this monitor.
//
////////////////////////////////////////////////////////////////////////////////

void MonitorRenderContext::RenderThreadProc()
{
    using namespace std::chrono;

    auto                  lastFrameTime = steady_clock::now();
    SharedState::Snapshot snapshot;


    while (!m_shouldStop)
    {
        auto  currentTime = steady_clock::now();
        float deltaTime   = duration_cast<duration<float>> (currentTime - lastFrameTime).count();
        lastFrameTime     = currentTime;

        // Clamp large deltas (e.g. after a window resize stalls the loop) so the
        // animation never jumps a long way in a single frame.
        if (deltaTime > 0.1f)
        {
            deltaTime = 0.1f;
        }

        // Skip rendering while a display-mode transition rebuilds the window
        if (m_inTransition && m_inTransition->load())
        {
            std::this_thread::sleep_for (milliseconds (1));
            continue;
        }


        std::lock_guard<std::mutex> renderLock (m_renderMutex);

        if (m_fpsCounter)
        {
            m_fpsCounter->Update (deltaTime);
        }

        // The primary context owns the shared color-cycle clock: advance it and
        // publish elapsedTime BEFORE snapshotting so this frame — and, in
        // multimon, every monitor — renders with the same elapsed time.
        if (m_primaryClock)
        {
            m_primaryClock->Update (deltaTime);

            std::lock_guard<std::mutex> lock (m_sharedState->mutex);
            m_sharedState->elapsedTime = m_primaryClock->GetElapsedTime();
        }

        // Snapshot shared state under lock, then push to subsystems.  Keeps the
        // lock hold time minimal while ensuring all subsystem writes happen on
        // the render thread.
        {
            std::lock_guard<std::mutex> lock (m_sharedState->mutex);
            snapshot = m_sharedState->GetSnapshot();
        }

        m_densityController->SetPercentage   (snapshot.densityPercent);
        m_animationSystem->SetAnimationSpeed (snapshot.animationSpeedPercent);
        m_renderSystem->SetGlowIntensity     (snapshot.glowIntensityPercent);
        m_renderSystem->SetGlowSize          (snapshot.glowSizePercent);

        // Update/Render hold the overlay lock (primary only); Present is kept
        // OUTSIDE it so the UI thread's Show/Dismiss is never blocked by VSync.
        {
            std::unique_lock<std::mutex> overlayLock;

            if (m_overlays)
            {
                overlayLock = std::unique_lock<std::mutex> (m_overlays->mutex);
            }

            Update (snapshot, deltaTime);
            Render (snapshot);
        }

        m_renderSystem->Present();
    }
}




////////////////////////////////////////////////////////////////////////////////
//
//  MonitorRenderContext::Update
//
////////////////////////////////////////////////////////////////////////////////

void MonitorRenderContext::Update (const SharedState::Snapshot & snapshot, float deltaTime)
{
    if (m_animationSystem && !snapshot.isPaused)
    {
        m_animationSystem->Update (deltaTime);
    }

    if (!m_overlays)
    {
        return;
    }


    OverlayState & overlays = *m_overlays;

    if ((overlays.helpOverlay   && overlays.helpOverlay->IsActive())   ||
        (overlays.hotkeyOverlay && overlays.hotkeyOverlay->IsActive()) ||
        (overlays.usageOverlay  && overlays.usageOverlay->IsActive()))
    {
        Color4 scheme = GetColorRGB (snapshot.colorScheme, snapshot.elapsedTime);

        if (overlays.helpOverlay && overlays.helpOverlay->IsActive())
        {
            overlays.helpOverlay->Update (deltaTime, scheme.r, scheme.g, scheme.b);
        }

        if (overlays.hotkeyOverlay && overlays.hotkeyOverlay->IsActive())
        {
            overlays.hotkeyOverlay->Update (deltaTime, scheme.r, scheme.g, scheme.b);
        }

        if (overlays.usageOverlay && overlays.usageOverlay->IsActive())
        {
            overlays.usageOverlay->Update (deltaTime, scheme.r, scheme.g, scheme.b);
        }
    }
}




////////////////////////////////////////////////////////////////////////////////
//
//  MonitorRenderContext::Render
//
////////////////////////////////////////////////////////////////////////////////

void MonitorRenderContext::Render (const SharedState::Snapshot & snapshot)
{
    if (!(m_renderSystem && m_animationSystem && m_viewport))
    {
        return;
    }


    // Only pass fps value if statistics are enabled
    float       fps                = (snapshot.showStatistics && m_fpsCounter) ? m_fpsCounter->GetFPS() : 0.0f;
    ColorScheme scheme             = snapshot.colorScheme;
    int         rainPercentage     = snapshot.densityPercent;
    int         streakCount        = static_cast<int> (m_animationSystem->GetActiveStreakCount());
    int         activeHeadCount    = static_cast<int> (m_animationSystem->GetActiveHeadCount());
    bool        showDebugFadeTimes = snapshot.showDebugFadeTimes;
    float       elapsedTime        = snapshot.elapsedTime;

    // Overlay pointers — only the primary context owns an OverlayState
    const Overlay * pHelpOverlay   = nullptr;
    const Overlay * pHotkeyOverlay = nullptr;
    const Overlay * pUsageOverlay  = nullptr;

    if (m_overlays)
    {
        pHelpOverlay   = (m_overlays->helpOverlay   && m_overlays->helpOverlay->IsActive())   ? m_overlays->helpOverlay.get()   : nullptr;
        pHotkeyOverlay = (m_overlays->hotkeyOverlay && m_overlays->hotkeyOverlay->IsActive()) ? m_overlays->hotkeyOverlay.get() : nullptr;
        pUsageOverlay  = (m_overlays->usageOverlay  && m_overlays->usageOverlay->IsActive())  ? m_overlays->usageOverlay.get()  : nullptr;
    }

    RenderParams renderParams =
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
}
