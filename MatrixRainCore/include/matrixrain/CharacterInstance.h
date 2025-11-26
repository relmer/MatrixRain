#pragma once
#include "Math.h"

namespace MatrixRain
{
    // CharacterInstance: Represents a single character in a streak with fade/animation state
    class CharacterInstance
    {
    public:
        size_t glyphIndex = 0;              // Index into CharacterSet glyphs array
        Color4 color{};                     // Current color (for white->green transition)
        float brightness = 1.0f;            // Current brightness (1.0 = full, 0.0 = faded out)
        float scale = 1.0f;                 // Scale multiplier for size variation
        Vector2 positionOffset{};           // Offset from streak base position
        bool isHead = false;                // True if this is the current head character (white)
        float lifetime = 0.0f;              // Total time remaining (bright time + fade time)
        float fadeTime = 3.0f;              // Duration of fade phase (constant)

        // Default constructor
        CharacterInstance();

        // Parameterized constructor
        CharacterInstance(size_t glyphIndex, const Color4& color, float brightness, float scale, const Vector2& offset);

        // Update fade over time (3 second linear fade)
        void Update(float deltaTime);
    };
}
