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
        , m_textureResourceView(nullptr)
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

    void* CharacterSet::GetTextureResourceView() const
    {
        return m_textureResourceView;
    }

    void CharacterSet::Shutdown()
    {
        // Release COM objects properly
        if (m_textureResourceView)
        {
            static_cast<ID3D11ShaderResourceView*>(m_textureResourceView)->Release();
            m_textureResourceView = nullptr;
        }
        if (m_textureResource)
        {
            static_cast<ID3D11Texture2D*>(m_textureResource)->Release();
            m_textureResource = nullptr;
        }
        
        m_glyphs.clear();
        m_initialized = false;
    }

    bool CharacterSet::CreateTextureAtlas(void* d3dDevice)
    {
        if (!d3dDevice)
        {
            MessageBoxW(nullptr, L"CreateTextureAtlas: d3dDevice is null", L"Error", MB_OK | MB_ICONERROR);
            return false;
        }
        
        if (m_textureResource != nullptr)
        {
            MessageBoxW(nullptr, L"CreateTextureAtlas: texture already created", L"Error", MB_OK | MB_ICONERROR);
            return false;
        }

        ID3D11Device* device = static_cast<ID3D11Device*>(d3dDevice);

        // Create texture atlas: 2048x2048 RGBA texture
        D3D11_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width = 2048;
        textureDesc.Height = 2048;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // D2D prefers BGRA format
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        textureDesc.CPUAccessFlags = 0;
        textureDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED; // Required for Direct2D interop

        ID3D11Texture2D* texture = nullptr;
        HRESULT hr = device->CreateTexture2D(&textureDesc, nullptr, &texture);
        if (FAILED(hr) || !texture)
        {
            wchar_t errorMsg[256];
            swprintf_s(errorMsg, L"Failed to create D3D11 texture. HRESULT: 0x%08X", hr);
            MessageBoxW(nullptr, errorMsg, L"Texture Creation Error", MB_OK | MB_ICONERROR);
            return false;
        }

        m_textureResource = texture;

        // Create shader resource view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = textureDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;

        ID3D11ShaderResourceView* srv = nullptr;
        hr = device->CreateShaderResourceView(texture, &srvDesc, &srv);
        if (FAILED(hr) || !srv)
        {
            wchar_t errorMsg[256];
            swprintf_s(errorMsg, L"Failed to create shader resource view. HRESULT: 0x%08X", hr);
            MessageBoxW(nullptr, errorMsg, L"SRV Creation Error", MB_OK | MB_ICONERROR);
            texture->Release();
            m_textureResource = nullptr;
            return false;
        }

        m_textureResourceView = srv;

        // Render glyphs to the atlas
        if (!RenderGlyphsToAtlas(d3dDevice))
        {
            MessageBoxW(nullptr, L"Failed to render glyphs to atlas", L"Glyph Rendering Error", MB_OK | MB_ICONERROR);
            srv->Release();
            texture->Release();
            m_textureResourceView = nullptr;
            m_textureResource = nullptr;
            return false;
        }

        return true;
    }

    bool CharacterSet::RenderGlyphsToAtlas(void* d3dDevice)
    {
        if (!m_textureResource || !d3dDevice)
        {
            MessageBoxW(nullptr, L"RenderGlyphsToAtlas: texture resource or device is null", L"Error", MB_OK | MB_ICONERROR);
            return false;
        }

        ID3D11Device* device = static_cast<ID3D11Device*>(d3dDevice);
        ID3D11Texture2D* texture = static_cast<ID3D11Texture2D*>(m_textureResource);
        
        // Get device context
        ComPtr<ID3D11DeviceContext> context;
        device->GetImmediateContext(&context);
        if (!context)
        {
            MessageBoxW(nullptr, L"Failed to get device context", L"Error", MB_OK | MB_ICONERROR);
            return false;
        }
        
        // Create a staging texture we can write to
        D3D11_TEXTURE2D_DESC desc;
        texture->GetDesc(&desc);
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;
        
        ComPtr<ID3D11Texture2D> stagingTexture;
        HRESULT hr = device->CreateTexture2D(&desc, nullptr, &stagingTexture);
        if (FAILED(hr))
        {
            wchar_t errorMsg[256];
            swprintf_s(errorMsg, L"Failed to create staging texture. HRESULT: 0x%08X", hr);
            MessageBoxW(nullptr, errorMsg, L"Staging Texture Error", MB_OK | MB_ICONERROR);
            return false;
        }
        
        // Map the staging texture and fill with green character data
        D3D11_MAPPED_SUBRESOURCE mapped;
        hr = context->Map(stagingTexture.Get(), 0, D3D11_MAP_WRITE, 0, &mapped);
        if (FAILED(hr))
        {
            wchar_t errorMsg[256];
            swprintf_s(errorMsg, L"Failed to map staging texture. HRESULT: 0x%08X", hr);
            MessageBoxW(nullptr, errorMsg, L"Map Error", MB_OK | MB_ICONERROR);
            return false;
        }
        
        // Fill with simple green rectangles for each glyph (temporary - will be replaced with DirectWrite later)
        uint32_t* pixels = static_cast<uint32_t*>(mapped.pData);
        constexpr size_t GRID_COLS = 16;
        constexpr size_t CELL_WIDTH = 128;
        constexpr size_t CELL_HEIGHT = 120;
        
        // Clear to transparent black
        for (size_t y = 0; y < 2048; y++)
        {
            for (size_t x = 0; x < 2048; x++)
            {
                size_t index = y * (mapped.RowPitch / 4) + x;
                pixels[index] = 0x00000000; // Transparent black (BGRA format)
            }
        }
        
        // Draw simple green rectangles for each glyph cell
        for (size_t i = 0; i < m_glyphs.size(); i++)
        {
            size_t col = i % GRID_COLS;
            size_t row = i / GRID_COLS;
            
            size_t startX = col * CELL_WIDTH + 20;
            size_t startY = row * CELL_HEIGHT + 20;
            size_t endX = (col + 1) * CELL_WIDTH - 20;
            size_t endY = (row + 1) * CELL_HEIGHT - 20;
            
            for (size_t y = startY; y < endY && y < 2048; y++)
            {
                for (size_t x = startX; x < endX && x < 2048; x++)
                {
                    size_t index = y * (mapped.RowPitch / 4) + x;
                    pixels[index] = 0xFF00FF00; // Bright green (BGRA format)
                }
            }
        }
        
        context->Unmap(stagingTexture.Get(), 0);
        
        // Copy from staging texture to actual texture
        context->CopyResource(texture, stagingTexture.Get());
        
        return true;

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
