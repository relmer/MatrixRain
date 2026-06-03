#pragma once



struct SharedState;
struct OverlayState;
class  ApplicationState;




////////////////////////////////////////////////////////////////////////////////
//
//  RenderThreadInputs
//
//  The per-context wiring handed to a render thread.  Overlays and the
//  color-cycle clock are populated only for the primary context; secondaries
//  receive nullptr.  Shared UI state and the transition flag reach every
//  context.
//
////////////////////////////////////////////////////////////////////////////////

struct RenderThreadInputs
{
    SharedState       * sharedState  = nullptr;
    OverlayState      * overlays     = nullptr;
    ApplicationState  * primaryClock = nullptr;
    std::atomic<bool> * inTransition = nullptr;
};




RenderThreadInputs MakeRenderThreadInputs (bool                isPrimary,
                                           SharedState       * sharedState,
                                           OverlayState      * overlays,
                                           ApplicationState  * appState,
                                           std::atomic<bool> * inTransition);
