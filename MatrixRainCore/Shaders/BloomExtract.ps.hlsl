Texture2D inputTexture : register(t0);
SamplerState samplerState : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
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

    // Low threshold so even dim characters get a subtle glow.
    // The wide smoothstep range (0.1 → 0.6) ensures bright streak
    // heads bloom strongly while dim tail characters still contribute
    // a soft halo rather than appearing flat.
    float threshold = 0.1;

    // Smooth ramp: dim chars get subtle bloom, bright chars get full
    float bloomAmount = smoothstep(threshold, threshold + 0.5, brightness);

    return float4(color.rgb * bloomAmount, 1.0);
}
