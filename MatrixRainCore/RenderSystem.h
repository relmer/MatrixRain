#pragma once





#include "AnimationSystem.h"
#include "CharacterInstance.h"
#include "Overlay.h"
#include "Viewport.h"
#include "ColorScheme.h"





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
    /// Parameters for rendering a single frame.
    /// </summary>
    struct RenderParams
    {
        ColorScheme             colorScheme        = ColorScheme::Green;
        float                   fps                = 0.0f;
        int                     rainPercentage     = 0;
        int                     streakCount        = 0;
        int                     activeHeadCount    = 0;
        bool                    showDebugFadeTimes = false;
        float                   elapsedTime        = 0.0f;
        const Overlay         * pHelpOverlay       = nullptr;
        const Overlay         * pHotkeyOverlay     = nullptr;
        const Overlay         * pUsageOverlay      = nullptr;
    };

    /// <summary>
    /// Render all character streaks from the animation system.
    /// </summary>
    void Render (const AnimationSystem & animationSystem, const Viewport & viewport, const RenderParams & params);

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

    /// <summary>
    /// Set glow intensity multiplier from percentage (0-200).
    /// </summary>
    /// <param name="intensityPercent">Glow intensity percentage (100 = default)</param>
    void SetGlowIntensity (int intensityPercent);

    /// <summary>
    /// Set glow blur size from percentage (50-200).
    /// </summary>
    /// <param name="sizePercent">Glow size percentage (100 = default)</param>
    void SetGlowSize (int sizePercent);

    /// <summary>
    /// Override character scale to prevent viewport-based scaling.
    /// When set, the constant buffer characterScale uses this value
    /// instead of computing from viewport height.
    /// </summary>
    /// <param name="scale">Fixed character scale (1.0 = full size)</param>
    void SetCharacterScaleOverride (float scale);

    /// <summary>
    /// Notify the render system of a DPI change.  Recreates DPI-sensitive
    /// resources (text formats) and stores the new scale factor for use
    /// by the character-scale constant buffer and overlay layout.
    /// </summary>
    /// <param name="dpi">New dots-per-inch value from GetDpiForWindow</param>
    void OnDpiChanged (UINT dpi);

    /// <summary>
    /// Returns the current DPI scale factor (1.0 at 96 DPI / 100%).
    /// </summary>
    float GetDpiScale() const { return m_dpiScale; }

    // Accessors
    ID3D11Device        * GetDevice()        const { return m_device.Get();        }
    ID3D11DeviceContext * GetContext()       const { return m_context.Get();       }
    ID2D1DeviceContext  * GetD2DContext()    const { return m_d2dContext.Get();    }
    IDWriteFactory      * GetDWriteFactory() const { return m_dwriteFactory.Get(); }

private:
    /// <summary>
    /// Instance data for rendering a single character glyph.
    /// Packed tightly for GPU upload.
    /// </summary>
    #pragma warning(push)
    #pragma warning(disable: 4324)  // structure was padded due to alignment specifier
    struct alignas(16) CharacterInstanceData
    {
        float position[3];      // World position (x, y, z)
        float uvMin[2];         // Top-left UV coordinate
        float uvMax[2];         // Bottom-right UV coordinate
        float color[4];         // RGBA color
        float brightness;       // Brightness multiplier (0-1)
        float scaleX;           // Horizontal scale multiplier
        float scaleY;           // Vertical scale multiplier

        CharacterInstanceData() :
            position   { 0.0f, 0.0f, 0.0f },
            uvMin      { 0.0f, 0.0f },
            uvMax      { 1.0f, 1.0f },
            color      { 0.0f, 1.0f, 0.0f, 1.0f },
            brightness ( 1.0f ),
            scaleX     ( 1.0f ),
            scaleY     ( 1.0f )
        {
        }
    };
    #pragma warning(pop)

    /// <summary>
    /// Constant buffer data passed to shaders each frame.
    /// </summary>
    struct ConstantBufferData
    {
        float projection[16];   // 4x4 projection matrix (column-major)
        float characterScale;   // Global character scale (1.0 = normal, <1.0 for preview)
        float charWidth;        // Base quad width in pixels (24.0 for rain, cell width for overlay)
        float charHeight;       // Base quad height in pixels (36.0 for rain, cell height for overlay)
        float padding[45];      // Padding to 256 bytes for optimal GPU alignment

        ConstantBufferData() :
            projection     { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 },
            characterScale ( 1.0f ),
            charWidth      ( 24.0f ),
            charHeight     ( 36.0f ),
            padding        {}
        {
        }
    };
    // Initialization helpers
    HRESULT CreateDevice();
    HRESULT CreateSwapChain            (HWND hwnd, UINT width, UINT height);
    HRESULT CreateRenderTargetView();
    HRESULT CompileCharacterShaders();
    HRESULT CompileBloomShaders();
    HRESULT CreateDummyVertexBuffer();
    HRESULT CreateInstanceBuffer();
    HRESULT CreateConstantBuffer();
    HRESULT CreateBloomConstantBuffer();
    HRESULT CreateBlendState();
    HRESULT CreateSamplerState();
    HRESULT CreateDirect2DResources();
    HRESULT CreateFpsTextFormat();
    void    UpdateDpiScale();
    HRESULT CreateBloomResources       (UINT width, UINT height);

    // Rendering helpers
    void    SortStreaksByDepth       (std::vector<const CharacterStreak *> & streaks);
    HRESULT UpdateInstanceBuffer     (const AnimationSystem & animationSystem, ColorScheme colorScheme, float elapsedTime);
    void    ClearRenderTarget();
    void    RenderFPSCounter         (float fps, int rainPercentage, int streakCount, int activeHeadCount);
    void    DrawFeatheredGlow        (const wchar_t * fpsText, UINT32 textLength, const D2D1_RECT_F & textRect);
    void    DrawFeatheredBackground  (std::span<const HintCharacter> chars, std::span<const float> xPositions, float advanceScale, float baseY, float cellHeight, int numRows, float padding, float opacityScale);
    void    ComputeRowRects          (std::span<const HintCharacter> chars, std::span<const float> xPositions, float advanceScale, float baseY, float cellHeight, int numRows, float hPad, float vPad);
    void    BuildOverlayInstances    (std::span<const HintCharacter> chars, float charScale, float baseY, float cellHeight, std::span<const float> xPositions, float advanceScale);
    void    RenderOverlayInstances   ();
    void    RenderTwoColumnOverlay   (std::span<const HintCharacter> chars, int marginCols, int keyColChars, int gapChars, int numRows, float cellHeight, float padding);
    void    RenderDebugFadeTimes     (const AnimationSystem & animationSystem);
    HRESULT ApplyBloom();
    void    RenderFullscreenPass     (ID3D11RenderTargetView * pRenderTarget, ID3D11PixelShader * pPixelShader, ID3D11ShaderResourceView * const * ppShaderResources, UINT numResources);
    void    SetRenderPipelineState   (ID3D11InputLayout * pInputLayout, D3D11_PRIMITIVE_TOPOLOGY topology, ID3D11Buffer * pVertexBuffer, UINT stride, ID3D11VertexShader * pVertexShader, ID3D11Buffer * pConstantBuffer, ID3D11PixelShader * pPixelShader);
    void    SetViewport              (UINT width, UINT height);
    
    static int  CodepointToUtf16                    (uint32_t codepoint, wchar_t * glyphStr);
    static void BuildCharacterInstanceData          (const CharacterInstance & character, const Vector3 & streakPos, const Color4 & schemeColor, CharacterInstanceData & data);
    void        ComputeOverlayLayout                (std::span<const HintCharacter> chars, int marginCols, int keyColChars, int gapChars, int numRows, float cellHeight, float padding, std::vector<float> & xPositions, D2D1_RECT_F & bounds, float & baseY, float & advanceScale);
    void        CalculateColumnAlignedTextPositions (std::span<const HintCharacter> chars, int marginCols, int keyColChars, int descColStart, float maxKeyWidth, const std::vector<float> & keyColWidths, float gapWidth, float advScaled, std::vector<float> & positions);

    // Resource cleanup helpers
    void ReleaseBloomResources();
    void ReleaseDirect2DResources();
    void ReleaseRenderTargetResources();
    void ReleaseDirectXResources();

    // Resource recreation helpers
    HRESULT RecreateDirect2DBitmap();

    // Shader compilation helpers
    struct ShaderCompileEntry;
    HRESULT CompileShadersFromTable       (std::span<const ShaderCompileEntry> entries);

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
    ComPtr<ID3D11PixelShader>         m_haloPS;
    ComPtr<ID3D11Buffer>              m_fullscreenQuadVB;
    ComPtr<ID3D11Buffer>              m_haloConstantBuffer;
    ComPtr<ID3D11Buffer>              m_bloomConstantBuffer;

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
    ComPtr<ID3D11PixelShader>  m_overlayPixelShader;
    ComPtr<ID3D11InputLayout>  m_inputLayout;
    ComPtr<ID3D11InputLayout>  m_fullscreenQuadInputLayout;
    ComPtr<ID3D11VertexShader> m_fullscreenQuadVS;

    // Buffers
    ComPtr<ID3D11Buffer> m_dummyVertexBuffer;
    ComPtr<ID3D11Buffer> m_instanceBuffer;
    ComPtr<ID3D11Buffer> m_constantBuffer;
    UINT                 m_instanceBufferCapacity { INITIAL_INSTANCE_CAPACITY };

    // Overlay GPU rendering
    ComPtr<ID3D11Buffer>                   m_overlayInstanceBuffer;
    UINT                                   m_overlayInstanceBufferCapacity { 0 };
    std::vector<CharacterInstanceData>     m_overlayInstanceData;
    std::vector<D2D1_RECT_F>               m_haloRowRects;
    const HintCharacter                  * m_haloRowRectsKey { nullptr };  // Cache key: chars data pointer

    // Render states
    ComPtr<ID3D11BlendState>   m_blendState;
    ComPtr<ID3D11BlendState>   m_premultipliedBlendState;
    ComPtr<ID3D11SamplerState> m_samplerState;

    // Texture atlas reference
    ComPtr<ID3D11ShaderResourceView> m_atlasTextureSRV;

    // Window handle (for DPI queries)
    HWND m_hwnd { nullptr };

    // Render target dimensions
    UINT m_renderWidth  { 0 };
    UINT m_renderHeight { 0 };

    // DPI scale factor (1.0 at 96 DPI / 100%)
    float m_dpiScale { 1.0f };

    // Glow effect parameters
    float m_glowIntensity { 2.5f };  // Bloom intensity multiplier (100% = 2.5)
    float m_glowSize      { 1.0f };  // Blur radius multiplier (100% = 1.0)

    // Character scale override (bypasses viewport-based scaling when set)
    std::optional<float> m_characterScaleOverride;

    // Per-frame working data (reused to avoid allocations)
    std::vector<CharacterInstanceData>   m_instanceData;
    std::vector<const CharacterStreak *> m_streakPtrs;

    static constexpr UINT INITIAL_INSTANCE_CAPACITY = 10000; // Max characters per frame
};






