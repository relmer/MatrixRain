#include "pch.h"
#include "matrixrain/AnimationSystem.h"
#include "matrixrain/DensityController.h"
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
        , m_densityController(nullptr)
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
        // Reduced density for less cluttered appearance
        size_t initialStreaks = 15;
        for (size_t i = 0; i < initialStreaks; i++)
        {
            SpawnStreak();
        }
    }

    void AnimationSystem::Initialize(const Viewport& viewport, DensityController& densityController)
    {
        m_viewport = &viewport;
        m_densityController = &densityController;
        m_streaks.clear();
        m_spawnTimer = 0.0f;
        
        // Spawn small initial set - let automatic spawn logic reach target density over time
        // This prevents overwhelming the screen at startup
        size_t initialStreaks = 15;
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

        float viewportHeight = static_cast<float>(m_viewport->GetHeight());

        // Update all existing streaks
        for (CharacterStreak& streak : m_streaks)
        {
            streak.Update(deltaTime, viewportHeight);
        }

        // Apply zoom effect (camera moves forward through depth)
        ApplyZoom(deltaTime);

        // Remove streaks that are off-screen
        DespawnOffscreenStreaks();

        // Auto-spawn new streaks based on density controller (if available)
        if (m_densityController)
        {
            // Use density controller to determine if we should spawn
            int currentCount = static_cast<int>(m_streaks.size());
            if (m_densityController->ShouldSpawnStreak(currentCount))
            {
                m_spawnTimer += deltaTime;
                if (m_spawnTimer >= m_spawnInterval)
                {
                    m_spawnTimer -= m_spawnInterval;
                    SpawnStreak();
                }
            }
        }
        else
        {
            // Fallback: auto-spawn to maintain fixed density
            m_spawnTimer += deltaTime;
            if (m_spawnTimer >= m_spawnInterval)
            {
                m_spawnTimer -= m_spawnInterval;
                SpawnStreak();
            }
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




    void AnimationSystem::SpawnStreakInView()
    {
        if (!m_viewport)
        {
            return; // Not initialized
        }

        // Random X position across viewport width
        std::uniform_real_distribution<float> xDist(0.0f, m_viewport->GetWidth());
        float x = xDist(g_generator);

        // Random Y position WITHIN viewport (0 to height) for immediate visibility
        std::uniform_real_distribution<float> yDist(0.0f, m_viewport->GetHeight());
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




    void AnimationSystem::RescaleStreaksForViewport(float oldWidth, float newWidth)
    {
        if (oldWidth <= 0.0f || newWidth <= 0.0f)
        {
            return;
        }

        float scaleRatio = newWidth / oldWidth;

        for (CharacterStreak& streak : m_streaks)
        {
            Vector3 position = streak.GetPosition();
            position.x *= scaleRatio;
            streak.SetPosition(position);
        }
    }
}

