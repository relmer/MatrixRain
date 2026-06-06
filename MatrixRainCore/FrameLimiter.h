#pragma once

#include <chrono>


// Pure predicate: should the frame limiter engage on a monitor whose
// native refresh is the given integer Hz?  Engages only when refresh
// exceeds 60 Hz; at <=60 Hz the existing vsync path is preferred (no
// added overhead).
bool ShouldEngageFrameLimiter (unsigned monitorRefreshHz);


// Wall-clock-based per-monitor frame pacer used when the monitor's
// native refresh exceeds 60 Hz (see ShouldEngageFrameLimiter).  Sleeps
// inside WaitForNextFrame to enforce a target frames-per-second cap.
// The first call returns immediately (no prior frame timestamp); each
// subsequent call sleeps until at least 1/targetFps seconds have elapsed
// since the previous call.
//
// Uses std::chrono::steady_clock for monotonic timing.  Safe to be
// owned per-monitor (one instance per MonitorRenderContext).
class FrameLimiter
{
    public:
        explicit FrameLimiter (unsigned targetFps);

        void TargetFps         (unsigned targetFps);
        void WaitForNextFrame  ();

    private:
        std::chrono::steady_clock::duration                m_frameInterval;
        std::optional<std::chrono::steady_clock::time_point> m_lastFrameTime;
};
