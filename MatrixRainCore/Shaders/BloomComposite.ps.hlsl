//
//  Bloom composite, in linear light, and usually the last pass of a frame.
//
//  v1.6 combined glow with a screen blend, scene + soft * (1 - scene). That
//  factor exists to stop gamma-space addition blowing out, and it is exactly
//  the kind of compensation linear light removes the need for: light adds.
//  Dropping it is what makes two overlapping halos come out brighter than
//  either alone instead of flattening into a plateau.
//
//  The soft saturation stays. It is not gamma compensation; it is a deliberate
//  ceiling so a dense field of streaks does not become a wall of glow when the
//  user raises Glow Intensity.
//
//  kBloomCeiling is a starting value that T018 calibrates against the
//  baseline.
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
// 0.571 is what brings the glow contribution back to 1.00x of v1.6's after
// kBloomFalloff below reduces it. The measurement is the glow term on its
// own -- defaults minus glow-min mean
// luminance -- because total luminance also carries the trail change, which is
// structural and not this constant's business. See specs/008-hdr-linear-
// rendering/baseline.md.
static const float kBloomCeiling = 0.571f;

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
    softBloom = pow(softBloom, kBloomFalloff) * kBloomCeiling;

    return float4(OutputTransform(scene.rgb + softBloom), 1.0);
}
