#include "pch.h"

#include "CharacterSet.h"
#include "CharacterConstants.h"





using Microsoft::WRL::ComPtr;





////////////////////////////////////////////////////////////////////////////////
//
//  CharacterSet::~CharacterSet
//
////////////////////////////////////////////////////////////////////////////////

CharacterSet::~CharacterSet()
{
    Shutdown();
}





////////////////////////////////////////////////////////////////////////////////
//
//  CharacterSet::GetInstance
//
//  Returns the singleton instance.
//
////////////////////////////////////////////////////////////////////////////////

CharacterSet & CharacterSet::GetInstance()
{
    static CharacterSet s_instance;

    return s_instance;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CharacterSet::Initialize
//
//  Populates the glyph table with paired normal/mirrored rain codepoints
//  and overlay symbols.  Measures proportional advance widths for overlay
//  text layout.
//
////////////////////////////////////////////////////////////////////////////////

bool CharacterSet::Initialize()
{
    if (m_initialized)
    {
        return true;
    }

    // Get all character codepoints (134 rain + 5 overlay symbols)
    auto codepoints        = CharacterConstants::GetAllCodepoints();
    auto overlayCodepoints = CharacterConstants::GetOverlayCodepoints();

    // Reserve space: 134 normal + 134 mirrored + 5 overlay = 273 total
    m_glyphs.clear();
    m_glyphs.reserve (codepoints.size() * 2 + overlayCodepoints.size());

    m_codepointToGlyph.clear();
    m_codepointToGlyph.reserve (codepoints.size() + overlayCodepoints.size());

    // Create glyph info for rain characters (normal + mirrored pairs)
    for (const auto & cp : codepoints)
    {
        GlyphInfo glyph;

        m_codepointToGlyph[cp] = m_glyphs.size();

        glyph.codepoint = cp;
        glyph.mirrored  = false;
        glyph.uvMin     = Vector2 (0.0f, 0.0f);
        glyph.uvMax     = Vector2 (0.0f, 0.0f);

        m_glyphs.push_back (glyph);

        glyph.mirrored = true;
        m_glyphs.push_back (glyph);
    }

    // Store rain-only glyph count (normal + mirrored, before overlay symbols)
    m_rainGlyphCount = codepoints.size() * 2;

    // Append overlay-only symbols (non-mirrored)
    for (const auto & cp : overlayCodepoints)
    {
        GlyphInfo glyph;

        m_codepointToGlyph[cp] = m_glyphs.size();

        glyph.codepoint = cp;
        glyph.mirrored  = false;
        glyph.uvMin     = Vector2 (0.0f, 0.0f);
        glyph.uvMax     = Vector2 (0.0f, 0.0f);

        m_glyphs.push_back (glyph);
    }

    // Calculate UV coordinates for 16x17 grid (272 rain glyphs)
    CalculateUVCoordinates();

    // Measure proportional advance widths using Segoe UI (for overlay positioning)
    MeasureProportionalAdvanceWidths (codepoints, overlayCodepoints);

    m_initialized = true;

    return true;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CharacterSet::GetRandomGlyphIndex
//
//  Returns a random index in the range [0, count).
//
////////////////////////////////////////////////////////////////////////////////

size_t CharacterSet::GetRandomGlyphIndex (size_t count) const
{
    static thread_local std::random_device s_rd;
    static thread_local std::mt19937       s_gen (s_rd());



    if (count == 0)
    {
        return 0;
    }

    std::uniform_int_distribution<size_t> dist (0, count - 1);
    return dist (s_gen);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CharacterSet::FindGlyphByCodepoint
//
//  Returns the glyph index for a non-mirrored glyph matching the given Unicode
//  codepoint, or SIZE_MAX if the codepoint is not in the atlas.
//
////////////////////////////////////////////////////////////////////////////////

size_t CharacterSet::FindGlyphByCodepoint (uint32_t codepoint) const
{
    auto it = m_codepointToGlyph.find (codepoint);

    if (it != m_codepointToGlyph.end())
    {
        return it->second;
    }

    return SIZE_MAX;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CharacterSet::MeasureProportionalAdvanceWidths
//
//  Uses DWrite with Segoe UI (a proportional font) to measure the advance
//  width of each codepoint.  Advance widths are stored normalized to the
//  em-height so callers can scale to any display size.  Using Segoe UI
//  rather than the atlas font (Consolas) gives tighter proportional spacing
//  that matches the original D2D overlay rendering.
//
////////////////////////////////////////////////////////////////////////////////

void CharacterSet::MeasureProportionalAdvanceWidths (const std::vector<uint32_t> & rainCodepoints,
                                                     const std::vector<uint32_t> & overlayCodepoints)
{
    ComPtr<IDWriteFactory>    dwriteFactory;
    ComPtr<IDWriteTextFormat> textFormat;
    HRESULT                   hr             = S_OK;
    float                     fontSize       = 100.0f;



    hr = DWriteCreateFactory (DWRITE_FACTORY_TYPE_SHARED,
                              __uuidof (IDWriteFactory),
                              reinterpret_cast<IUnknown **> (dwriteFactory.GetAddressOf()));
    CHRA (hr);

    hr = dwriteFactory->CreateTextFormat (L"Segoe UI",
                                          nullptr,
                                          DWRITE_FONT_WEIGHT_NORMAL,
                                          DWRITE_FONT_STYLE_NORMAL,
                                          DWRITE_FONT_STRETCH_NORMAL,
                                          fontSize,
                                          L"en-us",
                                          &textFormat);
    CHRA (hr);

    // Measure rain codepoints (normal + mirrored pairs, indices 0,1,2,3,...267)
    for (size_t i = 0; i < rainCodepoints.size(); i++)
    {
        m_glyphs[i * 2].advanceWidth = MeasureCodepointAdvanceWidth (dwriteFactory.Get(), textFormat.Get(),
                                                                     rainCodepoints[i], fontSize, false);

        // Mirrored glyph gets the same advance width
        m_glyphs[i * 2 + 1].advanceWidth = m_glyphs[i * 2].advanceWidth;
    }

    // Measure overlay symbol codepoints (indices 268..272)
    for (size_t i = 0; i < overlayCodepoints.size(); i++)
    {
        size_t glyphIdx = rainCodepoints.size() * 2 + i;

        m_glyphs[glyphIdx].advanceWidth = MeasureCodepointAdvanceWidth (dwriteFactory.Get(), textFormat.Get(),
                                                                        overlayCodepoints[i], fontSize, true);
    }

    // Measure space character advance width
    m_spaceAdvanceWidth = MeasureCodepointAdvanceWidth (dwriteFactory.Get(), textFormat.Get(),
                                                        0x0020, fontSize, true);

Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CharacterSet::MeasureCodepointAdvanceWidth
//
//  Measures the advance width of a single codepoint using DirectWrite,
//  returned as a fraction of em-height.  When includeTrailingWhitespace
//  is true, uses widthIncludingTrailingWhitespace (needed for space).
//
////////////////////////////////////////////////////////////////////////////////

float CharacterSet::MeasureCodepointAdvanceWidth (IDWriteFactory * pFactory,
                                                  IDWriteTextFormat * pFormat,
                                                  uint32_t codepoint,
                                                  float fontSize,
                                                  bool includeTrailingWhitespace)
{
    HRESULT                   hr      = S_OK;
    float                     advance = 0.0f;
    wchar_t                   text[3] = {};
    int                       len     = 0;
    ComPtr<IDWriteTextLayout> layout;
    DWRITE_TEXT_METRICS       metrics = {};



    // Convert codepoint to UTF-16
    if (codepoint <= 0xFFFF)
    {
        text[0] = static_cast<wchar_t> (codepoint);
        len     = 1;
    }
    else
    {
        uint32_t cp = codepoint - 0x10000;

        text[0] = static_cast<wchar_t> (0xD800 + (cp >> 10));
        text[1] = static_cast<wchar_t> (0xDC00 + (cp & 0x3FF));
        len     = 2;
    }

    hr = pFactory->CreateTextLayout (text,
                                     static_cast<UINT32> (len),
                                     pFormat,
                                     10000.0f,
                                     10000.0f,
                                     &layout);
    CHRA (hr);

    layout->GetMetrics (&metrics);
    advance = (includeTrailingWhitespace ? metrics.widthIncludingTrailingWhitespace : metrics.width) / fontSize;



Error:
    return advance;
}











////////////////////////////////////////////////////////////////////////////////
//
//  CharacterSet::Shutdown
//
//  Releases all GPU resources and clears glyph data.
//
////////////////////////////////////////////////////////////////////////////////

void CharacterSet::Shutdown()
{
    m_overlayUVs.clear();
    m_overlayTextureResourceView.Reset();
    m_overlayTextureResource.Reset();
    m_textureResourceView.Reset();
    m_textureResource.Reset();
    m_glyphs.clear();
    m_initialized = false;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CharacterSet::RecreateOverlayAtlas
//
//  Destroys the existing overlay atlas and recreates it at the new DPI scale.
//  Called when the system DPI changes (WM_DPICHANGED).
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CharacterSet::RecreateOverlayAtlas (ID3D11Device * d3dDevice, float dpiScale)
{
    m_overlayUVs.clear();
    m_overlayTextureResourceView.Reset();
    m_overlayTextureResource.Reset();

    return CreateOverlayAtlas (d3dDevice, dpiScale);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CharacterSet::CreateTextureAtlas
//
//  Creates the 2048x2048 rain glyph atlas and the DPI-aware overlay atlas.
//  Both atlases share the same D3D11 device.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CharacterSet::CreateTextureAtlas (ID3D11Device * d3dDevice, float dpiScale)
{
    HRESULT                            hr          = S_OK;
    ID3D11Device                     * device      = d3dDevice;
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
    hr = RenderGlyphsToAtlas (device);
    CHRA (hr);

    // Create overlay atlas (DPI-aware, sized for 1:1 texel-to-pixel mapping)
    hr = CreateOverlayAtlas (device, dpiScale);
    CHR (hr);

    
Error:
    if (FAILED (hr))
    {
        m_overlayUVs.clear();
        m_overlayTextureResourceView.Reset();
        m_overlayTextureResource.Reset();
        m_textureResourceView.Reset();
        m_textureResource.Reset();
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CharacterSet::CreateOverlayAtlas
//
//  Creates the overlay texture atlas at DPI-appropriate dimensions for 1:1
//  texel-to-pixel mapping.  Cell sizes are computed so that glyphs rendered
//  at the target font size fill each cell, eliminating the quality loss from
//  downsampling a large atlas.  The atlas is recreated on DPI changes.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CharacterSet::CreateOverlayAtlas (ID3D11Device * d3dDevice, float dpiScale)
{
    HRESULT                            hr          = S_OK;
    ID3D11Device                     * device      = d3dDevice;
    D3D11_TEXTURE2D_DESC               textureDesc = {};
    D3D11_SHADER_RESOURCE_VIEW_DESC    srvDesc     = {};
    int                                fontSize    = 0;
    int                                gridCols    = 16;
    int                                gridRows    = 18;



    CBRAEx (d3dDevice != nullptr, E_INVALIDARG);

    // Compute atlas dimensions for target pixel size at current DPI.
    // Font size: 12pt Segoe UI = 16 DIP at 96 DPI, scaled by dpiScale.
    // Cell content proportions match rain atlas: 1.4x em width, 1.3x em height.
    //
    // Display dimensions are the on-screen quad size.  The atlas renders
    // at 3x resolution for high-quality supersampled anti-aliasing and
    // relies on the linear texture sampler to downsample.
    static constexpr int SUPERSAMPLE = 3;

    fontSize                   = static_cast<int> (ceil (16.0f * dpiScale));
    m_overlayDisplayWidth      = static_cast<int> (ceil (static_cast<float> (fontSize) * 1.4f));
    m_overlayDisplayHeight     = static_cast<int> (ceil (static_cast<float> (fontSize) * 1.3f));
    m_overlayFontSize          = static_cast<float> (fontSize * SUPERSAMPLE);
    m_overlayCellContentWidth  = m_overlayDisplayWidth  * SUPERSAMPLE;
    m_overlayCellContentHeight = m_overlayDisplayHeight * SUPERSAMPLE;
    m_overlayPadding           = std::max (2, static_cast<int> (ceil (m_overlayFontSize * 0.15f)));
    m_overlayAtlasWidth        = gridCols * (m_overlayCellContentWidth  + 2 * m_overlayPadding);
    m_overlayAtlasHeight       = gridRows * (m_overlayCellContentHeight + 2 * m_overlayPadding);

    // Create overlay texture atlas at computed dimensions
    textureDesc.Width              = static_cast<UINT> (m_overlayAtlasWidth);
    textureDesc.Height             = static_cast<UINT> (m_overlayAtlasHeight);
    textureDesc.MipLevels          = 1;
    textureDesc.ArraySize          = 1;
    textureDesc.Format             = DXGI_FORMAT_B8G8R8A8_UNORM;
    textureDesc.SampleDesc.Count   = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage              = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags          = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    textureDesc.CPUAccessFlags     = 0;
    textureDesc.MiscFlags          = D3D11_RESOURCE_MISC_SHARED;

    hr = device->CreateTexture2D (&textureDesc, nullptr, &m_overlayTextureResource);
    CHRA (hr);

    srvDesc.Format                    = textureDesc.Format;
    srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels       = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    hr = device->CreateShaderResourceView (m_overlayTextureResource.Get(), &srvDesc, &m_overlayTextureResourceView);
    CHRA (hr);

    hr = RenderOverlayGlyphsToAtlas (device);
    CHRA (hr);

    CalculateOverlayUVCoordinates();

Error:
    if (FAILED (hr))
    {
        m_overlayUVs.clear();
        m_overlayTextureResourceView.Reset();
        m_overlayTextureResource.Reset();
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CharacterSet::CreateD2DRenderContext
//
//  Creates a Direct2D device context, DirectWrite factory, and white brush
//  for rendering glyphs into the given texture atlas.  Shared setup used by
//  both RenderGlyphsToAtlas and RenderOverlayGlyphsToAtlas.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CharacterSet::CreateD2DRenderContext (ID3D11Device          * d3dDevice,
                                              ID3D11Texture2D       * texture,
                                              ID2D1DeviceContext   ** ppContext,
                                              IDWriteFactory       ** ppDWriteFactory,
                                              ID2D1SolidColorBrush ** ppBrush)
{
    HRESULT                    hr            = S_OK;
    ComPtr<IDXGIDevice>        dxgiDevice;
    ComPtr<ID2D1Factory1>      d2dFactory;
    ComPtr<ID2D1Device>        d2dDevice;
    ComPtr<ID2D1DeviceContext> d2dContext;
    ComPtr<IDXGISurface>       dxgiSurface;
    ComPtr<ID2D1Bitmap1>       d2dBitmap;
    ComPtr<IDWriteFactory>     dwriteFactory;
    D2D1_FACTORY_OPTIONS       options       = {};
    D2D1_BITMAP_PROPERTIES1    bitmapProps   = {};



    CBRAEx (d3dDevice != nullptr, E_INVALIDARG);
    CBRAEx (texture   != nullptr, E_INVALIDARG);
    CBRAEx (ppContext != nullptr, E_INVALIDARG);

    // Get DXGI device for D2D
    hr = d3dDevice->QueryInterface (__uuidof (IDXGIDevice), &dxgiDevice);
    CHRA (hr);

    // Create D2D factory
#ifdef _DEBUG
    options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
    hr = D2D1CreateFactory (D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof (ID2D1Factory1), &options,
                            reinterpret_cast<void **> (d2dFactory.GetAddressOf()));
    CHRA (hr);

    // Create D2D device and context
    hr = d2dFactory->CreateDevice (dxgiDevice.Get(), &d2dDevice);
    CHRA (hr);

    hr = d2dDevice->CreateDeviceContext (D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2dContext);
    CHRA (hr);

    // Get DXGI surface and create D2D bitmap render target
    hr = texture->QueryInterface (__uuidof (IDXGISurface), &dxgiSurface);
    CHRA (hr);

    bitmapProps = D2D1::BitmapProperties1 (
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat (DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );

    hr = d2dContext->CreateBitmapFromDxgiSurface (dxgiSurface.Get(), &bitmapProps, &d2dBitmap);
    CHRA (hr);

    d2dContext->SetTarget (d2dBitmap.Get());

    // Create DirectWrite factory
    hr = DWriteCreateFactory (DWRITE_FACTORY_TYPE_SHARED, __uuidof (IDWriteFactory),
                              reinterpret_cast<IUnknown **> (dwriteFactory.GetAddressOf()));
    CHRA (hr);

    // Create white brush (color will be applied by pixel shader)
    hr = d2dContext->CreateSolidColorBrush (D2D1::ColorF (1.0f, 1.0f, 1.0f, 1.0f), ppBrush);
    CHRA (hr);

    // Transfer ownership to caller
    *ppContext       = d2dContext.Detach();
    *ppDWriteFactory = dwriteFactory.Detach();

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CharacterSet::RenderGlyphsToAtlas
//
//  Renders all rain glyphs (normal and mirrored) into the 2048x2048 texture
//  atlas using Direct2D/DirectWrite.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CharacterSet::RenderGlyphsToAtlas (ID3D11Device * d3dDevice)
{
    HRESULT                      hr             = S_OK;
    ComPtr<ID2D1DeviceContext>   d2dContext;
    ComPtr<IDWriteFactory>       dwriteFactory;
    ComPtr<IDWriteTextFormat>    textFormat;
    ComPtr<ID2D1SolidColorBrush> brush;
    constexpr size_t             GRID_COLS      = 16;
    constexpr size_t             CELL_WIDTH     = 128;
    constexpr size_t             CELL_HEIGHT    = 120;
    size_t                       rainGlyphCount = std::min (m_glyphs.size(), GRID_COLS * static_cast<size_t> (17));



    CBRAEx (m_textureResource.Get() != nullptr, E_UNEXPECTED);

    hr = CreateD2DRenderContext (d3dDevice, m_textureResource.Get(),
                                &d2dContext, &dwriteFactory, &brush);
    CHR (hr);

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

    // Begin drawing
    d2dContext->BeginDraw();
    d2dContext->Clear (D2D1::ColorF (0.0f, 0.0f, 0.0f, 0.0f));

    // Render all glyphs (rain atlas only holds 272 glyphs — overlay-only symbols are excluded)
    for (size_t i = 0; i < rainGlyphCount; i++)
    {
        size_t col = i % GRID_COLS;
        size_t row = i / GRID_COLS;

        float x = static_cast<float> (col * CELL_WIDTH);
        float y = static_cast<float> (row * CELL_HEIGHT);

        D2D1_RECT_F rect = D2D1::RectF (x, y, x + CELL_WIDTH, y + CELL_HEIGHT);

        // For mirrored glyphs, apply horizontal mirroring
        if (m_glyphs[i].mirrored)
        {
            d2dContext->SetTransform (
                D2D1::Matrix3x2F::Scale (-1.0f, 1.0f, D2D1::Point2F (x + CELL_WIDTH / 2, y + CELL_HEIGHT / 2))
            );
        }

        wchar_t text[2] = { static_cast<wchar_t> (m_glyphs[i].codepoint), L'\0' };
        d2dContext->DrawText (text, 1, textFormat.Get(), rect, brush.Get());

        // Reset transform if it was changed
        if (m_glyphs[i].mirrored)
        {
            d2dContext->SetTransform (D2D1::Matrix3x2F::Identity());
        }
    }

    hr = d2dContext->EndDraw();
    CHRA (hr);

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CharacterSet::RenderOverlayGlyphsToAtlas
//
//  Renders all glyphs into the overlay atlas using Segoe UI (proportional
//  font) instead of Consolas.  This gives overlay text natural proportional
//  glyph shapes that match the Segoe UI advance widths used for positioning.
//  Same grid layout and cell sizes as the rain atlas so UV coordinates are
//  shared.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CharacterSet::RenderOverlayGlyphsToAtlas (ID3D11Device * d3dDevice)
{
    HRESULT                      hr            = S_OK;
    ComPtr<ID2D1DeviceContext>   d2dContext;
    ComPtr<IDWriteFactory>       dwriteFactory;
    ComPtr<IDWriteTextFormat>    textFormat;
    ComPtr<ID2D1SolidColorBrush> brush;
    constexpr size_t             GRID_COLS     = 16;
    int                          cellWidth     = m_overlayCellContentWidth  + 2 * m_overlayPadding;
    int                          cellHeight    = m_overlayCellContentHeight + 2 * m_overlayPadding;



    CBRAEx (m_overlayTextureResource.Get() != nullptr, E_UNEXPECTED);

    hr = CreateD2DRenderContext (d3dDevice, m_overlayTextureResource.Get(),
                                &d2dContext, &dwriteFactory, &brush);
    CHR (hr);

    // Grayscale anti-aliasing for best quality on transparent background
    // (ClearType requires an opaque background and cannot be used with atlas textures)
    d2dContext->SetTextAntialiasMode (D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

    // Create text format (Segoe UI, left-aligned at target pixel size)
    // Font size matches the atlas cell content so glyphs are rendered at the
    // exact pixel size they will be displayed on screen (1:1 texel-to-pixel).
    hr = dwriteFactory->CreateTextFormat (L"Segoe UI",
                                          nullptr,
                                          DWRITE_FONT_WEIGHT_NORMAL,
                                          DWRITE_FONT_STYLE_NORMAL,
                                          DWRITE_FONT_STRETCH_NORMAL,
                                          m_overlayFontSize,
                                          L"en-us",
                                          &textFormat);
    CHRA (hr);

    textFormat->SetTextAlignment      (DWRITE_TEXT_ALIGNMENT_LEADING);
    textFormat->SetParagraphAlignment (DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Begin drawing
    d2dContext->BeginDraw();
    d2dContext->Clear (D2D1::ColorF (0.0f, 0.0f, 0.0f, 0.0f));

    // Render all glyphs with Segoe UI
    for (size_t i = 0; i < m_glyphs.size(); i++)
    {
        size_t col = i % GRID_COLS;
        size_t row = i / GRID_COLS;

        // Use the padded rect that matches the UV coordinate boundaries.
        // This ensures left-aligned glyphs start exactly at uvMin.x.
        float x = static_cast<float> (col * cellWidth  + m_overlayPadding);
        float y = static_cast<float> (row * cellHeight + m_overlayPadding);

        D2D1_RECT_F rect = D2D1::RectF (x, y, x + static_cast<float> (m_overlayCellContentWidth), y + static_cast<float> (m_overlayCellContentHeight));

        // For mirrored glyphs, apply horizontal mirroring around the padded rect center
        if (m_glyphs[i].mirrored)
        {
            float cellCenterX = static_cast<float> (col * cellWidth + cellWidth / 2);
            float cellCenterY = static_cast<float> (row * cellHeight + cellHeight / 2);

            d2dContext->SetTransform (
                D2D1::Matrix3x2F::Scale (-1.0f, 1.0f, D2D1::Point2F (cellCenterX, cellCenterY))
            );
        }

        wchar_t text[2] = { static_cast<wchar_t> (m_glyphs[i].codepoint), L'\0' };
        d2dContext->DrawText (text, 1, textFormat.Get(), rect, brush.Get());

        // Reset transform if it was changed
        if (m_glyphs[i].mirrored)
        {
            d2dContext->SetTransform (D2D1::Matrix3x2F::Identity());
        }
    }

    hr = d2dContext->EndDraw();
    CHRA (hr);

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CharacterSet::CalculateUVCoordinates
//
//  Computes UV coordinates for each rain glyph within the 2048x2048 atlas
//  based on the 16-column grid layout with padding.
//
////////////////////////////////////////////////////////////////////////////////

void CharacterSet::CalculateUVCoordinates()
{
    // Layout: 2048x2048 texture atlas
    // Grid arrangement: 16 columns × 17 rows = 272 cells (272 used)
    // Each glyph cell: 128x120 pixels (with 8 pixel padding per cell)
    constexpr size_t ATLAS_SIZE    = 2048;
    constexpr size_t GRID_COLS     = 16;
    constexpr size_t GRID_ROWS     = 17;
    constexpr size_t CELL_WIDTH    = ATLAS_SIZE / GRID_COLS;   // 128 pixels
    constexpr size_t CELL_HEIGHT   = ATLAS_SIZE / GRID_ROWS;  // 120 pixels
    constexpr size_t GLYPH_PADDING = 8;  // Padding to prevent texture bleeding

    size_t rainGlyphCount = std::min (m_glyphs.size(), GRID_COLS * GRID_ROWS);



    for (size_t i = 0; i < rainGlyphCount; i++)
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





////////////////////////////////////////////////////////////////////////////////
//
//  CharacterSet::CalculateOverlayUVCoordinates
//
//  Computes UV coordinates for the DPI-aware overlay atlas.  Uses the overlay
//  cell dimensions (which differ from the rain atlas) to map each glyph's
//  content area within the overlay texture.
//
////////////////////////////////////////////////////////////////////////////////

void CharacterSet::CalculateOverlayUVCoordinates()
{
    size_t GRID_COLS  = 16;
    
    int    cellWidth  = m_overlayCellContentWidth  + 2 * m_overlayPadding;
    int    cellHeight = m_overlayCellContentHeight + 2 * m_overlayPadding;



    m_overlayUVs.resize (m_glyphs.size());

    for (size_t i = 0; i < m_glyphs.size(); i++)
    {
        int col = static_cast<int> (i % GRID_COLS);
        int row = static_cast<int> (i / GRID_COLS);

        float pixelMinX = static_cast<float> (col * cellWidth  + m_overlayPadding);
        float pixelMinY = static_cast<float> (row * cellHeight + m_overlayPadding);
        float pixelMaxX = static_cast<float> ((col + 1) * cellWidth  - m_overlayPadding);
        float pixelMaxY = static_cast<float> ((row + 1) * cellHeight - m_overlayPadding);

        m_overlayUVs[i].uvMin.x = pixelMinX / static_cast<float> (m_overlayAtlasWidth);
        m_overlayUVs[i].uvMin.y = pixelMinY / static_cast<float> (m_overlayAtlasHeight);
        m_overlayUVs[i].uvMax.x = pixelMaxX / static_cast<float> (m_overlayAtlasWidth);
        m_overlayUVs[i].uvMax.y = pixelMaxY / static_cast<float> (m_overlayAtlasHeight);
    }
}
