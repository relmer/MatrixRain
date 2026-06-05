#pragma once

#include "FrameLimiter.h"
#include "SharedState.h"




class Viewport;
class AnimationSystem;
class RenderSystem;
class DensityController;
class FPSCounter;
class ApplicationState;
struct OverlayState;




////////////////////////////////////////////////////////////////////////////////
//
//  MonitorRenderContext — Owns one monitor's render pipeline and render thread
//
//  Bundles the per-monitor render subsystems (RenderSystem with its own D3D11
//  device + swap chain, Viewport, AnimationSystem, DensityController,
//  FPSCounter) and the dedicated render thread that drives them.  The owning
//  Application creates and destroys the HWND on the UI thread; this context
//  only OBSERVES that HWND and never calls DestroyWindow.
//
//  Only the PRIMARY context renders overlays/statistics and advances the shared
//  color-cycle clock; secondary contexts receive a null OverlayState and a null
//  primary-clock pointer.
//
////////////////////////////////////////////////////////////////////////////////

class MonitorRenderContext
{
public:
    explicit MonitorRenderContext (bool isPrimary);
    ~MonitorRenderContext();

    // Construction — called on the UI thread before the render thread starts
    HRESULT Initialize         (HWND hwnd, UINT width, UINT height, std::optional<LUID> adapterLuid = std::nullopt);
    void    InitializeAnimation();
    HRESULT BuildGlyphAtlas();

    // Render-thread lifecycle
    void    StartRenderThread (SharedState      & sharedState,
                               OverlayState     * overlays,
                               ApplicationState * primaryClock,
                               std::atomic<bool> & inTransition);
    void    RequestStop();
    void    Join();

    // UI-thread window events — serialized against the render thread
    void    Resize       (UINT width, UINT height, bool rescaleStreaks);
    void    OnDpiChanged (UINT dpi);

    // Pre-thread-start accessors used by Application to wire up subsystems
    RenderSystem      & Renderer();
    AnimationSystem   & Animation();
    DensityController & Density();
    Viewport          & ViewportRef();
    float               GetDpiScale() const;

    HWND   Hwnd()      const { return m_hwnd;      }
    bool   IsPrimary() const { return m_isPrimary; }

    ////////////////////////////////////////////////////////////////////////////
    //
    //  FPS publisher (T023, FR-010, contracts/fps-publisher.md, research.md R3)
    //
    //  Lock-free pair used by the property-sheet 1 Hz title timer running on
    //  the dialog thread.  Producer is the render thread, in RenderThreadProc
    //  immediately after FPSCounter::Update().  Consumer reads via the
    //  combined-getter form below; memory_order_relaxed is correct because
    //  the float is the only shared datum and a torn read is impossible on
    //  4-byte-aligned x64/ARM64 atomic float load/store.
    //
    ////////////////////////////////////////////////////////////////////////////

    void   PublishFps        (float fps) noexcept
    {
        m_publishedFps   .store (fps,  std::memory_order_relaxed);
        m_hasPublishedFps.store (true, std::memory_order_relaxed);
    }

    float  GetPublishedFps   (bool & outHasValue) const noexcept
    {
        outHasValue = m_hasPublishedFps.load (std::memory_order_relaxed);
        return       m_publishedFps    .load (std::memory_order_relaxed);
    }

    bool   HasPublishedFps   () const noexcept
    {
        return m_hasPublishedFps.load (std::memory_order_relaxed);
    }

private:
    void RenderThreadProc();
    void Update (const SharedState::Snapshot & snapshot, float deltaTime);
    void Render (const SharedState::Snapshot & snapshot);

    bool m_isPrimary;
    HWND m_hwnd { nullptr };

    std::unique_ptr<Viewport>          m_viewport;
    std::unique_ptr<AnimationSystem>   m_animationSystem;
    std::unique_ptr<RenderSystem>      m_renderSystem;
    std::unique_ptr<DensityController> m_densityController;
    std::unique_ptr<FPSCounter>        m_fpsCounter;
    std::optional<FrameLimiter>        m_frameLimiter;

    std::mutex        m_renderMutex;
    std::thread       m_renderThread;
    std::atomic<bool> m_shouldStop { false };

    // Observer pointers — valid only while the render thread is running
    SharedState       * m_sharedState  { nullptr };
    OverlayState      * m_overlays     { nullptr };
    ApplicationState  * m_primaryClock { nullptr };
    std::atomic<bool> * m_inTransition { nullptr };

    // T023 (US1, FR-010): lock-free FPS publisher, written once per frame by
    // the render thread and read by the dialog 1 Hz title timer.  See the
    // accessor block above for ordering rationale.
    std::atomic<float> m_publishedFps    { 0.0f  };
    std::atomic<bool>  m_hasPublishedFps { false };
};
