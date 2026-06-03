#pragma once

#include "RenderParams.h"




class AnimationSystem;
class Viewport;




////////////////////////////////////////////////////////////////////////////////
//
//  IRenderSystem
//
//  Narrow, per-frame rendering seam.  Captures only what a per-monitor render
//  thread drives each frame, so orchestration tests can inject a spy and assert
//  that every monitor context is rendered and presented without any GPU.
//
//  Deliberately excludes construction/device concerns (Initialize, GetDevice):
//  those are production-only setup performed against the concrete renderer, and
//  keeping them off this interface avoids cementing a single-shared-device
//  assumption in a multi-device (one device per monitor) design.
//
////////////////////////////////////////////////////////////////////////////////

class IRenderSystem
{
public:
    virtual ~IRenderSystem() = default;

    // Render all character streaks for one frame.  params is consumed
    // synchronously and must not be retained past the call.
    virtual void Render (const AnimationSystem & animationSystem, const Viewport & viewport, const RenderParams & params) = 0;

    // Present the rendered frame; blocks on this monitor's VBlank.
    virtual void Present() = 0;

    // Recreate swap-chain buffers for a new client size.
    virtual void Resize (UINT width, UINT height) = 0;

    // React to a DPI change for this context's monitor.
    virtual void OnDpiChanged (UINT dpi) = 0;

    // Live-tunable render settings broadcast from shared UI state.
    virtual void SetGlowIntensity (int intensityPercent) = 0;
    virtual void SetGlowSize      (int sizePercent)      = 0;

    // Fixed character scale that bypasses viewport-based scaling.
    virtual void SetCharacterScaleOverride (float scale) = 0;

    // Current DPI scale factor (1.0 at 96 DPI / 100%).
    virtual float GetDpiScale() const = 0;
};
