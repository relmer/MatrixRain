//
//  Glyph pixel shader.
//
//  Reproduces v1.6's displayed pixel exactly, then converts it to linear light
//  for everything downstream. v1.6 wrote
//      rgb = color * tex.rgb * b * (1 + 0.3 b)
//      a   = tex.a * b
//  and alpha-blended over a black scene into an 8-bit texture, so the pixel it
//  showed was  min(1, X * c * c)  where X is the instance's gamma-space color
//  with both brightness factors folded in (InstanceDisplayColor) and c is the
//  atlas coverage -- the atlas is Direct2D-rendered white into a premultiplied
//  target, so an edge texel is (c, c, c, c) and tex.rgb equals tex.a.
//
//  Computing that value per pixel and only THEN linearizing is what makes the
//  match exact for every color and every coverage. An earlier version
//  linearized on the CPU and shaped coverage separately with a power of 2.4,
//  which is exact only where the sRGB curve is a pure power law; the curve's
//  offset made mid-tone colors noticeably dark (custom-color read 15% low).
//
//  Output is premultiplied: rgb is the finished over-black result, alpha is
//  v1.6's blend alpha, c * b, so partly covered background attenuates as it
//  did. The blend state for this pass is ONE / INV_SRC_ALPHA.
//
//  g_linearizeColors is 0 on the no-scene fallback path, which draws straight
//  to the 8-bit back buffer with no output transform; there the shader stops
//  at the gamma-space value, which is exactly the pixel v1.6 wrote.
//

#include "ColorConstants.h"

cbuffer Constants : register(b0)
{
    float4x4 projection;
    float    characterScale;
    float    charWidth;
    float    charHeight;
    float    linearizeColors;   // 1: convert to linear light; 0: leave as v1.6 wrote it
};

Texture2D atlasTexture : register(t0);
SamplerState samplerState : register(s0);

struct PSInput
{
    float4 position   : SV_POSITION;
    float2 uv         : TEXCOORD;
    float4 color      : COLOR;
    float  brightness : BRIGHTNESS;
};

float SrgbToLinearChannel(float encoded)
{
    if (encoded <= MR_SRGB_ENCODED_KNEE)
    {
        return encoded / MR_SRGB_LINEAR_SLOPE;
    }

    return pow((encoded + MR_SRGB_CURVE_OFFSET) / MR_SRGB_CURVE_SCALE, MR_SRGB_CURVE_GAMMA);
}

float4 main(PSInput input) : SV_TARGET
{
    float4 texColor  = atlasTexture.Sample(samplerState, input.uv);
    float  coverage  = texColor.a;

    // The pixel v1.6 displayed, clipped where its 8-bit target clipped.
    float3 displayed = min(input.color.rgb * coverage * coverage, 1.0);

    if (linearizeColors > 0.5)
    {
        displayed = float3(SrgbToLinearChannel(displayed.r),
                           SrgbToLinearChannel(displayed.g),
                           SrgbToLinearChannel(displayed.b));
    }

    return float4(displayed, input.color.a * coverage * input.brightness);
}
