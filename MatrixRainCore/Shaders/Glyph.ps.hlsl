//
//  Glyph pixel shader.
//
//  input.color arrives already in LINEAR light, with the brightness fade and
//  the 30% self-glow already folded in by InstanceLinearColor on the CPU. Both
//  of those were gamma-space operations in v1.6, applied to the color AND to
//  the alpha it blended with, and reproducing them in linear light changes the
//  picture. Doing them once per instance in gamma space keeps the glyph
//  identical to v1.6 (FR-005) and leaves this shader with nothing to do but
//  mask by coverage.
//
//  Alpha is therefore coverage alone. input.brightness is deliberately not
//  applied: it is already inside input.color, and applying it here as well
//  makes every fading trail brighter than v1.6, because a fade in linear light
//  removes far less light than the same fade applied to encoded values.
//
//  Coverage gets the same treatment. The atlas is drawn by Direct2D with a
//  white brush into a premultiplied target, so an antialiased edge texel is
//  (c, c, c, c) and v1.6's color * tex.rgb, then SRC_ALPHA blend, put
//  color * c * c on screen -- in gamma space. Compositing those two factors in
//  linear light instead lifts every edge pixel, and a thin stroke is mostly
//  edge pixels, so the whole glyph reads lighter and heavier than v1.6.
//  Raising coverage to the encode curve's exponent before it is used cancels
//  that lift, exactly as kBloomFalloff does for the halo. tex.rgb is not read:
//  it equals tex.a and would carry the unshaped value.
//

#include "ColorConstants.h"

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
    float  coverage = pow(texColor.a, MR_SRGB_CURVE_GAMMA);

    return float4(input.color.rgb * coverage,
                  input.color.a * coverage);
}
