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
    float texelY = glowSize / height;
    
    float4 color = float4(0, 0, 0, 0);
    
    // 13-tap Gaussian blur (vertical), spread scaled by glowSize.
    // High-smoothness variant; see ..._Tap5 / ..._Tap9 below.
    float weights[13] = { 0.02, 0.04, 0.06, 0.08, 0.10, 0.12, 0.16, 0.12, 0.10, 0.08, 0.06, 0.04, 0.02 };
    for (int i = -6; i <= 6; i++)
    {
        float2 offset = float2(0, i * texelY);
        color += inputTexture.Sample(samplerState, input.uv + offset) * weights[i + 6];
    }
    
    return color;
}
