#include "pch.h"

#include "CharacterStreak.h"
#include "CharacterSet.h"





float CharacterStreak::GetVelocityScale (float depth)
{
    constexpr float MIN_SCALE        = 1.0f;
    constexpr float MAX_SCALE        = 6.0f;
    constexpr float MAX_DEPTH        = 100.0f;

    float           normalizedDepth  = std::clamp (depth / MAX_DEPTH, 0.0f, 1.0f);
    
    return MIN_SCALE + normalizedDepth * (MAX_SCALE - MIN_SCALE);
}





void CharacterStreak::Spawn (const Vector3 & position)
{
    static uint64_t s_nextID = 1;
    
    m_id       = s_nextID++;
    m_position = position; // This is the head position where new characters spawn

    // Random length between 5 and 30
    std::uniform_int_distribution<size_t> lengthDist (MIN_LENGTH, MAX_LENGTH);
    m_maxLength = lengthDist (s_generator);

    // Start with no characters - they'll be added as the streak "drops"
    m_characters.clear();
    m_characters.reserve (m_maxLength);

    // No horizontal drift - streaks stay at fixed X position
    m_velocity.x = 0.0f;
    m_velocity.y = 0.0f;
    m_velocity.z = 0.0f;

    // Calculate drop interval once at spawn based on initial depth
    // This ensures constant speed throughout the streak's lifetime
    float velocityScale   = GetVelocityScale (m_position.z);
    
    m_baseDropInterval    = BASE_DROP_INTERVAL / velocityScale;
    m_dropInterval        = m_baseDropInterval; // Will be adjusted by speed multiplier if set
    m_mutationTimer       = 0.0f;
    m_dropTimer           = 0.0f;
    m_isInFadingPhase     = false;
    
    // Spawn the first character immediately at the head position
    CharacterSet & charSet = CharacterSet::GetInstance();
    charSet.Initialize();
    
    CharacterInstance character;
    
    character.glyphIndex     = charSet.GetRandomGlyphIndex();
    character.color          = Color4 (1.0f, 1.0f, 1.0f, 1.0f); // White (head)
    character.brightness     = 1.0f;
    character.scale          = 1.0f;
    character.positionOffset = Vector2 (0.0f, m_position.y);
    character.isHead         = true;
    character.lifetime       = 0.0f; // Head has no lifetime (stays alive while isHead=true)
    character.fadeTime       = 3.0f;
    
    m_characters.push_back (character);
}





void CharacterStreak::Update (float deltaTime, float viewportHeight)
{
    // Use cached drop interval (constant for streak lifetime)
    m_dropTimer += deltaTime;
    if (m_dropTimer >= m_dropInterval)
    {
        m_dropTimer -= m_dropInterval;
            
        // Continue adding characters until head reaches bottom (Phase 1 and 2)
        // Stop adding when head reaches viewport bottom to enter Phase 3 (fade out)
        if (m_position.y < viewportHeight)
        {
            // Turn the previous head character green and calculate its bright time
            if (!m_characters.empty())
            {
                CharacterInstance & oldHead = m_characters.back();
                oldHead.isHead = false;
                oldHead.color = Color4 (0.0f, 1.0f, 0.0f, 1.0f); // Green
                
                // Set lifetime ONLY for this character (don't recalculate others!)
                // Character index from front determines bright time
                size_t characterIndex = m_characters.size() - 1;
                float  brightTime     = characterIndex * m_dropInterval;
                
                oldHead.lifetime = brightTime + oldHead.fadeTime;
            }
            
            CharacterSet & charSet = CharacterSet::GetInstance();
            charSet.Initialize();
            
            CharacterInstance character;
            
            character.glyphIndex     = charSet.GetRandomGlyphIndex();
            character.color          = Color4 (1.0f, 1.0f, 1.0f, 1.0f); // White (head)
            character.brightness     = 1.0f;
            character.scale          = 1.0f;
            // Store absolute position where this character was born
            character.positionOffset = Vector2 (0.0f, m_position.y);
            character.isHead         = true;
            character.lifetime       = 0.0f; // Head has no lifetime (stays alive while isHead=true)
            character.fadeTime       = 3.0f;

            m_characters.push_back (character);
            
            // Move the head down by one cell for the next character
            m_position.y += m_characterSpacing;
        }
        else if (!m_isInFadingPhase)
        {
            // Head has just gone offscreen - transition to fading phase (only once)
            if (!m_characters.empty())
            {
                CharacterInstance & oldHead = m_characters.back();
                oldHead.isHead = false;
                oldHead.color = Color4 (0.0f, 1.0f, 0.0f, 1.0f); // Green
                
                // Set lifetime for the final head character
                size_t characterIndex = m_characters.size() - 1;
                float  brightTime     = characterIndex * m_dropInterval;
                
                oldHead.lifetime = brightTime + oldHead.fadeTime;
            }
            
            m_isInFadingPhase = true;
        }
    }

    // Update character state: decrement timers and calculate brightness
    for (CharacterInstance & character : m_characters)
    {
        // Skip the head - it stays at full brightness
        if (character.isHead)
        {
            character.brightness = 1.0f;
            continue;
        }

        character.Update (deltaTime);
    }

    // Remove characters that have fully faded (only from the tail/front of vector)
    while (!m_characters.empty() && m_characters.front ().brightness <= 0.0f)
    {
        m_characters.erase (m_characters.begin());
    }
    
    // Also remove any faded characters from the back (e.g., when head goes offscreen and fades)
    while (!m_characters.empty() && m_characters.back ().brightness <= 0.0f)
    {
        m_characters.pop_back();
    }

    // Handle character mutation (5% probability per character per second)
    std::uniform_real_distribution<float> mutationDist (0.0f, 1.0f);
    CharacterSet & charSet = CharacterSet::GetInstance();

    for (CharacterInstance & character : m_characters)
    {
        float mutationChance = MUTATION_PROBABILITY * deltaTime;
        if (mutationDist (s_generator) < mutationChance)
        {
            // Mutate to a new random glyph (keep existing fade state)
            character.glyphIndex = charSet.GetRandomGlyphIndex();
        }
    }
}





bool CharacterStreak::ShouldDespawn() const
{
    // The streak should only despawn when all characters have faded out
    // Characters are removed one by one as they fade in the Update() method
    // Once the vector is empty, the streak is done
    return m_characters.empty();
}






bool CharacterStreak::IsHeadOffscreen (float viewportHeight) const
{
    // Head is offscreen once it has passed the bottom of the viewport
    // This allows new streaks to spawn while old ones are still fading
    return m_position.y >= viewportHeight;
}






void CharacterStreak::RescalePositions (float scaleX, float scaleY)
{
    // Scale the streak head position
    m_position.x *= scaleX;
    m_position.y *= scaleY;

    // Add small random jitter to X to break up banding patterns from scaling
    std::uniform_real_distribution<float> jitterDist (-16.0f, 16.0f);
    m_position.x += jitterDist (s_generator);

    // Recalculate character positions based on fixed spacing from the new head position
    // Characters are stored back-to-front (tail at [0], head at [size-1])
    // Each character should be 32px above the next one
    for (size_t i = 0; i < m_characters.size(); i++)
    {
        CharacterInstance & character = m_characters[i];
        
        // Scale X offset
        character.positionOffset.x *= scaleX;
        
        // Recalculate Y position: start from current head, go backwards by spacing
        size_t distanceFromHead = m_characters.size() - 1 - i;
        character.positionOffset.y = m_position.y - (distanceFromHead * m_characterSpacing);
    }
}





void CharacterStreak::SetCharacterSpacing (float spacing)
{
    m_characterSpacing = spacing;

    // Characters are stored back-to-front (tail at [0], head at [size-1])
    for (size_t i = 0; i < m_characters.size(); i++)
    {
        CharacterInstance & character        = m_characters[i];
        size_t              distanceFromHead = m_characters.size() - 1 - i;

        character.positionOffset.y = m_position.y - (distanceFromHead * m_characterSpacing);
    }
}





void CharacterStreak::SetSpeedMultiplier (int speedPercent)
{
    // Speed multiplier: 100 = normal, 50 = half speed, 200 = double speed
    // Lower speed percentage = longer drop interval (slower)
    // Higher speed percentage = shorter drop interval (faster)
    m_dropInterval = m_baseDropInterval * (100.0f / static_cast<float>(speedPercent));
}
