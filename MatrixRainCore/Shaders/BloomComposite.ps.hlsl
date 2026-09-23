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
// 0.116 = 1 / 8.60, the measured ratio of glow contribution to v1.6's. The
// measurement is the glow term on its own -- defaults minus glow-min mean
// luminance -- because total luminance also carries the trail change, which is
// structural and not this constant's business. See specs/008-hdr-linear-
// rendering/baseline.md.
static const float kBloomCeiling = 0.116f;

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
    float3 softBloom    = (1.0 - exp(-bloomContrib)) * kBloomCeiling;

    return float4(OutputTransform(scene.rgb + softBloom), 1.0);
}
