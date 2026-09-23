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

    // 5-tap Gaussian blur (horizontal) - Low quality variant.
    float weights[5] = { 0.10, 0.24, 0.32, 0.24, 0.10 };
    for (int i = -2; i <= 2; i++)
    {
        float2 offset = float2(i * texelX, 0);
        color += inputTexture.Sample(samplerState, input.uv + offset) * weights[i + 2];
    }

    return color;
}
