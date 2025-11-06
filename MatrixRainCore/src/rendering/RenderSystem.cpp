#include "pch.h"
#include "matrixrain/RenderSystem.h"
#include "matrixrain/CharacterSet.h"
#include <d3dcompiler.h>
#include <algorithm>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace MatrixRain
{
    RenderSystem::RenderSystem()
        : m_instanceBufferCapacity(INITIAL_INSTANCE_CAPACITY)
    {
    }

    RenderSystem::~RenderSystem()
    {
        Shutdown();
    }

    bool RenderSystem::Initialize(HWND hwnd, UINT width, UINT height)
    {
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
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.OutputWindow = hwnd;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Windowed = TRUE;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        hr = dxgiFactory->CreateSwapChain(m_device.Get(), &swapChainDesc, &m_swapChain);
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
                
                // Scale and position the quad
                float2 charSize = float2(20.0, 30.0) * input.scale; // Character size in pixels
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

    void RenderSystem::UpdateInstanceBuffer(const AnimationSystem& animationSystem)
    {
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

                // Color and brightness
                data.color[0] = character.color.r;
                data.color[1] = character.color.g;
                data.color[2] = character.color.b;
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

    void RenderSystem::Render(const AnimationSystem& animationSystem, const Viewport& viewport)
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
        UpdateInstanceBuffer(animationSystem);

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
        m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);

        // Draw instanced (6 vertices per quad, one instance per character)
        m_context->DrawInstanced(6, static_cast<UINT>(totalCharacters), 0, 0);
    }

    void RenderSystem::Present()
    {
        if (m_swapChain)
        {
            m_swapChain->Present(1, 0); // VSync enabled
        }
    }

    void RenderSystem::Resize(UINT width, UINT height)
    {
        if (!m_swapChain)
        {
            return;
        }

        // Release render target view
        m_renderTargetView.Reset();

        // Resize swap chain buffers
        m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);

        // Recreate render target view
        CreateRenderTargetView();

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
}
