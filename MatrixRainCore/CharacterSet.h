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
    static CharacterSet & GetInstance();

    bool    Initialize();
    HRESULT CreateTextureAtlas   (ID3D11Device * d3dDevice, float dpiScale);
    size_t  GetRandomGlyphIndex  (size_t count) const;
    size_t  FindGlyphByCodepoint (uint32_t codepoint) const;

    const GlyphInfo & GetGlyph             (size_t index) const { return m_glyphs[index]; }
    float             GetSpaceAdvanceWidth ()             const { return m_spaceAdvanceWidth; }
    size_t            GetGlyphCount        ()             const { return m_glyphs.size(); }
    size_t            GetRainGlyphCount    ()             const { return m_rainGlyphCount; }

    ID3D11Texture2D          * GetTextureResource()             const { return m_textureResource.Get();              }
    ID3D11ShaderResourceView * GetTextureResourceView()          const { return m_textureResourceView.Get();          }
    ID3D11ShaderResourceView * GetOverlayTextureResourceView()  const { return m_overlayTextureResourceView.Get();   }
    const OverlayUV          & GetOverlayUV (size_t index)      const { return m_overlayUVs[index];                  }

    // Overlay atlas cell content dimensions (display pixels at current DPI)
    float GetOverlayCellContentWidth()  const { return static_cast<float> (m_overlayDisplayWidth);  }
    float GetOverlayCellContentHeight() const { return static_cast<float> (m_overlayDisplayHeight); }

    HRESULT RecreateOverlayAtlas (ID3D11Device * d3dDevice, float dpiScale);

    void Shutdown();

private:
    // Private constructor for singleton
    CharacterSet() = default;
    ~CharacterSet();

    // Prevent copying
    CharacterSet             (const CharacterSet &) = delete;
    CharacterSet & operator= (const CharacterSet &) = delete;

    // Internal initialization helpers
    HRESULT CreateD2DRenderContext         (ID3D11Device * d3dDevice, ID3D11Texture2D * texture, ID2D1DeviceContext ** ppContext, IDWriteFactory ** ppDWriteFactory, ID2D1SolidColorBrush ** ppBrush);
    HRESULT RenderGlyphsToAtlas                (ID3D11Device * d3dDevice);
    HRESULT CreateOverlayAtlas                 (ID3D11Device * d3dDevice, float dpiScale);
    HRESULT RenderOverlayGlyphsToAtlas         (ID3D11Device * d3dDevice);
    void    CalculateUVCoordinates             ();
    void    CalculateOverlayUVCoordinates      ();
    void    MeasureProportionalAdvanceWidths   (const std::vector<uint32_t> & rainCodepoints, const std::vector<uint32_t> & overlayCodepoints);
    float   MeasureCodepointAdvanceWidth       (IDWriteFactory * pFactory, IDWriteTextFormat * pFormat, uint32_t codepoint, float fontSize, bool includeTrailingWhitespace);

    // Member data
    std::vector<GlyphInfo>                       m_glyphs;                         // Array of all glyphs (rain + overlay)
    size_t                                       m_rainGlyphCount = 0;             // Count of rain-only glyphs (normal + mirrored)
    std::unordered_map<uint32_t, size_t>         m_codepointToGlyph;               // Codepoint → glyph index (non-mirrored)
    ComPtr<ID3D11Texture2D>                      m_textureResource;                // DirectX texture resource (Consolas rain atlas)
    ComPtr<ID3D11ShaderResourceView>             m_textureResourceView;            // DirectX shader resource view (rain)
    ComPtr<ID3D11Texture2D>                      m_overlayTextureResource;         // DirectX texture resource (Segoe UI overlay atlas)
    ComPtr<ID3D11ShaderResourceView>             m_overlayTextureResourceView;     // DirectX shader resource view (overlay)

    // Overlay atlas parameters (DPI-dependent, recomputed on DPI change)
    std::vector<OverlayUV>                       m_overlayUVs;
    int                                          m_overlayCellContentWidth   = 0;  // Atlas cell content (supersampled)
    int                                          m_overlayCellContentHeight  = 0;  // Atlas cell content (supersampled)
    int                                          m_overlayDisplayWidth       = 0;  // On-screen quad width (display pixels)
    int                                          m_overlayDisplayHeight      = 0;  // On-screen quad height (display pixels)
    int                                          m_overlayPadding            = 0;
    int                                          m_overlayAtlasWidth         = 0;
    int                                          m_overlayAtlasHeight        = 0;
    float                                        m_overlayFontSize           = 0.0f;
    float                                        m_spaceAdvanceWidth         = 0.25f;  // Proportional space width (fraction of em-height)
    bool                                         m_initialized               = false;  // Initialization state
};




