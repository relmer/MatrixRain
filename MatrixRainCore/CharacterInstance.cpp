#include "pch.h"

#include "CharacterInstance.h"





CharacterInstance::CharacterInstance (size_t glyphIndex, const Color4 & color, float brightness, float scale, const Vector2 & offset) :
    glyphIndex     (glyphIndex),
    color          (color),
    brightness     (brightness),
    scale          (scale),
    positionOffset (offset)
{
}





void CharacterInstance::Update (float deltaTime)
{
    // Decrement lifetime
    lifetime -= deltaTime;
    
    if (lifetime <= 0.0f)
    {
        // Dead - ready for removal
        lifetime   = 0.0f;
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