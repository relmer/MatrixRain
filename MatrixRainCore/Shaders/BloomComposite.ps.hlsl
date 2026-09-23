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

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 main(PSInput input) : SV_TARGET
{
    float4 scene = sceneTexture.Sample(samplerState, input.uv);
    float4 bloom = bloomTexture.Sample(samplerState, input.uv);
    
    // Exponential soft-saturation: low bloom values pass through
    // nearly linearly (good glow on isolated streaks) while high
    // bloom values from dense overlapping areas hit a ceiling.
    // This lets the user crank up bloomIntensity without dense
    // regions becoming a solid wall of glow.
    float3 bloomContrib = bloom.rgb * bloomIntensity;
    float3 softBloom   = 1.0 - exp(-bloomContrib);

    return float4(scene.rgb + softBloom * (1.0 - scene.rgb), 1.0);
}
