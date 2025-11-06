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
        if (!d3dDevice || m_textureResource != nullptr)
        {
            return false;
        }

        ID3D11Device* device = static_cast<ID3D11Device*>(d3dDevice);

        // Create texture atlas: 2048x2048 RGBA texture
        D3D11_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width = 2048;
        textureDesc.Height = 2048;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        textureDesc.CPUAccessFlags = 0;
        textureDesc.MiscFlags = 0;

        ID3D11Texture2D* texture = nullptr;
        HRESULT hr = device->CreateTexture2D(&textureDesc, nullptr, &texture);
        if (FAILED(hr) || !texture)
        {
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
            texture->Release();
            m_textureResource = nullptr;
            return false;
        }

        m_textureResourceView = srv;

        // Render glyphs to the atlas
        if (!RenderGlyphsToAtlas(d3dDevice))
        {
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
        (void)d3dDevice; // Currently unused, but may be needed for future D3D11-based rendering
        
        if (!m_textureResource)
        {
            return false;
        }

        ID3D11Texture2D* texture = static_cast<ID3D11Texture2D*>(m_textureResource);

        // Create D2D factory
        ComPtr<ID2D1Factory> d2dFactory;
        HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2dFactory.GetAddressOf());
        if (FAILED(hr))
        {
            return false;
        }

        // Create DirectWrite factory
        ComPtr<IDWriteFactory> dwFactory;
        hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), 
                                 reinterpret_cast<IUnknown**>(dwFactory.GetAddressOf()));
        if (FAILED(hr))
        {
            return false;
        }

        // Get DXGI surface from texture
        ComPtr<IDXGISurface> dxgiSurface;
        hr = texture->QueryInterface(__uuidof(IDXGISurface), &dxgiSurface);
        if (FAILED(hr))
        {
            return false;
        }

        // Create D2D render target from DXGI surface
        D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
            96.0f, 96.0f
        );

        ComPtr<ID2D1RenderTarget> renderTarget;
        hr = d2dFactory->CreateDxgiSurfaceRenderTarget(dxgiSurface.Get(), &rtProps, &renderTarget);
        if (FAILED(hr))
        {
            return false;
        }

        // Create text format (Consolas, 80pt for good quality at 128x120 cells)
        ComPtr<IDWriteTextFormat> textFormat;
        hr = dwFactory->CreateTextFormat(
            L"Consolas",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            80.0f,
            L"en-us",
            &textFormat
        );
        if (FAILED(hr))
        {
            return false;
        }

        textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        // Create green brush for rendering
        ComPtr<ID2D1SolidColorBrush> brush;
        hr = renderTarget->CreateSolidColorBrush(
            D2D1::ColorF(0.0f, 1.0f, 0.0f, 1.0f), // Bright green
            &brush
        );
        if (FAILED(hr))
        {
            return false;
        }

        // Begin rendering
        renderTarget->BeginDraw();
        renderTarget->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f)); // Transparent black

        // Render each glyph
        constexpr size_t GRID_COLS = 16;
        constexpr size_t CELL_WIDTH = 128;
        constexpr size_t CELL_HEIGHT = 120;

        for (size_t i = 0; i < m_glyphs.size(); i++)
        {
            size_t col = i % GRID_COLS;
            size_t row = i / GRID_COLS;

            // Calculate cell rectangle
            D2D1_RECT_F cellRect = D2D1::RectF(
                static_cast<FLOAT>(col * CELL_WIDTH),
                static_cast<FLOAT>(row * CELL_HEIGHT),
                static_cast<FLOAT>((col + 1) * CELL_WIDTH),
                static_cast<FLOAT>((row + 1) * CELL_HEIGHT)
            );

            // Convert codepoint to wstring
            wchar_t charStr[2] = { static_cast<wchar_t>(m_glyphs[i].codepoint), 0 };

            // Apply mirroring transform if needed
            if (m_glyphs[i].mirrored)
            {
                // Save current transform
                D2D1_MATRIX_3X2_F oldTransform;
                renderTarget->GetTransform(&oldTransform);

                // Apply horizontal flip around cell center
                FLOAT centerX = (cellRect.left + cellRect.right) / 2.0f;
                D2D1_MATRIX_3X2_F flipTransform = D2D1::Matrix3x2F::Scale(-1.0f, 1.0f, D2D1::Point2F(centerX, 0.0f));
                renderTarget->SetTransform(flipTransform * oldTransform);

                // Draw text
                renderTarget->DrawText(charStr, 1, textFormat.Get(), cellRect, brush.Get());

                // Restore transform
                renderTarget->SetTransform(oldTransform);
            }
            else
            {
                // Draw text normally
                renderTarget->DrawText(charStr, 1, textFormat.Get(), cellRect, brush.Get());
            }
        }

        hr = renderTarget->EndDraw();
        if (FAILED(hr))
        {
            return false;
        }

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
