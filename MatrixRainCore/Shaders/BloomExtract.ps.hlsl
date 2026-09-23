//
//  Bloom extract, reading a LINEAR-light scene.
//
//  The shape of the ramp is unchanged from v1.6: the brightness metric is
//  still max(luminance, maxChannel) so a saturated blue or red triggers bloom
//  as readily as a bright green, and the ramp is still a smoothstep.
//
//  Only the two endpoints moved, and they had to. v1.6's 0.1 and 0.6 were
//  positions on the sRGB curve; the same positions in linear light are
//  SrgbToLinear(0.1) and SrgbToLinear(0.6). Leaving them where they were would
//  have pushed the threshold far up the curve and cut every dim trail
//  character out of the glow.
//
//  These are starting values that T018 calibrates against the baseline.
//

#include "ColorConstants.h"

Texture2D inputTexture : register(t0);
SamplerState samplerState : register(s0);

// SrgbToLinear(0.1) and SrgbToLinear(0.6), the v1.6 endpoints carried across
// the transfer function rather than reused as raw numbers.
static const float kExtractThreshold = 0.010023f;
static const float kExtractRampEnd   = 0.318547f;

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
};

float4 main(PSInput input) : SV_TARGET
{
    float4 color = inputTexture.Sample(samplerState, input.uv);

    // Extract only bright pixels (consider luminance and max channel)
    float luminance = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));

    // Also consider the max color channel so saturated blues/reds can trigger bloom
    float maxComp = max(max(color.r, color.g), color.b);

    // Use the higher of luminance or max component as brightness metric
    float brightness = max(luminance, maxComp);

    // Smooth ramp: dim chars get subtle bloom, bright chars get full
    float bloomAmount = smoothstep(kExtractThreshold, kExtractRampEnd, brightness);

    return float4(color.rgb * bloomAmount, 1.0);
}
