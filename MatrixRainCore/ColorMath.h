#pragma once

#include "Math.h"
#include "Shaders\ColorConstants.h"





/// <summary>
/// The constants of the sRGB transfer function (IEC 61966-2-1), as typed C++
/// values.
///
/// Every number here comes from Shaders/ColorConstants.h, which FXC compiles
/// into ColorTransfer.hlsli as well. That indirection is the whole point:
/// the HLSL curves have to be line-for-line transliterations of
/// SrgbToLinearPolynomial and LinearToSrgbPolynomial, and a second copy of a
/// constant is a bug waiting to be written. Editing the shared header moves
/// both sides at once.
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

    /// <summary>The GPU's polynomial for the decoding curve, highest power first.</summary>
    inline constexpr float kDecodePolynomial[6] = { MR_SRGB_DECODE_C5, MR_SRGB_DECODE_C4, MR_SRGB_DECODE_C3,
                                                    MR_SRGB_DECODE_C2, MR_SRGB_DECODE_C1, MR_SRGB_DECODE_C0 };

    /// <summary>The GPU's polynomial for the encoding curve, in the square root of the input, highest power first.</summary>
    inline constexpr float kEncodePolynomial[6] = { MR_SRGB_ENCODE_C5, MR_SRGB_ENCODE_C4, MR_SRGB_ENCODE_C3,
                                                    MR_SRGB_ENCODE_C2, MR_SRGB_ENCODE_C1, MR_SRGB_ENCODE_C0 };
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
/// SrgbToLinear as the GPU computes it: the same straight segment, then the
/// polynomial in Shaders/ColorConstants.h instead of pow().
///
/// This exists so the shader's arithmetic can be tested. ColorTransfer.hlsli
/// is a transliteration of this function and takes its coefficients from the
/// same header, so holding this to its error bound holds the shader to it.
/// Nothing on the CPU should prefer it over SrgbToLinear.
/// </summary>
/// <param name="encoded">sRGB-encoded value in [0, 1]</param>
/// <returns>Linear-light value, within 0.013 of an 8-bit code value of the exact curve</returns>
float SrgbToLinearPolynomial (float encoded) noexcept;





/// <summary>
/// LinearToSrgb as the GPU computes it: clamp, the same straight segment,
/// then the polynomial in the square root from Shaders/ColorConstants.h
/// instead of pow(). See SrgbToLinearPolynomial for why this exists.
/// </summary>
/// <param name="linear">Linear-light value; values outside [0, 1] are clamped</param>
/// <returns>sRGB-encoded value, within 0.084 of an 8-bit code value of the exact curve</returns>
float LinearToSrgbPolynomial (float linear) noexcept;





/// <summary>
/// The 30% self-glow the glyph shader has always added to a character's own
/// color, scaled by its brightness so only bright heads get much of it.
/// </summary>
inline constexpr float kGlyphSelfGlow = 0.3f;





/// <summary>
/// A glyph's color as v1.6 put it on screen at full coverage, in gamma space,
/// with both brightness factors folded in.
///
/// This exists to keep v1.6's look exactly (FR-005). The v1.6 glyph pixel
/// shader wrote
///     rgb = color * texture * brightness * (1 + 0.3 * brightness)
///     a   = texture.a * brightness
/// and alpha-blended over a black scene, so the pixel a viewer saw was
/// color * brightness * (1 + 0.3 * brightness) * brightness times coverage
/// squared. None of that is a linear operation, so it all stays in gamma
/// space. This function does the per-instance part; the glyph shader applies
/// coverage, clips at white as the 8-bit target did, and only then converts
/// the finished pixel to linear light. Linear light governs what happens
/// afterwards -- the blur and the bloom -- which is where it belongs.
///
/// The result may exceed 1 (a white head at full brightness is 1.3); the
/// shader clips it, exactly where v1.6's render target did.
///
/// Alpha is passed through untouched.
/// </summary>
/// <param name="srgbColor">The glyph's sRGB color: white for a head, the scheme color for a trail</param>
/// <param name="brightness">Character brightness in [0, 1]</param>
/// <returns>Gamma-space displayed color at full coverage, unclipped, with alpha unchanged</returns>
Color4 InstanceDisplayColor (const Color4 & srgbColor, float brightness) noexcept;
