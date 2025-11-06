#include "pch.h"
#include "matrixrain/CharacterInstance.h"

namespace MatrixRain
{
    CharacterInstance::CharacterInstance()
        : glyphIndex(0)
        , color(1.0f, 1.0f, 1.0f, 1.0f) // White default
        , brightness(1.0f)
        , scale(1.0f)
        , positionOffset(0.0f, 0.0f)
        , fadeTimer(0.0f)
    {
    }

    CharacterInstance::CharacterInstance(size_t glyphIndex, const Color4& color, float brightness, float scale, const Vector2& offset)
        : glyphIndex(glyphIndex)
        , color(color)
        , brightness(brightness)
        , scale(scale)
        , positionOffset(offset)
        , fadeTimer(0.0f)
    {
    }

    void CharacterInstance::Update(float deltaTime)
    {
        // Accumulate fade timer
        fadeTimer += deltaTime;

        // Linear fade from 1.0 to 0.0 over 3 seconds
        constexpr float FADE_DURATION = 3.0f;
        
        if (fadeTimer >= FADE_DURATION)
        {
            brightness = 0.0f;
        }
        else
        {
            // Linear interpolation: brightness = 1.0 - (time / duration)
            brightness = 1.0f - (fadeTimer / FADE_DURATION);
        }
    }
}
