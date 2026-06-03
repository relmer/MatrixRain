#pragma once

#include "..\MatrixRainCore\IRenderSystem.h"




////////////////////////////////////////////////////////////////////////////////
//
//  SpyRenderSystem
//
//  Test double for IRenderSystem.  Records how many times each frame-driving
//  method was invoked and the most recent argument values, so orchestration
//  tests can assert that every per-monitor render context is driven without any
//  real GPU.  Render() deliberately ignores the animation/viewport references.
//
////////////////////////////////////////////////////////////////////////////////

class SpyRenderSystem : public IRenderSystem
{
public:
    int   m_renderCount      = 0;
    int   m_presentCount     = 0;
    int   m_resizeCount      = 0;
    int   m_dpiChangedCount  = 0;
    int   m_glowIntensity    = -1;
    int   m_glowSize         = -1;
    float m_characterScale   = -1.0f;
    float m_dpiScale         = 1.0f;

    UINT  m_lastResizeWidth  = 0;
    UINT  m_lastResizeHeight = 0;
    UINT  m_lastDpi          = 0;

    RenderParams m_lastParams;


    void Render (const AnimationSystem & animationSystem, const Viewport & viewport, const RenderParams & params) override
    {
        UNREFERENCED_PARAMETER (animationSystem);
        UNREFERENCED_PARAMETER (viewport);

        m_renderCount++;
        m_lastParams = params;
    }


    void Present() override
    {
        m_presentCount++;
    }


    void Resize (UINT width, UINT height) override
    {
        m_resizeCount++;
        m_lastResizeWidth  = width;
        m_lastResizeHeight = height;
    }


    void OnDpiChanged (UINT dpi) override
    {
        m_dpiChangedCount++;
        m_lastDpi = dpi;
    }


    void SetGlowIntensity (int intensityPercent) override
    {
        m_glowIntensity = intensityPercent;
    }


    void SetGlowSize (int sizePercent) override
    {
        m_glowSize = sizePercent;
    }


    void SetCharacterScaleOverride (float scale) override
    {
        m_characterScale = scale;
    }


    float GetDpiScale() const override
    {
        return m_dpiScale;
    }
};
