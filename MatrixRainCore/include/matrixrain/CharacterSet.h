#pragma once
#include "Math.h"

namespace MatrixRain
{
    // GlyphInfo: Information about a single glyph in the texture atlas
    struct GlyphInfo
    {
        Vector2 uvMin;       // Top-left UV coordinate in texture atlas (0.0 to 1.0)
        Vector2 uvMax;       // Bottom-right UV coordinate in texture atlas (0.0 to 1.0)
        uint32_t codepoint;  // Unicode codepoint (e.g., 0x30A0 for katakana)
        bool mirrored;       // True if this is a horizontally mirrored variant

        GlyphInfo() : uvMin(), uvMax(), codepoint(0), mirrored(false) {}
        GlyphInfo(Vector2 uvMin, Vector2 uvMax, uint32_t codepoint, bool mirrored)
            : uvMin(uvMin), uvMax(uvMax), codepoint(codepoint), mirrored(mirrored) {}
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
        static CharacterSet& GetInstance();

        // Initialize the character set (without texture creation)
        // Must be called once before using any other methods
        // Returns true on success, false on failure
        bool Initialize();

        // Create the texture atlas using the provided D3D11 device
        // Must be called after Initialize() and after D3D11 device is created
        // Returns true on success, false on failure
        bool CreateTextureAtlas(void* d3dDevice);

        // Get a random glyph from the set (uniform distribution)
        // Returns the index into the glyphs array
        size_t GetRandomGlyphIndex() const;

        // Get glyph information by index
        const GlyphInfo& GetGlyph(size_t index) const;

        // Get total number of glyphs (should be 266: 133 normal + 133 mirrored)
        size_t GetGlyphCount() const;

        // Get the texture atlas (for binding to GPU)
        // Returns nullptr if not initialized
        void* GetTextureResource() const;

        // Get the shader resource view for the texture atlas
        // Returns nullptr if texture not created
        void* GetTextureResourceView() const;

        // Cleanup resources
        void Shutdown();

    private:
        // Private constructor for singleton
        CharacterSet();
        ~CharacterSet();

        // Prevent copying
        CharacterSet(const CharacterSet&) = delete;
        CharacterSet& operator=(const CharacterSet&) = delete;

        // Internal initialization helpers
        bool RenderGlyphsToAtlas(void* d3dDevice);
        void CalculateUVCoordinates();

        // Member data
        std::vector<GlyphInfo> m_glyphs;  // Array of all 268 glyphs
        void* m_textureResource;          // DirectX texture resource (ID3D11Texture2D*)
        void* m_textureResourceView;      // DirectX shader resource view (ID3D11ShaderResourceView*)
        bool m_initialized;
    };
}
