#pragma once





// Timer: High-resolution timer utility using QueryPerformanceCounter
// Provides precise timing for animation delta times and performance measurement
class Timer
{
public:
    Timer();

    // Start or restart the timer
    void Start();

    // Get elapsed time in seconds since Start() was called
    double GetElapsedSeconds() const;

    // Get elapsed time in milliseconds since Start() was called
    double GetElapsedMilliseconds() const;

    // Reset the timer to zero
    void Reset();

    

private:
    long long m_startTime { 0 };
    long long m_frequency { 0 };

    // Get current high-resolution timestamp
    long long GetCurrentTime() const;
};
