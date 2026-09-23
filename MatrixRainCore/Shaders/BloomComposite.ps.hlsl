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
static const float kBloomCeiling = 1.0f;

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
