#include "pch.h"
#include "matrixrain/CharacterSet.h"
#include "matrixrain/CharacterConstants.h"
#include <random>
#include <d3d11.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace MatrixRain
{
    CharacterSet::CharacterSet()
        : m_textureResource(nullptr)
        , m_initialized(false)
    {
    }

    CharacterSet::~CharacterSet()
    {
        Shutdown();
    }

    CharacterSet& CharacterSet::GetInstance()
    {
        static CharacterSet instance;
        return instance;
    }

    bool CharacterSet::Initialize()
    {
        if (m_initialized)
        {
            return true;
        }

        // Get all character codepoints (134 total: 72 katakana + 26 upper + 26 lower + 10 numerals)
        auto codepoints = CharacterConstants::GetAllCodepoints();
        
        // Reserve space for normal + mirrored glyphs (268 total)
        m_glyphs.clear();
        m_glyphs.reserve(codepoints.size() * 2);
        
        // Create glyph info for normal characters
        for (size_t i = 0; i < codepoints.size(); i++)
        {
            GlyphInfo glyph;
            glyph.codepoint = codepoints[i];
            glyph.mirrored = false;
            glyph.uvMin = Vector2(0.0f, 0.0f); // Will be calculated later
            glyph.uvMax = Vector2(0.0f, 0.0f);
            m_glyphs.push_back(glyph);
        }
        
        // Create glyph info for mirrored characters
        for (size_t i = 0; i < codepoints.size(); i++)
        {
            GlyphInfo glyph;
            glyph.codepoint = codepoints[i];
            glyph.mirrored = true;
            glyph.uvMin = Vector2(0.0f, 0.0f); // Will be calculated later
            glyph.uvMax = Vector2(0.0f, 0.0f);
            m_glyphs.push_back(glyph);
        }

        // Create DirectX texture atlas (requires D3D device, so defer to first render)
        // For now, calculate UV coordinates for a theoretical 16x17 grid layout
        // (134 glyphs normal + 134 mirrored = 268 total, arranged in grid)
        CalculateUVCoordinates();
        
        m_initialized = true;
        return true;
    }

    size_t CharacterSet::GetRandomGlyphIndex() const
    {
        if (m_glyphs.empty())
        {
            return 0;
        }

        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> dist(0, m_glyphs.size() - 1);
        return dist(gen);
    }

    const GlyphInfo& CharacterSet::GetGlyph(size_t index) const
    {
        static GlyphInfo defaultGlyph;
        if (index >= m_glyphs.size())
        {
            return defaultGlyph;
        }
        return m_glyphs[index];
    }

    size_t CharacterSet::GetGlyphCount() const
    {
        return m_glyphs.size();
    }

    void* CharacterSet::GetTextureResource() const
    {
        return m_textureResource;
    }

    void CharacterSet::Shutdown()
    {
        m_glyphs.clear();
        m_textureResource = nullptr;
        m_initialized = false;
    }

    bool CharacterSet::CreateTextureAtlas()
    {
        // NOTE: This requires a D3D11 device to be created first
        // Will be called from RenderSystem::Initialize() later
        // For now, texture creation is deferred until rendering system is ready
        
        // Texture atlas specs:
        // - Size: 2048x2048 pixels
        // - Format: DXGI_FORMAT_R8G8B8A8_UNORM (32-bit RGBA)
        // - Usage: D3D11_USAGE_DEFAULT (GPU read/write)
        // - Bind flags: D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET
        // - CPU access: None (will use Direct2D for rendering)
        
        return true; // Placeholder - will be implemented when RenderSystem exists
    }

    bool CharacterSet::RenderGlyphsToAtlas()
    {
        // NOTE: This requires Direct2D and DirectWrite factories
        // Will be called from RenderSystem::Initialize() after texture creation
        
        // Rendering process:
        // 1. Create ID2D1RenderTarget from ID3D11Texture2D using DXGI surface
        // 2. Create IDWriteTextFormat with "Consolas" font, 96pt size
        // 3. For each glyph:
        //    a. Clear cell background to transparent black
        //    b. Draw character at cell position using DrawText
        //    c. For mirrored glyphs, apply horizontal flip transform
        // 4. Flush render target
        
        return true; // Placeholder - will be implemented when RenderSystem exists
    }

    void CharacterSet::CalculateUVCoordinates()
    {
        // Layout: 2048x2048 texture atlas
        // Grid arrangement: 16 columns × 17 rows = 272 cells (268 used)
        // Each glyph cell: 128x120 pixels (with 8 pixel padding per cell)
        
        constexpr size_t ATLAS_SIZE = 2048;
        constexpr size_t GRID_COLS = 16;
        constexpr size_t GRID_ROWS = 17;
        constexpr size_t CELL_WIDTH = ATLAS_SIZE / GRID_COLS;   // 128 pixels
        constexpr size_t CELL_HEIGHT = ATLAS_SIZE / GRID_ROWS;  // 120 pixels
        constexpr size_t GLYPH_PADDING = 8;  // Padding to prevent texture bleeding
        
        for (size_t i = 0; i < m_glyphs.size(); i++)
        {
            size_t col = i % GRID_COLS;
            size_t row = i / GRID_COLS;
            
            // Calculate pixel coordinates with padding
            float pixelMinX = static_cast<float>(col * CELL_WIDTH + GLYPH_PADDING);
            float pixelMinY = static_cast<float>(row * CELL_HEIGHT + GLYPH_PADDING);
            float pixelMaxX = static_cast<float>((col + 1) * CELL_WIDTH - GLYPH_PADDING);
            float pixelMaxY = static_cast<float>((row + 1) * CELL_HEIGHT - GLYPH_PADDING);
            
            // Convert to UV coordinates [0, 1]
            m_glyphs[i].uvMin.x = pixelMinX / static_cast<float>(ATLAS_SIZE);
            m_glyphs[i].uvMin.y = pixelMinY / static_cast<float>(ATLAS_SIZE);
            m_glyphs[i].uvMax.x = pixelMaxX / static_cast<float>(ATLAS_SIZE);
            m_glyphs[i].uvMax.y = pixelMaxY / static_cast<float>(ATLAS_SIZE);
        }
    }
}
