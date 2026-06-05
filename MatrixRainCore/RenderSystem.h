#pragma once





#include "AnimationSystem.h"
#include "CharacterInstance.h"
#include "GlyphAtlas.h"
#include "IRenderSystem.h"
#include "Overlay.h"
#include "QualityPresets.h"
#include "RenderParams.h"
#include "Viewport.h"
#include "ColorScheme.h"





using Microsoft::WRL::ComPtr;





// Forward declarations
class CharacterStreak;



////////////////////////////////////////////////////////////////////////////////
//
//  QueryProcessGpuLoadPercent
//
//  Returns the throttled (~1 Hz internally) process-scoped GPU load
//  percentage that matches Task Manager's per-process "GPU" column.
//  Returns -1.0 until the first PDH collection produces data, or if
//  PDH initialisation has permanently failed.  Wired into the v1.5
//  property-sheet 1 Hz title timer (T032) alongside the per-frame
//  FPS publisher.
//
////////////////////////////////////////////////////////////////////////////////

double QueryProcessGpuLoadPercent();

////////////////////////////////////////////////////////////////////////////////
//
//  RenderSystem
//
//  Manages the Direct3D 11 rendering pipeline for the Matrix rain effect:
//  device creation, shader compilation, instanced rendering, and presentation.
//
////////////////////////////////////////////////////////////////////////////////

class RenderSystem : public IRenderSystem
{
public:
    ~RenderSystem();

    HRESULT Initialize (HWND hwnd, UINT width, UINT height, std::optional<LUID> adapterLuid = std::nullopt);

    HRESULT BuildGlyphAtlas();

    HRESULT RebuildOverlayAtlas();

    void Render (const AnimationSystem & animationSystem, const Viewport & viewport, const RenderParams & params) override;

    HRESULT Present() override;

    void Resize (UINT width, UINT height) override;

    bool RecreateSwapChain (HWND hwnd, UINT width, UINT height, bool fullscreen);

    bool SetFullscreen();

    bool SetWindowed();

    void Shutdown();

    void SetGlowIntensity (int intensityPercent) override;

    void SetGlowSize (int sizePercent) override;

    void SetBlurPasses     (int passes)  override;
    void SetBloomResolution (int divisor) override;
    void SetBlurTaps       (int taps)    override;

    void SetCharacterScaleOverride (float scale) override;

    void OnDpiChanged (UINT dpi) override;

    float GetDpiScale() const override { return m_dpiScale; }

    // Accessors
    ID3D11Device        * GetDevice()        const { return m_device.Get();        }
    ID3D11DeviceContext * GetContext()       const { return m_context.Get();       }
    ID2D1DeviceContext  * GetD2DContext()    const { return m_d2dContext.Get();    }
    IDWriteFactory      * GetDWriteFactory() const { return m_dwriteFactory.Get(); }

private:
    // Instance data for rendering a single character glyph; packed tightly for
    // GPU upload.
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

    // Constant buffer data passed to shaders each frame.
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
    void    RenderFPSCounter         (float fps, int rainPercentage, int streakCount, int activeHeadCount, double gpuLoadPercent, bool gpuLoadValid);
    void    DrawFeatheredGlow        (const wchar_t * fpsText, UINT32 textLength, const D2D1_RECT_F & textRect);
    void    DrawFeatheredBackground  (std::span<const HintCharacter> chars, std::span<const float> xPositions, float advanceScale, float baseY, float cellHeight, int numRows, float padding, float opacityScale);
    void    ComputeRowRects          (std::span<const HintCharacter> chars, std::span<const float> xPositions, float advanceScale, float baseY, float cellHeight, int numRows, float hPad, float vPad);
    void    BuildOverlayInstances    (std::span<const HintCharacter> chars, float charScale, float baseY, float cellHeight, std::span<const float> xPositions, float advanceScale);
    void    RenderOverlayInstances   ();
    void    RenderTwoColumnOverlay   (std::span<const HintCharacter> chars, int marginCols, int keyColChars, int gapChars, int numRows, float cellHeight, float padding);
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
    ComPtr<ID3D11PixelShader>         m_blurHorizontalPS9;
    ComPtr<ID3D11PixelShader>         m_blurHorizontalPS5;
    ComPtr<ID3D11PixelShader>         m_blurVerticalPS;
    ComPtr<ID3D11PixelShader>         m_blurVerticalPS9;
    ComPtr<ID3D11PixelShader>         m_blurVerticalPS5;
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

    // Texture atlas (per-device: rain + overlay glyph atlases on this device)
    GlyphAtlas m_glyphAtlas;

    // Window handle (for DPI queries)
    HWND m_hwnd { nullptr };

    // User-selected adapter LUID, or nullopt for system default.  Captured
    // in Initialize and consumed by CreateDevice to route D3D device
    // creation to the requested GPU on hybrid laptops.
    std::optional<LUID> m_requestedAdapterLuid;

    // Render target dimensions
    UINT m_renderWidth  { 0 };
    UINT m_renderHeight { 0 };

    // DPI scale factor (1.0 at 96 DPI / 100%)
    float m_dpiScale { 1.0f };

    // Glow effect parameters
    float m_glowIntensity { 2.5f };  // Bloom intensity multiplier (100% = 2.5)
    float m_glowSize      { 1.0f };  // Blur radius multiplier (100% = 1.0)

    // User Story 5 - graphics quality runtime parameters.  Driven by the
    // settings snapshot path the same way m_glowIntensity is.
    int               m_blurPasses              { 3 };
    ResolutionDivisor m_bloomResolutionDivisor  { ResolutionDivisor::Half };
    BlurTaps          m_blurTaps                { BlurTaps::High };

    // Character scale override (bypasses viewport-based scaling when set)
    std::optional<float> m_characterScaleOverride;

    // Per-frame working data (reused to avoid allocations)
    std::vector<CharacterInstanceData>   m_instanceData;
    std::vector<const CharacterStreak *> m_streakPtrs;

    static constexpr UINT INITIAL_INSTANCE_CAPACITY = 10000; // Max characters per frame
};






