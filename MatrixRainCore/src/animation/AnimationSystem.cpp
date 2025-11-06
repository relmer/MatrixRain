#include "pch.h"
#include "matrixrain/AnimationSystem.h"
#include <random>

namespace MatrixRain
{
    namespace
    {
        // Random number generator
        std::random_device g_randomDevice;
        std::mt19937 g_generator(g_randomDevice());
    }

    AnimationSystem::AnimationSystem()
        : m_viewport(nullptr)
        , m_zoomVelocity(DEFAULT_ZOOM_VELOCITY)
        , m_spawnTimer(0.0f)
        , m_spawnInterval(SPAWN_INTERVAL)
    {
    }

    void AnimationSystem::Initialize(const Viewport& viewport)
    {
        m_viewport = &viewport;
        m_streaks.clear();
        m_spawnTimer = 0.0f;
        
        // Spawn initial set of streaks to fill the screen
        // Estimate: ~20-30 streaks for a 1920x1080 viewport
        size_t initialStreaks = 25;
        for (size_t i = 0; i < initialStreaks; i++)
        {
            SpawnStreak();
        }
    }

    void AnimationSystem::Update(float deltaTime)
    {
        if (!m_viewport)
        {
            return; // Not initialized
        }

        // Update all existing streaks
        for (CharacterStreak& streak : m_streaks)
        {
            streak.Update(deltaTime);
        }

        // Apply zoom effect (camera moves forward through depth)
        ApplyZoom(deltaTime);

        // Remove streaks that are off-screen
        DespawnOffscreenStreaks();

        // Auto-spawn new streaks to maintain density
        m_spawnTimer += deltaTime;
        if (m_spawnTimer >= m_spawnInterval)
        {
            m_spawnTimer -= m_spawnInterval;
            SpawnStreak();
        }
    }

    void AnimationSystem::SpawnStreak()
    {
        if (!m_viewport)
        {
            return; // Not initialized
        }

        // Random X position across viewport width
        std::uniform_real_distribution<float> xDist(0.0f, m_viewport->GetWidth());
        float x = xDist(g_generator);

        // Random Y position above viewport (between -200 and 0)
        std::uniform_real_distribution<float> yDist(-200.0f, 0.0f);
        float y = yDist(g_generator);

        // Random Z depth (0 = near, 100 = far)
        std::uniform_real_distribution<float> zDist(0.0f, MAX_DEPTH);
        float z = zDist(g_generator);

        Vector3 position(x, y, z);

        CharacterStreak streak;
        streak.Spawn(position);
        m_streaks.push_back(streak);
    }

    void AnimationSystem::DespawnOffscreenStreaks()
    {
        if (!m_viewport)
        {
            return; // Not initialized
        }

        float viewportHeight = m_viewport->GetHeight();

        // Remove streaks that should despawn
        // Use erase-remove idiom
        m_streaks.erase(
            std::remove_if(m_streaks.begin(), m_streaks.end(),
                [viewportHeight](const CharacterStreak& streak)
                {
                    return streak.ShouldDespawn(viewportHeight);
                }),
            m_streaks.end()
        );
    }

    void AnimationSystem::ApplyZoom(float deltaTime)
    {
        // Camera moves forward (toward Z=0), so we decrease Z of all streaks
        // When a streak reaches Z < 0, wrap it to Z + MAX_DEPTH
        
        float zoomDistance = m_zoomVelocity * deltaTime;

        for (CharacterStreak& streak : m_streaks)
        {
            Vector3 position = streak.GetPosition();
            position.z -= zoomDistance;

            // Wrap at Z=0 boundary
            if (position.z < 0.0f)
            {
                position.z += MAX_DEPTH;
            }

            // Update streak position
            streak.SetPosition(position);
        }
    }
}
