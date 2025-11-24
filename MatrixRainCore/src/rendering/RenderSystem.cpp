#include "pch.h"

#include "MatrixRain/RenderSystem.h"

#include "MatrixRain/CharacterSet.h"
#include "MatrixRain/ColorScheme.h"





namespace MatrixRain
{
    RenderSystem::RenderSystem()
        : m_instanceBufferCapacity(INITIAL_INSTANCE_CAPACITY)
        , m_renderWidth(0)
        , m_renderHeight(0)
    {
    }

    RenderSystem::~RenderSystem()
    {
        Shutdown();
    }

    bool RenderSystem::Initialize(HWND hwnd, UINT width, UINT height)
    {
        // Store dimensions for viewport and render target sizing
        m_renderWidth = width;
        m_renderHeight = height;
        
        if (!CreateDevice())
        {
            return false;
        }

        if (!CreateSwapChain(hwnd, width, height))
        {
            return false;
        }

        if (!CreateRenderTargetView())
        {
            return false;
        }

        if (!CompileShaders())
        {
            return false;
        }

        if (!CreateInstanceBuffer())
        {
            return false;
        }

        if (!CreateConstantBuffer())
        {
            return false;
        }

        if (!CreateBlendState())
        {
            return false;
        }

        if (!CreateSamplerState())
        {
            return false;
        }

        if (!CreateDirect2DResources())
        {
            return false;
        }

        if (!CreateBloomResources(width, height))
        {
            return false;
        }

        // Set viewport
        D3D11_VIEWPORT viewport = {};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = static_cast<FLOAT>(width);
        viewport.Height = static_cast<FLOAT>(height);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        m_context->RSSetViewports(1, &viewport);

        return true;
    }

    bool RenderSystem::CreateDevice()
    {
        UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT; // Required for Direct2D interop
#ifdef _DEBUG
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
        };

        D3D_FEATURE_LEVEL featureLevel;

        HRESULT hr = D3D11CreateDevice(
            nullptr,                    // Default adapter
            D3D_DRIVER_TYPE_HARDWARE,   // Hardware acceleration
            nullptr,                    // No software rasterizer
            createDeviceFlags,
            featureLevels,
            _countof(featureLevels),
            D3D11_SDK_VERSION,
            &m_device,
            &featureLevel,
            &m_context
        );

        return SUCCEEDED(hr);
    }

    bool RenderSystem::CreateSwapChain(HWND hwnd, UINT width, UINT height)
    {
        // Get DXGI factory from device
        Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
        HRESULT hr = m_device.As(&dxgiDevice);
        if (FAILED(hr)) return false;

        Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
        hr = dxgiDevice->GetAdapter(&dxgiAdapter);
        if (FAILED(hr)) return false;

        Microsoft::WRL::ComPtr<IDXGIFactory> dxgiFactory;
        hr = dxgiAdapter->GetParent(__uuidof(IDXGIFactory), &dxgiFactory);
        if (FAILED(hr)) return false;

        // Create swap chain description
        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        swapChainDesc.BufferCount = 2;
        swapChainDesc.BufferDesc.Width = width;
        swapChainDesc.BufferDesc.Height = height;
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // Match D2D format requirement
        swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.OutputWindow = hwnd;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Windowed = TRUE;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        hr = dxgiFactory->CreateSwapChain(m_device.Get(), &swapChainDesc, &m_swapChain);
        if (FAILED(hr)) return false;

        // Disable DXGI's built-in Alt+Enter exclusive fullscreen toggle
        // We handle fullscreen transitions manually for borderless windowed mode
        hr = dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
        
        return SUCCEEDED(hr);
    }

    bool RenderSystem::CreateRenderTargetView()
    {
        Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
        HRESULT hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer);
        if (FAILED(hr)) return false;

        hr = m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_renderTargetView);
        return SUCCEEDED(hr);
    }

    bool RenderSystem::CompileShaders()
    {
        // Simple vertex shader for instanced quads
        const char* vsSource = R"(
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

        // Simple pixel shader with texture sampling and glow
        const char* psSource = R"(
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

        // Compile vertex shader
        Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        
        HRESULT hr = D3DCompile(
            vsSource, strlen(vsSource),
            "VS", nullptr, nullptr,
            "main", "vs_5_0",
            D3DCOMPILE_ENABLE_STRICTNESS, 0,
            &vsBlob, &errorBlob
        );

        if (FAILED(hr))
        {
            if (errorBlob)
            {
                OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            }
            return false;
        }

        hr = m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vertexShader);
        if (FAILED(hr)) return false;

        // Create input layout
        D3D11_INPUT_ELEMENT_DESC inputLayout[] = {
            { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "TEXCOORD",    1, DXGI_FORMAT_R32G32_FLOAT,    0, 20, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "COLOR",       0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 28, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "BRIGHTNESS",  0, DXGI_FORMAT_R32_FLOAT,       0, 44, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "SCALE",       0, DXGI_FORMAT_R32_FLOAT,       0, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        };

        hr = m_device->CreateInputLayout(inputLayout, _countof(inputLayout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);
        if (FAILED(hr)) return false;

        // Compile pixel shader
        Microsoft::WRL::ComPtr<ID3DBlob> psBlob;
        hr = D3DCompile(
            psSource, strlen(psSource),
            "PS", nullptr, nullptr,
            "main", "ps_5_0",
            D3DCOMPILE_ENABLE_STRICTNESS, 0,
            &psBlob, &errorBlob
        );

        if (FAILED(hr))
        {
            if (errorBlob)
            {
                OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            }
            return false;
        }

        hr = m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pixelShader);
        return SUCCEEDED(hr);
    }

    bool RenderSystem::CreateInstanceBuffer()
    {
        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.ByteWidth = sizeof(CharacterInstanceData) * m_instanceBufferCapacity;
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        HRESULT hr = m_device->CreateBuffer(&bufferDesc, nullptr, &m_instanceBuffer);
        return SUCCEEDED(hr);
    }

    bool RenderSystem::CreateConstantBuffer()
    {
        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.ByteWidth = sizeof(ConstantBufferData);
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        HRESULT hr = m_device->CreateBuffer(&bufferDesc, nullptr, &m_constantBuffer);
        return SUCCEEDED(hr);
    }

    bool RenderSystem::CreateBlendState()
    {
        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.AlphaToCoverageEnable = FALSE;
        blendDesc.IndependentBlendEnable = FALSE;
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        HRESULT hr = m_device->CreateBlendState(&blendDesc, &m_blendState);
        return SUCCEEDED(hr);
    }

    bool RenderSystem::CreateSamplerState()
    {
        D3D11_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        samplerDesc.MinLOD = 0;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

        HRESULT hr = m_device->CreateSamplerState(&samplerDesc, &m_samplerState);
        return SUCCEEDED(hr);
    }

    bool RenderSystem::CreateDirect2DResources()
    {
        HRESULT hr;

        // Create D2D factory
        D2D1_FACTORY_OPTIONS options = {};
#ifdef _DEBUG
        options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
        hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &options,
            reinterpret_cast<void**>(m_d2dFactory.GetAddressOf()));
        if (FAILED(hr))
        {
            return false;
        }

        // Get DXGI device
        Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
        hr = m_device.As(&dxgiDevice);
        if (FAILED(hr))
        {
            return false;
        }

        // Create D2D device
        hr = m_d2dFactory->CreateDevice(dxgiDevice.Get(), &m_d2dDevice);
        if (FAILED(hr))
        {
            return false;
        }

        // Create D2D device context
        hr = m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_d2dContext);
        if (FAILED(hr))
        {
            return false;
        }

        // Get back buffer as DXGI surface
        Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
        hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer);
        if (FAILED(hr))
        {
            return false;
        }

        Microsoft::WRL::ComPtr<IDXGISurface> dxgiSurface;
        hr = backBuffer.As(&dxgiSurface);
        if (FAILED(hr))
        {
            return false;
        }

        // Create D2D bitmap from DXGI surface
        D2D1_BITMAP_PROPERTIES1 bitmapProps = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
        );

        hr = m_d2dContext->CreateBitmapFromDxgiSurface(dxgiSurface.Get(), &bitmapProps, &m_d2dBitmap);
        if (FAILED(hr))
        {
            return false;
        }

        // Set the bitmap as the render target
        m_d2dContext->SetTarget(m_d2dBitmap.Get());

        // Create DirectWrite factory
        hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(m_dwriteFactory.GetAddressOf()));
        if (FAILED(hr))
        {
            return false;
        }

        // Create text format for FPS display (small, top-right aligned)
        hr = m_dwriteFactory->CreateTextFormat(
            L"Consolas",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            20.0f,
            L"en-us",
            &m_fpsTextFormat
        );
        if (FAILED(hr))
        {
            return false;
        }

        m_fpsTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
        m_fpsTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);

        // Create white brush for FPS text
        hr = m_d2dContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_fpsBrush);
        if (FAILED(hr))
        {
            return false;
        }

        return true;
    }




    bool RenderSystem::CreateBloomResources(UINT width, UINT height)
    {
        HRESULT hr;

        // Safety check - don't create bloom resources with invalid dimensions
        if (width == 0 || height == 0)
        {
            OutputDebugStringA("CreateBloomResources: Invalid dimensions (0x0), skipping bloom\n");
            return true; // Return true to not fail initialization, just skip bloom
        }

        OutputDebugStringA("CreateBloomResources: Starting...\n");

        // Create scene render target (full resolution)
        D3D11_TEXTURE2D_DESC sceneTexDesc = {};
        sceneTexDesc.Width = width;
        sceneTexDesc.Height = height;
        sceneTexDesc.MipLevels = 1;
        sceneTexDesc.ArraySize = 1;
        sceneTexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sceneTexDesc.SampleDesc.Count = 1;
        sceneTexDesc.Usage = D3D11_USAGE_DEFAULT;
        sceneTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        hr = m_device->CreateTexture2D(&sceneTexDesc, nullptr, &m_sceneTexture);
        if (FAILED(hr)) { OutputDebugStringA("FAILED: CreateTexture2D for scene\n"); return false; }

        hr = m_device->CreateRenderTargetView(m_sceneTexture.Get(), nullptr, &m_sceneRTV);
        if (FAILED(hr)) { OutputDebugStringA("FAILED: CreateRenderTargetView for scene\n"); return false; }

        hr = m_device->CreateShaderResourceView(m_sceneTexture.Get(), nullptr, &m_sceneSRV);
        if (FAILED(hr)) { OutputDebugStringA("FAILED: CreateShaderResourceView for scene\n"); return false; }

        // Create bloom extraction texture (half resolution for performance)
        UINT bloomWidth = width / 2;
        UINT bloomHeight = height / 2;

        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = bloomWidth;
        texDesc.Height = bloomHeight;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        hr = m_device->CreateTexture2D(&texDesc, nullptr, &m_bloomTexture);
        if (FAILED(hr)) return false;

        hr = m_device->CreateRenderTargetView(m_bloomTexture.Get(), nullptr, &m_bloomRTV);
        if (FAILED(hr)) return false;

        hr = m_device->CreateShaderResourceView(m_bloomTexture.Get(), nullptr, &m_bloomSRV);
        if (FAILED(hr)) return false;

        // Create temporary blur texture (same size as bloom)
        hr = m_device->CreateTexture2D(&texDesc, nullptr, &m_blurTempTexture);
        if (FAILED(hr)) return false;

        hr = m_device->CreateRenderTargetView(m_blurTempTexture.Get(), nullptr, &m_blurTempRTV);
        if (FAILED(hr)) return false;

        hr = m_device->CreateShaderResourceView(m_blurTempTexture.Get(), nullptr, &m_blurTempSRV);
        if (FAILED(hr)) return false;

        // Shader compilation resources
        Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> psBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

        // Compile fullscreen quad vertex shader
        const char* quadVSSource = R"(
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

        
        hr = D3DCompile(quadVSSource, strlen(quadVSSource), "QuadVS", nullptr, nullptr,
            "main", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &vsBlob, &errorBlob);
        if (FAILED(hr))
        {
            if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            return false;
        }
        hr = m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_fullscreenQuadVS);
        if (FAILED(hr)) return false;

        // Create input layout for fullscreen quad
        D3D11_INPUT_ELEMENT_DESC quadLayoutDesc[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };
        hr = m_device->CreateInputLayout(quadLayoutDesc, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_fullscreenQuadInputLayout);
        if (FAILED(hr)) return false;

        // Compile horizontal blur shader
        const char* blurHSource = R"(
            Texture2D inputTexture : register(t0);
            SamplerState samplerState : register(s0);

            struct PSInput
            {
                float4 position : SV_POSITION;
                float2 uv : TEXCOORD;
            };

            float4 main(PSInput input) : SV_TARGET
            {
                float2 texelSize = float2(1.0 / 960.0, 1.0 / 540.0);
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

        // Compile vertical blur shader
        const char* blurVSource = R"(
            Texture2D inputTexture : register(t0);
            SamplerState samplerState : register(s0);

            struct PSInput
            {
                float4 position : SV_POSITION;
                float2 uv : TEXCOORD;
            };

            float4 main(PSInput input) : SV_TARGET
            {
                float2 texelSize = float2(1.0 / 960.0, 1.0 / 540.0);
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

        // Compile bloom extraction shader (extract bright pixels only)
        const char* extractSource = R"(
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

        // Compile composite shader
        const char* compositeSource = R"(
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
                
                // Additive blend with moderate bloom intensity
                return scene + bloom * 1.5;
            }
        )";

        // Compile bloom extraction shader
        psBlob.Reset();
        errorBlob.Reset();
        hr = D3DCompile(extractSource, strlen(extractSource), "Extract", nullptr, nullptr,
            "main", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &psBlob, &errorBlob);
        if (FAILED(hr))
        {
            if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            return false;
        }
        hr = m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_bloomExtractPS);
        if (FAILED(hr)) return false;

        // Compile horizontal blur
        psBlob.Reset();
        errorBlob.Reset();
        hr = D3DCompile(blurHSource, strlen(blurHSource), "BlurH", nullptr, nullptr,
            "main", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &psBlob, &errorBlob);
        if (FAILED(hr))
        {
            if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            return false;
        }
        hr = m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_blurHorizontalPS);
        if (FAILED(hr)) return false;

        // Compile vertical blur
        psBlob.Reset();
        errorBlob.Reset();
        hr = D3DCompile(blurVSource, strlen(blurVSource), "BlurV", nullptr, nullptr,
            "main", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &psBlob, &errorBlob);
        if (FAILED(hr))
        {
            if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            return false;
        }
        hr = m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_blurVerticalPS);
        if (FAILED(hr)) return false;

        // Compile composite shader
        psBlob.Reset();
        errorBlob.Reset();
        hr = D3DCompile(compositeSource, strlen(compositeSource), "Composite", nullptr, nullptr,
            "main", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &psBlob, &errorBlob);
        if (FAILED(hr))
        {
            if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            return false;
        }
        hr = m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_compositePS);
        if (FAILED(hr)) return false;

        // Create fullscreen quad vertex buffer
        struct QuadVertex
        {
            float pos[3];
            float uv[2];
        };

        QuadVertex quadVertices[] = {
            { {-1, -1, 0}, {0, 1} },
            { {-1,  1, 0}, {0, 0} },
            { { 1,  1, 0}, {1, 0} },
            { {-1, -1, 0}, {0, 1} },
            { { 1,  1, 0}, {1, 0} },
            { { 1, -1, 0}, {1, 1} }
        };

        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.ByteWidth = sizeof(quadVertices);
        vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vbData = {};
        vbData.pSysMem = quadVertices;

        hr = m_device->CreateBuffer(&vbDesc, &vbData, &m_fullscreenQuadVB);
        return SUCCEEDED(hr);
    }




    void RenderSystem::ApplyBloom()
    {
        // Safety check - if bloom resources failed to create, skip
        if (!m_sceneTexture || !m_bloomTexture || !m_bloomExtractPS || !m_compositePS)
        {
            OutputDebugStringA("ApplyBloom: Missing resources, skipping\n");
            return;
        }
        
        OutputDebugStringA("ApplyBloom: Running...\n");
        
        // Scene texture already has the rendered characters - no need to copy from backbuffer
        
        // Set viewport for half-resolution processing
        D3D11_VIEWPORT halfViewport = {};
        halfViewport.Width = static_cast<float>(m_renderWidth / 2);
        halfViewport.Height = static_cast<float>(m_renderHeight / 2);
        halfViewport.MinDepth = 0.0f;
        halfViewport.MaxDepth = 1.0f;
        m_context->RSSetViewports(1, &halfViewport);
        
        // EXTRACTION PASS: Extract only bright pixels from scene to bloom texture
        m_context->OMSetRenderTargets(1, m_bloomRTV.GetAddressOf(), nullptr);
        m_context->IASetInputLayout(m_fullscreenQuadInputLayout.Get());
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        UINT stride = sizeof(float) * 5;
        UINT offset = 0;
        m_context->IASetVertexBuffers(0, 1, m_fullscreenQuadVB.GetAddressOf(), &stride, &offset);
        m_context->VSSetShader(m_fullscreenQuadVS.Get(), nullptr, 0);
        m_context->VSSetConstantBuffers(0, 0, nullptr);
        m_context->PSSetShader(m_bloomExtractPS.Get(), nullptr, 0);
        m_context->PSSetShaderResources(0, 1, m_sceneSRV.GetAddressOf());
        m_context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());
        m_context->Draw(6, 0);
        
        ID3D11ShaderResourceView* nullSRV = nullptr;
        m_context->PSSetShaderResources(0, 1, &nullSRV);
        
        // Horizontal blur pass (bloom → temp)
        m_context->OMSetRenderTargets(1, m_blurTempRTV.GetAddressOf(), nullptr);
        m_context->PSSetShader(m_blurHorizontalPS.Get(), nullptr, 0);
        m_context->PSSetShaderResources(0, 1, m_bloomSRV.GetAddressOf());
        m_context->Draw(6, 0);
        
        m_context->PSSetShaderResources(0, 1, &nullSRV);
        
        // Vertical blur pass (temp → bloom)
        m_context->OMSetRenderTargets(1, m_bloomRTV.GetAddressOf(), nullptr);
        m_context->PSSetShader(m_blurVerticalPS.Get(), nullptr, 0);
        m_context->PSSetShaderResources(0, 1, m_blurTempSRV.GetAddressOf());
        m_context->Draw(6, 0);
        
        m_context->PSSetShaderResources(0, 1, &nullSRV);
        
        // Restore full viewport
        D3D11_VIEWPORT fullViewport = {};
        fullViewport.Width = static_cast<float>(m_renderWidth);
        fullViewport.Height = static_cast<float>(m_renderHeight);
        fullViewport.MinDepth = 0.0f;
        fullViewport.MaxDepth = 1.0f;
        m_context->RSSetViewports(1, &fullViewport);
        
        // Composite back to backbuffer
        m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);
        
        // Disable blending for composite (we want to replace, not blend)
        m_context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
        
        m_context->PSSetShader(m_compositePS.Get(), nullptr, 0);
        ID3D11ShaderResourceView* srvs[2] = { m_sceneSRV.Get(), m_bloomSRV.Get() };
        m_context->PSSetShaderResources(0, 2, srvs);
        m_context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());
        m_context->Draw(6, 0);
        
        OutputDebugStringA("ApplyBloom: Composite pass done\n");
        
        // Cleanup
        ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
        m_context->PSSetShaderResources(0, 2, nullSRVs);
        
        // Restore render state
        m_context->IASetInputLayout(m_inputLayout.Get());
        m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
        m_context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
        m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    }




    void RenderSystem::ClearRenderTarget()
    {
        // Clear to black
        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        m_context->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);
    }

    void RenderSystem::SortStreaksByDepth(std::vector<const CharacterStreak*>& streaks)
    {
        // Sort back-to-front (far to near) for proper alpha blending
        std::stable_sort(streaks.begin(), streaks.end(),
            [](const CharacterStreak* a, const CharacterStreak* b)
            {
                return a->GetPosition().z > b->GetPosition().z;
            });
    }

    void RenderSystem::UpdateInstanceBuffer(const AnimationSystem& animationSystem, ColorScheme colorScheme)
    {
        // Get color scheme RGB values
        Color4 schemeColor = GetColorRGB(colorScheme);

        // Collect all character instances from all streaks
        std::vector<CharacterInstanceData> instanceData;
        
        // Get streaks sorted by depth
        std::vector<const CharacterStreak*> streakPtrs;
        for (const auto& streak : animationSystem.GetStreaks())
        {
            streakPtrs.push_back(&streak);
        }
        SortStreaksByDepth(streakPtrs);

        CharacterSet& charSet = CharacterSet::GetInstance();

        // Build instance data
        for (const CharacterStreak* streak : streakPtrs)
        {
            const auto& characters = streak->GetCharacters();
            Vector3 streakPos = streak->GetPosition();

            // Render all characters (they manage their own fading/removal)
            for (const auto& character : characters)
            {
                CharacterInstanceData data;
                
                // Position - character stores its absolute Y, streak provides X and Z
                data.position[0] = streakPos.x + character.positionOffset.x;
                data.position[1] = character.positionOffset.y; // Absolute Y position
                data.position[2] = streakPos.z;

                // Get UV coordinates from glyph
                const GlyphInfo& glyph = charSet.GetGlyph(character.glyphIndex);
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
                
                data.color[3] = character.color.a;
                data.brightness = character.brightness;
                data.scale = character.scale;

                instanceData.push_back(data);
            }
        }

        if (instanceData.empty())
        {
            return;
        }

        // Map instance buffer and upload data
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT hr = m_context->Map(m_instanceBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (SUCCEEDED(hr))
        {
            size_t bytesToCopy = sizeof(CharacterInstanceData) * std::min(instanceData.size(), static_cast<size_t>(m_instanceBufferCapacity));
            memcpy(mappedResource.pData, instanceData.data(), bytesToCopy);
            m_context->Unmap(m_instanceBuffer.Get(), 0);
        }
    }

    void RenderSystem::Render(const AnimationSystem& animationSystem, const Viewport& viewport, ColorScheme colorScheme, float fps)
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
        HRESULT hr = m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (SUCCEEDED(hr))
        {
            ConstantBufferData* cbData = reinterpret_cast<ConstantBufferData*>(mappedResource.pData);
            
            // Copy projection matrix (4x4 = 16 floats)
            memcpy(cbData->projection, &projection.m[0][0], sizeof(float) * 16);
            
            m_context->Unmap(m_constantBuffer.Get(), 0);
        }

        // Update instance buffer with character data
        UpdateInstanceBuffer(animationSystem, colorScheme);

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
        vp.Width = static_cast<FLOAT>(viewport.GetWidth());
        vp.Height = static_cast<FLOAT>(viewport.GetHeight());
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        m_context->RSSetViewports(1, &vp);

        // Set render state
        m_context->IASetInputLayout(m_inputLayout.Get());
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        UINT stride = sizeof(CharacterInstanceData);
        UINT offset = 0;
        m_context->IASetVertexBuffers(0, 1, m_instanceBuffer.GetAddressOf(), &stride, &offset);

        m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
        m_context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

        m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
        m_context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());
        
        // Bind texture atlas from CharacterSet
        CharacterSet& charSet = CharacterSet::GetInstance();
        ID3D11ShaderResourceView* srv = static_cast<ID3D11ShaderResourceView*>(charSet.GetTextureResourceView());
        if (srv)
        {
            m_context->PSSetShaderResources(0, 1, &srv);
        }

        float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        m_context->OMSetBlendState(m_blendState.Get(), blendFactor, 0xffffffff);
        
        // Set full viewport for rendering
        D3D11_VIEWPORT fullViewport = {};
        fullViewport.Width = static_cast<float>(m_renderWidth);
        fullViewport.Height = static_cast<float>(m_renderHeight);
        fullViewport.MinDepth = 0.0f;
        fullViewport.MaxDepth = 1.0f;
        m_context->RSSetViewports(1, &fullViewport);
        
        // Render characters to scene texture (NOT directly to backbuffer)
        if (m_sceneRTV)
        {
            // Clear scene texture
            float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            m_context->ClearRenderTargetView(m_sceneRTV.Get(), clearColor);
            
            m_context->OMSetRenderTargets(1, m_sceneRTV.GetAddressOf(), nullptr);
            m_context->DrawInstanced(6, static_cast<UINT>(totalCharacters), 0, 0);
            
            // Now apply bloom (which will composite to backbuffer)
            ApplyBloom();
        }
        else
        {
            // Fallback: render directly to backbuffer if bloom not available
            m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);
            m_context->DrawInstanced(6, static_cast<UINT>(totalCharacters), 0, 0);
        }

        // Render FPS counter overlay if fps > 0
        // Render FPS counter overlay if fps > 0
        if (fps > 0.0f)
        {
            RenderFPSCounter(fps);
        }
    }

    void RenderSystem::Present()
    {
        if (m_swapChain)
        {
            m_swapChain->Present(1, 0); // VSync enabled
        }
    }

    void RenderSystem::RenderFPSCounter(float fps)
    {
        if (!m_d2dContext || !m_fpsBrush || !m_fpsTextFormat)
        {
            return;
        }

        // Begin D2D rendering
        m_d2dContext->BeginDraw();

        // Format FPS text
        wchar_t fpsText[32];
        swprintf_s(fpsText, L"FPS: %.1f", fps);

        // Get render target size for positioning
        D2D1_SIZE_F size = m_d2dContext->GetSize();

        // Create text layout rect (bottom-right corner with 10px padding)
        D2D1_RECT_F textRect = D2D1::RectF(
            size.width - 150.0f,    // Left (150px from right edge)
            size.height - 40.0f,    // Top (40px from bottom)
            size.width - 10.0f,     // Right (10px padding)
            size.height - 10.0f     // Bottom (10px padding)
        );

        // Draw text
        m_d2dContext->DrawText(
            fpsText,
            static_cast<UINT32>(wcslen(fpsText)),
            m_fpsTextFormat.Get(),
            textRect,
            m_fpsBrush.Get()
        );

        // End D2D rendering
        m_d2dContext->EndDraw();
    }

    void RenderSystem::Resize(UINT width, UINT height)
    {
        m_renderWidth = width;
        m_renderHeight = height;

        if (!m_swapChain)
        {
            return;
        }

        // Release render target view and D2D bitmap
        m_renderTargetView.Reset();
        m_d2dBitmap.Reset();
        
        // CRITICAL: Clear D2D target before resizing
        if (m_d2dContext)
        {
            m_d2dContext->SetTarget(nullptr);
        }

        // Resize swap chain buffers
        HRESULT hr = m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
        if (FAILED(hr))
        {
            return; // Failed to resize
        }

        // Recreate render target view
        CreateRenderTargetView();

        // Recreate D2D bitmap for the new back buffer
        if (m_d2dContext)
        {
            Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
            HRESULT hrBackBuffer = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer);
            if (SUCCEEDED(hrBackBuffer))
            {
                Microsoft::WRL::ComPtr<IDXGISurface> dxgiSurface;
                hrBackBuffer = backBuffer.As(&dxgiSurface);
                if (SUCCEEDED(hrBackBuffer))
                {
                    D2D1_BITMAP_PROPERTIES1 bitmapProps = D2D1::BitmapProperties1(
                        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
                    );

                    m_d2dContext->CreateBitmapFromDxgiSurface(dxgiSurface.Get(), &bitmapProps, &m_d2dBitmap);
                    m_d2dContext->SetTarget(m_d2dBitmap.Get());
                }
            }
        }

        // Update viewport
        D3D11_VIEWPORT viewport = {};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = static_cast<FLOAT>(width);
        viewport.Height = static_cast<FLOAT>(height);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        m_context->RSSetViewports(1, &viewport);
    }

    void RenderSystem::Shutdown()
    {
        // Direct2D/DirectWrite resources
        m_fpsBrush.Reset();
        m_fpsTextFormat.Reset();
        m_dwriteFactory.Reset();
        m_d2dBitmap.Reset();
        m_d2dContext.Reset();
        m_d2dDevice.Reset();
        m_d2dFactory.Reset();

        // Bloom resources
        m_fullscreenQuadVS.Reset();
        m_fullscreenQuadVB.Reset();
        m_compositePS.Reset();
        m_blurVerticalPS.Reset();
        m_blurHorizontalPS.Reset();
        m_blurTempSRV.Reset();
        m_blurTempRTV.Reset();
        m_blurTempTexture.Reset();
        m_bloomSRV.Reset();
        m_bloomRTV.Reset();
        m_bloomTexture.Reset();
        m_sceneSRV.Reset();
        m_sceneRTV.Reset();
        m_sceneTexture.Reset();

        // DirectX resources
        m_atlasTextureSRV.Reset();
        m_samplerState.Reset();
        m_blendState.Reset();
        m_constantBuffer.Reset();
        m_instanceBuffer.Reset();
        m_inputLayout.Reset();
        m_pixelShader.Reset();
        m_vertexShader.Reset();
        m_renderTargetView.Reset();
        m_swapChain.Reset();
        m_context.Reset();
        m_device.Reset();
    }




    bool RenderSystem::RecreateSwapChain(HWND hwnd, UINT width, UINT height, bool fullscreen)
    {
        // Release render target view and D2D bitmap before recreating swap chain
        m_renderTargetView.Reset();
        m_d2dBitmap.Reset();

        // Release the old swap chain
        m_swapChain.Reset();

        // Create new swap chain with updated settings
        if (!CreateSwapChain(hwnd, width, height))
        {
            return false;
        }

        // Set fullscreen state if requested
        if (fullscreen)
        {
            HRESULT hr = m_swapChain->SetFullscreenState(TRUE, nullptr);
            if (FAILED(hr))
            {
                // Fullscreen transition failed - stay windowed
                return false;
            }
        }

        // Recreate render target view
        if (!CreateRenderTargetView())
        {
            return false;
        }

        // Recreate D2D bitmap for the new back buffer
        if (m_d2dContext)
        {
            Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
            HRESULT hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer);
            if (SUCCEEDED(hr))
            {
                Microsoft::WRL::ComPtr<IDXGISurface> dxgiSurface;
                hr = backBuffer.As(&dxgiSurface);
                if (SUCCEEDED(hr))
                {
                    D2D1_BITMAP_PROPERTIES1 bitmapProps = D2D1::BitmapProperties1(
                        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
                    );

                    m_d2dContext->CreateBitmapFromDxgiSurface(dxgiSurface.Get(), &bitmapProps, &m_d2dBitmap);
                    m_d2dContext->SetTarget(m_d2dBitmap.Get());
                }
            }
        }

        // Resize viewport to match new dimensions
        Resize(width, height);

        return true;
    }




    bool RenderSystem::SetFullscreen()
    {
        if (!m_swapChain)
        {
            return false;
        }

        HRESULT hr = m_swapChain->SetFullscreenState(TRUE, nullptr);
        return SUCCEEDED(hr);
    }




    bool RenderSystem::SetWindowed()
    {
        if (!m_swapChain)
        {
            return false;
        }

        HRESULT hr = m_swapChain->SetFullscreenState(FALSE, nullptr);
        return SUCCEEDED(hr);
    }
}
