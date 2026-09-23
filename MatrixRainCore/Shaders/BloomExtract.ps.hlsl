//
//  Bloom extract.
//
//  Reads the linear-light scene but hands the blur chain ENCODED values, so
//  that from here on the glow is computed exactly as v1.6 computed it.
//
//  v1.6 blurred, soft-saturated and composited gamma-encoded values. Doing
//  those steps in linear light and compensating afterward was tried, one
//  compensation at a time, and each one left something visibly different from
//  v1.6: the halo's width, then its hue, then its strength where streaks
//  overlap, then the head cores. The soft saturation was the last of them --
//  it saturates by magnitude, and linear magnitudes in dim regions are far
//  smaller than encoded ones, so dense regions kept adding glow where v1.6's
//  had already clamped. Handing the chain encoded values removes the whole
//  class of problem instead of the next instance of it.
//
//  The blur chain's float textures still remove the 8-bit banding v1.6 had;
//  the numbers in them are simply encoded ones. The thresholds are v1.6's own
//  again, on encoded brightness.
//
//  LinearToSrgb saturates at white, which is also what v1.6's 8-bit scene did
//  before its extract sampled it, so the self-glow overshoot on heads never
//  reaches the glow. Phase 3 note: that means a head above SDR white blooms as
//  a white head; highlight bloom will need its own handling then.
//

#include "ColorTransfer.hlsli"

Texture2D inputTexture : register(t0);
SamplerState samplerState : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
};

////////////////////////////////////////////////////////////////////////////////
//
//  SampleEncodedBilinear
//
//  v1.6's scene texture was 8-bit and encoded, and this pass sampled it
//  bilinearly at reduced resolution: an average of ENCODED texels. Sampling
//  the linear scene and encoding the average is not the same thing -- the
//  encode curve is concave, so encode(mean) is always at least mean(encode),
//  and for a 2x2 block with one lit texel it is 0.54 against v1.6's 0.25.
//  Every glyph edge then fed the blur more than twice what it fed v1.6's, and
//  the whole glow came out 1.7x too strong. Encoding each texel first and
//  averaging afterward is exactly the value v1.6's sampler produced.
//
////////////////////////////////////////////////////////////////////////////////

float3 SampleEncodedBilinear(float2 uv)
{
    uint width;
    uint height;

    inputTexture.GetDimensions(width, height);

    float2 texel = uv * float2(width, height) - 0.5;
    float2 base  = floor(texel);
    float2 f     = texel - base;
    int2   p     = int2(base);

    float3 c00 = LinearToSrgb3(inputTexture.Load(int3(p,             0)).rgb);
    float3 c10 = LinearToSrgb3(inputTexture.Load(int3(p + int2(1, 0), 0)).rgb);
    float3 c01 = LinearToSrgb3(inputTexture.Load(int3(p + int2(0, 1), 0)).rgb);
    float3 c11 = LinearToSrgb3(inputTexture.Load(int3(p + int2(1, 1), 0)).rgb);

    return lerp(lerp(c00, c10, f.x), lerp(c01, c11, f.x), f.y);
}

float4 main(PSInput input) : SV_TARGET
{
    float3 encoded = SampleEncodedBilinear(input.uv);

    // Extract only bright pixels (consider luminance and max channel)
    float luminance = dot(encoded, float3(0.2126, 0.7152, 0.0722));

    // Also consider the max color channel so saturated blues/reds can trigger bloom
    float maxComp = max(max(encoded.r, encoded.g), encoded.b);

    // Use the higher of luminance or max component as brightness metric
    float brightness = max(luminance, maxComp);

    // Low threshold so even dim characters get a subtle glow. The wide
    // smoothstep range (0.1 -> 0.6) ensures bright streak heads bloom strongly
    // while dim tail characters still contribute a soft halo rather than
    // appearing flat.
    float threshold = 0.1;

    // Smooth ramp: dim chars get subtle bloom, bright chars get full
    float bloomAmount = smoothstep(threshold, threshold + 0.5, brightness);

    return float4(encoded * bloomAmount, 1.0);
}
