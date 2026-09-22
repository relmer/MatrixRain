#pragma once





/// <summary>
/// Numbers that describe a rendered frame, so the look of the rain can be
/// compared across a pipeline change by measurement rather than by eye.
///
/// The calibration harness renders one deterministic frame (see RandomSource)
/// before and after a change and compares them. The comparison itself is the
/// primary signal: CompareFrames reduces two frames of the same scene to how
/// far apart they are and where. MeanLuminance sits alongside it as a headline
/// -- one number saying whether the scene as a whole got brighter or darker.
///
/// An earlier revision also reported the half-maximum radius of the glow
/// around a streak head. It was retired: a streak head is a saturated glyph,
/// so half of its peak is reached at the edge of the letter's own ink, and the
/// radius reported the stroke width rather than the glow. Comparing the frames
/// directly needs no such proxy, and shows glow SHAPE as well as size.
/// </summary>

/// <summary>
/// Average Rec.709 luminance of every pixel in the frame, in linear light --
/// linear because that is where "twice as bright" means twice as much light,
/// so averaging is meaningful.
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
/// How far two renderings of the same scene have drifted apart.
///
/// Differences are stated in 8-bit code values on the 0..255 scale, NOT in
/// linear light, because the question this answers is "would anyone see it".
/// The frame being compared is the one the display receives, and the sRGB
/// curve exists precisely so that a step of one code value is about equally
/// visible in shadow and in highlight. A difference of 1 or 2 is invisible; 20
/// is obvious. In linear light the same judgement would need a different
/// threshold for every brightness level.
/// </summary>
struct FrameDifference
{
    float  m_maxDifference       = 0.0f;   // Largest single-channel difference anywhere
    float  m_meanDifference      = 0.0f;   // Mean per-pixel difference over the whole frame
    float  m_p99Difference       = 0.0f;   // 99th percentile, so a handful of outliers cannot hide
    size_t m_pixelsOverThreshold = 0;      // Pixels whose difference exceeded the caller's threshold
    POINT  m_maxDifferenceAt     = { 0, 0 };  // Where the largest difference is, for going and looking
};





/// <summary>
/// Compares a candidate frame against a baseline frame of the same scene.
///
/// Each pixel's difference is the largest absolute difference across its blue,
/// green and red channels, so a change of hue counts even when overall
/// brightness holds steady. Alpha is ignored: it is not displayed.
/// </summary>
/// <param name="baseline">Baseline frame, 4 bytes per pixel in B, G, R, A order</param>
/// <param name="candidate">Candidate frame, same size and layout</param>
/// <param name="width">Frame width in pixels</param>
/// <param name="height">Frame height in pixels</param>
/// <param name="threshold">Difference, in code values, above which a pixel is counted as changed</param>
/// <returns>The difference summary, all zeroes when either buffer is empty or too short</returns>
FrameDifference CompareFrames (std::span<const uint8_t> baseline,
                               std::span<const uint8_t> candidate,
                               UINT                     width,
                               UINT                     height,
                               float                    threshold) noexcept;
