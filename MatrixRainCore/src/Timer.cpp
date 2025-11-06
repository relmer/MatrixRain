#include "pch.h"
#include "matrixrain/Timer.h"

namespace MatrixRain
{
    Timer::Timer()
        : m_startTime(0)
        , m_frequency(0)
    {
        // Query the performance counter frequency (ticks per second)
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        m_frequency = freq.QuadPart;

        // Initialize start time
        Start();
    }

    void Timer::Start()
    {
        m_startTime = GetCurrentTime();
    }

    double Timer::GetElapsedSeconds() const
    {
        long long currentTime = GetCurrentTime();
        long long elapsed = currentTime - m_startTime;
        return static_cast<double>(elapsed) / static_cast<double>(m_frequency);
    }

    double Timer::GetElapsedMilliseconds() const
    {
        return GetElapsedSeconds() * 1000.0;
    }

    void Timer::Reset()
    {
        m_startTime = GetCurrentTime();
    }

    long long Timer::GetCurrentTime() const
    {
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        return counter.QuadPart;
    }
}
