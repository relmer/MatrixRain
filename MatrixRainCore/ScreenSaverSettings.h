#pragma once

using SystemClockTimePoint = std::chrono::system_clock::time_point;





struct ScreenSaverSettings
{
    static constexpr int MIN_DENSITY_PERCENT         = 0;
    static constexpr int MAX_DENSITY_PERCENT         = 100;
    static constexpr int MIN_ANIMATION_SPEED_PERCENT = 1;
    static constexpr int MAX_ANIMATION_SPEED_PERCENT = 100;
    static constexpr int MIN_GLOW_INTENSITY_PERCENT  = 0;
    static constexpr int MAX_GLOW_INTENSITY_PERCENT  = 200;
    static constexpr int MIN_GLOW_SIZE_PERCENT       = 50;
    static constexpr int MAX_GLOW_SIZE_PERCENT       = 200;

    int                                 m_densityPercent        { 100   };
    std::wstring                        m_colorSchemeKey;
    int                                 m_animationSpeedPercent { 75    };
    int                                 m_glowIntensityPercent  { 100   };
    int                                 m_glowSizePercent       { 100   };
    bool                                m_startFullscreen       { true  };
    bool                                m_showDebugStats        { false };
    bool                                m_showFadeTimers        { false };
    std::optional<SystemClockTimePoint> m_lastSavedTimestamp;

    void Clamp();

    static int ClampDensityPercent (int value);
    static int ClampPercent        (int value, int minimum, int maximum);
};





inline void ScreenSaverSettings::Clamp()
{
    m_densityPercent        = ClampDensityPercent (m_densityPercent);
    m_animationSpeedPercent = ClampPercent        (m_animationSpeedPercent, MIN_ANIMATION_SPEED_PERCENT, MAX_ANIMATION_SPEED_PERCENT);
    m_glowIntensityPercent  = ClampPercent        (m_glowIntensityPercent, MIN_GLOW_INTENSITY_PERCENT, MAX_GLOW_INTENSITY_PERCENT);
    m_glowSizePercent       = ClampPercent        (m_glowSizePercent, MIN_GLOW_SIZE_PERCENT, MAX_GLOW_SIZE_PERCENT);
}





inline int ScreenSaverSettings::ClampDensityPercent (int value)
{
    return std::clamp (value, MIN_DENSITY_PERCENT, MAX_DENSITY_PERCENT);
}





inline int ScreenSaverSettings::ClampPercent (int value, int minimum, int maximum)
{
    return std::clamp (value, minimum, maximum);
}





