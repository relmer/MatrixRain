cbuffer BloomConstants : register(b0)
{
    float bloomIntensity;
    float glowSize;
    float2 padding;
};

Texture2D inputTexture : register(t0);
SamplerState samplerState : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 main(PSInput input) : SV_TARGET
{
    uint width, height;
    inputTexture.GetDimensions(width, height);
    float texelX = glowSize / width;

    float4 color = float4(0, 0, 0, 0);

    // 9-tap Gaussian blur (horizontal) - Medium quality variant.
    float weights[9] = { 0.05, 0.09, 0.12, 0.15, 0.18, 0.15, 0.12, 0.09, 0.05 };
    for (int i = -4; i <= 4; i++)
    {
        float2 offset = float2(i * texelX, 0);
        color += inputTexture.Sample(samplerState, input.uv + offset) * weights[i + 4];
    }

    return color;
}
