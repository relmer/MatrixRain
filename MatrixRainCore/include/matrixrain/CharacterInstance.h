#pragma once
#include "Math.h"

namespace MatrixRain
{
    // CharacterInstance: Represents a single character in a streak with fade/animation state
    class CharacterInstance
    {
    public:
        size_t glyphIndex;          // Index into CharacterSet glyphs array
        Color4 color;               // Current color (for white->green transition)
        float brightness;           // Current brightness (1.0 = full, 0.0 = faded out)
        float scale;                // Scale multiplier for size variation
        Vector2 positionOffset;     // Offset from streak base position
        float fadeTimer;            // Time elapsed for fade calculation (0 to 3 seconds)

        // Default constructor
        CharacterInstance();

        // Parameterized constructor
        CharacterInstance(size_t glyphIndex, const Color4& color, float brightness, float scale, const Vector2& offset);

        // Update fade over time (3 second linear fade)
        void Update(float deltaTime);
    };
}
