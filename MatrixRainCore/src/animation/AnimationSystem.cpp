#include "pch.h"

#include "MatrixRain/AnimationSystem.h"
#include "MatrixRain/DensityController.h"





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
        , m_previousTargetCount(0)
    {
    }

    void AnimationSystem::Initialize(const Viewport& viewport, DensityController& densityController)
    {
        m_viewport = &viewport;
        m_densityController = &densityController;
        m_streaks.clear();
        m_spawnTimer = 0.0f;
        
        // Calculate initial streak count based on target density for this viewport
        // Spawn them distributed throughout the viewport to prevent dark zone at top
        int targetCount = m_densityController->GetTargetStreakCount();
        m_previousTargetCount = targetCount;
        
        for (int i = 0; i < targetCount; i++)
        {
            // Every third streak spawns fully within view for better initial coverage
            SpawnStreakInView();
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
            int currentCount = static_cast<int>(m_streaks.size());
            int targetCount  = m_densityController->GetTargetStreakCount();
            
            // Remove excess streaks only if significantly above target
            // Small fluctuations (±5) are natural from spawning/despawning timing
            if (currentCount > targetCount + 5)
            {
                RemoveExcessStreaks(targetCount);
                m_previousTargetCount = targetCount;
            }
            // Spawn new streaks if below target
            else if (m_densityController->ShouldSpawnStreak(currentCount))
            {
                int deficit = targetCount - currentCount;
                
                // Detect density change: target increased from previous frame
                // BUT only spawn in-view if the percentage changed (not just viewport resize)
                // Viewport resize changes target but shouldn't burst-spawn on screen
                bool densityChanged = (targetCount > m_previousTargetCount) && 
                                     (currentCount > 0) && 
                                     (deficit > 10);  // Only burst if significant gap
                
                // Spawn ALL needed streaks immediately
                for (int i = 0; i < deficit; i++)
                {
                    if (densityChanged)
                    {
                        // Density increased significantly - spawn anywhere on screen
                        SpawnStreakInView();
                    }
                    else
                    {
                        // Replacing aged-off streaks or gradual fill - spawn at top
                        SpawnStreak();
                    }
                }
                
                m_previousTargetCount = targetCount;
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

#ifdef _DEBUG
        DebugCheckFrameDiff();
#endif
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




    void AnimationSystem::RemoveExcessStreaks(int targetCount)
    {
        int currentCount = static_cast<int>(m_streaks.size());
        if (currentCount <= targetCount)
        {
            return; // No excess to remove
        }

        // Calculate how many to remove
        int removeCount = currentCount - targetCount;

        // Sort streaks by Z depth (furthest first) - these are oldest/least visible
        std::sort(m_streaks.begin(), m_streaks.end(),
            [](const CharacterStreak& a, const CharacterStreak& b) {
                return a.GetPosition().z > b.GetPosition().z;
            });

        // Remove the furthest streaks
        m_streaks.erase(m_streaks.begin(), m_streaks.begin() + removeCount);
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




    void AnimationSystem::RescaleStreaksForViewport(float oldWidth, float newWidth, float oldHeight, float newHeight)
    {
        if (oldWidth <= 0.0f || newWidth <= 0.0f || oldHeight <= 0.0f || newHeight <= 0.0f)
        {
            return;
        }

        float scaleX = newWidth / oldWidth;
        float scaleY = newHeight / oldHeight;

        for (CharacterStreak& streak : m_streaks)
        {
            streak.RescalePositions(scaleX, scaleY);
        }
    }




    void AnimationSystem::ClearAllStreaks()
    {
        m_streaks.clear();
        m_previousTargetCount = 0;
    }




#ifdef _DEBUG
    void AnimationSystem::DebugCheckFrameDiff()
    {
        // Compare current frame to previous frame to detect whole-streak changes
        // Use unique IDs to track streaks across frames (position changes due to movement)
        
        // Build maps of streaks by ID
        std::map<uint64_t, const CharacterStreak*> prevStreakMap;
        std::map<uint64_t, const CharacterStreak*> currStreakMap;
        
        for (const CharacterStreak& streak : m_previousFrameStreaks)
        {
            prevStreakMap[streak.GetID()] = &streak;
        }
        
        for (const CharacterStreak& streak : m_streaks)
        {
            currStreakMap[streak.GetID()] = &streak;
        }
        
        // Check current streaks against previous
        for (const CharacterStreak& currentStreak : m_streaks)
        {
            uint64_t id = currentStreak.GetID();
            auto it = prevStreakMap.find (id);
            
            if (it == prevStreakMap.end())
            {
                // New streak appeared
                size_t charCount = currentStreak.GetCharacterCount();
                
                if (charCount > 5)
                {
                    Vector3 pos = currentStreak.GetPosition();
                    
                    DEBUGMSG (L"NEW STREAK: ID=%llu, Count=%zu, Pos=(%.1f, %.1f, %.1f)\n",
                              id, charCount, pos.x, pos.y, pos.z);
                    
                    ASSERT (charCount <= 10);  // Whole streak appeared instantly!
                }
            }
            else
            {
                // Streak exists in both frames - check for sudden character count changes
                const CharacterStreak* prevStreak = it->second;
                
                size_t prevCount = prevStreak->GetCharacterCount();
                size_t currCount = currentStreak.GetCharacterCount();
                int delta = abs (static_cast<int> (currCount) - static_cast<int> (prevCount));
                
                if (delta > 3)
                {
                    Vector3 pos = currentStreak.GetPosition();
                    
                    DEBUGMSG (L"COUNT JUMP: ID=%llu, %zu->%zu (delta=%d), Pos=(%.1f, %.1f, %.1f)\n",
                              id, prevCount, currCount, delta, pos.x, pos.y, pos.z);
                    
                    ASSERT (delta <= 3);  // Character count shouldn't jump suddenly
                }
            }
        }
        
        // Check for streaks that disappeared entirely
        int removedStreakCount = 0;
        for (const CharacterStreak& prevStreak : m_previousFrameStreaks)
        {
            uint64_t id = prevStreak.GetID();
            
            if (currStreakMap.find (id) == currStreakMap.end())
            {
                // Streak disappeared
                size_t charCount = prevStreak.GetCharacterCount();
                removedStreakCount++;
                
                if (charCount > 5)
                {
                    Vector3 pos = prevStreak.GetPosition();
                    
                    DEBUGMSG (L"REMOVED STREAK: ID=%llu, Count=%zu, Pos=(%.1f, %.1f, %.1f)\n",
                              id, charCount, pos.x, pos.y, pos.z);
                    
                    // Only assert if we're NOT over target density
                    // RemoveExcessStreaks legitimately removes whole streaks when density is reduced
                    int prevCount = static_cast<int>(m_previousFrameStreaks.size());
                    int targetCount = m_densityController ? m_densityController->GetTargetStreakCount() : prevCount;
                    bool wasOverTarget = prevCount > targetCount;
                    
                    ASSERT (charCount <= 10 || wasOverTarget);  // Whole streak disappeared (allowed if over target)
                }
            }
        }
        
        // Verify we didn't remove MORE streaks than necessary
        if (m_densityController && removedStreakCount > 0)
        {
            int prevStreakCount = static_cast<int>(m_previousFrameStreaks.size());
            int targetStreakCount = m_densityController->GetTargetStreakCount();
            
            // If we were over target, verify we didn't remove more than needed
            if (prevStreakCount > targetStreakCount)
            {
                int expectedRemoval = prevStreakCount - targetStreakCount;
                
                // Allow some variance (e.g., offscreen despawns can happen at same time)
                // But if we removed significantly more than expected, that's a bug
                ASSERT (removedStreakCount <= expectedRemoval + 5);  // +5 tolerance for concurrent despawns
            }
        }
        
        // Save current frame as previous for next comparison
        m_previousFrameStreaks = m_streaks;
    }
#endif
}




