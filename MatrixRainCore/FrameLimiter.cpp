#include "pch.h"

#include "FrameLimiter.h"




////////////////////////////////////////////////////////////////////////////////
//
//  ShouldEngageFrameLimiter
//
////////////////////////////////////////////////////////////////////////////////

bool ShouldEngageFrameLimiter (unsigned monitorRefreshHz)
{
    return monitorRefreshHz > 60;
}




////////////////////////////////////////////////////////////////////////////////
//
//  FrameLimiter::FrameLimiter
//
////////////////////////////////////////////////////////////////////////////////

FrameLimiter::FrameLimiter (unsigned targetFps)
{
    TargetFps (targetFps);
}




////////////////////////////////////////////////////////////////////////////////
//
//  FrameLimiter::TargetFps
//
////////////////////////////////////////////////////////////////////////////////

void FrameLimiter::TargetFps (unsigned targetFps)
{
    if (targetFps == 0)
    {
        m_frameInterval = std::chrono::steady_clock::duration::zero();
    }
    else
    {
        m_frameInterval = std::chrono::nanoseconds (1'000'000'000ull / targetFps);
    }
}




////////////////////////////////////////////////////////////////////////////////
//
//  FrameLimiter::WaitForNextFrame
//
////////////////////////////////////////////////////////////////////////////////

void FrameLimiter::WaitForNextFrame()
{
    using clock = std::chrono::steady_clock;


    clock::time_point now = clock::now();

    if (!m_lastFrameTime.has_value())
    {
        // First-ever call: no prior frame to pace against; render now.
        m_lastFrameTime = now;
        return;
    }

    clock::time_point nextDeadline = *m_lastFrameTime + m_frameInterval;

    if (now < nextDeadline)
    {
        std::this_thread::sleep_until (nextDeadline);
        now = clock::now();
    }

    m_lastFrameTime = now;
}
