Texture2D atlasTexture : register(t0);
SamplerState samplerState : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
    float brightness : BRIGHTNESS;
};

float4 main(PSInput input) : SV_TARGET
{
    // Sample texture atlas
    float4 texColor = atlasTexture.Sample(samplerState, input.uv);
    
    // Apply color and brightness
    float4 finalColor = input.color * texColor * input.brightness;
    
    // Add glow effect (brighten the color slightly)
    finalColor.rgb += finalColor.rgb * 0.3 * input.brightness;
    
    return finalColor;
}
