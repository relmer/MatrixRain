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
        , lifetime(0.0f)
        , fadeTime(3.0f)
    {
    }

    CharacterInstance::CharacterInstance(size_t glyphIndex, const Color4& color, float brightness, float scale, const Vector2& offset)
        : glyphIndex(glyphIndex)
        , color(color)
        , brightness(brightness)
        , scale(scale)
        , positionOffset(offset)
        , isHead(false)
        , lifetime(0.0f)
        , fadeTime(3.0f)
    {
    }

    void CharacterInstance::Update(float deltaTime)
    {
        // Decrement lifetime
        lifetime -= deltaTime;
        
        if (lifetime <= 0.0f)
        {
            // Dead - ready for removal
            lifetime = 0.0f;
            brightness = 0.0f;
        }
        else if (lifetime > fadeTime)
        {
            // Still in bright phase
            brightness = 1.0f;
        }
        else
        {
            // In fade phase: brightness fades from 1.0 to 0.0 as lifetime goes from fadeTime to 0
            brightness = lifetime / fadeTime;
        }
    }
}
