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
        m_position = position;

        // Random length between 5 and 30
        std::uniform_int_distribution<size_t> lengthDist(MIN_LENGTH, MAX_LENGTH);
        size_t length = lengthDist(g_generator);

        // Initialize characters (but only show them as they "drop")
        m_characters.clear();
        m_characters.reserve(length);

        CharacterSet& charSet = CharacterSet::GetInstance();
        charSet.Initialize(); // Ensure character set is initialized
        for (size_t i = 0; i < length; i++)
        {
            CharacterInstance character;
            character.glyphIndex = charSet.GetRandomGlyphIndex();
            character.color = Color4(0.0f, 1.0f, 0.0f, 1.0f); // Green
            character.brightness = 1.0f;
            character.scale = 1.0f;
            character.positionOffset = Vector2(0.0f, static_cast<float>(i) * m_characterSpacing);
            character.fadeTimer = 0.0f;

            m_characters.push_back(character);
        }

        // Set velocity based on depth (only used for horizontal drift now)
        std::uniform_real_distribution<float> driftDist(-5.0f, 5.0f);
        
        m_velocity.x = driftDist(g_generator);
        m_velocity.y = 0.0f; // Y velocity no longer used - we drop in discrete cells
        m_velocity.z = 0.0f;

        m_mutationTimer = 0.0f;
        m_dropTimer = 0.0f;
        m_currentLength = 1; // Start with just the head visible
    }

    void CharacterStreak::Update(float deltaTime)
    {
        // Apply horizontal drift (smooth)
        m_position.x += m_velocity.x * deltaTime;

        // Discrete cell dropping based on depth
        float velocityScale = GetVelocityScale(m_position.z);
        float dropInterval = BASE_DROP_INTERVAL / velocityScale; // Faster streaks drop more frequently
        
        m_dropTimer += deltaTime;
        if (m_dropTimer >= dropInterval)
        {
            m_dropTimer -= dropInterval;
            
            // Move down by one cell (discrete step)
            m_position.y += m_characterSpacing;
            
            // Extend the visible length if we haven't shown all characters yet
            if (m_currentLength < m_characters.size())
            {
                m_currentLength++;
            }
        }

        // Update character fades (only for visible characters)
        for (size_t i = 0; i < m_currentLength && i < m_characters.size(); i++)
        {
            m_characters[i].Update(deltaTime);
        }

        // Handle character mutation (5% probability per character per second)
        m_mutationTimer += deltaTime;
        
        // Check for mutations based on accumulated time
        std::uniform_real_distribution<float> mutationDist(0.0f, 1.0f);
        CharacterSet& charSet = CharacterSet::GetInstance();

        for (size_t i = 0; i < m_currentLength && i < m_characters.size(); i++)
        {
            CharacterInstance& character = m_characters[i];
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

        // The streak should despawn when the TOP character (last in array)
        // has moved below the viewport
        float topCharacterOffset = m_characters.back().positionOffset.y;
        float topCharacterY = m_position.y + topCharacterOffset;

        // Despawn when top character is below viewport (with some margin)
        constexpr float MARGIN = 50.0f;
        return topCharacterY > viewportHeight + MARGIN;
    }
}
