#pragma once

#include "Math.h"
#include "Shaders\ColorConstants.h"





/// <summary>
/// The constants of the sRGB transfer function (IEC 61966-2-1), as typed C++
/// values.
///
/// Every number here comes from Shaders/ColorConstants.h, which FXC compiles
/// into OutputTransform.hlsli as well. That indirection is the whole point:
/// the HLSL output transform has to be a line-for-line transliteration of
/// LinearToSrgb, and a second copy of a constant is a bug waiting to be
/// written. Editing the shared header moves both sides at once.
/// </summary>
namespace ColorMathConstants
{
    /// <summary>Encoded value below which the transfer function is a straight line.</summary>
    inline constexpr float kEncodedKnee  = MR_SRGB_ENCODED_KNEE;

    /// <summary>Linear value below which the transfer function is a straight line.</summary>
    inline constexpr float kLinearKnee   = MR_SRGB_LINEAR_KNEE;

    /// <summary>Slope of that straight segment.</summary>
    inline constexpr float kLinearSlope  = MR_SRGB_LINEAR_SLOPE;

    /// <summary>Offset of the curved segment.</summary>
    inline constexpr float kCurveOffset  = MR_SRGB_CURVE_OFFSET;

    /// <summary>Scale of the curved segment.</summary>
    inline constexpr float kCurveScale   = MR_SRGB_CURVE_SCALE;

    /// <summary>Exponent of the curved segment, decoding to linear.</summary>
    inline constexpr float kCurveGamma   = MR_SRGB_CURVE_GAMMA;
}





/// <summary>
/// Decodes an sRGB-encoded value to linear light.
///
/// This is the conversion that makes arithmetic on color mean anything.
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
/// color, scaled by its brightness so only bright heads get much of it.
/// </summary>
inline constexpr float kGlyphSelfGlow = 0.3f;





/// <summary>
/// Converts a glyph's final displayed color into the linear-light value the
/// GPU should blend with.
///
/// This exists to keep v1.6's look exactly (FR-005). The v1.6 glyph pixel
/// shader wrote
///     rgb = color * texture * brightness * (1 + 0.3 * brightness)
///     a   = texture.a * brightness
/// in gamma space and alpha-blended over a black scene, so the pixel a viewer
/// saw was color * brightness * (1 + 0.3 * brightness) * brightness. Doing
/// that arithmetic in linear light would change the result, because neither
/// the brightness terms nor the self-glow are linear operations. So all of it
/// stays in gamma space, applied to the sRGB color on the CPU once per
/// instance, and only the FINISHED displayed color is converted. The glyph
/// therefore comes out identical to v1.6, and linear light governs only what
/// happens afterwards: the blur and the bloom, which is where it belongs.
///
/// The shader's alpha is coverage alone as a result. Brightness must NOT be
/// applied there a second time.
///
/// Alpha in the returned color is passed through untouched.
/// </summary>
/// <param name="srgbColor">The glyph's color as v1.6 would have displayed it</param>
/// <param name="brightness">Character brightness in [0, 1]</param>
/// <param name="highlightGain">Multiplier for HDR highlights; 1 outside HDR (Phase 3)</param>
/// <returns>Linear-light color, with alpha unchanged</returns>
Color4 InstanceLinearColor (const Color4 & srgbColor, float brightness, float highlightGain) noexcept;
