#pragma once
#include "matrixrain/AnimationSystem.h"
#include "matrixrain/CharacterSet.h"
#include "matrixrain/Viewport.h"
#include "matrixrain/ColorScheme.h"
#include <d3d11.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include <wrl/client.h>

namespace MatrixRain
{
    /// <summary>
    /// Manages DirectX 11 rendering pipeline for Matrix Rain effect.
    /// Handles device creation, shader compilation, instanced rendering, and presentation.
    /// </summary>
    class RenderSystem
    {
    public:
        RenderSystem();
        ~RenderSystem();

        /// <summary>
        /// Initialize DirectX 11 device, swap chain, and rendering resources.
        /// </summary>
        /// <param name="hwnd">Window handle for swap chain creation</param>
        /// <param name="width">Initial viewport width</param>
        /// <param name="height">Initial viewport height</param>
        /// <returns>True on success, false on failure</returns>
        bool Initialize(HWND hwnd, UINT width, UINT height);

        /// <summary>
        /// Render all character streaks from the animation system.
        /// </summary>
        /// <param name="animationSystem">Source of streak data to render</param>
        /// <param name="viewport">Viewport for projection matrix</param>
        /// <param name="colorScheme">Current color scheme for rendering</param>
        /// <param name="fps">Current FPS for display (0 to hide)</param>
        void Render(const AnimationSystem& animationSystem, const Viewport& viewport, ColorScheme colorScheme = ColorScheme::Green, float fps = 0.0f);

        /// <summary>
        /// Present the rendered frame to the screen.
        /// </summary>
        void Present();

        /// <summary>
        /// Handle window resize by recreating swap chain buffers.
        /// </summary>
        /// <param name="width">New viewport width</param>
        /// <param name="height">New viewport height</param>
        void Resize(UINT width, UINT height);

        /// <summary>
        /// Recreate swap chain for display mode transitions (windowed ↔ fullscreen).
        /// </summary>
        /// <param name="hwnd">Window handle</param>
        /// <param name="width">New width</param>
        /// <param name="height">New height</param>
        /// <param name="fullscreen">True for fullscreen, false for windowed</param>
        /// <returns>True on success, false on failure</returns>
        bool RecreateSwapChain(HWND hwnd, UINT width, UINT height, bool fullscreen);

        /// <summary>
        /// Set swap chain to fullscreen mode.
        /// </summary>
        /// <returns>True on success, false on failure</returns>
        bool SetFullscreen();

        /// <summary>
        /// Set swap chain to windowed mode.
        /// </summary>
        /// <returns>True on success, false on failure</returns>
        bool SetWindowed();

        /// <summary>
        /// Clean up all DirectX resources.
        /// </summary>
        void Shutdown();

        // Accessors
        ID3D11Device* GetDevice() const { return m_device.Get(); }
        ID3D11DeviceContext* GetContext() const { return m_context.Get(); }

    private:
        // Initialization helpers
        bool CreateDevice();
        bool CreateSwapChain(HWND hwnd, UINT width, UINT height);
        bool CreateRenderTargetView();
        bool CompileShaders();
        bool CreateInstanceBuffer();
        bool CreateConstantBuffer();
        bool CreateBlendState();
        bool CreateSamplerState();
        bool CreateDirect2DResources();

        // Rendering helpers
        void SortStreaksByDepth(std::vector<const CharacterStreak*>& streaks);
        void UpdateInstanceBuffer(const AnimationSystem& animationSystem, ColorScheme colorScheme);
        void ClearRenderTarget();
        void RenderFPSCounter(float fps);

        // DirectX resources
        Microsoft::WRL::ComPtr<ID3D11Device> m_device;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
        Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;

        // Direct2D/DirectWrite resources for FPS display
        Microsoft::WRL::ComPtr<ID2D1Factory1> m_d2dFactory;
        Microsoft::WRL::ComPtr<ID2D1Device> m_d2dDevice;
        Microsoft::WRL::ComPtr<ID2D1DeviceContext> m_d2dContext;
        Microsoft::WRL::ComPtr<ID2D1Bitmap1> m_d2dBitmap;
        Microsoft::WRL::ComPtr<IDWriteFactory> m_dwriteFactory;
        Microsoft::WRL::ComPtr<IDWriteTextFormat> m_fpsTextFormat;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_fpsBrush;

        // Shader resources
        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;

        // Buffers
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_instanceBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_constantBuffer;
        UINT m_instanceBufferCapacity;

        // Render states
        Microsoft::WRL::ComPtr<ID3D11BlendState> m_blendState;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> m_samplerState;

        // Texture atlas reference
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_atlasTextureSRV;

        static constexpr UINT INITIAL_INSTANCE_CAPACITY = 10000; // Max characters per frame
    };

    /// <summary>
    /// Instance data for rendering a single character glyph.
    /// Packed tightly for GPU upload.
    /// </summary>
    struct CharacterInstanceData
    {
        float position[3];      // World position (x, y, z)
        float uvMin[2];         // Top-left UV coordinate
        float uvMax[2];         // Bottom-right UV coordinate
        float color[4];         // RGBA color
        float brightness;       // Brightness multiplier (0-1)
        float scale;            // Scale multiplier
        float padding[2];       // Padding to align to 16 bytes

        CharacterInstanceData()
            : position{ 0.0f, 0.0f, 0.0f }
            , uvMin{ 0.0f, 0.0f }
            , uvMax{ 1.0f, 1.0f }
            , color{ 0.0f, 1.0f, 0.0f, 1.0f }
            , brightness(1.0f)
            , scale(1.0f)
            , padding{ 0.0f, 0.0f }
        {
        }
    };

    /// <summary>
    /// Constant buffer data passed to shaders each frame.
    /// </summary>
    struct ConstantBufferData
    {
        float projection[16];   // 4x4 projection matrix (column-major)
        float padding[48];      // Padding to 256 bytes for optimal GPU alignment

        ConstantBufferData()
            : projection{ 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 }
            , padding{}
        {
        }
    };
}
