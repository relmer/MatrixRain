#include "pch.h"

#include "MatrixRain/ColorScheme.h"





// Static color table - indexed by ColorScheme enum values (0-3)
static constexpr Color4 s_colorTable[static_cast<int> (ColorScheme::__StaticColorCount)] = {
    Color4 (0.0f, 1.0f,            100.0f / 255.0f),    // Green
    Color4 (0.0f, 100.0f / 255.0f, 1.0f),               // Blue
    Color4 (1.0f, 50.0f / 255.0f,  50.0f / 255.0f),     // Red
    Color4 (1.0f, 191.0f / 255.0f, 0.0f)                // Amber
};

// Precomputed color cycle lookup table (720 samples = 60 Hz * 12 seconds)
static constexpr int   COLOR_CYCLE_TABLE_SIZE = 720;
static constexpr float COLOR_CYCLE_DURATION   = 12.0f;
static constexpr float TIME_PER_COLOR         = 3.0f;





// Helper to compute color at a specific time index (compile-time)
constexpr Color4 ComputeCycleColorAtIndex (int index)
{
    // Map index to elapsed time
    float elapsedTime = (static_cast<float> (index) / COLOR_CYCLE_TABLE_SIZE) * COLOR_CYCLE_DURATION;
    
    // Normalize to [0, COLOR_CYCLE_DURATION)
    float normalizedTime = elapsedTime;
    while (normalizedTime >= COLOR_CYCLE_DURATION)
    {
        normalizedTime -= COLOR_CYCLE_DURATION;
    }
    
    // Determine color index and interpolation factor
    int colorIndex = static_cast<int> (normalizedTime / TIME_PER_COLOR);
    if (colorIndex >= 4) colorIndex = 3; // Safety clamp
    
    float timeInSegment = normalizedTime - (colorIndex * TIME_PER_COLOR);
    float t             = timeInSegment / TIME_PER_COLOR;
    
    // Get colors to interpolate
    const Color4 & fromColor = s_colorTable[colorIndex];
    const Color4 & toColor   = s_colorTable[(colorIndex + 1) % static_cast<int> (ColorScheme::__StaticColorCount)];
    
    return Math::LerpConstexpr (fromColor, toColor, t);
}





// Recursive template to build compile-time array
template<std::size_t... Is>
constexpr auto MakeColorCycleTable (std::index_sequence<Is...>)
{
    return std::array<Color4, sizeof... (Is)> {{ ComputeCycleColorAtIndex (Is)... }};
}

// Generate the table at compile time
static constexpr auto s_colorCycleTable = MakeColorCycleTable (
    std::make_index_sequence<COLOR_CYCLE_TABLE_SIZE>{}
);





ColorScheme GetNextColorScheme (ColorScheme current)
{
    int next = (static_cast<int> (current) + 1) % (static_cast<int> (ColorScheme::ColorCycle) + 1);

    return static_cast<ColorScheme> (next);
}





Color4 GetColorRGB (ColorScheme scheme, float elapsedTime)
{
    // Handle ColorCycle mode with precomputed lookup table
    if (scheme == ColorScheme::ColorCycle)
    {
        // Map elapsed time to table index
        float normalizedTime = fmodf (elapsedTime, COLOR_CYCLE_DURATION);
        int   index          = static_cast<int> ((normalizedTime / COLOR_CYCLE_DURATION) * COLOR_CYCLE_TABLE_SIZE);
        
        // Clamp to valid range
        if (index < 0) index = 0;
        if (index >= COLOR_CYCLE_TABLE_SIZE) index = COLOR_CYCLE_TABLE_SIZE - 1;
        
        return s_colorCycleTable[index];
    }
    
    // For static color schemes, use direct array lookup
    int index = static_cast<int> (scheme);
    if (index >= 0 && index < static_cast<int> (ColorScheme::__StaticColorCount))
    {
        return s_colorTable[index];
    }
    
    // Fallback to green
    return s_colorTable[0];
}
