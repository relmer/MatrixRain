cbuffer Constants : register(b0)
{
    float4x4 projection;
    float characterScale;  // Global scale for preview mode
    float charWidth;       // Base quad width in pixels
    float charHeight;      // Base quad height in pixels
    float cbPadding;
};

struct VSInput
{
    float3 position : POSITION;
    float2 uvMin : TEXCOORD0;
    float2 uvMax : TEXCOORD1;
    float4 color : COLOR;
    float brightness : BRIGHTNESS;
    float scaleX : SCALEX;
    float scaleY : SCALEY;
    float highlightGain : HIGHLIGHT;   // Spec 008 R14: linear-light gain above SDR white; 1 for most glyphs
    uint instanceID : SV_InstanceID;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
    float brightness : BRIGHTNESS;
    float highlightGain : HIGHLIGHT;   // Last, so Overlay.ps, which does not read it, keeps a matching prefix
};

// Quad vertices (unit square)
static const float2 quadVertices[6] = {
    float2(0.0, 0.0),  // Top-left
    float2(1.0, 0.0),  // Top-right
    float2(0.0, 1.0),  // Bottom-left
    float2(1.0, 0.0),  // Top-right
    float2(1.0, 1.0),  // Bottom-right
    float2(0.0, 1.0)   // Bottom-left
};

PSInput main(VSInput input, uint vertexID : SV_VertexID)
{
    PSInput output;
    
    // Get quad vertex position
    float2 quadPos = quadVertices[vertexID % 6];
    
    // Character size in world space (scaled for viewport and per-character)
    float2 charSize = float2(charWidth * input.scaleX, charHeight * input.scaleY) * characterScale;
    float2 worldPos = input.position.xy + quadPos * charSize;
    
    // Apply projection
    float4 pos = float4(worldPos, input.position.z, 1.0);
    output.position = mul(projection, pos);
    
    // Interpolate UV coordinates
    output.uv = lerp(input.uvMin, input.uvMax, quadPos);
    output.color = input.color;
    output.brightness = input.brightness;
    output.highlightGain = input.highlightGain;
    
    return output;
}
