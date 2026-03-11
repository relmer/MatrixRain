#pragma once
#include "Math.h"





using Microsoft::WRL::ComPtr;





struct ID3D11Device;
struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;





// GlyphInfo: Information about a single glyph in the texture atlas
struct GlyphInfo
{
    Vector2  uvMin;         // Top-left UV coordinate in texture atlas (0.0 to 1.0)
    Vector2  uvMax;         // Bottom-right UV coordinate in texture atlas (0.0 to 1.0)
    float    advanceWidth;  // Proportional advance width as a fraction of em-height
    uint32_t codepoint;     // Unicode codepoint (e.g., 0x30A0 for katakana)
    bool     mirrored;      // True if this is a horizontally mirrored variant

    GlyphInfo() : uvMin(), uvMax(), advanceWidth (0.0f), codepoint (0), mirrored (false) { }
    GlyphInfo (Vector2 uvMin, Vector2 uvMax, uint32_t codepoint, bool mirrored) :
        uvMin        (uvMin),
        uvMax        (uvMax),
        advanceWidth (0.0f),
        codepoint    (codepoint),
        mirrored     (mirrored)
    {
    }
};





// OverlayUV: separate UV coordinates for the DPI-aware overlay atlas
struct OverlayUV
{
    Vector2 uvMin;
    Vector2 uvMax;
};





// CharacterSet: Singleton managing the texture atlas and glyph information
// Responsible for:
// - Creating a 2048x2048 texture atlas with all glyphs (133 normal + 133 mirrored = 266 total)
// - Providing random glyph selection
// - Storing UV coordinates for each glyph
class CharacterSet
{

public:
    // Get the singleton instance
    static CharacterSet & GetInstance();

    // Initialize the character set (without texture creation)
    // Must be called once before using any other methods
    // Returns true on success, false on failure
    bool    Initialize();

    // Create the texture atlas using the provided D3D11 device
    // Must be called after Initialize() and after D3D11 device is created
    // Returns HRESULT indicating success or failure
    HRESULT CreateTextureAtlas (ID3D11Device * d3dDevice, float dpiScale);

    // Get a random glyph from the set (uniform distribution)
    // Returns the index into the glyphs array
    size_t GetRandomGlyphIndex() const;

    // Get glyph information by index
    const GlyphInfo & GetGlyph (size_t index) const;

    // Find glyph index by Unicode codepoint (non-mirrored only)
    // Returns the index into the glyphs array, or SIZE_MAX if not found
    size_t FindGlyphByCodepoint (uint32_t codepoint) const;

    // Proportional advance width for a space character (fraction of em-height)
    float  GetSpaceAdvanceWidth() const { return m_spaceAdvanceWidth; }

    // Get total number of glyphs (should be 266: 133 normal + 133 mirrored)
    size_t GetGlyphCount() const;

    // Get the texture atlas (for binding to GPU)
    // Returns nullptr if not initialized
    ID3D11Texture2D          * GetTextureResource() const;

    // Get the shader resource view for the texture atlas
    // Returns nullptr if texture not created
    ID3D11ShaderResourceView * GetTextureResourceView() const;

    // Get the overlay texture atlas (Segoe UI glyphs for overlay rendering)
    // Returns nullptr if not created
    ID3D11ShaderResourceView * GetOverlayTextureResourceView() const;

    // Get overlay-specific UV coordinates for a glyph (DPI-aware atlas)
    const OverlayUV & GetOverlayUV (size_t index) const;

    // Overlay atlas cell content dimensions (pixels at current DPI)
    float GetOverlayCellContentWidth()  const { return static_cast<float> (m_overlayCellContentWidth);  }
    float GetOverlayCellContentHeight() const { return static_cast<float> (m_overlayCellContentHeight); }

    // Recreate the overlay atlas at a new DPI scale
    HRESULT RecreateOverlayAtlas (ID3D11Device * d3dDevice, float dpiScale);

    // Cleanup resources
    void Shutdown();

private:
    // Private constructor for singleton
    CharacterSet() = default;
    ~CharacterSet();

    // Prevent copying
    CharacterSet             (const CharacterSet &) = delete;
    CharacterSet & operator= (const CharacterSet &) = delete;

    // Internal initialization helpers
    HRESULT RenderGlyphsToAtlas                (ID3D11Device * d3dDevice);
    HRESULT CreateOverlayAtlas                 (ID3D11Device * d3dDevice, float dpiScale);
    HRESULT RenderOverlayGlyphsToAtlas         (ID3D11Device * d3dDevice);
    void    CalculateUVCoordinates             ();
    void    CalculateOverlayUVCoordinates      ();
    void    MeasureProportionalAdvanceWidths   (const std::vector<uint32_t> & rainCodepoints, const std::vector<uint32_t> & overlayCodepoints);

    // Member data
    std::vector<GlyphInfo>                       m_glyphs;                         // Array of all 268 glyphs
    std::unordered_map<uint32_t, size_t>          m_codepointToGlyph;               // Codepoint → glyph index (non-mirrored)
    ComPtr<ID3D11Texture2D>                      m_textureResource;                // DirectX texture resource (Consolas rain atlas)
    ComPtr<ID3D11ShaderResourceView>             m_textureResourceView;            // DirectX shader resource view (rain)
    ComPtr<ID3D11Texture2D>                      m_overlayTextureResource;         // DirectX texture resource (Segoe UI overlay atlas)
    ComPtr<ID3D11ShaderResourceView>             m_overlayTextureResourceView;     // DirectX shader resource view (overlay)

    // Overlay atlas parameters (DPI-dependent, recomputed on DPI change)
    std::vector<OverlayUV>                       m_overlayUVs;
    int                                          m_overlayCellContentWidth   = 0;
    int                                          m_overlayCellContentHeight  = 0;
    int                                          m_overlayPadding            = 0;
    int                                          m_overlayAtlasWidth         = 0;
    int                                          m_overlayAtlasHeight        = 0;
    float                                        m_overlayFontSize           = 0.0f;

    float                                        m_spaceAdvanceWidth         = 0.25f;  // Proportional space width (fraction of em-height)
    bool                                         m_initialized               = false;  // Initialization state
};




