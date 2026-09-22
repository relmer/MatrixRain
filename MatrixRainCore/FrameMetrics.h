#pragma once





/// <summary>
/// Numbers that describe a rendered frame, so the look of the rain can be
/// compared across a pipeline change by measurement rather than by eye.
///
/// The calibration harness renders one deterministic frame (see RandomSource)
/// before and after a change and compares these two metrics per settings case:
/// MeanLuminance catches an overall shift in exposure, HaloFalloffRadius
/// catches a change in the size of the glow around a streak head. Both are
/// computed in LINEAR light, because that is where "twice as bright" means
/// twice as much light.
///
/// Each metric takes either a read-back 8-bit BGRA back buffer, whose channels
/// are decoded from sRGB first, or a buffer that is already linear float RGBA.
/// </summary>

/// <summary>
/// Average Rec.709 luminance of every pixel in the frame, in linear light.
/// </summary>
/// <param name="bgra">Frame bytes, 4 per pixel in B, G, R, A order, tightly packed</param>
/// <param name="width">Frame width in pixels</param>
/// <param name="height">Frame height in pixels</param>
/// <returns>Mean linear luminance, or 0 when the buffer is empty or too short</returns>
float MeanLuminance (std::span<const uint8_t> bgra, UINT width, UINT height) noexcept;


/// <summary>
/// Average Rec.709 luminance of a frame that is already in linear light.
/// </summary>
/// <param name="rgba">Frame samples, 4 floats per pixel in R, G, B, A order, tightly packed</param>
/// <param name="width">Frame width in pixels</param>
/// <param name="height">Frame height in pixels</param>
/// <returns>Mean linear luminance, or 0 when the buffer is empty or too short</returns>
float MeanLuminance (std::span<const float> rgba, UINT width, UINT height) noexcept;





/// <summary>
/// Distance from center, in pixels, at which the glow around it has fallen to
/// half of its peak linear luminance -- the half-maximum radius, the standard
/// way to state the width of a falloff that never truly ends.
///
/// Luminance is averaged over thin rings around the center before the crossing
/// is found, so an asymmetric or slightly noisy halo still yields a stable
/// radius, and the crossing itself is interpolated between rings so the result
/// is not quantised to whole pixels.
/// </summary>
/// <param name="bgra">Frame bytes, 4 per pixel in B, G, R, A order, tightly packed</param>
/// <param name="width">Frame width in pixels</param>
/// <param name="height">Frame height in pixels</param>
/// <param name="center">Pixel the halo is centred on, in frame coordinates</param>
/// <returns>Half-maximum radius in pixels, or 0 when the frame is unusable or never falls to half</returns>
float HaloFalloffRadius (std::span<const uint8_t> bgra, UINT width, UINT height, POINT center) noexcept;


/// <summary>
/// Half-maximum radius of the glow around center, for a frame that is already
/// in linear light. See the 8-bit overload for what the radius means.
/// </summary>
/// <param name="rgba">Frame samples, 4 floats per pixel in R, G, B, A order, tightly packed</param>
/// <param name="width">Frame width in pixels</param>
/// <param name="height">Frame height in pixels</param>
/// <param name="center">Pixel the halo is centred on, in frame coordinates</param>
/// <returns>Half-maximum radius in pixels, or 0 when the frame is unusable or never falls to half</returns>
float HaloFalloffRadius (std::span<const float> rgba, UINT width, UINT height, POINT center) noexcept;
