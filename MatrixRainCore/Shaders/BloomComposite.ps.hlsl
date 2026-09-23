//
//  Bloom composite, usually the last pass of a frame.
//
//  The bloom texture holds ENCODED values -- see BloomExtract.ps.hlsl for why
//  -- so this pass is v1.6's composite, line for line: soft-saturate the
//  blurred glow, screen it over the encoded scene, and only then decode the
//  finished pixel to linear light for the output transform.
//
//  The soft saturation is a deliberate ceiling so a dense field of streaks does
//  not become a wall of glow when the user raises Glow Intensity. It acts on
//  encoded magnitudes, as v1.6's did, which is what makes overlapping streaks
//  saturate where v1.6's did instead of continuing to add.
//
//  The screen blend, scene + soft * (1 - scene), does nothing where the scene
//  is dark and attenuates the glow landing on a bright glyph, per channel, as
//  v1.6's did. Without it every glyph body carried extra glow in its secondary
//  channels.
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

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
};

float4 main(PSInput input) : SV_TARGET
{
    float4 scene = sceneTexture.Sample(samplerState, input.uv);   // linear light
    float4 bloom = bloomTexture.Sample(samplerState, input.uv);   // encoded

    // Exponential soft-saturation: low bloom values pass through nearly
    // linearly (good glow on isolated streaks) while high bloom values from
    // dense overlapping areas hit a ceiling.
    float3 bloomContrib = bloom.rgb * bloomIntensity;
    float3 softBloom    = 1.0 - exp(-bloomContrib);

    float3 encodedScene = LinearToSrgb3(scene.rgb);
    float3 composited   = encodedScene + softBloom * (1.0 - encodedScene);

    // The result is already encoded. When this pass writes the SDR back
    // buffer, decoding it only for OutputTransform to encode it again is an
    // identity that costs six pow() per pixel at full resolution -- which
    // showed up as a 15-35% frame cost on the Low preset, where the blur is
    // cheap and this pass dominates. Hand it over directly. Every other case
    // (an intermediate pass, or HDR output) needs the linear value and takes
    // the full path.
    if (g_isFinalPass != 0 && g_outputMode == 0)
    {
        return float4(composited, 1.0);
    }

    return float4(OutputTransform(SrgbToLinear3(composited)), 1.0);
}
