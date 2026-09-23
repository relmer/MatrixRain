Texture2D    atlasTexture : register(t0);
SamplerState samplerState : register(s0);

struct PSInput
{
    float4 position   : SV_POSITION;
    float2 uv         : TEXCOORD;
    float4 color      : COLOR;
    float  brightness : BRIGHTNESS;
};

float4 main(PSInput input) : SV_TARGET
{
    // Atlas is rendered by D2D with premultiplied alpha.
    // Tint RGB by instance color and brightness, preserving
    // the premultiplied relationship so the blend state
    // (ONE / INV_SRC_ALPHA) composites correctly.
    float4 texColor  = atlasTexture.Sample(samplerState, input.uv);
    float3 tintedRGB = texColor.rgb * input.color.rgb * input.brightness;
    float  tintedA   = texColor.a   * input.brightness;

    return float4(tintedRGB, tintedA);
}
