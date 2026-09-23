cbuffer HaloConstants : register(b0)
{
    float4 rowRects[16];    // (left, top, right, bottom) per row — must match MAX_HALO_ROWS
    float  cornerRadius;
    float  maxExpand;
    float  maxOpacity;
    int    numRows;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

// Signed distance to a rounded rectangle (negative = inside)
float sdRoundedRect(float2 p, float2 center, float2 halfSize, float radius)
{
    float2 d = abs(p - center) - halfSize + float2(radius, radius);
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - radius;
}

float4 main(PSInput input) : SV_TARGET
{
    float2 pixelPos = input.position.xy;

    // Find minimum distance to any row's rounded rect
    float minDist = 1e9;

    for (int i = 0; i < numRows; i++)
    {
        float4 r = rowRects[i];
        float2 center   = float2((r.x + r.z) * 0.5, (r.y + r.w) * 0.5);
        float2 halfSize = float2((r.z - r.x) * 0.5, (r.w - r.y) * 0.5);

        float d = sdRoundedRect(pixelPos, center, halfSize, cornerRadius);
        minDist = min(minDist, d);
    }

    // Map distance to opacity: inside = maxOpacity, feather over maxExpand
    float t = saturate(minDist / maxExpand);
    float opacity = maxOpacity * (1.0 - t * t);  // Quadratic falloff

    return float4(0, 0, 0, opacity);
}
