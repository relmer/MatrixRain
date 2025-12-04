#include "pch.h"

#include "CharacterSet.h"
#include "CharacterConstants.h"





using Microsoft::WRL::ComPtr;





CharacterSet::~CharacterSet()
{
    Shutdown();
}





CharacterSet & CharacterSet::GetInstance()
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
        glyph.mirrored  = false;
        glyph.uvMin     = Vector2 (0.0f, 0.0f); // Will be calculated later
        glyph.uvMax     = Vector2 (0.0f, 0.0f);

        m_glyphs.push_back (glyph);
    }
    
    // Create glyph info for mirrored characters
    for (size_t i = 0; i < codepoints.size(); i++)
    {
        GlyphInfo glyph;

        glyph.codepoint = codepoints[i];
        glyph.mirrored  = true;
        glyph.uvMin     = Vector2 (0.0f, 0.0f); // Will be calculated later
        glyph.uvMax     = Vector2 (0.0f, 0.0f);

        m_glyphs.push_back (glyph);
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
    static std::random_device rd;
    static std::mt19937       gen (rd());



    if (m_glyphs.empty())
    {
        return 0;
    }

    // Create distribution fresh each time to avoid stale size issues
    std::uniform_int_distribution<size_t> dist (0, m_glyphs.size() - 1);
    return dist (gen);
}





const GlyphInfo & CharacterSet::GetGlyph (size_t index) const
{
    static GlyphInfo defaultGlyph;
    
    if (m_glyphs.empty() || index >= m_glyphs.size())
    {
        return defaultGlyph;
    }

    return m_glyphs[index];
}





size_t CharacterSet::GetGlyphCount() const
{
    return m_glyphs.size();
}





void * CharacterSet::GetTextureResource() const
{
    return m_textureResource.Get();
}





void * CharacterSet::GetTextureResourceView() const
{
    return m_textureResourceView.Get();
}





void CharacterSet::Shutdown()
{
    m_textureResourceView.Reset();
    m_textureResource.Reset();
    m_glyphs.clear();
    m_initialized = false;
}





HRESULT CharacterSet::CreateTextureAtlas (void * d3dDevice)
{
    HRESULT                            hr          = S_OK;
    ID3D11Device                     * device      = static_cast<ID3D11Device *> (d3dDevice);
    D3D11_TEXTURE2D_DESC               textureDesc = {};
    D3D11_SHADER_RESOURCE_VIEW_DESC    srvDesc     = {};
    


    CBRAEx (d3dDevice != nullptr, E_INVALIDARG);
    CBRAEx (!m_textureResource,   E_UNEXPECTED);
    

    // Create texture atlas: 2048x2048 RGBA texture
    textureDesc.Width              = 2048;
    textureDesc.Height             = 2048;
    textureDesc.MipLevels          = 1;
    textureDesc.ArraySize          = 1;
    textureDesc.Format             = DXGI_FORMAT_B8G8R8A8_UNORM; // D2D prefers BGRA format
    textureDesc.SampleDesc.Count   = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage              = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags          = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    textureDesc.CPUAccessFlags     = 0;
    textureDesc.MiscFlags          = D3D11_RESOURCE_MISC_SHARED; // Required for Direct2D interop

    hr = device->CreateTexture2D (&textureDesc, nullptr, &m_textureResource);
    CHRA (hr);
    
    // Create shader resource view
    srvDesc.Format                    = textureDesc.Format;
    srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels       = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    hr = device->CreateShaderResourceView (m_textureResource.Get(), &srvDesc, &m_textureResourceView);
    CHRA (hr);

    // Render glyphs to the atlas
    hr = RenderGlyphsToAtlas (d3dDevice);
    CHRA (hr);

    
Error:
    if (FAILED (hr))
    {
        m_textureResourceView.Reset();
        m_textureResource.Reset();
    }

    return hr;
}





HRESULT CharacterSet::RenderGlyphsToAtlas (void * d3dDevice)
{
    HRESULT                              hr              = S_OK;
    ID3D11Device                       * device          = static_cast<ID3D11Device *> (d3dDevice);
    ID3D11Texture2D                    * texture         = m_textureResource.Get();
    ComPtr<IDXGIDevice>                  dxgiDevice;
    ComPtr<ID2D1Factory1>                d2dFactory;
    ComPtr<ID2D1Device>                  d2dDevice;
    ComPtr<ID2D1DeviceContext>           d2dContext;
    ComPtr<IDXGISurface>                 dxgiSurface;
    ComPtr<ID2D1Bitmap1>                 d2dBitmap;
    ComPtr<IDWriteFactory>               dwriteFactory;
    ComPtr<IDWriteTextFormat>            textFormat;
    ComPtr<ID2D1SolidColorBrush>         brush;
    D2D1_FACTORY_OPTIONS                 options         = {};
    D2D1_BITMAP_PROPERTIES1              bitmapProps     = {};
    constexpr size_t                     GRID_COLS       = 16;
    constexpr size_t                     CELL_WIDTH      = 128;
    constexpr size_t                     CELL_HEIGHT     = 120;



    CBRAEx (m_textureResource.Get() != nullptr, E_UNEXPECTED);
    CBRAEx (d3dDevice != nullptr,               E_INVALIDARG);
    
    // Get DXGI device for D2D
    hr = device->QueryInterface (__uuidof (IDXGIDevice), &dxgiDevice);
    CHRA (hr);
    
    // Create D2D factory
#ifdef _DEBUG
    options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
    hr = D2D1CreateFactory (D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof (ID2D1Factory1), &options, 
                            reinterpret_cast<void **> (d2dFactory.GetAddressOf()));
    CHRA (hr);
    
    // Create D2D device
    hr = d2dFactory->CreateDevice (dxgiDevice.Get(), &d2dDevice);
    CHRA (hr);
    
    // Create D2D device context
    hr = d2dDevice->CreateDeviceContext (D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2dContext);
    CHRA (hr);
    
    // Get DXGI surface from the D3D11 texture
    hr = texture->QueryInterface (__uuidof (IDXGISurface), &dxgiSurface);
    CHRA (hr);

    // Create D2D bitmap from DXGI surface
    bitmapProps = D2D1::BitmapProperties1 (
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat (DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );

    hr = d2dContext->CreateBitmapFromDxgiSurface (dxgiSurface.Get(), &bitmapProps, &d2dBitmap);
    CHRA (hr);

    // Set the bitmap as the render target
    d2dContext->SetTarget (d2dBitmap.Get());

    // Create DirectWrite factory
    hr = DWriteCreateFactory (DWRITE_FACTORY_TYPE_SHARED, __uuidof (IDWriteFactory), 
                                reinterpret_cast<IUnknown **> (dwriteFactory.GetAddressOf()));
    CHRA (hr);

    // Create text format (Consolas, 80pt, centered)
    hr = dwriteFactory->CreateTextFormat (L"Consolas",
                                            nullptr,
                                            DWRITE_FONT_WEIGHT_NORMAL,
                                            DWRITE_FONT_STYLE_NORMAL,
                                            DWRITE_FONT_STRETCH_NORMAL,
                                            80.0f,
                                            L"en-us",
                                            &textFormat);
    CHRA (hr);

    textFormat->SetTextAlignment      (DWRITE_TEXT_ALIGNMENT_CENTER);
    textFormat->SetParagraphAlignment (DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Create brush (white - color will be applied by pixel shader)
    hr = d2dContext->CreateSolidColorBrush (D2D1::ColorF (1.0f, 1.0f, 1.0f, 1.0f), &brush);
    CHRA (hr);

    // Begin drawing
    d2dContext->BeginDraw();
    d2dContext->Clear (D2D1::ColorF (0.0f, 0.0f, 0.0f, 0.0f)); // Transparent black

    // Render all glyphs
    for (size_t i = 0; i < m_glyphs.size(); i++)
    {
        size_t col = i % GRID_COLS;
        size_t row = i / GRID_COLS;
        
        float x = static_cast<float> (col * CELL_WIDTH);
        float y = static_cast<float> (row * CELL_HEIGHT);
        
        D2D1_RECT_F rect = D2D1::RectF (x, y, x + CELL_WIDTH, y + CELL_HEIGHT);
        
        // For glyphs in the second half (indices 134-267), apply horizontal mirroring
        if (i >= 134)
        {
            d2dContext->SetTransform (
                D2D1::Matrix3x2F::Scale (-1.0f, 1.0f, D2D1::Point2F (x + CELL_WIDTH / 2, y + CELL_HEIGHT / 2))
            );
        }
        
        wchar_t text[2] = { static_cast<wchar_t> (m_glyphs[i].codepoint), L'\0' };
        d2dContext->DrawText (text, 1, textFormat.Get(), rect, brush.Get());
        
        // Reset transform if it was changed
        if (i >= 134)
        {
            d2dContext->SetTransform (D2D1::Matrix3x2F::Identity());
        }
    }

    hr = d2dContext->EndDraw();
    CHRA (hr);
    
Error:
    return hr;
}





void CharacterSet::CalculateUVCoordinates()
{
    // Layout: 2048x2048 texture atlas
    // Grid arrangement: 16 columns × 17 rows = 272 cells (268 used)
    // Each glyph cell: 128x120 pixels (with 8 pixel padding per cell)
    
    constexpr size_t ATLAS_SIZE    = 2048;
    constexpr size_t GRID_COLS     = 16;
    constexpr size_t GRID_ROWS     = 17;
    constexpr size_t CELL_WIDTH    = ATLAS_SIZE / GRID_COLS;   // 128 pixels
    constexpr size_t CELL_HEIGHT   = ATLAS_SIZE / GRID_ROWS;  // 120 pixels
    constexpr size_t GLYPH_PADDING = 8;  // Padding to prevent texture bleeding
    


    for (size_t i = 0; i < m_glyphs.size(); i++)
    {
        size_t col = i % GRID_COLS;
        size_t row = i / GRID_COLS;
        
        // Calculate pixel coordinates with padding
        float pixelMinX = static_cast<float> (col * CELL_WIDTH + GLYPH_PADDING);
        float pixelMinY = static_cast<float> (row * CELL_HEIGHT + GLYPH_PADDING);
        float pixelMaxX = static_cast<float> ((col + 1) * CELL_WIDTH - GLYPH_PADDING);
        float pixelMaxY = static_cast<float> ((row + 1) * CELL_HEIGHT - GLYPH_PADDING);
        
        // Convert to UV coordinates [0, 1]
        m_glyphs[i].uvMin.x = pixelMinX / static_cast<float> (ATLAS_SIZE);
        m_glyphs[i].uvMin.y = pixelMinY / static_cast<float> (ATLAS_SIZE);
        m_glyphs[i].uvMax.x = pixelMaxX / static_cast<float> (ATLAS_SIZE);
        m_glyphs[i].uvMax.y = pixelMaxY / static_cast<float> (ATLAS_SIZE);
    }
}
