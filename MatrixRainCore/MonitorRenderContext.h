#pragma once

#include "FrameLimiter.h"
#include "IDisplayLuminanceProvider.h"
#include "OutputModeTracker.h"
#include "ScreenSaverMode.h"
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

    // Construction — called on the UI thread before the render thread starts.
    // displayMode is how the app was launched; the preview window never
    // presents in HDR (FR-017), and the mode is chosen here from it.
    HRESULT Initialize         (HWND hwnd, UINT width, UINT height, std::optional<LUID> adapterLuid, ScreenSaverMode displayMode);

    // Test seam: replaces the OS-backed luminance provider. Call before
    // Initialize; the default is WindowsDisplayLuminanceProvider.
    void    SetDisplayLuminanceProvider (std::unique_ptr<IDisplayLuminanceProvider> provider);

    // The mode the swap chain presents in right now.
    OutputMode CurrentOutputMode() const noexcept;
    void    InitializeAnimation();
    HRESULT BuildGlyphAtlas();

    // Render-thread lifecycle
    void    StartRenderThread (SharedState      & sharedState,
                               OverlayState     * overlays,
                               ApplicationState * primaryClock,
                               std::atomic<bool> & inTransition);
    void    RequestStop();
    void    Join();

    // UI-thread window events.  While the render thread is running these
    // record the change and return at once; the render thread applies it at
    // the top of its next frame.  Before the thread starts they apply
    // immediately.  The UI thread must never wait on the render thread: a
    // flip-model Present blocks until the window has finished handling a
    // resize, so a WM_SIZE handler that waits for the render lock while the
    // render thread sits in Present is a deadlock that only DWM's timeout
    // breaks -- the drag-across-monitors freeze and its ghost window.
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
    // A window change the UI thread has handed to the render thread.  The
    // size and the DPI arrive as separate messages and are kept separately;
    // a later request of the same kind replaces an earlier unapplied one.
    struct PendingWindowChange
    {
        bool hasSize        { false };
        UINT width          { 0 };
        UINT height         { 0 };
        bool rescaleStreaks { false };
        bool hasDpi         { false };
        UINT dpi            { 0 };
    };

    void RenderThreadProc();
    void RunOutputModeDetection();
    void PublishHdrPresence (bool isHdr);
    void ApplyPendingWindowChanges();
    void ApplyResize       (UINT width, UINT height, bool rescaleStreaks);
    void ApplyDpiChange    (UINT dpi);
    void Update (const SharedState::Snapshot & snapshot, float deltaTime);
    void Render (const SharedState::Snapshot & snapshot);

    bool            m_isPrimary;
    HWND            m_hwnd        { nullptr };
    ScreenSaverMode m_displayMode { ScreenSaverMode::Normal };

    // Output mode detection (research R5, R6; T032). The provider asks the
    // OS, the tracker decides, RenderSystem does the switch. Detection runs
    // on the render thread at 1 Hz and whenever the provider says the display
    // configuration changed, and once at Initialize before the first frame.
    std::unique_ptr<IDisplayLuminanceProvider> m_luminanceProvider;
    std::optional<OutputModeTracker>           m_outputModeTracker;
    std::chrono::steady_clock::time_point      m_lastDetection {};
    bool                                       m_isHdr         { false };   // What the last detection concluded
    bool                                       m_countedAsHdr  { false };   // Whether SharedState::hdrMonitorCount includes this context

    std::unique_ptr<Viewport>          m_viewport;
    std::unique_ptr<AnimationSystem>   m_animationSystem;
    std::unique_ptr<RenderSystem>      m_renderSystem;
    std::unique_ptr<DensityController> m_densityController;
    std::unique_ptr<FPSCounter>        m_fpsCounter;
    std::optional<FrameLimiter>        m_frameLimiter;

    std::mutex        m_renderMutex;
    std::thread       m_renderThread;
    std::atomic<bool> m_shouldStop { false };

    // Guards m_pending only.  Held for a copy in or out, never while waiting
    // on anything, so neither thread can block on it for long.
    std::mutex          m_pendingMutex;
    PendingWindowChange m_pending;

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
