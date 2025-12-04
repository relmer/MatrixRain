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
#ifdef _DEBUG
    float oldBrightness = brightness;
#endif

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

#ifdef _DEBUG
    // Brightness should NEVER increase (except for heads which stay at 1.0)
    ASSERT (isHead || brightness <= (oldBrightness + 0.001f))
    
    // Check for excessive darkening during normal fade phase only
    // Skip check for edge cases:
    // - Character death (lifetime <= 0)
    // - Initial bright phase (lifetime > fadeTime)
    // - Already nearly faded (oldBrightness < 0.1)
    if (lifetime > 0.0f && lifetime <= fadeTime && oldBrightness >= 0.1f)
    {
        float brightnessDecrease = oldBrightness - brightness;
        
        // At 30fps, deltaTime ~= 0.033s, fadeTime = 3.0s
        // Expected max decrease = 0.033 / 3.0 = 0.011
        // Allow 10x margin for lag spikes: 0.11 threshold
        ASSERT (brightnessDecrease <= 0.11f);
    }
#endif
}