#include "pch.h"

#include "MatrixRain/CharacterStreak.h"
#include "MatrixRain/CharacterSet.h"





namespace MatrixRain
{
    namespace
    {
        // Random number generator
        std::random_device g_randomDevice;
        std::mt19937 g_generator(g_randomDevice());

        // Map depth (Z: 0-100) to velocity scale (1.0 - 3.0)
        // Far streaks (Z=100) move 3x faster than near streaks (Z=0)
        float GetVelocityScale(float depth)
        {
            constexpr float MIN_SCALE = 1.0f;
            constexpr float MAX_SCALE = 3.0f;
            constexpr float MAX_DEPTH = 100.0f;

            float normalizedDepth = std::clamp(depth / MAX_DEPTH, 0.0f, 1.0f);
            return MIN_SCALE + normalizedDepth * (MAX_SCALE - MIN_SCALE);
        }
    }

    void CharacterStreak::Spawn(const Vector3& position)
    {
        m_position = position; // This is the head position where new characters spawn

        // Random length between 5 and 30
        std::uniform_int_distribution<size_t> lengthDist(MIN_LENGTH, MAX_LENGTH);
        m_maxLength = lengthDist(g_generator);

        // Start with no characters - they'll be added as the streak "drops"
        m_characters.clear();
        m_characters.reserve(m_maxLength);

        // No horizontal drift - streaks stay at fixed X position
        m_velocity.x = 0.0f;
        m_velocity.y = 0.0f;
        m_velocity.z = 0.0f;

        // Calculate drop interval once at spawn based on initial depth
        // This ensures constant speed throughout the streak's lifetime
        float velocityScale = GetVelocityScale(m_position.z);
        m_dropInterval = BASE_DROP_INTERVAL / velocityScale;

        m_mutationTimer = 0.0f;
        m_dropTimer = 0.0f;
        
        // Spawn the first character immediately at the head position
        CharacterSet& charSet = CharacterSet::GetInstance();
        charSet.Initialize();
        
        CharacterInstance character;
        character.glyphIndex = charSet.GetRandomGlyphIndex();
        character.color = Color4(1.0f, 1.0f, 1.0f, 1.0f); // White (head)
        character.brightness = 1.0f;
        character.scale = 1.0f;
        character.positionOffset = Vector2(0.0f, m_position.y);
        character.isHead = true;
        character.brightTimeRemaining = 0.0f; // Head doesn't need bright time
        character.fadeTimeRemaining = 3.0f;
        
        m_characters.push_back(character);
    }

    void CharacterStreak::Update(float deltaTime, float viewportHeight)
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
                    CharacterInstance& oldHead = m_characters.back();
                    oldHead.isHead = false;
                    oldHead.color = Color4(0.0f, 1.0f, 0.0f, 1.0f); // Green
                    
                    // Calculate how long this character should remain at full brightness
                    // Characters further from head get less bright time
                    if (m_characters.size() < m_maxLength)
                    {
                        size_t charactersStillToCome = m_maxLength - m_characters.size();
                        oldHead.brightTimeRemaining = charactersStillToCome * m_dropInterval;
                    }
                    else
                    {
                        // Streak is at max length, start fading immediately
                        oldHead.brightTimeRemaining = 0.0f;
                    }
                }
                
                CharacterSet& charSet = CharacterSet::GetInstance();
                charSet.Initialize();
                
                CharacterInstance character;
                character.glyphIndex = charSet.GetRandomGlyphIndex();
                character.color = Color4(1.0f, 1.0f, 1.0f, 1.0f); // White (head)
                character.brightness = 1.0f;
                character.scale = 1.0f;
                // Store absolute position where this character was born
                character.positionOffset = Vector2(0.0f, m_position.y);
                character.isHead = true;
                character.brightTimeRemaining = 0.0f; // Head doesn't need bright time
                character.fadeTimeRemaining = 3.0f;

                m_characters.push_back(character);
                
                // Move the head down by one cell for the next character
                m_position.y += m_characterSpacing;
            }
            else
            {
                // Head has gone offscreen - mark the current head as no longer head
                if (!m_characters.empty())
                {
                    CharacterInstance& oldHead = m_characters.back();
                    oldHead.isHead = false;
                    oldHead.color = Color4(0.0f, 1.0f, 0.0f, 1.0f); // Green
                    oldHead.brightTimeRemaining = 0.0f; // Start fading immediately
                }
            }
        }

        // Update character state: decrement timers and calculate brightness
        for (CharacterInstance& character : m_characters)
        {
            // Skip the head - it stays at full brightness
            if (character.isHead)
            {
                character.brightness = 1.0f;
                continue;
            }

            character.Update(deltaTime);
        }

        // Remove characters that have fully faded (only from the tail/front of vector)
        while (!m_characters.empty() && m_characters.front().brightness <= 0.0f)
        {
            m_characters.erase(m_characters.begin());
        }

        // Handle character mutation (5% probability per character per second)
        std::uniform_real_distribution<float> mutationDist(0.0f, 1.0f);
        CharacterSet& charSet = CharacterSet::GetInstance();

        for (CharacterInstance& character : m_characters)
        {
            float mutationChance = MUTATION_PROBABILITY * deltaTime;
            if (mutationDist(g_generator) < mutationChance)
            {
                // Mutate to a new random glyph (keep existing fade state)
                character.glyphIndex = charSet.GetRandomGlyphIndex();
            }
        }
    }

    bool CharacterStreak::ShouldDespawn([[maybe_unused]] float viewportHeight) const
    {
        // The streak should only despawn when all characters have faded out
        // Characters are removed one by one as they fade in the Update() method
        // Once the vector is empty, the streak is done
        return m_characters.empty();
    }
}
