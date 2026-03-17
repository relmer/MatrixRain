#include "pch.h"

#include "RenderSystem.h"

#include "CharacterConstants.h"
#include "CharacterSet.h"
#include "ColorScheme.h"
#include "HelpHintOverlay.h"
#include "HotkeyOverlay.h"
#include "OverlayColor.h"

using Microsoft::WRL::ComPtr;





RenderSystem::~RenderSystem()
{
    Shutdown();
}





HRESULT RenderSystem::Initialize (HWND hwnd, UINT width, UINT height)
{
    HRESULT        hr       = S_OK;
    D3D11_VIEWPORT viewport = {};


    // Store dimensions for viewport and render target sizing
    m_hwnd         = hwnd;
    m_renderWidth  = width;
    m_renderHeight = height;
    
    hr = CreateDevice();
    CHR (hr);

    hr = CreateSwapChain (hwnd, width, height);
    CHR (hr);

    hr = CreateRenderTargetView();
    CHR (hr);

    hr = CompileCharacterShaders();
    CHR (hr);

    hr = CreateDummyVertexBuffer();
    CHR (hr);

    hr = CreateInstanceBuffer();
    CHR (hr);

    hr = CreateConstantBuffer();
    CHR (hr);

    hr = CreateBloomConstantBuffer();
    CHR (hr);

    hr = CreateBlendState();
    CHR (hr);

    hr = CreateSamplerState();
    CHR (hr);

    // Compute DPI scale BEFORE creating D2D resources so font sizes are correct
    UpdateDpiScale();

    hr = CreateDirect2DResources();
    CHR (hr);

    hr = CreateBloomResources (width, height);
    CHR (hr);

    // Set viewport
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width    = static_cast<FLOAT> (width);
    viewport.Height   = static_cast<FLOAT> (height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    m_context->RSSetViewports (1, &viewport);

Error:
    return hr;
}





HRESULT RenderSystem::CreateDevice()
{
    HRESULT           hr                = S_OK;
    UINT              createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT; // Required for Direct2D interop
    D3D_FEATURE_LEVEL featureLevels[]   = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL featureLevel;


    
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    hr = D3D11CreateDevice (nullptr,                   // Default adapter
                            D3D_DRIVER_TYPE_HARDWARE,  // Hardware acceleration
                            nullptr,                   // No software rasterizer
                            createDeviceFlags,
                            featureLevels,
                            _countof (featureLevels),
                            D3D11_SDK_VERSION,
                            &m_device,
                            &featureLevel,
                            &m_context);

#ifdef _DEBUG
    // D3D11_CREATE_DEVICE_DEBUG requires the Graphics Tools optional feature.
    // Install via: Settings > System > Optional Features > Graphics Tools
    CBRLA (hr != DXGI_ERROR_SDK_COMPONENT_MISSING, L"D3D debug layer unavailable — install Graphics Tools optional feature");
#endif

    CHRA (hr);


Error:
    return hr;
}





HRESULT RenderSystem::CreateSwapChain (HWND hwnd, UINT width, UINT height)
{
    HRESULT               hr          = S_OK;
    ComPtr<IDXGIDevice>   dxgiDevice;
    ComPtr<IDXGIAdapter>  dxgiAdapter;
    ComPtr<IDXGIFactory>  dxgiFactory;
    DXGI_SWAP_CHAIN_DESC  swapChainDesc = {};



    // Get DXGI factory from device
    hr = m_device.As (&dxgiDevice);
    CHRA (hr);

    hr = dxgiDevice->GetAdapter (&dxgiAdapter);
    CHRA (hr);

    hr = dxgiAdapter->GetParent (__uuidof (IDXGIFactory), &dxgiFactory);
    CHRA (hr);

    // Create swap chain description
    swapChainDesc.BufferCount                        = 2;
    swapChainDesc.BufferDesc.Width                   = width;
    swapChainDesc.BufferDesc.Height                  = height;
    swapChainDesc.BufferDesc.Format                  = DXGI_FORMAT_B8G8R8A8_UNORM; // Match D2D format requirement
    swapChainDesc.BufferDesc.RefreshRate.Numerator   = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow                       = hwnd;
    swapChainDesc.SampleDesc.Count                   = 1;
    swapChainDesc.SampleDesc.Quality                 = 0;
    swapChainDesc.Windowed                           = TRUE;
    swapChainDesc.SwapEffect                         = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    hr = dxgiFactory->CreateSwapChain (m_device.Get(), &swapChainDesc, &m_swapChain);
    CHRA (hr);

    // Disable DXGI's built-in Alt+Enter exclusive fullscreen toggle
    // We handle fullscreen transitions manually for borderless windowed mode
    hr = dxgiFactory->MakeWindowAssociation (hwnd, DXGI_MWA_NO_ALT_ENTER);
    CHRA (hr);

Error:
    return hr;
}





HRESULT RenderSystem::CreateRenderTargetView()
{
    HRESULT                 hr         = S_OK;
    ComPtr<ID3D11Texture2D> backBuffer;



    hr = m_swapChain->GetBuffer (0, __uuidof (ID3D11Texture2D), &backBuffer);
    CHRA (hr);

    hr = m_device->CreateRenderTargetView (backBuffer.Get(), nullptr, &m_renderTargetView);
    CHRA (hr);

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
// Shader source code
//
////////////////////////////////////////////////////////////////////////////////

static const char* s_kszVertexShaderSource = R"(
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
            uint instanceID : SV_InstanceID;
        };

        struct PSInput
        {
            float4 position : SV_POSITION;
            float2 uv : TEXCOORD;
            float4 color : COLOR;
            float brightness : BRIGHTNESS;
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
            
            return output;
        }
    )";

static const char* s_kszPixelShaderSource = R"(
        Texture2D atlasTexture : register(t0);
        SamplerState samplerState : register(s0);

        struct PSInput
        {
            float4 position : SV_POSITION;
            float2 uv : TEXCOORD;
            float4 color : COLOR;
            float brightness : BRIGHTNESS;
        };

        float4 main(PSInput input) : SV_TARGET
        {
            // Sample texture atlas
            float4 texColor = atlasTexture.Sample(samplerState, input.uv);
            
            // Apply color and brightness
            float4 finalColor = input.color * texColor * input.brightness;
            
            // Add glow effect (brighten the color slightly)
            finalColor.rgb += finalColor.rgb * 0.3 * input.brightness;
            
            return finalColor;
        }
    )";

static const char * s_kszOverlayPixelShaderSource = R"(
        Texture2D    atlasTexture : register(t0);
        SamplerState samplerState : register(s0);

        struct PSInput
        {
            float4 position   : SV_POSITION;
            float2 uv         : TEXCOORD;
            float4 color      : COLOR;
            float  brightness : BRIGHTNESS;
        };

        float4 main(PSInput input) : SV_TARGET
        {
            // Atlas is rendered by D2D with premultiplied alpha.
            // Tint RGB by instance color and brightness, preserving
            // the premultiplied relationship so the blend state
            // (ONE / INV_SRC_ALPHA) composites correctly.
            float4 texColor  = atlasTexture.Sample(samplerState, input.uv);
            float3 tintedRGB = texColor.rgb * input.color.rgb * input.brightness;
            float  tintedA   = texColor.a   * input.brightness;

            return float4(tintedRGB, tintedA);
        }
    )";


static const D3D11_INPUT_ELEMENT_DESC s_krgInputLayout[] = {
    { "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT,    1, 0,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
    { "TEXCOORD",   0, DXGI_FORMAT_R32G32_FLOAT,       1, 12, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
    { "TEXCOORD",   1, DXGI_FORMAT_R32G32_FLOAT,       1, 20, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
    { "COLOR",      0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 28, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
    { "BRIGHTNESS", 0, DXGI_FORMAT_R32_FLOAT,          1, 44, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
    { "SCALEX",     0, DXGI_FORMAT_R32_FLOAT,          1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
    { "SCALEY",     0, DXGI_FORMAT_R32_FLOAT,          1, 52, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
};




////////////////////////////////////////////////////////////////////////////////
//
//  Shader compilation table structure
//
////////////////////////////////////////////////////////////////////////////////

struct RenderSystem::ShaderCompileEntry
{
    const char *   pszSource;
    const char *   pszName;
    const char *   pszEntryPoint;
    const char *   pszTarget;
    LPCWSTR        pszErrorMsg;
    ID3DBlob   **  ppBlob;
};




////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::CompileShadersFromTable
//
////////////////////////////////////////////////////////////////////////////////

HRESULT RenderSystem::CompileShadersFromTable (const ShaderCompileEntry* pTable, size_t cEntries)
{
    HRESULT          hr        = S_OK;
    ComPtr<ID3DBlob> errorBlob;


    for (size_t i = 0; i < cEntries; ++i)
    {
        const ShaderCompileEntry& entry = pTable[i];

        errorBlob.Reset();

        hr = D3DCompile (entry.pszSource,
                            strlen (entry.pszSource),
                            entry.pszName,
                            nullptr,
                            nullptr,
                            entry.pszEntryPoint,
                            entry.pszTarget,
                            D3DCOMPILE_ENABLE_STRICTNESS,
                            0,
                            entry.ppBlob,
                            &errorBlob);
        if (FAILED (hr) && errorBlob)
        {
            OutputDebugStringA ((char*)errorBlob->GetBufferPointer());
        }
        CHRLA (hr, entry.pszErrorMsg);
    }

Error:
    return hr;
}




HRESULT RenderSystem::CompileCharacterShaders()
{
    HRESULT             hr            = S_OK;
    ComPtr<ID3DBlob>    vsBlob;
    ComPtr<ID3DBlob>    psBlob;
    ComPtr<ID3DBlob>    overlayPsBlob;
    ShaderCompileEntry  shaderTable[] = {
        { s_kszVertexShaderSource,        "VS",        "main", "vs_5_0",  L"D3DCompile failed for vertex shader",         vsBlob.GetAddressOf()        },
        { s_kszPixelShaderSource,         "PS",        "main", "ps_5_0",  L"D3DCompile failed for pixel shader",          psBlob.GetAddressOf()        },
        { s_kszOverlayPixelShaderSource,  "OverlayPS", "main", "ps_5_0",  L"D3DCompile failed for overlay pixel shader",  overlayPsBlob.GetAddressOf() }
    };
    


    // Compile shaders from table
    hr = CompileShadersFromTable (shaderTable, _countof (shaderTable));
    CHR (hr);        // Create vertex shader

    hr = m_device->CreateVertexShader (vsBlob->GetBufferPointer(),
                                        vsBlob->GetBufferSize(),
                                        nullptr,
                                        &m_vertexShader);
    CHRA (hr);

    // Create pixel shader
    hr = m_device->CreatePixelShader (psBlob->GetBufferPointer(),
                                        psBlob->GetBufferSize(),
                                        nullptr,
                                        &m_pixelShader);
    CHRA (hr);

    // Create overlay pixel shader (no additive glow)
    hr = m_device->CreatePixelShader (overlayPsBlob->GetBufferPointer(),
                                        overlayPsBlob->GetBufferSize(),
                                        nullptr,
                                        &m_overlayPixelShader);
    CHRA (hr);

    // Create input layout using vertex shader blob
    hr = m_device->CreateInputLayout (s_krgInputLayout,
                                        _countof (s_krgInputLayout),
                                        vsBlob->GetBufferPointer(),
                                        vsBlob->GetBufferSize(),
                                        &m_inputLayout);
    CHRA (hr);


Error:
    return hr;
}





HRESULT RenderSystem::CreateDummyVertexBuffer()
{
    HRESULT                hr       = S_OK;
    D3D11_BUFFER_DESC      vbDesc   = {};
    D3D11_SUBRESOURCE_DATA vbData   = {};
    float                  dummyData[6] = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };  // 6 dummy vertices (4-byte aligned)



    // Create a dummy vertex buffer with 6 floats (4 bytes each, 24 bytes total)
    // The shader generates vertices procedurally, but D3D11 requires a valid buffer
    vbDesc.ByteWidth = sizeof (dummyData);
    vbDesc.Usage     = D3D11_USAGE_IMMUTABLE;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    vbData.pSysMem = dummyData;

    hr = m_device->CreateBuffer (&vbDesc, &vbData, &m_dummyVertexBuffer);
    CHRA (hr);

Error:
    return hr;
}





HRESULT RenderSystem::CreateInstanceBuffer()
{
    HRESULT           hr         = S_OK;
    D3D11_BUFFER_DESC bufferDesc = {};



    bufferDesc.ByteWidth      = sizeof (CharacterInstanceData) * m_instanceBufferCapacity;
    bufferDesc.Usage          = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = m_device->CreateBuffer (&bufferDesc, nullptr, &m_instanceBuffer);
    CHRA (hr);

Error:
    return hr;
}





HRESULT RenderSystem::CreateConstantBuffer()
{
    HRESULT           hr         = S_OK;
    D3D11_BUFFER_DESC bufferDesc = {};


    
    bufferDesc.ByteWidth      = sizeof (ConstantBufferData);
    bufferDesc.Usage          = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = m_device->CreateBuffer (&bufferDesc, nullptr, &m_constantBuffer);
    CHRA (hr);

Error:
    return hr;
}





HRESULT RenderSystem::CreateBloomConstantBuffer()
{
    HRESULT           hr         = S_OK;
    D3D11_BUFFER_DESC bufferDesc = {};


    
    bufferDesc.ByteWidth      = 16;  // sizeof(float) * 4 for alignment
    bufferDesc.Usage          = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = m_device->CreateBuffer (&bufferDesc, nullptr, &m_bloomConstantBuffer);
    CHRA (hr);

Error:
    return hr;
}





HRESULT RenderSystem::CreateBlendState()
{
    HRESULT          hr        = S_OK;
    D3D11_BLEND_DESC blendDesc = {};

    

    blendDesc.AlphaToCoverageEnable                  = FALSE;
    blendDesc.IndependentBlendEnable                 = FALSE;
    blendDesc.RenderTarget[0].BlendEnable            = TRUE;
    blendDesc.RenderTarget[0].SrcBlend               = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend              = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp                = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha          = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha         = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha           = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask  = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = m_device->CreateBlendState (&blendDesc, &m_blendState);
    CHRA (hr);

    // Premultiplied alpha blend state for overlay text
    // (D2D renders with premultiplied alpha, so SrcBlend = ONE)
    blendDesc.RenderTarget[0].SrcBlend  = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;

    hr = m_device->CreateBlendState (&blendDesc, &m_premultipliedBlendState);
    CHRA (hr);

Error:
    return hr;
}





HRESULT RenderSystem::CreateSamplerState()
{
    HRESULT            hr           = S_OK;
    D3D11_SAMPLER_DESC samplerDesc = {};



    samplerDesc.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD         = 0;
    samplerDesc.MaxLOD         = D3D11_FLOAT32_MAX;

    hr = m_device->CreateSamplerState (&samplerDesc, &m_samplerState);
    CHRA (hr);

    // Point sampler for overlay text (1:1 texel-to-pixel, no interpolation)
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;

    hr = m_device->CreateSamplerState (&samplerDesc, &m_pointSamplerState);
    CHRA (hr);

Error:
    return hr;
}





HRESULT RenderSystem::CreateDirect2DResources()
{
    HRESULT              hr         = S_OK;
    D2D1_FACTORY_OPTIONS options    = {};
    ComPtr<IDXGIDevice>  dxgiDevice;



#ifdef _DEBUG
    options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

    // Create D2D factory
    hr = D2D1CreateFactory (D2D1_FACTORY_TYPE_SINGLE_THREADED, 
                            __uuidof (ID2D1Factory1), 
                            &options,
                            reinterpret_cast<void**> (m_d2dFactory.GetAddressOf()));
    CHRA (hr);

    // Get DXGI device
    hr = m_device.As (&dxgiDevice);
    CHRA (hr);

    // Create D2D device
    hr = m_d2dFactory->CreateDevice (dxgiDevice.Get(), &m_d2dDevice);
    CHRA (hr);

    // Create D2D device context
    hr = m_d2dDevice->CreateDeviceContext (D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_d2dContext);
    CHRA (hr);

    // Create D2D bitmap from swap chain back buffer
    hr = RecreateDirect2DBitmap();
    CHRA (hr);

    // Create DirectWrite factory
    hr = DWriteCreateFactory (DWRITE_FACTORY_TYPE_SHARED, 
                              __uuidof (IDWriteFactory),
                              reinterpret_cast<IUnknown**> (m_dwriteFactory.GetAddressOf()));
    CHRA (hr);

    // Create text format for FPS display (small, top-right aligned)
    hr = CreateFpsTextFormat();
    CHR (hr);

    // Create white brush for FPS text
    hr = m_d2dContext->CreateSolidColorBrush (D2D1::ColorF (D2D1::ColorF::White), &m_fpsBrush);
    CHRA (hr);

    // Create black brush for FPS text glow
    hr = m_d2dContext->CreateSolidColorBrush (D2D1::ColorF (D2D1::ColorF::Black, 0.0f), &m_fpsGlowBrush);
    CHRA (hr);

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::CreateFpsTextFormat
//
//  Creates (or recreates) the DirectWrite text format used by the FPS counter
//  and overlay text measurement.  The font size is scaled by m_dpiScale so
//  the text maintains a consistent logical size across DPI settings.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT RenderSystem::CreateFpsTextFormat()
{
    HRESULT hr       = S_OK;
    float   fontSize = 12.0f * m_dpiScale;


    CBRAEx (m_dwriteFactory.Get() != nullptr, E_UNEXPECTED);

    m_fpsTextFormat.Reset();

    hr = m_dwriteFactory->CreateTextFormat (L"Consolas",
                                            nullptr,
                                            DWRITE_FONT_WEIGHT_NORMAL,
                                            DWRITE_FONT_STYLE_NORMAL,
                                            DWRITE_FONT_STRETCH_NORMAL,
                                            fontSize,
                                            L"en-us",
                                            &m_fpsTextFormat);
    CHRA (hr);

    m_fpsTextFormat->SetTextAlignment (DWRITE_TEXT_ALIGNMENT_TRAILING);
    m_fpsTextFormat->SetParagraphAlignment (DWRITE_PARAGRAPH_ALIGNMENT_FAR);

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::UpdateDpiScale
//
//  Queries the window's current DPI and updates m_dpiScale accordingly.
//
////////////////////////////////////////////////////////////////////////////////

void RenderSystem::UpdateDpiScale()
{
    if (m_hwnd)
    {
        UINT dpi   = GetDpiForWindow (m_hwnd);
        m_dpiScale = static_cast<float> (dpi) / 96.0f;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::OnDpiChanged
//
//  Called when the system notifies us of a DPI change (WM_DPICHANGED).
//  Updates the DPI scale factor and recreates DPI-sensitive resources.
//
////////////////////////////////////////////////////////////////////////////////

void RenderSystem::OnDpiChanged (UINT dpi)
{
    m_dpiScale = static_cast<float> (dpi) / 96.0f;

    // Recreate text formats at the new DPI scale
    if (m_dwriteFactory)
    {
        CreateFpsTextFormat();
    }

    // Recreate overlay atlas at the new DPI scale for 1:1 texel-to-pixel mapping
    if (m_device)
    {
        CharacterSet::GetInstance().RecreateOverlayAtlas (m_device.Get(), m_dpiScale);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
// Bloom shader source code
//
////////////////////////////////////////////////////////////////////////////////

static const char * s_kszQuadVertexShaderSource = R"(
        struct VSInput
        {
            float3 position : POSITION;
            float2 uv : TEXCOORD;
        };

        struct PSInput
        {
            float4 position : SV_POSITION;
            float2 uv : TEXCOORD;
        };

        PSInput main(VSInput input)
        {
            PSInput output;
            output.position = float4(input.position, 1.0);
            output.uv = input.uv;
            return output;
        }
    )";

static const char * s_kszBlurHorizontalShaderSource = R"(
        cbuffer BloomConstants : register(b0)
        {
            float bloomIntensity;
            float glowSize;
            float2 padding;
        };

        Texture2D inputTexture : register(t0);
        SamplerState samplerState : register(s0);

        struct PSInput
        {
            float4 position : SV_POSITION;
            float2 uv : TEXCOORD;
        };

        float4 main(PSInput input) : SV_TARGET
        {
            uint width, height;
            inputTexture.GetDimensions(width, height);
            float texelX = glowSize / width;
            
            float4 color = float4(0, 0, 0, 0);
            
            // 13-tap Gaussian blur (horizontal), spread scaled by glowSize
            float weights[13] = { 0.02, 0.04, 0.06, 0.08, 0.10, 0.12, 0.16, 0.12, 0.10, 0.08, 0.06, 0.04, 0.02 };
            for (int i = -6; i <= 6; i++)
            {
                float2 offset = float2(i * texelX, 0);
                color += inputTexture.Sample(samplerState, input.uv + offset) * weights[i + 6];
            }
            
            return color;
        }
    )";

static const char * s_kszBlurVerticalShaderSource = R"(
        cbuffer BloomConstants : register(b0)
        {
            float bloomIntensity;
            float glowSize;
            float2 padding;
        };

        Texture2D inputTexture : register(t0);
        SamplerState samplerState : register(s0);

        struct PSInput
        {
            float4 position : SV_POSITION;
            float2 uv : TEXCOORD;
        };

        float4 main(PSInput input) : SV_TARGET
        {
            uint width, height;
            inputTexture.GetDimensions(width, height);
            float texelY = glowSize / height;
            
            float4 color = float4(0, 0, 0, 0);
            
            // 13-tap Gaussian blur (vertical), spread scaled by glowSize
            float weights[13] = { 0.02, 0.04, 0.06, 0.08, 0.10, 0.12, 0.16, 0.12, 0.10, 0.08, 0.06, 0.04, 0.02 };
            for (int i = -6; i <= 6; i++)
            {
                float2 offset = float2(0, i * texelY);
                color += inputTexture.Sample(samplerState, input.uv + offset) * weights[i + 6];
            }
            
            return color;
        }
    )";

static const char * s_kszBloomExtractShaderSource = R"(
        Texture2D inputTexture : register(t0);
        SamplerState samplerState : register(s0);

        struct PSInput
        {
            float4 position : SV_POSITION;
            float2 uv : TEXCOORD;
        };

        float4 main(PSInput input) : SV_TARGET
        {
            float4 color = inputTexture.Sample(samplerState, input.uv);
            
            // Extract only bright pixels (consider luminance and max channel)
            float luminance = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));

            // Also consider the max color channel so saturated blues/reds can trigger bloom
            float maxComp = max(max(color.r, color.g), color.b);

            // Use the higher of luminance or max component as brightness metric
            float brightness = max(luminance, maxComp);

            // Low threshold so even dim characters get a subtle glow.
            // The wide smoothstep range (0.1 → 0.6) ensures bright streak
            // heads bloom strongly while dim tail characters still contribute
            // a soft halo rather than appearing flat.
            float threshold = 0.1;

            // Smooth ramp: dim chars get subtle bloom, bright chars get full
            float bloomAmount = smoothstep(threshold, threshold + 0.5, brightness);

            return float4(color.rgb * bloomAmount, 1.0);
        }
    )";

static const char * s_kszBloomCompositeShaderSource = R"(
        cbuffer BloomConstants : register(b0)
        {
            float bloomIntensity;
            float glowSize;
            float2 padding;
        };

        Texture2D sceneTexture : register(t0);
        Texture2D bloomTexture : register(t1);
        SamplerState samplerState : register(s0);

        struct PSInput
        {
            float4 position : SV_POSITION;
            float2 uv : TEXCOORD;
        };

        float4 main(PSInput input) : SV_TARGET
        {
            float4 scene = sceneTexture.Sample(samplerState, input.uv);
            float4 bloom = bloomTexture.Sample(samplerState, input.uv);
            
            // Exponential soft-saturation: low bloom values pass through
            // nearly linearly (good glow on isolated streaks) while high
            // bloom values from dense overlapping areas hit a ceiling.
            // This lets the user crank up bloomIntensity without dense
            // regions becoming a solid wall of glow.
            float3 bloomContrib = bloom.rgb * bloomIntensity;
            float3 softBloom   = 1.0 - exp(-bloomContrib);

            return float4(scene.rgb + softBloom * (1.0 - scene.rgb), 1.0);
        }
    )";

static const D3D11_INPUT_ELEMENT_DESC s_krgQuadInputLayout[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::CompileBloomShaders
//
////////////////////////////////////////////////////////////////////////////////

HRESULT RenderSystem::CompileBloomShaders()
{
    struct QuadVertex
    {
        float pos[3];
        float uv[2];
    };

    HRESULT                hr                 = S_OK;
    ComPtr<ID3DBlob>       quadVSBlob;
    ComPtr<ID3DBlob>       extractPSBlob;
    ComPtr<ID3DBlob>       blurHPSBlob;
    ComPtr<ID3DBlob>       blurVPSBlob;
    ComPtr<ID3DBlob>       compositePSBlob;
    QuadVertex             quadVertices[]     = {
        { {-1, -1, 0}, {0, 1} },
        { {-1,  1, 0}, {0, 0} },
        { { 1,  1, 0}, {1, 0} },
        { {-1, -1, 0}, {0, 1} },
        { { 1,  1, 0}, {1, 0} },
        { { 1, -1, 0}, {1, 1} }
    };
    D3D11_BUFFER_DESC      vbDesc             = { };
    D3D11_SUBRESOURCE_DATA vbData             = { };
    ShaderCompileEntry     bloomShaderTable[] = {
        { s_kszQuadVertexShaderSource,         "QuadVS",    "main", "vs_5_0", L"D3DCompile failed for quad vertex shader",     quadVSBlob.GetAddressOf() },
        { s_kszBloomExtractShaderSource,       "Extract",   "main", "ps_5_0", L"D3DCompile failed for bloom extract shader",   extractPSBlob.GetAddressOf() },
        { s_kszBlurHorizontalShaderSource,     "BlurH",     "main", "ps_5_0", L"D3DCompile failed for horizontal blur shader", blurHPSBlob.GetAddressOf() },
        { s_kszBlurVerticalShaderSource,       "BlurV",     "main", "ps_5_0", L"D3DCompile failed for vertical blur shader",   blurVPSBlob.GetAddressOf() },
        { s_kszBloomCompositeShaderSource,     "Composite", "main", "ps_5_0", L"D3DCompile failed for composite shader",       compositePSBlob.GetAddressOf() }
    };


    // Compile shaders from table
    hr = CompileShadersFromTable (bloomShaderTable, _countof (bloomShaderTable));
    CHR (hr);
    
    hr = m_device->CreateVertexShader (quadVSBlob->GetBufferPointer(), quadVSBlob->GetBufferSize(), nullptr, &m_fullscreenQuadVS);
    CHRA (hr);
    
    hr = m_device->CreatePixelShader (extractPSBlob->GetBufferPointer(), extractPSBlob->GetBufferSize(), nullptr, &m_bloomExtractPS);
    CHRA (hr);
    
    hr = m_device->CreatePixelShader (blurHPSBlob->GetBufferPointer(), blurHPSBlob->GetBufferSize(), nullptr, &m_blurHorizontalPS);
    CHRA (hr);
    
    hr = m_device->CreatePixelShader (blurVPSBlob->GetBufferPointer(), blurVPSBlob->GetBufferSize(), nullptr, &m_blurVerticalPS);
    CHRA (hr);
    
    hr = m_device->CreatePixelShader (compositePSBlob->GetBufferPointer(), compositePSBlob->GetBufferSize(), nullptr, &m_compositePS);
    CHRA (hr);
    
    hr = m_device->CreateInputLayout (s_krgQuadInputLayout, _countof (s_krgQuadInputLayout), 
                                        quadVSBlob->GetBufferPointer(), quadVSBlob->GetBufferSize(), &m_fullscreenQuadInputLayout);
    CHRA (hr);

    // Create fullscreen quad vertex buffer
    vbDesc.ByteWidth = sizeof (quadVertices);
    vbDesc.Usage     = D3D11_USAGE_IMMUTABLE;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    vbData.pSysMem = quadVertices;

    hr = m_device->CreateBuffer (&vbDesc, &vbData, &m_fullscreenQuadVB);
    CHRA (hr);

Error:
    return hr;
}





HRESULT RenderSystem::CreateBloomResources (UINT width, UINT height)
{
    HRESULT              hr           = S_OK;
    UINT                 bloomWidth;
    UINT                 bloomHeight;
    D3D11_TEXTURE2D_DESC sceneTexDesc = { };
    D3D11_TEXTURE2D_DESC texDesc      = { };



    // Safety check - don't create bloom resources with invalid dimensions
    BAIL_OUT_IF (width == 0 || height == 0, S_OK);  // Return success without creating resources

    // Create scene render target (full resolution)
    sceneTexDesc.Width             = width;
    sceneTexDesc.Height            = height;
    sceneTexDesc.MipLevels         = 1;
    sceneTexDesc.ArraySize         = 1;
    sceneTexDesc.Format            = DXGI_FORMAT_R8G8B8A8_UNORM;
    sceneTexDesc.SampleDesc.Count  = 1;
    sceneTexDesc.Usage             = D3D11_USAGE_DEFAULT;
    sceneTexDesc.BindFlags         = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    hr = m_device->CreateTexture2D (&sceneTexDesc, nullptr, &m_sceneTexture);
    CHRA (hr);

    hr = m_device->CreateRenderTargetView (m_sceneTexture.Get(), nullptr, &m_sceneRTV);
    CHRA (hr);

    hr = m_device->CreateShaderResourceView (m_sceneTexture.Get(), nullptr, &m_sceneSRV);
    CHRA (hr);

    // Create bloom extraction texture (half resolution for performance)
    bloomWidth  = width / 2;
    bloomHeight = height / 2;

    texDesc.Width            = bloomWidth;
    texDesc.Height           = bloomHeight;
    texDesc.MipLevels        = 1;
    texDesc.ArraySize        = 1;
    texDesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage            = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags        = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    hr = m_device->CreateTexture2D (&texDesc, nullptr, &m_bloomTexture);
    CHRA (hr);

    hr = m_device->CreateRenderTargetView (m_bloomTexture.Get(), nullptr, &m_bloomRTV);
    CHRA (hr);

    hr = m_device->CreateShaderResourceView (m_bloomTexture.Get(), nullptr, &m_bloomSRV);
    CHRA (hr);

    // Create temporary blur texture (same size as bloom)
    hr = m_device->CreateTexture2D (&texDesc, nullptr, &m_blurTempTexture);
    CHRA (hr);

    hr = m_device->CreateRenderTargetView (m_blurTempTexture.Get(), nullptr, &m_blurTempRTV);
    CHRA (hr);

    hr = m_device->CreateShaderResourceView (m_blurTempTexture.Get(), nullptr, &m_blurTempSRV);
    CHRA (hr);

    // Compile bloom shaders and create fullscreen quad resources (only on first call)
    if (!m_bloomExtractPS)
    {
        hr = CompileBloomShaders();
        CHR (hr);
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::SetRenderPipelineState
//
//  Generic helper to set up rendering pipeline state.
//
////////////////////////////////////////////////////////////////////////////////

void RenderSystem::SetRenderPipelineState(ID3D11InputLayout* pInputLayout,
                                          D3D11_PRIMITIVE_TOPOLOGY topology,
                                          ID3D11Buffer* pVertexBuffer,
                                          UINT stride,
                                          ID3D11VertexShader* pVertexShader,
                                          ID3D11Buffer* pConstantBuffer,
                                          ID3D11PixelShader* pPixelShader)
{
    UINT offset = 0;



    m_context->IASetInputLayout       (pInputLayout);
    m_context->IASetPrimitiveTopology (topology);
    m_context->IASetVertexBuffers     (0, 1, &pVertexBuffer, &stride, &offset);
    m_context->VSSetShader            (pVertexShader, nullptr, 0);
    
    if (pConstantBuffer)
    {
        m_context->VSSetConstantBuffers (0, 1, &pConstantBuffer);
    }
    else
    {
        m_context->VSSetConstantBuffers (0, 0, nullptr);
    }
    
    if (pPixelShader)
    {
        m_context->PSSetShader (pPixelShader, nullptr, 0);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::RenderFullscreenPass
//
//  Renders a fullscreen quad pass with the specified render target, pixel shader,
//  and shader resources. Automatically unbinds shader resources after drawing.
//
////////////////////////////////////////////////////////////////////////////////

void RenderSystem::RenderFullscreenPass(ID3D11RenderTargetView* pRenderTarget,
                                        ID3D11PixelShader* pPixelShader,
                                        ID3D11ShaderResourceView* const* ppShaderResources,
                                        UINT numResources)
{
    ID3D11ShaderResourceView * nullSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {};



    m_context->OMSetRenderTargets   (1, &pRenderTarget, nullptr);
    m_context->PSSetShader          (pPixelShader, nullptr, 0);
    m_context->PSSetShaderResources (0, numResources, ppShaderResources);
    m_context->Draw (6, 0);
    
    // Unbind shader resources to avoid hazards
    m_context->PSSetShaderResources (0, numResources, nullSRVs);
}





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::SetViewport
//
//  Sets the viewport dimensions.
//
////////////////////////////////////////////////////////////////////////////////

void RenderSystem::SetViewport(UINT width, UINT height)
{
    D3D11_VIEWPORT viewport = {};



    viewport.Width    = static_cast<float> (width);
    viewport.Height   = static_cast<float> (height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    
    m_context->RSSetViewports (1, &viewport);
}





HRESULT RenderSystem::ApplyBloom()
{
    HRESULT                    hr            = S_OK;
    ID3D11ShaderResourceView * srvs[2];
    ID3D11Buffer             * nullCB        = nullptr;
    D3D11_MAPPED_SUBRESOURCE   mappedBloomCB;



    // Safety check - if render target or bloom resources are being recreated during resize, skip
    CBREx (m_renderTargetView && m_sceneTexture && m_bloomTexture && m_bloomExtractPS && m_compositePS, E_UNEXPECTED);
    
    // Scene texture already has the rendered characters - no need to copy from backbuffer
    
    // Set viewport for half-resolution processing
    SetViewport (m_renderWidth / 2, m_renderHeight / 2);
    
    // EXTRACTION PASS: Extract only bright pixels from scene to bloom texture
    SetRenderPipelineState (m_fullscreenQuadInputLayout.Get(),
                            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
                            m_fullscreenQuadVB.Get(),
                            sizeof (float) * 5,
                            m_fullscreenQuadVS.Get(),
                            nullptr,
                            nullptr);
    m_context->PSSetSamplers (0, 1, m_samplerState.GetAddressOf());
    
    RenderFullscreenPass (m_bloomRTV.Get(), m_bloomExtractPS.Get(), m_sceneSRV.GetAddressOf(), 1);
    
    // Update bloom constant buffer (shared by blur and composite shaders)
    hr = m_context->Map (m_bloomConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBloomCB);
    if (SUCCEEDED (hr))
    {
        float * bloomData = static_cast<float*> (mappedBloomCB.pData);

        bloomData[0] = m_glowIntensity;  // Bloom intensity
        bloomData[1] = m_glowSize;       // Blur radius multiplier
        bloomData[2] = 0.0f;             // Padding
        bloomData[3] = 0.0f;             // Padding

        m_context->Unmap (m_bloomConstantBuffer.Get(), 0);
    }

    // Bind constant buffer for blur shaders (glowSize controls spread)
    m_context->PSSetConstantBuffers (0, 1, m_bloomConstantBuffer.GetAddressOf());

    // Multiple blur passes: each H+V pass blurs the previous result,
    // creating an exponentially wider and softer glow.  Three passes with
    // a 13-tap kernel produce a very wide, cinematic bloom falloff.
    for (int pass = 0; pass < 3; ++pass)
    {
        // Horizontal blur pass (bloom → temp)
        RenderFullscreenPass (m_blurTempRTV.Get(), m_blurHorizontalPS.Get(), m_bloomSRV.GetAddressOf(), 1);

        // Vertical blur pass (temp → bloom)
        RenderFullscreenPass (m_bloomRTV.Get(), m_blurVerticalPS.Get(), m_blurTempSRV.GetAddressOf(), 1);
    }
    
    // Restore full viewport
    SetViewport (m_renderWidth, m_renderHeight);
    
    // Composite back to backbuffer
    m_context->OMSetRenderTargets (1, m_renderTargetView.GetAddressOf(), nullptr);
    
    // Disable blending for composite (we want to replace, not blend)
    m_context->OMSetBlendState (nullptr, nullptr, 0xffffffff);
    
    // Bind bloom constant buffer to pixel shader
    m_context->PSSetConstantBuffers (0, 1, m_bloomConstantBuffer.GetAddressOf());
    
    srvs[0] = m_sceneSRV.Get();
    srvs[1] = m_bloomSRV.Get();
    RenderFullscreenPass (m_renderTargetView.Get(), m_compositePS.Get(), srvs, 2);
    
    // Unbind constant buffer from pixel shader
    m_context->PSSetConstantBuffers (0, 1, &nullCB);
    
    // Restore render state
    SetRenderPipelineState (m_inputLayout.Get(),
                            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
                            m_instanceBuffer.Get(),
                            sizeof (CharacterInstanceData),
                            m_vertexShader.Get(),
                            m_constantBuffer.Get(),
                            m_pixelShader.Get());

Error:
    return hr;
}





void RenderSystem::ClearRenderTarget()
{
    // Clear to black
    constexpr float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };



    if (!m_renderTargetView)
    {
        return;  // Skip if render target is being recreated during resize
    }
    
    m_context->ClearRenderTargetView (m_renderTargetView.Get(), clearColor);
}





void RenderSystem::SortStreaksByDepth (std::vector<const CharacterStreak*>& streaks)
{
    // Sort back-to-front (far to near) for proper alpha blending
    std::stable_sort (streaks.begin(), streaks.end(),
        [](const CharacterStreak * a, const CharacterStreak * b)
        {
            return a->GetPosition().z > b->GetPosition().z;
        });
}





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::BuildCharacterInstanceData
//
//  Builds instance data for a single character.
//
////////////////////////////////////////////////////////////////////////////////

void RenderSystem::BuildCharacterInstanceData (const CharacterInstance & character, const Vector3 & streakPos, const Color4 & schemeColor, RenderSystem::CharacterInstanceData & data)
{
    CharacterSet & charSet = CharacterSet::GetInstance();



    // Position - character stores its absolute Y, streak provides X and Z
    data.position[0] = streakPos.x + character.positionOffset.x;
    data.position[1] = character.positionOffset.y; // Absolute Y position
    data.position[2] = streakPos.z;

    // Get UV coordinates from glyph
    const GlyphInfo & glyph = charSet.GetGlyph (character.glyphIndex);
    data.uvMin[0] = glyph.uvMin.x;
    data.uvMin[1] = glyph.uvMin.y;
    data.uvMax[0] = glyph.uvMax.x;
    data.uvMax[1] = glyph.uvMax.y;

    // Color and brightness - apply color scheme to trailing characters
    // White characters (lead) should stay white regardless of color scheme
    bool isWhite = (character.color.r > 0.9f && character.color.g > 0.9f && character.color.b > 0.9f);
    
    if (isWhite)
    {
        // Keep white characters white (lead character)
        data.color[0] = character.color.r;
        data.color[1] = character.color.g;
        data.color[2] = character.color.b;
    }
    else
    {
        // Replace trail color with current color scheme
        data.color[0] = schemeColor.r;
        data.color[1] = schemeColor.g;
        data.color[2] = schemeColor.b;
    }
    
    data.color[3]   = character.color.a;
    data.brightness = character.brightness;
    data.scaleX     = character.scale;
    data.scaleY     = character.scale;
}





HRESULT RenderSystem::UpdateInstanceBuffer (const AnimationSystem& animationSystem, ColorScheme colorScheme, float elapsedTime, const D2D1_RECT_F * pOcclusionRect)
{
    HRESULT                  hr             = S_OK;
    Color4                   schemeColor    = GetColorRGB (colorScheme, elapsedTime);
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    size_t                   bytesToCopy;



    // Clear working data from previous frame (reuse allocated capacity)
    m_instanceData.clear();
    m_streakPtrs.clear();

    // Get streaks sorted by depth
    for (const auto& streak : animationSystem.GetStreaks())
    {
        m_streakPtrs.push_back (&streak);
    }

    SortStreaksByDepth (m_streakPtrs);

    // Build instance data
    for (const CharacterStreak* streak : m_streakPtrs)
    {
        const auto & characters = streak->GetCharacters();
        Vector3      streakPos  = streak->GetPosition();

        // Render all characters (they manage their own fading/removal)
        for (const auto & character : characters)
        {
            CharacterInstanceData data;



            // Skip characters occluded by the help hint overlay
            if (pOcclusionRect)
            {
                float charX = streakPos.x + character.positionOffset.x;
                float charY = character.positionOffset.y;

                if (charX >= pOcclusionRect->left && charX <= pOcclusionRect->right &&
                    charY >= pOcclusionRect->top  && charY <= pOcclusionRect->bottom)
                {
                    continue;
                }
            }

            BuildCharacterInstanceData (character, streakPos, schemeColor, data);
            m_instanceData.push_back (data);
        }
    }

    // Render overlay characters (standalone characters not attached to any streak,
    // such as horizontal tracer animations in the help dialog)
    for (const auto & overlay : animationSystem.GetOverlayCharacters())
    {
        CharacterInstanceData data;



        BuildCharacterInstanceData (overlay.character, overlay.position, schemeColor, data);
        m_instanceData.push_back (data);
    }

    BAIL_OUT_IF (m_instanceData.empty(), S_OK);

    // Resize buffer if needed (double capacity each time to reduce allocations)
    if (m_instanceData.size() > m_instanceBufferCapacity)
    {
        UINT newCapacity = static_cast<UINT> (m_instanceData.size() * 2);
        

        
        m_instanceBuffer.Reset();
        m_instanceBufferCapacity = newCapacity;
        
        hr = CreateInstanceBuffer();
        CHRA (hr);
    }

    // Map instance buffer and upload data
    hr = m_context->Map (m_instanceBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    CHRA (hr);

    bytesToCopy = sizeof (CharacterInstanceData) * m_instanceData.size();
    memcpy (mappedResource.pData, m_instanceData.data(), bytesToCopy);
    
    m_context->Unmap (m_instanceBuffer.Get(), 0);

Error:
    return hr;
}





void RenderSystem::Render (const AnimationSystem & animationSystem, const Viewport & viewport, ColorScheme colorScheme, float fps, int rainPercentage, int streakCount, int activeHeadCount, bool showDebugFadeTimes, float elapsedTime, const HelpHintOverlay * pOverlay, const HotkeyOverlay * pHotkeyOverlay, const D2D1_RECT_F * pOcclusionRect)
{
    if (!m_device || !m_context || !m_renderTargetView)
    {
        return;
    }

    // Clear render target
    ClearRenderTarget();

    // Update constant buffer with projection matrix
    const Matrix4x4          & projection = viewport.GetProjectionMatrix();
    D3D11_MAPPED_SUBRESOURCE   mappedResource;
    HRESULT                    hr = m_context->Map (m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

    if (SUCCEEDED (hr))
    {
        ConstantBufferData* cbData = reinterpret_cast<ConstantBufferData*> (mappedResource.pData);
        
        // Copy projection matrix (4x4 = 16 floats = 64 bytes)
        memcpy (cbData->projection, projection.m, sizeof (projection.m));

        // Rain characters use the standard 24x36 base quad dimensions
        cbData->charWidth  = 24.0f;
        cbData->charHeight = 36.0f;

        // Calculate character scale based on viewport height
        // Scale down proportionally for preview mode to fit the entire effect
        // Minimum scale ensures characters remain visible (12px tall minimum)
        //
        // DPI scaling: The base character size (32×48 in the shader) is in physical pixels.
        // We multiply by m_dpiScale so characters maintain a consistent logical size
        // regardless of the monitor's DPI setting.
        // The reference viewport height (1080) is also DPI-adjusted so that the
        // same logical viewport produces the same scale across DPI levels.
        if (m_characterScaleOverride.has_value())
        {
            // Use explicit override (e.g., UsageDialog forcing full-size characters)
            // UsageDialog handles its own DPI scaling — do NOT multiply by m_dpiScale
            cbData->characterScale = m_characterScaleOverride.value();
        }
        else
        {
            float viewportHeight    = static_cast<float> (viewport.GetHeight());
            float referenceHeight   = 1080.0f * m_dpiScale;
            float viewportBaseScale = 1.0f;

            if (viewportHeight < referenceHeight)
            {
                // Scale linearly based on viewport height (reference = 1.0)
                viewportBaseScale = viewportHeight / referenceHeight;

                // Clamp to minimum 0.5 (24px tall from 48px base at 96 DPI)
                if (viewportBaseScale < 0.5f)
                    viewportBaseScale = 0.5f;
            }

            cbData->characterScale = viewportBaseScale * m_dpiScale;
        }

        m_context->Unmap (m_constantBuffer.Get(), 0);
    }

    // Update instance buffer with character data (pass occlusion rect if provided)
    (void) UpdateInstanceBuffer (animationSystem, colorScheme, elapsedTime, pOcclusionRect);  // Ignore return - errors already handled within

    if (m_instanceData.empty())
    {
        return;
    }

    // Set viewport for rendering
    D3D11_VIEWPORT vp = {};
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width    = static_cast<FLOAT> (viewport.GetWidth());
    vp.Height   = static_cast<FLOAT> (viewport.GetHeight());
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    m_context->RSSetViewports (1, &vp);

    // Set render state
    m_context->IASetInputLayout (m_inputLayout.Get());
    m_context->IASetPrimitiveTopology (D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Set vertex buffers: slot 0 = dummy (for vertex count), slot 1 = instance data
    ID3D11Buffer* buffers[2]     = { m_dummyVertexBuffer.Get(), m_instanceBuffer.Get() };
    UINT          strides[2]     = { 4, sizeof (CharacterInstanceData) };
    UINT          offsets[2]     = { 0, 0 };
    
    
    m_context->IASetVertexBuffers (0, 2, buffers, strides, offsets);

    m_context->VSSetShader (m_vertexShader.Get(), nullptr, 0);
    m_context->VSSetConstantBuffers (0, 1, m_constantBuffer.GetAddressOf());

    m_context->PSSetShader (m_pixelShader.Get(), nullptr, 0);
    m_context->PSSetSamplers (0, 1, m_samplerState.GetAddressOf());
    
    // Bind texture atlas from CharacterSet
    CharacterSet& charSet = CharacterSet::GetInstance();
    ID3D11ShaderResourceView * srv = charSet.GetTextureResourceView();
    if (srv)
    {
        m_context->PSSetShaderResources (0, 1, &srv);
    }

    float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    m_context->OMSetBlendState (m_blendState.Get(), blendFactor, 0xffffffff);
    
    // Set full viewport for rendering
    D3D11_VIEWPORT fullViewport = {};
    fullViewport.Width    = static_cast<float> (m_renderWidth);
    fullViewport.Height   = static_cast<float> (m_renderHeight);
    fullViewport.MinDepth = 0.0f;
    fullViewport.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &fullViewport);
    
    // Render characters to scene texture (NOT directly to backbuffer)
    if (m_sceneRTV)
    {
        // Clear scene texture
        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        m_context->ClearRenderTargetView (m_sceneRTV.Get(), clearColor);
        
        m_context->OMSetRenderTargets (1, m_sceneRTV.GetAddressOf(), nullptr);
        m_context->DrawInstanced (6, static_cast<UINT> (m_instanceData.size()), 0, 0);
        
        // Now apply bloom (which will composite to backbuffer)
        (void)ApplyBloom();  // Ignore return - errors already handled within
    }
    else
    {
        // Fallback: render directly to backbuffer if bloom not available
        m_context->OMSetRenderTargets (1, m_renderTargetView.GetAddressOf(), nullptr);
        m_context->DrawInstanced (6, static_cast<UINT> (m_instanceData.size()), 0, 0);
    }

    // Render help hint overlay if active (after bloom, before FPS counter)
    if (pOverlay && pOverlay->IsActive())
    {
        RenderTwoColumnOverlay (pOverlay->GetCharacters(),
                                HelpHintOverlay::MARGIN_COLS,
                                pOverlay->GetLeftColChars(),
                                pOverlay->GetGapChars(),
                                pOverlay->GetRows(),
                                pOverlay->GetCharHeight(),
                                pOverlay->GetPadding(),
                                HelpHintOverlay::GLOW_LAYERS);
    }

    // Render hotkey overlay if active (after help hint, before FPS counter)
    if (pHotkeyOverlay && pHotkeyOverlay->IsActive())
    {
        RenderTwoColumnOverlay (pHotkeyOverlay->GetCharacters(),
                                HotkeyOverlay::MARGIN_COLS,
                                pHotkeyOverlay->GetKeyColChars(),
                                pHotkeyOverlay->GetGapChars(),
                                pHotkeyOverlay->GetCharRows(),
                                pHotkeyOverlay->GetRowHeight(),
                                pHotkeyOverlay->GetPadding(),
                                HotkeyOverlay::GLOW_LAYERS);
    }

    // Render FPS counter overlay if fps > 0
    if (fps > 0.0f)
    {
        RenderFPSCounter (fps, rainPercentage, streakCount, activeHeadCount);
    }

    // Debug: Render fade times when enabled and only one streak is visible
    if (showDebugFadeTimes)
    {
        RenderDebugFadeTimes (animationSystem);
    }
}





void RenderSystem::Present()
{
    if (m_swapChain)
    {
        m_swapChain->Present (1, 0); // VSync enabled
    }
}





void RenderSystem::RenderFPSCounter (float fps, int rainPercentage, int streakCount, int activeHeadCount)
{
    HRESULT                   hr           = S_OK;
    bool                      drawing      = false;
    wchar_t                   fpsText[128];
    D2D1_SIZE_F               size         = { 0 };
    ComPtr<IDWriteTextLayout> textLayout;
    UINT32                    textLength   = 0; 
    DWRITE_TEXT_METRICS       metrics      = { };
    D2D1_RECT_F               textRect     = { };



    CBRAEx (m_d2dContext && m_fpsBrush && m_fpsTextFormat && m_fpsGlowBrush, E_UNEXPECTED);

    // Begin D2D rendering
    m_d2dContext->BeginDraw();
    drawing = true;

    // Format FPS text with rain density info: "Rain xxx% (yyy heads / zzz total), ww FPS"
    swprintf_s (fpsText, L"Rain %d%% (%d heads / %d total), %.0f FPS", rainPercentage, activeHeadCount, streakCount, fps);

    // Get render target size for positioning
    size = m_d2dContext->GetSize();

    // Create a DirectWrite text layout to measure the text and avoid wrapping.
    textLength = static_cast<UINT32> (wcslen (fpsText));
    hr = m_dwriteFactory->CreateTextLayout (fpsText,
                                            textLength,
                                            m_fpsTextFormat.Get(),
                                            10000.0f,   // large max width to prevent wrapping
                                            1000.0f,    // large max height
                                            &textLayout);
    CHRA (hr);
    CBRAEx (textLayout != nullptr, E_UNEXPECTED);

    hr = textLayout->GetMetrics (&metrics);
    CHRA (hr);

    // Create text rect sized to measured width to prevent wrapping, anchored bottom-right
    textRect.left   = size.width  - metrics.widthIncludingTrailingWhitespace - 10.0f;
    textRect.top    = size.height - metrics.height - 10.0f;
    textRect.right  = size.width  - 10.0f;
    textRect.bottom = size.height - 10.0f;
    
    if (textRect.left < 10.0f) 
    {
        textRect.left = 10.0f;
    }

    DrawFeatheredGlow (fpsText, textLength, textRect);

    // Draw main text
    m_d2dContext->DrawText (fpsText,
                            textLength,
                            m_fpsTextFormat.Get(),
                            textRect,
                            m_fpsBrush.Get());

    
Error:
    if (drawing)
    {
        // End D2D rendering
        m_d2dContext->EndDraw();
    }

    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::CodepointToUtf16
//
//  Converts a Unicode codepoint to a UTF-16 wchar_t string.
//  Returns the number of wchar_t units written (1 for BMP, 2 for surrogate).
//
////////////////////////////////////////////////////////////////////////////////

int RenderSystem::CodepointToUtf16 (uint32_t codepoint, wchar_t * glyphStr)
{
    int glyphLen = 0;



    glyphStr[0] = L'\0';
    glyphStr[1] = L'\0';
    glyphStr[2] = L'\0';

    if (codepoint <= 0xFFFF)
    {
        glyphStr[0] = static_cast<wchar_t> (codepoint);
        glyphLen    = 1;
    }
    else
    {
        glyphStr[0] = static_cast<wchar_t> (0xD800 + ((codepoint - 0x10000) >> 10));
        glyphStr[1] = static_cast<wchar_t> (0xDC00 + ((codepoint - 0x10000) & 0x3FF));
        glyphLen    = 2;
    }

    return glyphLen;
}





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::DrawFeatheredBackground
//
//  Draws a feathered dark background with expanding layers of decreasing
//  opacity, plus a solid dark center.
//
////////////////////////////////////////////////////////////////////////////////

void RenderSystem::DrawFeatheredBackground (const D2D1_RECT_F & boundingRect, float opacityScale)
{
    const int   featherLayers  = 24;
    const float maxExpand      = 60.0f;
    const float centerOpacity  = 0.35f * opacityScale;



    for (int i = featherLayers; i > 0; --i)
    {
        float t       = static_cast<float>(i) / static_cast<float>(featherLayers);
        float expand  = maxExpand * t;
        float opacity = centerOpacity * (1.0f - t);

        D2D1_RECT_F expandedRect = D2D1::RectF (boundingRect.left   - expand,
                                                boundingRect.top    - expand,
                                                boundingRect.right  + expand,
                                                boundingRect.bottom + expand);

        m_fpsGlowBrush->SetColor (D2D1::ColorF (D2D1::ColorF::Black, opacity));
        m_d2dContext->FillRectangle (expandedRect, m_fpsGlowBrush.Get());
    }

    // Solid dark center
    m_fpsGlowBrush->SetColor (D2D1::ColorF (D2D1::ColorF::Black, centerOpacity));
    m_d2dContext->FillRectangle (boundingRect, m_fpsGlowBrush.Get());
}






// Overlay rendering constants
static constexpr float OVERLAY_FONT_DIP       = 16.0f;   // Overlay font design size in DIP (matches CharacterSet)
static constexpr float OVERLAY_CELL_DIP       = 28.0f;   // Base cell height in DIP (matches BASE_CHAR_HEIGHT / BASE_ROW_HEIGHT)
static constexpr float OVERLAY_ADV_SCALE      = OVERLAY_FONT_DIP / OVERLAY_CELL_DIP;  // Font-to-cell advance ratio
static constexpr float OVERLAY_CHAR_SCALE     = 1.0f;    // 1:1 pixel mapping (no scale)
static constexpr float OVERLAY_GLOW_OFFSET    = 1.0f;    // Shadow offset in pixels per layer
static constexpr float OVERLAY_MIN_OPACITY    = 0.01f;   // Below this opacity, skip rendering
static constexpr float OVERLAY_MAX_GLOW_ALPHA = 0.9f;    // Peak glow opacity at innermost layer





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::BuildOverlayInstances
//
//  Builds GPU instance data for overlay characters.  For each visible
//  character, creates N shadow instances (dark glow) plus one foreground
//  instance.  Character x-positions are provided via a pre-computed
//  array of proportional offsets (one per character in the span).
//
////////////////////////////////////////////////////////////////////////////////

void RenderSystem::BuildOverlayInstances (std::span<const HintCharacter> chars,
                                          int                            glowLayers,
                                          float                          charScale,
                                          float                          glowOffset,
                                          float                          baseY,
                                          float                          cellHeight,
                                          std::span<const float>         xPositions,
                                          float                          advanceScale)
{
    CharacterSet           & charSet    = CharacterSet::GetInstance();
    float                    posX       = 0.0f;
    float                    posY       = 0.0f;
    float                    offset     = 0.0f;
    float                    baseAlpha  = 0.0f;
    float                    alpha      = 0.0f;
    float                    widthRatio = 1.0f;
    float                    uvWidth    = 0.0f;
    CharacterInstanceData    inst;



    m_overlayInstanceData.clear();

    for (size_t i = 0; i < chars.size(); i++)
    {
        const HintCharacter & ch        = chars[i];
        const OverlayUV     & overlayUV = charSet.GetOverlayUV (ch.currentGlyphIndex);
        const GlyphInfo     & glyph     = charSet.GetGlyph (ch.currentGlyphIndex);



        if (ch.phase == CharPhase::Hidden || ch.opacity <= OVERLAY_MIN_OPACITY)
        {
            continue;
        }

        // Compute width ratio: how much of the atlas cell this character
        // actually occupies.  Shrink the quad and crop the UV so the
        // rendered width matches the proportional advance width.
        widthRatio = std::min (glyph.advanceWidth * advanceScale, 1.0f);
        uvWidth    = (overlayUV.uvMax.x - overlayUV.uvMin.x) * widthRatio;

        // Normal glyphs are left-aligned in the atlas cell, so crop from
        // the left.  Mirrored glyphs are horizontally flipped, placing
        // their ink on the right side — crop from the right instead.
        float uvMinX = glyph.mirrored ? (overlayUV.uvMax.x - uvWidth) : overlayUV.uvMin.x;
        float uvMaxX = glyph.mirrored ? (overlayUV.uvMax.x)           : overlayUV.uvMin.x + uvWidth;

        posX = xPositions[i];
        posY = roundf (baseY + static_cast<float> (ch.row) * cellHeight);

        // Shadow/glow instances (outermost layer first for correct blending)
        for (int layer = glowLayers; layer > 0; --layer)
        {
            offset    = static_cast<float> (layer) * glowOffset;
            baseAlpha = OVERLAY_MAX_GLOW_ALPHA - (static_cast<float> (layer) / static_cast<float> (glowLayers));

            if (baseAlpha <= 0.0f)
            {
                continue;
            }

            alpha = baseAlpha * ch.opacity;

            for (int dy = -1; dy <= 1; ++dy)
            {
                for (int dx = -1; dx <= 1; ++dx)
                {
                    if (dx == 0 && dy == 0)
                    {
                        continue;
                    }

                    inst             = {};
                    inst.position[0] = posX + offset * static_cast<float> (dx);
                    inst.position[1] = posY + offset * static_cast<float> (dy);
                    inst.position[2] = 0.0f;
                    inst.uvMin[0]    = uvMinX;
                    inst.uvMin[1]    = overlayUV.uvMin.y;
                    inst.uvMax[0]    = uvMaxX;
                    inst.uvMax[1]    = overlayUV.uvMax.y;
                    inst.color[0]    = 0.0f;
                    inst.color[1]    = 0.0f;
                    inst.color[2]    = 0.0f;
                    inst.color[3]    = 1.0f;
                    inst.brightness  = alpha;
                    inst.scaleX      = widthRatio;
                    inst.scaleY      = charScale;

                    m_overlayInstanceData.push_back (inst);
                }
            }
        }

        // Foreground instance
        inst             = {};
        inst.position[0] = posX;
        inst.position[1] = posY;
        inst.position[2] = 0.0f;
        inst.uvMin[0]    = uvMinX;
        inst.uvMin[1]    = overlayUV.uvMin.y;
        inst.uvMax[0]    = uvMaxX;
        inst.uvMax[1]    = overlayUV.uvMax.y;
        inst.color[0]    = ch.colorR;
        inst.color[1]    = ch.colorG;
        inst.color[2]    = ch.colorB;
        inst.color[3]    = 1.0f;
        inst.brightness  = ch.opacity;
        inst.scaleX      = widthRatio;
        inst.scaleY      = charScale;

        m_overlayInstanceData.push_back (inst);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::RenderOverlayInstances
//
//  Uploads overlay instance data to the GPU and renders them using the
//  overlay pixel shader (no additive glow).  Sets characterScale = 1.0
//  in the constant buffer so per-instance scale controls character size.
//
////////////////////////////////////////////////////////////////////////////////

void RenderSystem::RenderOverlayInstances ()
{
    HRESULT                    hr            = S_OK;
    UINT                       instanceCount = static_cast<UINT> (m_overlayInstanceData.size());
    D3D11_MAPPED_SUBRESOURCE   mapped        = {};
    size_t                     bytesToCopy   = 0;
    CharacterSet             & charSet       = CharacterSet::GetInstance();
    ID3D11ShaderResourceView * srv           = nullptr;
    float                      width         = static_cast<float> (m_renderWidth);
    float                      height        = static_cast<float> (m_renderHeight);
    float                      overlayCW     = charSet.GetOverlayCellContentWidth();
    float                      overlayCH     = charSet.GetOverlayCellContentHeight();



    BAIL_OUT_IF (instanceCount == 0, S_OK);

    // Grow overlay instance buffer if needed
    if (instanceCount > m_overlayInstanceBufferCapacity)
    {
        UINT newCapacity = instanceCount * 2;

        m_overlayInstanceBuffer.Reset();
        m_overlayInstanceBufferCapacity = newCapacity;

        D3D11_BUFFER_DESC bufDesc = {};

        bufDesc.ByteWidth      = sizeof (CharacterInstanceData) * newCapacity;
        bufDesc.Usage          = D3D11_USAGE_DYNAMIC;
        bufDesc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
        bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        hr = m_device->CreateBuffer (&bufDesc, nullptr, &m_overlayInstanceBuffer);
        CHRA (hr);
    }

    // Upload instance data
    hr = m_context->Map (m_overlayInstanceBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    CHRA (hr);

    bytesToCopy = sizeof (CharacterInstanceData) * instanceCount;
    memcpy (mapped.pData, m_overlayInstanceData.data(), bytesToCopy);
    m_context->Unmap (m_overlayInstanceBuffer.Get(), 0);

    // Update constant buffer: orthographic projection + characterScale = 1.0
    hr = m_context->Map (m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    CHRA (hr);

    {
        ConstantBufferData * cbData = static_cast<ConstantBufferData *> (mapped.pData);

        memset (cbData->projection, 0, sizeof (cbData->projection));
        cbData->projection[0]  = 2.0f / width;     // m[0][0]
        cbData->projection[5]  = -2.0f / height;   // m[1][1]
        cbData->projection[10] = 0.01f;             // m[2][2]  1/(farZ-nearZ) = 1/100
        cbData->projection[12] = -1.0f;             // m[3][0]
        cbData->projection[13] = 1.0f;              // m[3][1]
        cbData->projection[15] = 1.0f;              // m[3][3]

        cbData->characterScale = 1.0f;

        // Exact pixel dimensions for 1:1 texel-to-pixel mapping
        cbData->charWidth      = overlayCW;
        cbData->charHeight     = overlayCH;
    }

    m_context->Unmap (m_constantBuffer.Get(), 0);

    // Set render target to backbuffer
    m_context->OMSetRenderTargets (1, m_renderTargetView.GetAddressOf(), nullptr);

    // Set viewport
    {
        D3D11_VIEWPORT vp = {};

        vp.Width    = width;
        vp.Height   = height;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;

        m_context->RSSetViewports (1, &vp);
    }

    // Set pipeline state
    m_context->IASetInputLayout (m_inputLayout.Get());
    m_context->IASetPrimitiveTopology (D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    {
        ID3D11Buffer * buffers[2] = { m_dummyVertexBuffer.Get(), m_overlayInstanceBuffer.Get() };
        UINT           strides[2] = { 4, sizeof (CharacterInstanceData) };
        UINT           offsets[2] = { 0, 0 };

        m_context->IASetVertexBuffers (0, 2, buffers, strides, offsets);
    }

    m_context->VSSetShader (m_vertexShader.Get(), nullptr, 0);
    m_context->VSSetConstantBuffers (0, 1, m_constantBuffer.GetAddressOf());
    m_context->PSSetShader (m_overlayPixelShader.Get(), nullptr, 0);
    m_context->PSSetSamplers (0, 1, m_samplerState.GetAddressOf());

    srv = charSet.GetOverlayTextureResourceView();
    if (srv)
    {
        m_context->PSSetShaderResources (0, 1, &srv);
    }

    {
        float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

        m_context->OMSetBlendState (m_premultipliedBlendState.Get(), blendFactor, 0xffffffff);
    }

    // Draw overlay characters
    m_context->DrawInstanced (6, instanceCount, 0, 0);

Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  ComputeMeanCharOpacity
//
//  Returns the average opacity of visible (non-space) characters.
//
////////////////////////////////////////////////////////////////////////////////

static float ComputeMeanCharOpacity (std::span<const HintCharacter> chars)
{
    float opacitySum   = 0.0f;
    int   visibleCount = 0;


    for (const auto & ch : chars)
    {
        if (!ch.isSpace)
        {
            opacitySum += ch.opacity;
            visibleCount++;
        }
    }

    return (visibleCount > 0) ? (opacitySum / static_cast<float> (visibleCount)) : 0.0f;
}





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::CalculateColumnAlignedTextPositions
//
//  Computes proportional x-positions for a two-column overlay layout.
//  The key column is right-aligned, the description column is left-aligned,
//  with a proportional gap between them.  Fills positions with relative
//  x-coordinates (before centering offset is applied).
//
//  Column layout (character indices):
//
//    |  margin  |     key col  |  gap  |  desc col           |  margin  |
//    |          |          F1  |       |  Toggle FPS         |          |
//    0          |              |       |
//           marginCols         |   descColStart
//                         gapColStart
//
////////////////////////////////////////////////////////////////////////////////

void RenderSystem::CalculateColumnAlignedTextPositions (
    std::span<const HintCharacter>   chars,
    int                              marginCols,
    int                              keyColChars,
    int                              descColStart,
    float                            maxKeyWidth,
    const std::vector<float>       & keyColWidths,
    float                            gapWidth,
    float                            advScaled,
    std::vector<float>             & positions)
{
    CharacterSet & charSet      = CharacterSet::GetInstance();
    int            gapColStart  = marginCols + keyColChars;
    float          charPos      = 0.0f;
    bool           foundKeyText = false;



    positions.resize (chars.size());

    for (size_t i = 0; i < chars.size(); i++)
    {
        const HintCharacter & ch      = chars[i];
        float                 advance = charSet.GetGlyph (ch.targetGlyphIndex).advanceWidth * advScaled;



        // Reset position at column boundaries
        if (ch.col <= marginCols)
        {
            charPos      = 0.0f;
            foundKeyText = false;
        }
        else if (ch.col == gapColStart)
        {
            charPos = maxKeyWidth;
        }
        else if (ch.col == descColStart)
        {
            charPos = maxKeyWidth + gapWidth;
        }

        // Right-align within key column: jump position on first visible char
        if (ch.col >= marginCols && ch.col < gapColStart && !ch.isSpace && !foundKeyText)
        {
            charPos      = maxKeyWidth - keyColWidths[ch.row];
            foundKeyText = true;
        }

        positions[i] = roundf (charPos);
        charPos     += advance;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::ComputeOverlayLayout
//
//  Measures text, computes proportional x-positions, and determines the
//  centered bounding rect for a two-column overlay.  Fills xPositions
//  with absolute screen coordinates, bounds with the overlay rect, baseY
//  with the first row's Y origin, and advanceScale with the ratio for
//  BuildOverlayInstances.
//
////////////////////////////////////////////////////////////////////////////////

void RenderSystem::ComputeOverlayLayout (
    std::span<const HintCharacter>   chars,
    int                              marginCols,
    int                              keyColChars,
    int                              gapChars,
    int                              numRows,
    float                            cellHeight,
    float                            padding,
    std::vector<float>             & xPositions,
    D2D1_RECT_F                    & bounds,
    float                          & baseY,
    float                          & advanceScale)
{
    CharacterSet     & charSet      = CharacterSet::GetInstance();
    float              advScaled    = cellHeight * OVERLAY_ADV_SCALE;
    int                descColStart = marginCols + keyColChars + gapChars;
    std::vector<float> keyColWidths;
    float              maxKeyWidth  = 0.0f;
    float              gapWidth     = 0.0f;
    float              maxRowWidth  = 0.0f;
    float              totalWidth   = 0.0f;
    float              totalHeight  = 0.0f;
    float              baseX        = 0.0f;



    advanceScale = advScaled / charSet.GetOverlayCellContentWidth();

    // Measure proportional width per row of key column (for right-alignment)
    keyColWidths.resize (numRows, 0.0f);

    for (const auto & ch : chars)
    {
        if (ch.col >= marginCols && ch.col < marginCols + keyColChars && !ch.isSpace)
        {
            keyColWidths[ch.row] += charSet.GetGlyph (ch.targetGlyphIndex).advanceWidth * advScaled;
        }
    }

    maxKeyWidth = *std::ranges::max_element (keyColWidths);
    gapWidth    = charSet.GetSpaceAdvanceWidth() * advScaled * static_cast<float> (gapChars);

    // Compute x-positions with two-column layout
    CalculateColumnAlignedTextPositions (chars, marginCols, keyColChars,
                                        descColStart,
                                        maxKeyWidth, keyColWidths,
                                        gapWidth, advScaled, xPositions);

    // Compute max row width for bounding rect
    for (size_t i = 0; i < chars.size(); i++)
    {
        maxRowWidth = std::max (maxRowWidth, xPositions[i] + charSet.GetGlyph (chars[i].targetGlyphIndex).advanceWidth * advScaled);
    }

    // Center overlay on screen
    totalWidth  = maxRowWidth + 2.0f * padding;
    totalHeight = static_cast<float> (numRows) * cellHeight + 2.0f * padding;

    bounds.left   = (static_cast<float> (m_renderWidth)  - totalWidth)  / 2.0f;
    bounds.top    = (static_cast<float> (m_renderHeight) - totalHeight) / 2.0f;
    bounds.right  = bounds.left + totalWidth;
    bounds.bottom = bounds.top  + totalHeight;

    baseX = roundf (bounds.left + padding);
    baseY = roundf (bounds.top  + padding);

    // Convert relative x-positions to absolute
    for (auto & x : xPositions)
    {
        x += baseX;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::RenderTwoColumnOverlay
//
//  Shared rendering logic for two-column overlay layouts (help hint and
//  hotkey overlays).  Computes layout positioning, draws a D2D feathered
//  background, and renders characters via GPU instancing.
//
////////////////////////////////////////////////////////////////////////////////

void RenderSystem::RenderTwoColumnOverlay (
    std::span<const HintCharacter> chars,
    int                            marginCols,
    int                            keyColChars,
    int                            gapChars,
    int                            numRows,
    float                          cellHeight,
    float                          padding,
    int                            glowLayers)
{
    HRESULT            hr           = S_OK;
    bool               drawing      = false;
    float              meanOpacity  = 0.0f;
    std::vector<float> xPositions;
    D2D1_RECT_F        bounds       = {};
    float              baseY        = 0.0f;
    float              advanceScale = 0.0f;



    CBRAEx (m_d2dContext && m_fpsBrush && m_fpsGlowBrush, E_UNEXPECTED);
    BAIL_OUT_IF (chars.empty(), S_OK);

    meanOpacity = ComputeMeanCharOpacity (chars);

    ComputeOverlayLayout (chars, marginCols, keyColChars, gapChars,
                          numRows, cellHeight, padding,
                          xPositions, bounds, baseY, advanceScale);

    // D2D: feathered background
    m_d2dContext->BeginDraw();
    drawing = true;
    DrawFeatheredBackground (bounds, meanOpacity);
    m_d2dContext->EndDraw();
    drawing = false;

    // GPU: overlay characters
    BuildOverlayInstances (chars, glowLayers,
                           OVERLAY_CHAR_SCALE, OVERLAY_GLOW_OFFSET, baseY, cellHeight,
                           std::span<const float> (xPositions), advanceScale);
    RenderOverlayInstances();

Error:
    if (drawing)
    {
        m_d2dContext->EndDraw();
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::DrawFeatheredGlow
//
//  Draw feathered glow effect by rendering text multiple times with offsets 
//  and decreasing opacity
//
////////////////////////////////////////////////////////////////////////////////

void RenderSystem::DrawFeatheredGlow (const wchar_t * fpsText, UINT32 textLength, const D2D1_RECT_F & textRect)
{
    const int glowLayers = 10;



    for (int i = glowLayers; i > 0; --i)
    {
        float offset  = static_cast<float> (i);
        float opacity = 1.0f - (static_cast<float> (i) / static_cast<float> (glowLayers));

        m_fpsGlowBrush->SetColor (D2D1::ColorF (D2D1::ColorF::Black, opacity));

        // Draw text at offsets to create uniform glow
        for (int dx = -1; dx <= 1; ++dx)
        {
            for (int dy = -1; dy <= 1; ++dy)
            {
                if (dx == 0 && dy == 0)
                {
                    continue;
                }

                D2D1_RECT_F glowRect = D2D1::RectF (textRect.left   + offset * dx,
                                                    textRect.top    + offset * dy,
                                                    textRect.right  + offset * dx,
                                                    textRect.bottom + offset * dy);

                m_d2dContext->DrawText (fpsText,
                                        textLength,
                                        m_fpsTextFormat.Get(),
                                        glowRect,
                                        m_fpsGlowBrush.Get());
            }
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::RenderDebugFadeTimes
//
//  Renders fade time remaining to the right of each character for debugging.
//
////////////////////////////////////////////////////////////////////////////////

void RenderSystem::RenderDebugFadeTimes (const AnimationSystem& animationSystem)
{
    if (!m_d2dContext || !m_fpsBrush || !m_fpsTextFormat)
    {
        return;
    }

    const auto& streaks = animationSystem.GetStreaks();

    m_d2dContext->BeginDraw();

    for (const CharacterStreak& streak : streaks)
    {
        const auto& characters = streak.GetCharacters();
        Vector3     streakPos  = streak.GetPosition();

        for (const auto& character : characters)
        {
            // Calculate screen position
            float screenX = streakPos.x + character.positionOffset.x;
            float screenY = character.positionOffset.y;

            // Format lifetime text
            wchar_t fadeText[32];
            swprintf_s (fadeText, L"%.2f", character.lifetime);

            // Position text to the right of the character
            D2D1_RECT_F textRect = D2D1::RectF (
                screenX + 40.0f,    // 40px to the right of character
                screenY,
                screenX + 140.0f,
                screenY + 30.0f
            );

            // Draw text in white
            m_d2dContext->DrawText (
                fadeText,
                static_cast<UINT32> (wcslen (fadeText)),
                m_fpsTextFormat.Get(),
                textRect,
            m_fpsBrush.Get()
        );
        }
    }

    m_d2dContext->EndDraw();
}





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::Resize
//
////////////////////////////////////////////////////////////////////////////////

void RenderSystem::Resize (UINT width, UINT height)
{
    HRESULT        hr       = S_OK;
    D3D11_VIEWPORT viewport = {};


    m_renderWidth  = width;
    m_renderHeight = height;

    if (!m_swapChain)
    {
        return;
    }

    // Release bloom resources before resizing - they need to be recreated with new dimensions
    ReleaseBloomResources();

    // CRITICAL: Clear D2D target before resizing
    if (m_d2dContext)
    {
        m_d2dContext->SetTarget (nullptr);
    }

    // Release render target view and D2D bitmap
    ReleaseRenderTargetResources();

    // Resize swap chain buffers
    hr = m_swapChain->ResizeBuffers (0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    CHR (hr);

    // Recreate render target view
    hr = CreateRenderTargetView();
    CHR (hr);

    // Recreate D2D bitmap for the new back buffer
    hr = RecreateDirect2DBitmap();
    CHR (hr);

    // Recreate bloom resources with new dimensions
    hr = CreateBloomResources (width, height);
    CHR (hr);

    // Update viewport
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width    = static_cast<FLOAT> (width);
    viewport.Height   = static_cast<FLOAT> (height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    m_context->RSSetViewports (1, &viewport);

Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::ReleaseBloomResources
//
////////////////////////////////////////////////////////////////////////////////

void RenderSystem::ReleaseBloomResources()
{
    m_sceneSRV.Reset();
    m_sceneRTV.Reset();
    m_sceneTexture.Reset();
    m_bloomSRV.Reset();
    m_bloomRTV.Reset();
    m_bloomTexture.Reset();
    m_blurTempSRV.Reset();
    m_blurTempRTV.Reset();
    m_blurTempTexture.Reset();
}





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::ReleaseDirect2DResources
//
////////////////////////////////////////////////////////////////////////////////

void RenderSystem::ReleaseDirect2DResources()
{
    m_fpsBrush.Reset();
    m_fpsGlowBrush.Reset();
    m_fpsTextFormat.Reset();
    m_dwriteFactory.Reset();
    m_d2dBitmap.Reset();
    m_d2dContext.Reset();
    m_d2dDevice.Reset();
    m_d2dFactory.Reset();
}





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::ReleaseRenderTargetResources
//
////////////////////////////////////////////////////////////////////////////////

void RenderSystem::ReleaseRenderTargetResources()
{
    m_renderTargetView.Reset();
    m_d2dBitmap.Reset();
}





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::ReleaseDirectXResources
//
////////////////////////////////////////////////////////////////////////////////

void RenderSystem::ReleaseDirectXResources()
{
    m_fullscreenQuadVS.Reset();
    m_fullscreenQuadVB.Reset();
    m_bloomExtractPS.Reset();
    m_blurHorizontalPS.Reset();
    m_blurVerticalPS.Reset();
    m_compositePS.Reset();
    m_bloomConstantBuffer.Reset();
    m_atlasTextureSRV.Reset();
    m_pointSamplerState.Reset();
    m_samplerState.Reset();
    m_premultipliedBlendState.Reset();
    m_blendState.Reset();
    m_constantBuffer.Reset();
    m_instanceBuffer.Reset();
    m_inputLayout.Reset();
    m_fullscreenQuadInputLayout.Reset();
    m_pixelShader.Reset();
    m_vertexShader.Reset();
    m_renderTargetView.Reset();
    m_swapChain.Reset();
    m_context.Reset();
    m_device.Reset();
}





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::RecreateDirect2DBitmap
//
////////////////////////////////////////////////////////////////////////////////

HRESULT RenderSystem::RecreateDirect2DBitmap()
{
    HRESULT                                  hr          = S_OK;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>  backBuffer;
    Microsoft::WRL::ComPtr<IDXGISurface>     dxgiSurface;
    D2D1_BITMAP_PROPERTIES1                  bitmapProps;



    if (!m_d2dContext)
    {
        return S_OK;
    }

    hr = m_swapChain->GetBuffer (0, __uuidof (ID3D11Texture2D), &backBuffer);
    CHR (hr);

    hr = backBuffer.As (&dxgiSurface);
    CHR (hr);

    bitmapProps = D2D1::BitmapProperties1 (
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat (DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );

    hr = m_d2dContext->CreateBitmapFromDxgiSurface (dxgiSurface.Get(), &bitmapProps, &m_d2dBitmap);
    CHR (hr);

    m_d2dContext->SetTarget (m_d2dBitmap.Get());

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::SetGlowIntensity
//
////////////////////////////////////////////////////////////////////////////////

void RenderSystem::SetGlowIntensity (int intensityPercent)
{
    // Convert percentage (0-200) to multiplier (0.0-5.0)
    // Default is 100% = 2.5 multiplier
    m_glowIntensity = (intensityPercent / 100.0f) * 2.5f;
}





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::SetGlowSize
//
////////////////////////////////////////////////////////////////////////////////

void RenderSystem::SetGlowSize (int sizePercent)
{
    // Convert percentage (50-200) to multiplier (0.5-2.0)
    // Default is 100% = 1.0 multiplier
    m_glowSize = sizePercent / 100.0f;
}





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::SetCharacterScaleOverride
//
//  Overrides the viewport-based character scale calculation.
//  Used by UsageDialog to force full-size rain characters in its
//  smaller window.
//
////////////////////////////////////////////////////////////////////////////////

void RenderSystem::SetCharacterScaleOverride (float scale)
{
    m_characterScaleOverride = scale;
}





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::Shutdown
//
////////////////////////////////////////////////////////////////////////////////

void RenderSystem::Shutdown()
{
    ReleaseDirect2DResources();
    ReleaseBloomResources();
    ReleaseDirectXResources();
}





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::RecreateSwapChain
//
////////////////////////////////////////////////////////////////////////////////

bool RenderSystem::RecreateSwapChain (HWND hwnd, UINT width, UINT height, bool fullscreen)
{
    HRESULT hr = S_OK;


    // Release render target view and D2D bitmap before recreating swap chain
    ReleaseRenderTargetResources();

    // Release the old swap chain
    m_swapChain.Reset();

    // Create new swap chain with updated settings
    hr = CreateSwapChain (hwnd, width, height);
    CHR (hr);

    // Set fullscreen state if requested
    if (fullscreen)
    {
        hr = m_swapChain->SetFullscreenState (TRUE, nullptr);
        CHRA (hr);
    }

    // Recreate render target view
    hr = CreateRenderTargetView();
    CHR (hr);

    // Recreate D2D bitmap for the new back buffer
    hr = RecreateDirect2DBitmap();
    CHRA (hr);

    // Resize viewport to match new dimensions
    Resize (width, height);

Error:
    return SUCCEEDED (hr);
}





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::SetFullscreen
//
////////////////////////////////////////////////////////////////////////////////

bool RenderSystem::SetFullscreen()
{
    if (!m_swapChain)
    {
        return false;
    }

    HRESULT hr = m_swapChain->SetFullscreenState (TRUE, nullptr);
    return SUCCEEDED (hr);
}





////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem::SetWindowed
//
////////////////////////////////////////////////////////////////////////////////

bool RenderSystem::SetWindowed()
{
    if (!m_swapChain)
    {
        return false;
    }

    HRESULT hr = m_swapChain->SetFullscreenState (FALSE, nullptr);
    return SUCCEEDED (hr);
}
