//
//  Bloom composite, usually the last pass of a frame.
//
//  The blur chain feeding this runs in linear light, so overlapping halos add
//  as light does. This pass then puts the glow on screen exactly as v1.6 did:
//  the halo's strength, falloff shape, hue and the way it lands on glyph
//  bodies are all computed in encoded space, where v1.6 authored them, and the
//  result is decoded to linear light for the output transform. Each of those
//  four terms was found to differ visibly when done in linear light and
//  matched one at a time; the comments below record why.
//
//  The soft saturation is a deliberate ceiling so a dense field of streaks does
//  not become a wall of glow when the user raises Glow Intensity.
//
//  kBloomCeiling and kBloomFalloff are calibrated in T018 against the
//  baseline; see specs/008-hdr-linear-rendering/baseline.md.
//

#include "OutputTransform.hlsli"

cbuffer BloomConstants : register(b0)
{
    float bloomIntensity;
    float glowSize;
    float2 padding;
};

Texture2D sceneTexture : register(t0);
Texture2D bloomTexture : register(t1);
SamplerState samplerState : register(s0);

// How much light the saturated glow may add, in multiples of SDR white.
//
// Calibrated in T018 against the v1.6 baseline, not guessed. Glow is added in
// LINEAR light and then encoded, and the encode curve is steep near black:
// adding 0.3 of linear light to a dark gap lands at sRGB 0.58. The same
// bloomIntensity that looked right in gamma space therefore produced about
// eight times too much apparent glow, and dark gaps are most of the frame.
//
// 1.60 is what brings the glow contribution to 1.00x of v1.6's once
// kBloomFalloff below has shaped it and the composite adds it in encoded
// space. The measurement is the glow term on its own -- defaults minus
// glow-min mean luminance -- so that it is not confused with the glyph and
// trail terms, which are matched separately. (1.74 was the fit before the
// encoded-space composite; addition in encoded space is brighter over dim
// trail pixels than addition in linear light, so the ceiling came down.)
//
// An earlier value of 0.871 was fitted while the glyph shader still let head
// overshoot (1.3, from the self-glow) into the scene. The extract runs at half
// resolution, so its bilinear sample averaged that overshoot into every head's
// surroundings BEFORE the extract's clamp could touch it, and each head bloomed
// harder than v1.6's clipped scene allowed. The glyph shader now clips at white
// as v1.6's 8-bit target did, the leak is gone, and the honest input needs
// twice the ceiling. See specs/008-hdr-linear-rendering/baseline.md.
static const float kBloomCeiling = 1.60f;

// Exponent applied to the glow before it is added, cancelling the lift the
// encode curve would otherwise give the halo's tail. It is the transfer
// function's own exponent for that reason, not a free parameter.
static const float kBloomFalloff = MR_SRGB_CURVE_GAMMA;

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
};

float4 main(PSInput input) : SV_TARGET
{
    float4 scene = sceneTexture.Sample(samplerState, input.uv);
    float4 bloom = bloomTexture.Sample(samplerState, input.uv);

    // Exponential soft-saturation: low bloom values pass through nearly
    // linearly (good glow on isolated streaks) while high bloom values from
    // dense overlapping areas approach a ceiling.
    float3 bloomContrib = bloom.rgb * bloomIntensity;
    float3 softBloom    = 1.0 - exp(-bloomContrib);

    // Shape the halo's falloff before it is added.
    //
    // The blur runs in linear light and the result is then encoded, and the
    // encode curve is concave: it lifts the halo's dim tail far more than its
    // core. A Gaussian in linear light therefore reaches the screen as a much
    // flatter, wider shape. Measured against a sigma-8 halo, v1.6 showed 13.5%
    // of core brightness 16 px out where this pipeline showed 39.6%, and 1.1%
    // at 24 px where this showed 9.5%. The glow stopped hugging the glyphs and
    // read as the same width all the way down a streak.
    //
    // Raising the glow to the encode curve's own exponent cancels that lift,
    // so what lands on screen falls off as v1.6's did. This is not gamma
    // compensation creeping back into the blending: light is still added in
    // linear light. It shapes the glow's PROFILE, which is art direction
    // rather than physics, and v1.6's art direction is what FR-005 protects.
    //
    // The exponent is applied to the glow's MAGNITUDE only. The hue is set
    // separately below, so nothing here may change the channel ratios.
    float bloomMax  = max(softBloom.r, max(softBloom.g, softBloom.b));
    float magnitude = pow(bloomMax, kBloomFalloff) * kBloomCeiling;

    if (bloomMax <= 0.0)
    {
        return float4(OutputTransform(scene.rgb), 1.0);
    }

    // Give the glow the ENCODED hue of its source at every brightness, as
    // v1.6's gamma-space blur did.
    //
    // The rain's default green is (0, 255, 100): an encoded blue/green ratio
    // of 0.39, but a linear-light ratio of 0.13. A blur in linear light keeps
    // the 0.13, and out in the dim halo, where the encode curve is nearly
    // straight, that is also what reaches the screen: a third of the blue
    // v1.6 showed, and a glow visibly greener and yellower than its glyphs.
    // Rob saw it as a tint difference between the two builds.
    //
    // Normalizing the bloom to full brightness and encoding it gives the hue's
    // encoded ratios; the shaped magnitude is encoded, split by those ratios,
    // and decoded again. The dominant channel is untouched, so the strength
    // calibration above still holds; the secondary channels are lifted to
    // v1.6's proportions.
    float3 encodedHue  = LinearToSrgb3(softBloom / bloomMax);
    float3 encodedGlow = LinearToSrgbChannel(magnitude) * encodedHue;

    // Composite as v1.6 did, in encoded space: scene + glow * (1 - scene),
    // per channel. Where the scene is dark, (1 - scene) is 1 and this is plain
    // addition -- the gaps, where halos overlap, still add in linear light
    // once decoded below. Where the scene is bright the factor attenuates the
    // glow landing ON the glyph, per channel: on a green glyph, v1.6 added no
    // green glow and only 60% of the blue. Adding the full glow there instead
    // made every glyph body carry extra blue, which showed in the difference
    // images as a blue silhouette of each character. The factor changes
    // nothing in the gaps and so costs none of the linear-light improvement.
    float3 encodedScene = LinearToSrgb3(scene.rgb);
    float3 composited   = encodedScene + encodedGlow * (1.0 - encodedScene);

    return float4(OutputTransform(SrgbToLinear3(composited)), 1.0);
}
