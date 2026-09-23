#pragma once

#include "Math.h"
#include "OutputModeSelection.h"
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
/// The numbers behind the display luminance math (data-model §2).
/// </summary>
namespace DisplayLuminanceConstants
{
    /// <summary>scRGB's reference white: 1.0 in a scRGB back buffer is 80 nits.</summary>
    inline constexpr float    kScRgbWhiteNits              = 80.0f;

    /// <summary>Peak the math assumes when the display reports none, or nonsense.</summary>
    inline constexpr float    kDefaultPeakNits             = 400.0f;

    /// <summary>A reported peak outside this range is treated as unknown.</summary>
    inline constexpr float    kMinPlausiblePeakNits        = 80.0f;
    inline constexpr float    kMaxPlausiblePeakNits        = 10000.0f;

    /// <summary>The SDR white level is held to this range, whatever DisplayConfig says.</summary>
    inline constexpr float    kMinSdrWhiteNits             = 80.0f;
    inline constexpr float    kMaxSdrWhiteNits             = 480.0f;

    /// <summary>DisplayConfig reports the SDR white level in units where 1000 is 80 nits.</summary>
    inline constexpr uint32_t kDisplayConfigWhiteLevelUnit = 1000;
}





/// <summary>
/// What the OS says about the monitor a window sits on, refreshed at 1 Hz and
/// on display changes (data-model §2). Read through IDisplayLuminanceProvider.
/// </summary>
struct DisplayLuminance
{
    bool         hdrEnabled       { false };   // Windows HDR is on for this monitor (output color space is PQ / BT.2020)
    bool         scRgbSupported   { false };   // The swap chain reports present support for scRGB
    float        sdrWhiteNits     { 80.0f };   // The Windows SDR brightness slider, in nits; 80 when unavailable
    float        reportedPeakNits { 0.0f  };   // DXGI_OUTPUT_DESC1::MaxLuminance; may be 0 or implausible
    std::wstring deviceName;                   // DXGI_OUTPUT_DESC1::DeviceName, used to pair DXGI with DisplayConfig
};





/// <summary>
/// The peak brightness the math should trust: the reported one when it is
/// plausible, otherwise 400 nits. Displays report 0 when they do not know,
/// and some report numbers no panel can make.
/// </summary>
/// <param name="reportedPeakNits">DXGI_OUTPUT_DESC1::MaxLuminance</param>
/// <returns>A peak in [80, 10 000] nits</returns>
float EffectivePeakNits (float reportedPeakNits) noexcept;





/// <summary>
/// How far above SDR white the display can go, as a multiplier: peak over
/// white, never below 1 (FR-026). Phase 3 spends it on highlights; Phase 2
/// ignores it and caps at white. A white level at or above the peak gives 1,
/// and a white level outside [80, 480] is clamped first.
/// </summary>
/// <param name="effectivePeakNits">From EffectivePeakNits</param>
/// <param name="sdrWhiteNits">The SDR white level</param>
/// <returns>A multiplier of at least 1</returns>
float Headroom (float effectivePeakNits, float sdrWhiteNits) noexcept;





/// <summary>
/// The scRGB value of SDR white: the white level over 80 nits, so the rain
/// lands at the brightness the user set for SDR content (SC-003). The white
/// level is clamped to [80, 480] first, so a missing or absurd reading can
/// neither black the screen nor blind the viewer.
/// </summary>
/// <param name="sdrWhiteNits">The SDR white level</param>
/// <returns>A multiplier in [1, 6]</returns>
float SdrWhiteScale (float sdrWhiteNits) noexcept;





/// <summary>
/// Converts DisplayConfig's SDR white level to nits: level / 1000 * 80,
/// clamped to [80, 480].
/// </summary>
/// <param name="sdrWhiteLevel">DISPLAYCONFIG_SDR_WHITE_LEVEL::SDRWhiteLevel</param>
/// <returns>Nits in [80, 480]</returns>
float SdrWhiteNitsFromDisplayConfig (uint32_t sdrWhiteLevel) noexcept;





/// <summary>
/// Which glyphs get highlight headroom, and how much (research R14, option B).
/// Starting values; T044 tunes them on hardware.
/// </summary>
namespace HighlightConstants
{
    /// <summary>The share of the highlight gain a trail glyph at full brightness gets.</summary>
    inline constexpr float kTrailHighlightShare = 0.6f;

    /// <summary>Trail glyphs at or below this brightness get no highlight headroom.</summary>
    inline constexpr float kTrailHighlightFloor = 0.5f;

    /// <summary>The top of the highlight brightness setting; the setting is a percentage.</summary>
    inline constexpr int   kMaxHighlightBrightness = 100;
}





/// <summary>
/// How far above SDR white a head may go on this monitor, as a multiplier
/// (data-model §5): the display's headroom raised to the highlight
/// brightness setting over 100, so 0 gives 1 (no boost) and 100 aims at the
/// display's peak. 1 whenever the monitor presents SDR or the user set HDR
/// mode Off (FR-021).
/// </summary>
/// <param name="headroom">From Headroom; at least 1</param>
/// <param name="highlightBrightness">The setting, 0 to 100; clamped</param>
/// <param name="hdrMode">The user's HDR mode</param>
/// <param name="outputMode">What this monitor presents</param>
/// <returns>A multiplier in [1, headroom]</returns>
float HighlightGain (float headroom, int highlightBrightness, HdrMode hdrMode, OutputMode outputMode) noexcept;





/// <summary>
/// The share of the highlight gain one glyph gets (research R14, option B):
/// all of it for a head; for a trail glyph, a share that is none at and
/// below kTrailHighlightFloor and rises with the square of the brightness
/// above it to kTrailHighlightShare at full brightness. The glyph's gain is
/// HighlightGain ^ HighlightWeight, so a weight of 0 is exactly 1.
/// </summary>
/// <param name="isHead">The glyph leads its streak (a white instance)</param>
/// <param name="brightness">The glyph's brightness, 0 to 1</param>
/// <returns>A weight in [0, 1]</returns>
float HighlightWeight (bool isHead, float brightness) noexcept;





/// <summary>
/// Rolls highlights off into the display's headroom without clipping or
/// changing hue (research R8, R14; FR-019). On the maximum channel m:
/// identity up to 1 (SDR white), then 1 + (h - 1) t / (1 + t) with
/// t = (m - 1) / (h - 1), which leaves white where it is, has slope 1 there
/// (no kink where a head crosses white), and approaches h without reaching
/// it. All three channels are scaled by one factor, so their ratios, and so
/// the hue, do not change. With h at or below 1 the result is capped at 1.
///
/// OutputTransform.hlsli carries a line-for-line transliteration.
/// </summary>
/// <param name="rgb">Linear light, 1 at SDR white; changed in place</param>
/// <param name="headroom">From Headroom, or 1 with HDR mode Off</param>
void ToneMapHighlights (float (& rgb)[3], float headroom) noexcept;





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
