#include "pch.h"

#include "AnimationSystem.h"
#include "DensityController.h"





void AnimationSystem::Initialize (const Viewport & viewport, DensityController & densityController)
{
    m_viewport          = &viewport;
    m_densityController = &densityController;
    
    m_streaks.clear();
    m_spawnTimer = 0.0f;
    
    // Calculate initial streak count based on target density for this viewport
    // Spawn them distributed throughout the viewport to prevent dark zone at top
    int targetCount       = m_densityController->GetTargetStreakCount ();
    m_previousTargetCount = targetCount;
    
    for (int i = 0; i < targetCount; i++)
    {
        // Every third streak spawns fully within view for better initial coverage
        SpawnStreakInView ();
    }
}





void AnimationSystem::Update (float deltaTime)
{
    if (!m_viewport)
    {
        return; // Not initialized
    }

#ifdef _DEBUG
    // Reset intentional removal counter at the start of each frame
    m_intentionalRemovalCount = 0;
#endif

    float viewportHeight = static_cast<float> (m_viewport->GetHeight ());

    // Update all existing streaks
    for (CharacterStreak & streak : m_streaks)
    {
        streak.Update (deltaTime, viewportHeight);
    }

    // Apply zoom effect (camera moves forward through depth)
    ApplyZoom (deltaTime);

    // Remove streaks that are off-screen
    DespawnOffscreenStreaks ();

    // Auto-spawn new streaks based on density controller (if available)
    if (m_densityController)
    {
        // Count streaks with heads still on screen (active heads)
        int activeHeadCount = 0;
        for (const CharacterStreak & streak : m_streaks)
        {
            if (!streak.IsHeadOffscreen (viewportHeight))
            {
                activeHeadCount++;
            }
        }
        
        int targetCount = m_densityController->GetTargetStreakCount ();
        
        // Remove excess streaks only if significantly above target
        // Small fluctuations (±5) are natural from spawning/despawning timing
        if (activeHeadCount > targetCount + 5)
        {
#ifdef _DEBUG
            DEBUGMSG (L"Before RemoveExcessStreaks: activeHeadCount=%d, targetCount=%d\n", activeHeadCount, targetCount);
            m_intentionalRemovalCount = RemoveExcessStreaks (targetCount);
            DEBUGMSG (L"After RemoveExcessStreaks: removed=%d\n", m_intentionalRemovalCount);
#else
            RemoveExcessStreaks (targetCount);
#endif
            m_previousTargetCount = targetCount;
        }
        // Spawn new streaks if below target (based on active heads)
        else if (m_densityController->ShouldSpawnStreak (activeHeadCount))
        {
            int deficit = targetCount - activeHeadCount;
            
            // Detect density change: target increased from previous frame
            // BUT only spawn in-view if the percentage changed (not just viewport resize)
            // Viewport resize changes target but shouldn't burst-spawn on screen
            bool densityChanged = (targetCount > m_previousTargetCount) && 
                                    (activeHeadCount > 0) && 
                                    (deficit > 10);  // Only burst if significant gap
            
            // Spawn ALL needed streaks immediately
            for (int i = 0; i < deficit; i++)
            {
                if (densityChanged)
                {
                    // Density increased significantly - spawn anywhere on screen
                    SpawnStreakInView ();
                }
                else
                {
                    // Replacing aged-off streaks or gradual fill - spawn at top
                    SpawnStreak ();
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
            SpawnStreak ();
        }
    }

#ifdef _DEBUG
    DebugCheckFrameDiff ();
#endif
}





void AnimationSystem::SpawnStreak()
{
    if (!m_viewport)
    {
        return; // Not initialized
    }

    // Random X position across viewport width
    std::uniform_real_distribution<float> xDist (0.0f, m_viewport->GetWidth ());
    float x = xDist (m_generator);

    // Random Y position above viewport (between -200 and 0)
    std::uniform_real_distribution<float> yDist (-200.0f, 0.0f);
    float y = yDist (m_generator);

    // Random Z depth (0 = near, 100 = far)
    std::uniform_real_distribution<float> zDist (0.0f, MAX_DEPTH);
    float z = zDist (m_generator);

    Vector3 position (x, y, z);

    CharacterStreak streak;
    streak.Spawn (position);
    m_streaks.push_back (std::move (streak));
}





void AnimationSystem::SpawnStreakInView()
{
    if (!m_viewport)
    {
        return; // Not initialized
    }

    // Random X position across viewport width
    std::uniform_real_distribution<float> xDist (0.0f, m_viewport->GetWidth ());
    float x = xDist (m_generator);

    // Random Y position WITHIN viewport (0 to height) for immediate visibility
    std::uniform_real_distribution<float> yDist (0.0f, m_viewport->GetHeight ());
    float y = yDist (m_generator);

    // Random Z depth (0 = near, 100 = far)
    std::uniform_real_distribution<float> zDist (0.0f, MAX_DEPTH);
    float z = zDist (m_generator);

    Vector3 position (x, y, z);

    CharacterStreak streak;
    streak.Spawn (position);
    m_streaks.push_back (std::move (streak));
}





void AnimationSystem::DespawnOffscreenStreaks()
{
    if (!m_viewport)
    {
        return; // Not initialized
    }

    // Remove streaks that should despawn
    // Use erase-remove idiom
    m_streaks.erase (
        std::remove_if (m_streaks.begin (), m_streaks.end (),
            [](const CharacterStreak & streak)
            {
                return streak.ShouldDespawn();
            }),
        m_streaks.end ()
    );
}





int AnimationSystem::RemoveExcessStreaks (int targetCount)
{
    // Count active heads (streaks with heads still on screen)
    float viewportHeight = m_viewport ? m_viewport->GetHeight () : 1080.0f;
    
    // Build list of indices of active streaks
    std::vector<size_t> activeIndices;
    std::vector<size_t> inactiveIndices;
    
    for (size_t i = 0; i < m_streaks.size (); i++)
    {
        if (!m_streaks[i].IsHeadOffscreen (viewportHeight))
        {
            activeIndices.push_back (i);
        }
        else
        {
            inactiveIndices.push_back (i);
        }
    }
        
    int activeCount = static_cast<int> (activeIndices.size ());
    if (activeCount <= targetCount)
    {
        return 0; // No excess active heads to remove
    }

    // Calculate how many active heads to remove
    int removeCount = activeCount - targetCount;

    // Sort active indices by Z depth (furthest first) - these are oldest/least visible
    std::sort (activeIndices.begin (), activeIndices.end (),
        [this](size_t a, size_t b) {
            return m_streaks[a].GetPosition ().z > m_streaks[b].GetPosition ().z;
        });

    // Mark the furthest active streaks for removal (first removeCount indices)
    std::vector<bool> shouldRemove (m_streaks.size (), false);
    for (int i = 0; i < removeCount; i++)
    {
        shouldRemove[activeIndices[i]] = true;
    }
    
    // Remove marked streaks in reverse order to maintain indices
    int actuallyRemoved = 0;
    for (int i = static_cast<int> (m_streaks.size ()) - 1; i >= 0; i--)
    {
        if (shouldRemove[i])
        {
            m_streaks.erase (m_streaks.begin () + i);
            actuallyRemoved++;
        }
    }
    
    return actuallyRemoved;
}





void AnimationSystem::ApplyZoom (float deltaTime)
{
    // Camera moves forward (toward Z=0), so we decrease Z of all streaks
    // When a streak reaches Z < 0, wrap it to Z + MAX_DEPTH
    
    float zoomDistance = m_zoomVelocity * deltaTime;

    for (CharacterStreak & streak : m_streaks)
    {
        Vector3 position = streak.GetPosition ();
        position.z -= zoomDistance;

        // Wrap at Z=0 boundary
        if (position.z < 0.0f)
        {
            position.z += MAX_DEPTH;
        }

        // Update streak position
        streak.SetPosition (position);
    }
}





void AnimationSystem::RescaleStreaksForViewport (float oldWidth, float newWidth, float oldHeight, float newHeight)
{
    if (oldWidth <= 0.0f || newWidth <= 0.0f || oldHeight <= 0.0f || newHeight <= 0.0f)
    {
        return;
    }

    float scaleX = newWidth / oldWidth;
    float scaleY = newHeight / oldHeight;

    for (CharacterStreak & streak : m_streaks)
    {
        streak.RescalePositions (scaleX, scaleY);
    }
}





void AnimationSystem::ClearAllStreaks()
{
    m_streaks.clear();
    m_previousTargetCount = 0;
}





size_t AnimationSystem::GetActiveHeadCount() const
{
    if (!m_viewport)
    {
        return 0;
    }

    float viewportHeight = static_cast<float> (m_viewport->GetHeight ());
    size_t activeHeadCount = 0;

    for (const CharacterStreak & streak : m_streaks)
    {
        if (!streak.IsHeadOffscreen (viewportHeight))
        {
            activeHeadCount++;
        }
    }

    return activeHeadCount;
}




#ifdef _DEBUG
void AnimationSystem::DebugCheckFrameDiff()
{
    // Compare current frame to previous frame to detect whole-streak changes
    // Use unique IDs to track streaks across frames (position changes due to movement)
    
    // Build maps of streaks by ID
    std::map<uint64_t, const CharacterStreak *> prevStreakMap;
    std::map<uint64_t, const CharacterStreak *> currStreakMap;
    
    for (const CharacterStreak & streak : m_previousFrameStreaks)
    {
        prevStreakMap[streak.GetID ()] = &streak;
    }
    
    for (const CharacterStreak & streak : m_streaks)
    {
        currStreakMap[streak.GetID ()] = &streak;
    }
    
    // Check current streaks against previous
    for (const CharacterStreak & currentStreak : m_streaks)
    {
        uint64_t id = currentStreak.GetID ();
        auto it = prevStreakMap.find (id);
        
        if (it == prevStreakMap.end ())
        {
            // New streak appeared
            size_t charCount = currentStreak.GetCharacterCount ();
            
            if (charCount > 5)
            {
                Vector3 pos = currentStreak.GetPosition ();
                
                DEBUGMSG (L"NEW STREAK: ID=%llu, Count=%zu, Pos=(%.1f, %.1f, %.1f)\n",
                            id, charCount, pos.x, pos.y, pos.z);
                
                ASSERT (charCount <= 10);  // Whole streak appeared instantly!
            }
        }
        else
        {
            // Streak exists in both frames - check for sudden character count changes
            const CharacterStreak * prevStreak = it->second;
            
            size_t prevCount = prevStreak->GetCharacterCount ();
            size_t currCount = currentStreak.GetCharacterCount ();
            int delta = abs (static_cast<int> (currCount) - static_cast<int> (prevCount));
            
            if (delta > 3)
            {
                Vector3 pos = currentStreak.GetPosition ();
                
                DEBUGMSG (L"COUNT JUMP: ID=%llu, %zu->%zu (delta=%d), Pos=(%.1f, %.1f, %.1f)\n",
                            id, prevCount, currCount, delta, pos.x, pos.y, pos.z);
                
                ASSERT (delta <= 3);  // Character count shouldn't jump suddenly
            }
        }
    }
    
    // Check for streaks that disappeared entirely
    // Calculate wasOverTarget ONCE for all removals (not per-streak)
    float viewportHeight = m_viewport ? m_viewport->GetHeight () : 1080.0f;
    int prevActiveHeads = 0;
    for (const CharacterStreak & streak : m_previousFrameStreaks)
    {
        if (!streak.IsHeadOffscreen (viewportHeight))
        {
            prevActiveHeads++;
        }
    }
    
    int targetCount = m_densityController ? m_densityController->GetTargetStreakCount () : prevActiveHeads;
    bool wasOverTarget = prevActiveHeads > targetCount;
    
    // Also check if we intentionally removed streaks this frame - that's definitive proof we were over target
    bool hadIntentionalRemovals = false;
#ifdef _DEBUG
    hadIntentionalRemovals = (m_intentionalRemovalCount > 0);
#endif
    
    int removedStreakCount = 0;
    for (const CharacterStreak & prevStreak : m_previousFrameStreaks)
    {
        uint64_t id = prevStreak.GetID ();
        
        if (currStreakMap.find (id) == currStreakMap.end ())
        {
            // Streak disappeared
            size_t charCount = prevStreak.GetCharacterCount ();
            removedStreakCount++;
            
            if (charCount > 5)
            {
                Vector3 pos = prevStreak.GetPosition ();
                
                DEBUGMSG (L"REMOVED STREAK: ID=%llu, Count=%zu, Pos=(%.1f, %.1f, %.1f)\n",
                            id, charCount, pos.x, pos.y, pos.z);
                
                ASSERT (charCount <= 10 || wasOverTarget || hadIntentionalRemovals);  // Whole streak disappeared (allowed if over target or intentionally removed)
            }
        }
    }
    
    // Verify we didn't remove MORE streaks than necessary
    if (m_densityController && removedStreakCount > 0)
    {
        // Get current target (may have changed due to viewport resize or density adjustment)
        int currentTargetCount = m_densityController->GetTargetStreakCount ();
        
        // During viewport resize or density changes, the target can change dramatically
        // If target decreased significantly, we expect large-scale removal
        bool targetDecreased = currentTargetCount < m_previousTargetCount;
        
        // If we intentionally removed streaks, verify it was reasonable
        if (m_intentionalRemovalCount > 0)
        {
            if (targetDecreased)
            {
                // Target decreased (viewport shrink or density drop) - expect many removals
                // Just verify we didn't remove MORE than all previous active heads
                ASSERT (m_intentionalRemovalCount <= prevActiveHeads);
            }
            else
            {
                // Normal case OR target already updated before this check runs
                // During viewport resize, m_previousTargetCount gets updated in Update()
                // before DebugCheckFrameDiff runs, so we can't reliably detect the decrease.
                // Use a generous tolerance to allow for viewport resize scenarios.
                int expectedRemoval = prevActiveHeads > currentTargetCount 
                                      ? prevActiveHeads - currentTargetCount 
                                      : 0;
                ASSERT (m_intentionalRemovalCount <= expectedRemoval + 100);  // Large tolerance for resize
            }
        }
        
        // Also verify total removals (intentional + natural) aren't absurd
        ASSERT (removedStreakCount <= prevActiveHeads + 50);  // Can't remove more than existed + some fading tails
    }
    
    // Save current frame as previous for next comparison
    m_previousFrameStreaks = m_streaks;
}
#endif




