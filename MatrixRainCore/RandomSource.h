#pragma once





/// <summary>
/// The one random engine every rain object draws from, one instance per
/// thread.
///
/// Each thread gets its own std::mt19937 seeded from std::random_device, so
/// the render thread and the UI thread never share mutable generator state --
/// the same guarantee the per-thread generators this replaced already gave.
/// What is new is Reseed: with a fixed seed the whole animation becomes
/// reproducible, which is what lets the calibration harness render the same
/// frame before and after a pipeline change and compare the two numerically
/// instead of by eye.
///
/// Nothing seeds the engine explicitly at runtime, so the screensaver stays
/// as random as it was.
/// </summary>
namespace RandomSource
{
    /// <summary>
    /// The calling thread's generator. The first call on a thread seeds it
    /// from std::random_device unless Reseed has already run on that thread.
    /// </summary>
    /// <returns>Reference to this thread's engine, valid for the thread's lifetime</returns>
    std::mt19937 & Engine() noexcept;


    /// <summary>
    /// Restart the calling thread's generator from a known seed, making every
    /// subsequent draw on that thread reproducible. Affects no other thread.
    /// </summary>
    /// <param name="seed">Seed value</param>
    void Reseed (uint32_t seed) noexcept;
}
