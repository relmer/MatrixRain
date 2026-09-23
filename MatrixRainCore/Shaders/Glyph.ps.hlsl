//
//  Glyph pixel shader.
//
//  input.color arrives already in LINEAR light, with the brightness and the
//  30% self-glow already folded in by InstanceLinearColor on the CPU. Those
//  two terms are not linear operations, so applying them here would change the
//  glyph; doing them once per instance in gamma space keeps the glyph core
//  identical to v1.6 (FR-005) and leaves this shader with nothing to do but
//  mask by coverage.
//
//  Alpha is the exception. It carries the trail's fade, which was always a
//  straight multiply, so it keeps the brightness factor here exactly as v1.6
//  applied it.
//

Texture2D atlasTexture : register(t0);
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
    float4 texColor = atlasTexture.Sample(samplerState, input.uv);

    return float4(input.color.rgb * texColor.rgb,
                  input.color.a * texColor.a * input.brightness);
}
