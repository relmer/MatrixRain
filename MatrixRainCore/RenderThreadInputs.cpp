#include "pch.h"

#include "RenderThreadInputs.h"




////////////////////////////////////////////////////////////////////////////////
//
//  MakeRenderThreadInputs
//
//  Gates the per-context render-thread wiring by role: the primary context
//  receives the overlay state and the color-cycle clock; secondaries receive
//  nullptr for both.  Shared UI state and the display-mode transition flag are
//  broadcast to every context regardless of role.
//
////////////////////////////////////////////////////////////////////////////////

RenderThreadInputs MakeRenderThreadInputs (bool                isPrimary,
                                           SharedState       * sharedState,
                                           OverlayState      * overlays,
                                           ApplicationState  * appState,
                                           std::atomic<bool> * inTransition)
{
    RenderThreadInputs inputs;


    inputs.sharedState  = sharedState;
    inputs.inTransition = inTransition;
    inputs.overlays     = isPrimary ? overlays : nullptr;
    inputs.primaryClock = isPrimary ? appState : nullptr;

    return inputs;
}
