#pragma once





#include "MatrixRain/AnimationSystem.h"
#include "MatrixRain/CharacterInstance.h"
#include "MatrixRain/Viewport.h"
#include "MatrixRain/ColorScheme.h"





using Microsoft::WRL::ComPtr;





// Forward declarations
class CharacterStreak;

/// <summary>
/// Manages DirectX 11 rendering pipeline for Matrix Rain effect.
/// Handles device creation, shader compilation, instanced rendering, and presentation.
/// </summary>
class RenderSystem
{
public:
    ~RenderSystem();

    /// <summary>
    /// Initialize DirectX 11 device, swap chain, and rendering resources.
    /// </summary>
    /// <param name="hwnd">Window handle for swap chain creation</param>
    /// <param name="width">Initial viewport width</param>
    /// <param name="height">Initial viewport height</param>
    /// <returns>True on success, false on failure</returns>
    HRESULT Initialize (HWND hwnd, UINT width, UINT height);

    /// <summary>
    /// Render all character streaks from the animation system.
    /// </summary>
    /// <param name="animationSystem">Source of streak data to render</param>
    /// <param name="viewport">Viewport for projection matrix</param>
    /// <param name="colorScheme">Current color scheme for rendering</param>
    /// <param name="fps">Current FPS for display (0 to hide)</param>
    /// <param name="rainPercentage">Current rain density percentage (0-100)</param>
    /// <param name="streakCount">Current number of active streaks</param>
    /// <param name="activeHeadCount">Current number of active streak heads on screen</param>
    /// <param name="showDebugFadeTimes">True to show debug fade time overlay</param>
    /// <param name="elapsedTime">Elapsed time in seconds for color cycling</param>
    void Render (const AnimationSystem & animationSystem, const Viewport & viewport, ColorScheme colorScheme = ColorScheme::Green, float fps = 0.0f, int rainPercentage = 0, int streakCount = 0, int activeHeadCount = 0, bool showDebugFadeTimes = false, float elapsedTime = 0.0f);

    /// <summary>
    /// Present the rendered frame to the screen.
    /// </summary>
    void Present();

    /// <summary>
    /// Handle window resize by recreating swap chain buffers.
    /// </summary>
    /// <param name="width">New viewport width</param>
    /// <param name="height">New viewport height</param>
    void Resize (UINT width, UINT height);

    /// <summary>
    /// Recreate swap chain for display mode transitions (windowed ↔ fullscreen).
    /// </summary>
    /// <param name="hwnd">Window handle</param>
    /// <param name="width">New width</param>
    /// <param name="height">New height</param>
    /// <param name="fullscreen">True for fullscreen, false for windowed</param>
    /// <returns>True on success, false on failure</returns>
    bool RecreateSwapChain (HWND hwnd, UINT width, UINT height, bool fullscreen);

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
    ID3D11Device        * GetDevice()  const { return m_device.Get();  }
    ID3D11DeviceContext * GetContext() const { return m_context.Get(); }

    // Internal data structures (public for static helper function access)
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

        CharacterInstanceData() :
            position   { 0.0f, 0.0f, 0.0f },
            uvMin      { 0.0f, 0.0f },
            uvMax      { 1.0f, 1.0f },
            color      { 0.0f, 1.0f, 0.0f, 1.0f },
            brightness ( 1.0f ),
            scale      ( 1.0f ),
            padding    { 0.0f, 0.0f }
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

        ConstantBufferData() :
            projection { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 },
            padding    {}
        {
        }
    };

private:
    // Initialization helpers
    HRESULT CreateDevice();
    HRESULT CreateSwapChain            (HWND hwnd, UINT width, UINT height);
    HRESULT CreateRenderTargetView();
    HRESULT CompileCharacterShaders();
    HRESULT CompileBloomShaders();
    HRESULT CreateDummyVertexBuffer();
    HRESULT CreateInstanceBuffer();
    HRESULT CreateConstantBuffer();
    HRESULT CreateBlendState();
    HRESULT CreateSamplerState();
    HRESULT CreateDirect2DResources();
    HRESULT CreateBloomResources       (UINT width, UINT height);

    // Rendering helpers
    void    SortStreaksByDepth       (std::vector<const CharacterStreak *> & streaks);
    HRESULT UpdateInstanceBuffer     (const AnimationSystem & animationSystem, ColorScheme colorScheme, float elapsedTime);
    void    ClearRenderTarget();
    void    RenderFPSCounter         (float fps, int rainPercentage, int streakCount, int activeHeadCount);
    void    RenderDebugFadeTimes     (const AnimationSystem & animationSystem);
    HRESULT ApplyBloom();
    void    RenderFullscreenPass     (ID3D11RenderTargetView * pRenderTarget, ID3D11PixelShader * pPixelShader, ID3D11ShaderResourceView * const * ppShaderResources, UINT numResources);
    void    SetRenderPipelineState   (ID3D11InputLayout * pInputLayout, D3D11_PRIMITIVE_TOPOLOGY topology, ID3D11Buffer * pVertexBuffer, UINT stride, ID3D11VertexShader * pVertexShader, ID3D11Buffer * pConstantBuffer, ID3D11PixelShader * pPixelShader);
    void    SetViewport              (UINT width, UINT height);
    
    static void BuildCharacterInstanceData (const CharacterInstance & character, const Vector3 & streakPos, const Color4 & schemeColor, CharacterInstanceData & data);

    // Resource cleanup helpers
    void ReleaseBloomResources();
    void ReleaseDirect2DResources();
    void ReleaseRenderTargetResources();
    void ReleaseDirectXResources();

    // Resource recreation helpers
    HRESULT RecreateDirect2DBitmap();

    // Shader compilation helpers
    struct ShaderCompileEntry;
    HRESULT CompileShadersFromTable (const ShaderCompileEntry * pTable, size_t cEntries);

    // DirectX resources
    ComPtr<ID3D11Device>           m_device;
    ComPtr<ID3D11DeviceContext>    m_context;
    ComPtr<IDXGISwapChain>         m_swapChain;
    ComPtr<ID3D11RenderTargetView> m_renderTargetView;

    // Bloom post-processing resources
    ComPtr<ID3D11Texture2D>           m_sceneTexture;
    ComPtr<ID3D11RenderTargetView>    m_sceneRTV;
    ComPtr<ID3D11ShaderResourceView>  m_sceneSRV;
    ComPtr<ID3D11Texture2D>           m_bloomTexture;
    ComPtr<ID3D11RenderTargetView>    m_bloomRTV;
    ComPtr<ID3D11ShaderResourceView>  m_bloomSRV;
    ComPtr<ID3D11Texture2D>           m_blurTempTexture;
    ComPtr<ID3D11RenderTargetView>    m_blurTempRTV;
    ComPtr<ID3D11ShaderResourceView>  m_blurTempSRV;
    ComPtr<ID3D11PixelShader>         m_bloomExtractPS;
    ComPtr<ID3D11PixelShader>         m_blurHorizontalPS;
    ComPtr<ID3D11PixelShader>         m_blurVerticalPS;
    ComPtr<ID3D11PixelShader>         m_compositePS;
    ComPtr<ID3D11Buffer>              m_fullscreenQuadVB;

    // Direct2D/DirectWrite resources for FPS display
    ComPtr<ID2D1Factory1>        m_d2dFactory;
    ComPtr<ID2D1Device>          m_d2dDevice;
    ComPtr<ID2D1DeviceContext>   m_d2dContext;
    ComPtr<ID2D1Bitmap1>         m_d2dBitmap;
    ComPtr<IDWriteFactory>       m_dwriteFactory;
    ComPtr<IDWriteTextFormat>    m_fpsTextFormat;
    ComPtr<ID2D1SolidColorBrush> m_fpsBrush;
    ComPtr<ID2D1SolidColorBrush> m_fpsGlowBrush;

    // Shader resources
    ComPtr<ID3D11VertexShader> m_vertexShader;
    ComPtr<ID3D11PixelShader>  m_pixelShader;
    ComPtr<ID3D11InputLayout>  m_inputLayout;
    ComPtr<ID3D11InputLayout>  m_fullscreenQuadInputLayout;
    ComPtr<ID3D11VertexShader> m_fullscreenQuadVS;

    // Buffers
    ComPtr<ID3D11Buffer> m_dummyVertexBuffer;
    ComPtr<ID3D11Buffer> m_instanceBuffer;
    ComPtr<ID3D11Buffer> m_constantBuffer;
    UINT                 m_instanceBufferCapacity { INITIAL_INSTANCE_CAPACITY};

    // Render states
    ComPtr<ID3D11BlendState>   m_blendState;
    ComPtr<ID3D11SamplerState> m_samplerState;

    // Texture atlas reference
    ComPtr<ID3D11ShaderResourceView> m_atlasTextureSRV;

    // Render target dimensions
    UINT m_renderWidth  { 0 };
    UINT m_renderHeight { 0 };

    // Per-frame working data (reused to avoid allocations)
    std::vector<CharacterInstanceData>   m_instanceData;
    std::vector<const CharacterStreak *> m_streakPtrs;

    static constexpr UINT INITIAL_INSTANCE_CAPACITY = 10000; // Max characters per frame
};






