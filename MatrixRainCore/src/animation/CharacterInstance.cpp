#include "pch.h"
#include "MatrixRain/CharacterInstance.h"

namespace MatrixRain
{
    CharacterInstance::CharacterInstance()
        : glyphIndex(0)
        , color(1.0f, 1.0f, 1.0f, 1.0f) // White default
        , brightness(1.0f)
        , scale(1.0f)
        , positionOffset(0.0f, 0.0f)
        , isHead(false)
        , brightTimeRemaining(0.0f)
        , fadeTimeRemaining(3.0f)
    {
    }

    CharacterInstance::CharacterInstance(size_t glyphIndex, const Color4& color, float brightness, float scale, const Vector2& offset)
        : glyphIndex(glyphIndex)
        , color(color)
        , brightness(brightness)
        , scale(scale)
        , positionOffset(offset)
        , isHead(false)
        , brightTimeRemaining(0.0f)
        , fadeTimeRemaining(3.0f)
    {
    }

    void CharacterInstance::Update(float deltaTime)
    {
        // Update bright time first
        if (brightTimeRemaining > 0.0f)
        {
            brightTimeRemaining -= deltaTime;
            if (brightTimeRemaining <= 0.0f)
            {
                brightTimeRemaining = 0.0f;
                // Transition to fading phase - fadeTimeRemaining already set to 3.0
            }
        }

        // If bright time is over, update fade
        if (brightTimeRemaining <= 0.0f)
        {
            fadeTimeRemaining -= deltaTime;
            
            if (fadeTimeRemaining <= 0.0f)
            {
                fadeTimeRemaining = 0.0f;
                brightness = 0.0f;
            }
            else
            {
                // Linear fade: brightness goes from 1.0 to 0.0 as fadeTimeRemaining goes from 3.0 to 0.0
                constexpr float FADE_DURATION = 3.0f;
                brightness = fadeTimeRemaining / FADE_DURATION;
            }
        }
        else
        {
            // Still in bright phase
            brightness = 1.0f;
        }
    }
}
