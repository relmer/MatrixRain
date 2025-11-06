#include "pch.h"
#include "matrixrain/CharacterStreak.h"
#include "matrixrain/CharacterSet.h"
#include <random>

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

        m_mutationTimer = 0.0f;
        m_dropTimer = 0.0f;
        
        // Spawn the first character immediately at the head position
        CharacterSet& charSet = CharacterSet::GetInstance();
        charSet.Initialize();
        
        CharacterInstance character;
        character.glyphIndex = charSet.GetRandomGlyphIndex();
        character.color = Color4(0.0f, 1.0f, 0.0f, 1.0f); // Green
        character.brightness = 1.0f;
        character.scale = 1.0f;
        character.positionOffset = Vector2(0.0f, m_position.y);
        character.fadeTimer = 0.0f;
        
        m_characters.push_back(character);
    }

    void CharacterStreak::Update(float deltaTime)
    {
        // Discrete cell dropping based on depth
        float velocityScale = GetVelocityScale(m_position.z);
        float dropInterval = BASE_DROP_INTERVAL / velocityScale; // Faster streaks drop more frequently
        
        m_dropTimer += deltaTime;
        if (m_dropTimer >= dropInterval)
        {
            m_dropTimer -= dropInterval;
            
            // Spawn a new character at the current head position
            if (m_characters.size() < m_maxLength)
            {
                CharacterSet& charSet = CharacterSet::GetInstance();
                charSet.Initialize();
                
                CharacterInstance character;
                character.glyphIndex = charSet.GetRandomGlyphIndex();
                character.color = Color4(0.0f, 1.0f, 0.0f, 1.0f); // Green
                character.brightness = 1.0f;
                character.scale = 1.0f;
                // Store absolute position where this character was born
                character.positionOffset = Vector2(0.0f, m_position.y);
                character.fadeTimer = 0.0f;

                m_characters.push_back(character);
            }
            
            // Move the head down by one cell for the next character
            m_position.y += m_characterSpacing;
        }

        // Update character fades for all characters
        for (CharacterInstance& character : m_characters)
        {
            character.Update(deltaTime);
        }

        // Remove characters that have fully faded
        m_characters.erase(
            std::remove_if(m_characters.begin(), m_characters.end(),
                [](const CharacterInstance& c) { return c.brightness <= 0.0f; }),
            m_characters.end()
        );

        // Handle character mutation (5% probability per character per second)
        std::uniform_real_distribution<float> mutationDist(0.0f, 1.0f);
        CharacterSet& charSet = CharacterSet::GetInstance();

        for (CharacterInstance& character : m_characters)
        {
            float mutationChance = MUTATION_PROBABILITY * deltaTime;
            if (mutationDist(g_generator) < mutationChance)
            {
                // Mutate to a new random glyph
                character.glyphIndex = charSet.GetRandomGlyphIndex();
                
                // Reset fade for the new character
                character.fadeTimer = 0.0f;
                character.brightness = 1.0f;
            }
        }
    }

    bool CharacterStreak::ShouldDespawn(float viewportHeight) const
    {
        if (m_characters.empty())
        {
            return true;
        }

        // The streak should despawn when the last remaining character
        // has moved below the viewport
        // Note: positionOffset.y is now absolute, not relative to m_position
        float lastCharacterY = m_characters.back().positionOffset.y;

        // Despawn when last character is below viewport (with some margin)
        constexpr float MARGIN = 50.0f;
        return lastCharacterY > viewportHeight + MARGIN;
    }
}
