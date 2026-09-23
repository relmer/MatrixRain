#pragma once

#include "Math.h"





/// <summary>
/// The constants of the sRGB transfer function (IEC 61966-2-1), gathered in
/// one place because they are needed twice: once by this module, and once by
/// the HLSL output transform, which must be a line-for-line transliteration of
/// LinearToSrgb. Two copies of a number are a bug waiting to be written, so
/// the shader source references these by value.
/// </summary>
namespace ColorMathConstants
{
    /// <summary>Encoded value below which the transfer function is a straight line.</summary>
    inline constexpr float kEncodedKnee  = 0.04045f;

    /// <summary>Linear value below which the transfer function is a straight line.</summary>
    inline constexpr float kLinearKnee   = 0.0031308f;

    /// <summary>Slope of that straight segment.</summary>
    inline constexpr float kLinearSlope  = 12.92f;

    /// <summary>Offset of the curved segment.</summary>
    inline constexpr float kCurveOffset  = 0.055f;

    /// <summary>Scale of the curved segment.</summary>
    inline constexpr float kCurveScale   = 1.055f;

    /// <summary>Exponent of the curved segment, decoding to linear.</summary>
    inline constexpr float kCurveGamma   = 2.4f;
}





/// <summary>
/// Decodes an sRGB-encoded value to linear light.
///
/// This is the conversion that makes arithmetic on colour mean anything.
/// Encoded values are not proportional to light -- 0.5 carries about 21.4% of
/// the light of 1.0, not 50% -- because the curve spends its limited precision
/// where the eye is sensitive. Blending, blurring and adding are only
/// physically correct once that curve is undone.
///
/// Inputs outside [0, 1] are returned through the same formulas rather than
/// clamped: linear light above 1 is meaningful (it is what HDR highlights are
/// made of), and clamping here would quietly destroy it.
/// </summary>
/// <param name="encoded">sRGB-encoded value, normally in [0, 1]</param>
/// <returns>Linear-light value</returns>
float SrgbToLinear (float encoded) noexcept;





/// <summary>
/// Encodes a linear-light value back to sRGB, for handing to an SDR display.
///
/// The input IS clamped to [0, 1], unlike SrgbToLinear: this is the last step
/// before an 8-bit buffer that cannot represent anything outside that range,
/// so the clamp is the honest description of what the display will do anyway.
/// </summary>
/// <param name="linear">Linear-light value; values outside [0, 1] are clamped</param>
/// <returns>sRGB-encoded value in [0, 1]</returns>
float LinearToSrgb (float linear) noexcept;





/// <summary>
/// The 30% self-glow the glyph shader has always added to a character's own
/// colour, scaled by its brightness so only bright heads get much of it.
/// </summary>
inline constexpr float kGlyphSelfGlow = 0.3f;





/// <summary>
/// Converts a glyph's final displayed colour into the linear-light value the
/// GPU should blend with.
///
/// This exists to keep v1.6's look exactly (FR-005). The v1.6 glyph pixel
/// shader computed its colour as
///     rgb = color * texture * brightness * (1 + 0.3 * brightness)
/// in gamma space. Doing that same arithmetic in linear light would change the
/// result, because the brightness and self-glow terms are not linear
/// operations. So the terms stay where they are -- applied to the sRGB colour,
/// on the CPU, once per instance -- and only the FINISHED colour is converted.
/// The glyph core therefore comes out identical to v1.6, and linear light
/// governs only what happens afterwards: the blending, the blur and the bloom,
/// which is where it belongs.
///
/// Alpha is passed through untouched. It carries the trail's fade, which the
/// shader still applies in the same place it always did.
/// </summary>
/// <param name="srgbColor">The glyph's colour as v1.6 would have displayed it</param>
/// <param name="brightness">Character brightness in [0, 1]</param>
/// <param name="highlightGain">Multiplier for HDR highlights; 1 outside HDR (Phase 3)</param>
/// <returns>Linear-light colour, with alpha unchanged</returns>
Color4 InstanceLinearColor (const Color4 & srgbColor, float brightness, float highlightGain) noexcept;
