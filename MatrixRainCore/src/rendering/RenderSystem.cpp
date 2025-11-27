#include "pch.h"

#include "MatrixRain/RenderSystem.h"

#include "MatrixRain/CharacterSet.h"
#include "MatrixRain/ColorScheme.h"





namespace MatrixRain
{
    RenderSystem::RenderSystem() :
        m_instanceBufferCapacity (INITIAL_INSTANCE_CAPACITY),
        m_renderWidth            (0),
        m_renderHeight           (0)
    {
    }





    RenderSystem::~RenderSystem()
    {
        Shutdown();
    }





    bool RenderSystem::Initialize (HWND hwnd, UINT width, UINT height)
    {
        HRESULT        hr       = S_OK;
        D3D11_VIEWPORT viewport = {};


        // Store dimensions for viewport and render target sizing
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

        hr = CreateBlendState();
        CHR (hr);

        hr = CreateSamplerState();
        CHR (hr);

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
        return SUCCEEDED (hr);
    }





    HRESULT RenderSystem::CreateDevice()
    {
        HRESULT           hr               = S_OK;
        UINT              createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT; // Required for Direct2D interop
        D3D_FEATURE_LEVEL featureLevels[]  = {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
        };
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
        CHRA (hr);

    Error:
        return hr;
    }





    HRESULT RenderSystem::CreateSwapChain (HWND hwnd, UINT width, UINT height)
    {
        HRESULT                                 hr          = S_OK;
        Microsoft::WRL::ComPtr<IDXGIDevice>     dxgiDevice;
        Microsoft::WRL::ComPtr<IDXGIAdapter>    dxgiAdapter;
        Microsoft::WRL::ComPtr<IDXGIFactory>    dxgiFactory;
        DXGI_SWAP_CHAIN_DESC                    swapChainDesc = {};


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
        HRESULT                                 hr = S_OK;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;


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
            };

            struct VSInput
            {
                float3 position : POSITION;
                float2 uvMin : TEXCOORD0;
                float2 uvMax : TEXCOORD1;
                float4 color : COLOR;
                float brightness : BRIGHTNESS;
                float scale : SCALE;
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
                
                // Character size in world space (scaled up for visibility)
                float2 charSize = float2(32.0, 48.0) * input.scale;
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

    static const D3D11_INPUT_ELEMENT_DESC s_krgInputLayout[] = {
        { "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT,    1, 0,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "TEXCOORD",   0, DXGI_FORMAT_R32G32_FLOAT,       1, 12, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "TEXCOORD",   1, DXGI_FORMAT_R32G32_FLOAT,       1, 20, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "COLOR",      0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 28, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "BRIGHTNESS", 0, DXGI_FORMAT_R32_FLOAT,          1, 44, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "SCALE",      0, DXGI_FORMAT_R32_FLOAT,          1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
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
        HRESULT                          hr = S_OK;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;


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
        HRESULT                          hr = S_OK;
        Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> psBlob;
        ShaderCompileEntry               shaderTable[] = {
            { s_kszVertexShaderSource, "VS", "main", "vs_5_0",  L"D3DCompile failed for vertex shader", vsBlob.GetAddressOf() },
            { s_kszPixelShaderSource,  "PS", "main", "ps_5_0",  L"D3DCompile failed for pixel shader",  psBlob.GetAddressOf() }
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

    Error:
        return hr;
    }





    HRESULT RenderSystem::CreateDirect2DResources()
    {
        HRESULT                             hr = S_OK;
        D2D1_FACTORY_OPTIONS                options = {};
        Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;


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
        hr = DWriteCreateFactory (DWRITE_FACTORY_TYPE_SHARED, __uuidof (IDWriteFactory),
                                  reinterpret_cast<IUnknown**> (m_dwriteFactory.GetAddressOf()));
        CHRA (hr);

        // Create text format for FPS display (small, top-right aligned)
        hr = m_dwriteFactory->CreateTextFormat (L"Consolas",
                                                nullptr,
                                                DWRITE_FONT_WEIGHT_NORMAL,
                                                DWRITE_FONT_STYLE_NORMAL,
                                                DWRITE_FONT_STRETCH_NORMAL,
                                                20.0f,
                                                L"en-us",
                                                &m_fpsTextFormat);
        CHRA (hr);

        m_fpsTextFormat->SetTextAlignment (DWRITE_TEXT_ALIGNMENT_TRAILING);
        m_fpsTextFormat->SetParagraphAlignment (DWRITE_PARAGRAPH_ALIGNMENT_FAR);

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
    // Bloom shader source code
    //
    ////////////////////////////////////////////////////////////////////////////////

    static const char* s_kszQuadVertexShaderSource = R"(
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

    static const char* s_kszBlurHorizontalShaderSource = R"(
            Texture2D inputTexture : register(t0);
            SamplerState samplerState : register(s0);

            struct PSInput
            {
                float4 position : SV_POSITION;
                float2 uv : TEXCOORD;
            };

            float4 main(PSInput input) : SV_TARGET
            {
                // Get texture dimensions dynamically
                uint width, height;
                inputTexture.GetDimensions(width, height);
                float2 texelSize = float2(1.0 / width, 1.0 / height);
                
                float4 color = float4(0, 0, 0, 0);
                
                // 13-tap Gaussian blur for wider spread (horizontal)
                float weights[13] = { 0.02, 0.04, 0.06, 0.08, 0.10, 0.12, 0.16, 0.12, 0.10, 0.08, 0.06, 0.04, 0.02 };
                for (int i = -6; i <= 6; i++)
                {
                    float2 offset = float2(i * texelSize.x, 0);
                    color += inputTexture.Sample(samplerState, input.uv + offset) * weights[i + 6];
                }
                
                return color;
            }
        )";

    static const char* s_kszBlurVerticalShaderSource = R"(
            Texture2D inputTexture : register(t0);
            SamplerState samplerState : register(s0);

            struct PSInput
            {
                float4 position : SV_POSITION;
                float2 uv : TEXCOORD;
            };

            float4 main(PSInput input) : SV_TARGET
            {
                // Get texture dimensions dynamically
                uint width, height;
                inputTexture.GetDimensions(width, height);
                float2 texelSize = float2(1.0 / width, 1.0 / height);
                
                float4 color = float4(0, 0, 0, 0);
                
                // 13-tap Gaussian blur for wider spread (vertical)
                float weights[13] = { 0.02, 0.04, 0.06, 0.08, 0.10, 0.12, 0.16, 0.12, 0.10, 0.08, 0.06, 0.04, 0.02 };
                for (int i = -6; i <= 6; i++)
                {
                    float2 offset = float2(0, i * texelSize.y);
                    color += inputTexture.Sample(samplerState, input.uv + offset) * weights[i + 6];
                }
                
                return color;
            }
        )";

    static const char* s_kszBloomExtractShaderSource = R"(
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
                
                // Extract only bright pixels (luminance threshold)
                // Calculate luminance
                float luminance = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));
                
                // Higher threshold to only extract very bright pixels
                float threshold = 0.6;
                
                // Smooth falloff near threshold for gradual bloom
                float bloomAmount = smoothstep(threshold, threshold + 0.2, luminance);
                
                // Return the bright color multiplied by bloom amount
                // This extracts only bright areas and blacks out dim areas
                return float4(color.rgb * bloomAmount, 1.0);
            }
        )";

    static const char* s_kszBloomCompositeShaderSource = R"(
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
                
                // Additive blend with increased bloom intensity for stronger glow
                return scene + bloom * 2.5;
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
        HRESULT                          hr = S_OK;
        Microsoft::WRL::ComPtr<ID3DBlob> quadVSBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> extractPSBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> blurHPSBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> blurVPSBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> compositePSBlob;
        struct QuadVertex
        {
            float pos[3];
            float uv[2];
        };
        QuadVertex             quadVertices[] = {
            { {-1, -1, 0}, {0, 1} },
            { {-1,  1, 0}, {0, 0} },
            { { 1,  1, 0}, {1, 0} },
            { {-1, -1, 0}, {0, 1} },
            { { 1,  1, 0}, {1, 0} },
            { { 1, -1, 0}, {1, 1} }
        };
        D3D11_BUFFER_DESC      vbDesc = {};
        D3D11_SUBRESOURCE_DATA vbData = {};
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
        HRESULT              hr         = S_OK;
        UINT                 bloomWidth;
        UINT                 bloomHeight;
        D3D11_TEXTURE2D_DESC sceneTexDesc = {};
        D3D11_TEXTURE2D_DESC texDesc      = {};


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

        // Compile bloom shaders and create fullscreen quad resources
        hr = CompileBloomShaders();
        CHR (hr);

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
        m_context->OMSetRenderTargets   (1, &pRenderTarget, nullptr);
        m_context->PSSetShader          (pPixelShader, nullptr, 0);
        m_context->PSSetShaderResources (0, numResources, ppShaderResources);
        m_context->Draw (6, 0);
        
        // Unbind shader resources to avoid hazards
        ID3D11ShaderResourceView* nullSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {};
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
        HRESULT                    hr      = S_OK;
        ID3D11ShaderResourceView * srvs[2];


        // Safety check - if bloom resources failed to create, skip
        CBREx (m_sceneTexture && m_bloomTexture && m_bloomExtractPS && m_compositePS, E_UNEXPECTED);
        
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
        
        // Horizontal blur pass (bloom → temp)
        RenderFullscreenPass (m_blurTempRTV.Get(), m_blurHorizontalPS.Get(), m_bloomSRV.GetAddressOf(), 1);
        
        // Vertical blur pass (temp → bloom)
        RenderFullscreenPass (m_bloomRTV.Get(), m_blurVerticalPS.Get(), m_blurTempSRV.GetAddressOf(), 1);
        
        // Restore full viewport
        SetViewport (m_renderWidth, m_renderHeight);
        
        // Composite back to backbuffer
        m_context->OMSetRenderTargets (1, m_renderTargetView.GetAddressOf(), nullptr);
        
        // Disable blending for composite (we want to replace, not blend)
        m_context->OMSetBlendState (nullptr, nullptr, 0xffffffff);
        
        srvs[0] = m_sceneSRV.Get();
        srvs[1] = m_bloomSRV.Get();
        RenderFullscreenPass (m_renderTargetView.Get(), m_compositePS.Get(), srvs, 2);
        
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
        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
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
        data.scale      = character.scale;
    }





    HRESULT RenderSystem::UpdateInstanceBuffer (const AnimationSystem& animationSystem, ColorScheme colorScheme)
    {
        HRESULT                  hr = S_OK;
        Color4                   schemeColor = GetColorRGB (colorScheme);
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
            const auto& characters = streak->GetCharacters();
            Vector3 streakPos = streak->GetPosition();

            // Render all characters (they manage their own fading/removal)
            for (const auto& character : characters)
            {
                CharacterInstanceData data;
                BuildCharacterInstanceData (character, streakPos, schemeColor, data);
                m_instanceData.push_back (data);
            }
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





    void RenderSystem::Render (const AnimationSystem& animationSystem, const Viewport& viewport, ColorScheme colorScheme, float fps, int rainPercentage, int streakCount, bool showDebugFadeTimes)
    {
        if (!m_device || !m_context)
        {
            return;
        }

        // Clear render target
        ClearRenderTarget();

        // Update constant buffer with projection matrix
        const Matrix4x4& projection = viewport.GetProjectionMatrix();
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT hr = m_context->Map (m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (SUCCEEDED (hr))
        {
            ConstantBufferData* cbData = reinterpret_cast<ConstantBufferData*> (mappedResource.pData);
            
            // Copy projection matrix (4x4 = 16 floats)
            memcpy (cbData->projection, &projection.m[0][0], sizeof (float) * 16);
            
            m_context->Unmap (m_constantBuffer.Get(), 0);
        }

        // Update instance buffer with character data
        (void)UpdateInstanceBuffer (animationSystem, colorScheme);  // Ignore return - errors already handled within

        // Count total characters to render
        size_t totalCharacters = 0;
        for (const auto& streak : animationSystem.GetStreaks())
        {
            totalCharacters += streak.GetLength();
        }

        if (totalCharacters == 0)
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
        ID3D11ShaderResourceView* srv = static_cast<ID3D11ShaderResourceView*> (charSet.GetTextureResourceView());
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
            m_context->DrawInstanced (6, static_cast<UINT> (totalCharacters), 0, 0);
            
            // Now apply bloom (which will composite to backbuffer)
            (void)ApplyBloom();  // Ignore return - errors already handled within
        }
        else
        {
            // Fallback: render directly to backbuffer if bloom not available
            m_context->OMSetRenderTargets (1, m_renderTargetView.GetAddressOf(), nullptr);
            m_context->DrawInstanced (6, static_cast<UINT> (totalCharacters), 0, 0);
        }

        // Render FPS counter overlay if fps > 0
        if (fps > 0.0f)
        {
            RenderFPSCounter (fps, rainPercentage, streakCount);
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





    void RenderSystem::RenderFPSCounter (float fps, int rainPercentage, int streakCount)
    {
        if (!m_d2dContext || !m_fpsBrush || !m_fpsTextFormat || !m_fpsGlowBrush)
        {
            return;
        }

        // Begin D2D rendering
        m_d2dContext->BeginDraw();

        // Format FPS text with rain density info: "Rain xxx% (yyy), zz FPS"
        wchar_t fpsText[64];
        swprintf_s (fpsText, L"Rain %d%% (%d), %.0f FPS", rainPercentage, streakCount, fps);

        // Get render target size for positioning
        D2D1_SIZE_F size = m_d2dContext->GetSize();

        // Create text layout rect (bottom-right corner with 10px padding)
        D2D1_RECT_F textRect = D2D1::RectF (
            size.width - 300.0f,    // Left (300px from right edge for longer text)
            size.height - 40.0f,    // Top (40px from bottom)
            size.width - 10.0f,     // Right (10px padding)
            size.height - 10.0f     // Bottom (10px padding)
        );

        // Draw feathered glow effect by rendering text multiple times with offsets and decreasing opacity
        // This creates a letter-shaped shadow that fades from opaque to transparent
        const int glowLayers = 10;
        for (int i = glowLayers; i > 0; --i)
        {
            float offset  = static_cast<float> (i);
            float opacity = 1.0f - (static_cast<float> (i) / static_cast<float> (glowLayers));

            m_fpsGlowBrush->SetColor (D2D1::ColorF (D2D1::ColorF::Black, opacity));

            // Draw text at 8 different offsets to create uniform glow
            for (int dx = -1; dx <= 1; ++dx)
            {
                for (int dy = -1; dy <= 1; ++dy)
                {
                    if (dx == 0 && dy == 0)
                    {
                        continue;
                    }

                    D2D1_RECT_F glowRect = D2D1::RectF (
                        textRect.left + offset * dx,
                        textRect.top + offset * dy,
                        textRect.right + offset * dx,
                        textRect.bottom + offset * dy
                    );

                    m_d2dContext->DrawText (
                        fpsText,
                        static_cast<UINT32> (wcslen (fpsText)),
                        m_fpsTextFormat.Get(),
                        glowRect,
                        m_fpsGlowBrush.Get()
                    );
                }
            }
        }

        // Draw text
        m_d2dContext->DrawText (
            fpsText,
            static_cast<UINT32> (wcslen (fpsText)),
            m_fpsTextFormat.Get(),
            textRect,
            m_fpsBrush.Get()
        );

        // End D2D rendering
        m_d2dContext->EndDraw();
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
        m_fullscreenQuadVS.Reset();
        m_fullscreenQuadVB.Reset();
        m_compositePS.Reset();
        m_blurVerticalPS.Reset();
        m_blurHorizontalPS.Reset();
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
        m_atlasTextureSRV.Reset();
        m_samplerState.Reset();
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
        HRESULT                                  hr = S_OK;
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





    void RenderSystem::Shutdown()
    {
        ReleaseDirect2DResources();
        ReleaseBloomResources();
        ReleaseDirectXResources();
    }





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





    bool RenderSystem::SetFullscreen()
    {
        if (!m_swapChain)
        {
            return false;
        }

        HRESULT hr = m_swapChain->SetFullscreenState (TRUE, nullptr);
        return SUCCEEDED (hr);
    }





    bool RenderSystem::SetWindowed()
    {
        if (!m_swapChain)
        {
            return false;
        }

        HRESULT hr = m_swapChain->SetFullscreenState (FALSE, nullptr);
        return SUCCEEDED (hr);
    }
}
