#include "..\..\MatrixRainCore\pch.h"

#include "..\..\MatrixRainCore\AnimationSystem.h"
#include "..\..\MatrixRainCore\CharacterSet.h"
#include "..\..\MatrixRainCore\DensityController.h"
#include "..\..\MatrixRainCore\FrameMetrics.h"
#include "..\..\MatrixRainCore\QualityPresets.h"
#include "..\..\MatrixRainCore\RandomSource.h"
#include "..\..\MatrixRainCore\RenderParams.h"
#include "..\..\MatrixRainCore\RenderSystem.h"
#include "..\..\MatrixRainCore\ScanlineStyleMapping.h"
#include "..\..\MatrixRainCore\ScreenSaverSettings.h"
#include "..\..\MatrixRainCore\Viewport.h"
#include "..\..\MatrixRainCore\WindowsDisplayLuminanceProvider.h"

//  WIC is used only by this tool, so the import library is listed here rather
//  than in pch.h, where it would follow MatrixRain.exe into a release build.
#pragma comment (lib, "windowscodecs.lib")
#pragma comment (lib, "version.lib")





//  A fixed frame size and a fixed time step, so a run depends on nothing but
//  the seed and the settings case. 1920x1080 is large enough that a head's
//  glow falls off well inside the frame.
static constexpr UINT     kFrameWidth      = 1920;
static constexpr UINT     kFrameHeight     = 1080;
static constexpr float    kStepSeconds     = 1.0f / 60.0f;

//  Three seconds of rain, enough for streaks to fill the frame and for trails
//  to reach full length.
static constexpr int      kWarmupFrames    = 180;

//  Frames timed per quality preset in benchmark mode.
static constexpr int      kBenchmarkFrames = 600;

//  Display scales every case is captured at. The rain's row pitch is 24 px
//  times this, so these are the 24, 30 and 36 px cells of a monitor at 100%,
//  125% and 150% -- the last two being the machine this was calibrated on.
//
//  The sweep earns its storage on the scanline cases. Scanline pitch is
//  derived from the cell, so at Style 1 a 24 px cell gives a ~2.4 px pitch,
//  below the 3 px floor where a fractional pitch beats against the pixel grid
//  and gaps drop out, while a 36 px cell gives a clean ~3.6 px. One scale
//  would pin the baseline to one side of that boundary and leave the other
//  side unguarded.
static constexpr float    kDpiScales[]     = { 1.0f, 1.25f, 1.5f };

//  The seed every run starts from. Any fixed value would do; what matters is
//  that it never changes, or reference numbers stop being comparable.
static constexpr uint32_t kReferenceSeed   = 1;

//  Rain is drawn on a black field, so the whole frame's mean luminance is a
//  small number; printing six decimals keeps a small drift readable.
static constexpr int      kPrintPrecision  = 6;

//  A pixel counts as changed above this many 8-bit code values. Two is the
//  point where a difference stops being rounding and starts being something
//  a viewer could in principle see on a dark field.
static constexpr float    kDiffThreshold   = 2.0f;

//  Difference images are multiplied by this before being written, because an
//  honest difference image of a near-match is an entirely black picture.
static constexpr int      kDiffAmplify     = 16;

//  Where baseline frames live by default, relative to the repository root.
static constexpr wchar_t  kszDefaultBaselineDir[] = L"specs\\008-hdr-linear-rendering\\baseline";





////////////////////////////////////////////////////////////////////////////////
//
//  SettingsCase
//
//  One fixed point in the settings space that calibration must preserve.
//
////////////////////////////////////////////////////////////////////////////////

struct SettingsCase
{
    const wchar_t * m_pszName;
    int             m_glowIntensityPercent;
    int             m_glowSizePercent;
    bool            m_scanlinesEnabled;
    int             m_scanlinesStyle;
    ColorScheme     m_colorScheme;
    COLORREF        m_customColor;
};





//  The cases task T004 pins: the defaults, both ends of each glow slider,
//  scanlines at both ends of the Style range, and a custom color, so a
//  calibration that only happens to match at the defaults is caught.
static const SettingsCase s_krgCases[] =
{
    { L"defaults",        100, 100, false,  50, ColorScheme::Green,  RGB (0, 255,   0) },
    { L"glow-min",          1, 100, false,  50, ColorScheme::Green,  RGB (0, 255,   0) },
    { L"glow-max",        200, 100, false,  50, ColorScheme::Green,  RGB (0, 255,   0) },
    { L"glow-size-min",   100,  50, false,  50, ColorScheme::Green,  RGB (0, 255,   0) },
    { L"glow-size-max",   100, 200, false,  50, ColorScheme::Green,  RGB (0, 255,   0) },
    { L"scanlines-1",     100, 100, true,    1, ColorScheme::Green,  RGB (0, 255,   0) },
    { L"scanlines-100",   100, 100, true,  100, ColorScheme::Green,  RGB (0, 255,   0) },
    { L"custom-color",    100, 100, false,  50, ColorScheme::Custom, RGB (0, 128, 255) },
};





//  The presets benchmark mode walks. Custom is a sentinel, not a row.
static const QualityPreset s_krgPresets[] =
{
    QualityPreset::Low,
    QualityPreset::Medium,
    QualityPreset::High,
};





////////////////////////////////////////////////////////////////////////////////
//
//  Options
//
////////////////////////////////////////////////////////////////////////////////

enum class RunMode
{
    Reference,   // Render each case and write it out as the baseline
    Compare,     // Render each case and diff it against the baseline
    Benchmark,   // Time frames per quality preset
    Luminance,   // Report what the display luminance provider sees, and how long it takes
};




struct Options
{
    bool         m_useWarp     = true;
    RunMode      m_mode        = RunMode::Reference;
    std::wstring m_baselineDir = kszDefaultBaselineDir;

    //  Benchmark mode only. Reference and compare stay pinned to the constants
    //  above, so nobody can quietly recapture a baseline at a size or scale the
    //  committed frames cannot be compared against.
    UINT         m_frameWidth  = kFrameWidth;
    UINT         m_frameHeight = kFrameHeight;
    float        m_dpiScale    = kDpiScales[0];
    bool         m_hdr         = false;
};





////////////////////////////////////////////////////////////////////////////////
//
//  Harness
//
//  A hidden window with one fully built render pipeline on it, plus the
//  read-back and timing plumbing. Everything it measures with lives in
//  MatrixRainCore; this is only the wiring.
//
////////////////////////////////////////////////////////////////////////////////

class Harness
{
public:
    HRESULT Initialize (bool useWarp, UINT frameWidth, UINT frameHeight);
    void    Shutdown();

    HRESULT RunReference (const std::wstring & baselineDir);
    HRESULT RunCompare   (const std::wstring & baselineDir);
    HRESULT RunBenchmark (float dpiScale, bool hdr);
    HRESULT RunLuminance ();

private:
    HRESULT CreateHiddenWindow();
    HRESULT ReadBackFrame (std::vector<uint8_t> & frame);
    void    ApplyCase     (const SettingsCase & settingsCase, RenderParams & params);
    void    RunFrames     (int frameCount);
    HRESULT RenderOnce    (const RenderParams & params);
    HRESULT RenderCase    (const SettingsCase & settingsCase, float dpiScale, std::vector<uint8_t> & frame);
    void    ApplyDpiScale (float dpiScale);

    static std::optional<LUID> FindWarpAdapterLuid();

    HWND                               m_hwnd   = nullptr;
    UINT                               m_width  = kFrameWidth;
    UINT                               m_height = kFrameHeight;
    std::unique_ptr<Viewport>          m_viewport;
    std::unique_ptr<DensityController> m_densityController;
    std::unique_ptr<AnimationSystem>   m_animationSystem;
    std::unique_ptr<RenderSystem>      m_renderSystem;
};





////////////////////////////////////////////////////////////////////////////////
//
//  Harness::FindWarpAdapterLuid
//
//  WARP is selected the same way any adapter is -- by LUID -- so the render
//  system needs no software-device path of its own.
//
////////////////////////////////////////////////////////////////////////////////

std::optional<LUID> Harness::FindWarpAdapterLuid()
{
    Microsoft::WRL::ComPtr<IDXGIFactory4> pFactory;
    Microsoft::WRL::ComPtr<IDXGIAdapter1> pWarp;
    DXGI_ADAPTER_DESC1                    desc = {};


    if (FAILED (CreateDXGIFactory1 (IID_PPV_ARGS (&pFactory))))
    {
        return std::nullopt;
    }

    if (FAILED (pFactory->EnumWarpAdapter (IID_PPV_ARGS (&pWarp))))
    {
        return std::nullopt;
    }

    if (FAILED (pWarp->GetDesc1 (&desc)))
    {
        return std::nullopt;
    }

    return desc.AdapterLuid;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Harness::CreateHiddenWindow
//
////////////////////////////////////////////////////////////////////////////////

HRESULT Harness::CreateHiddenWindow()
{
    HRESULT     hr        = S_OK;
    WNDCLASSEXW wc        = {};
    HINSTANCE   hInstance = GetModuleHandleW (nullptr);


    wc.cbSize        = sizeof (wc);
    wc.lpfnWndProc   = DefWindowProcW;
    wc.hInstance     = hInstance;
    wc.lpszClassName = L"MatrixRainHdrCalibration";

    CBRA (RegisterClassExW (&wc) != 0);

    //  Never shown: a swap chain renders happily to a window that was created
    //  without WS_VISIBLE, and an invisible window keeps the measurement free
    //  of whatever else is on screen.
    m_hwnd = CreateWindowExW (0,
                              wc.lpszClassName,
                              L"MatrixRain HDR calibration",
                              WS_OVERLAPPEDWINDOW,
                              0,
                              0,
                              static_cast<int> (m_width),
                              static_cast<int> (m_height),
                              nullptr,
                              nullptr,
                              hInstance,
                              nullptr);

    CBRA (m_hwnd != nullptr);

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Harness::Initialize
//
////////////////////////////////////////////////////////////////////////////////

HRESULT Harness::Initialize (bool useWarp, UINT frameWidth, UINT frameHeight)
{
    HRESULT             hr          = S_OK;
    std::optional<LUID> adapterLuid = std::nullopt;
    CharacterSet      & charSet     = CharacterSet::GetInstance();


    if (useWarp)
    {
        adapterLuid = FindWarpAdapterLuid();
        CBRAExL (adapterLuid.has_value(), E_FAIL, L"WARP adapter not available");
    }

    m_width  = frameWidth;
    m_height = frameHeight;

    hr = CreateHiddenWindow();
    CHR (hr);

    m_viewport = std::make_unique<Viewport>();
    m_viewport->Resize (static_cast<float> (m_width), static_cast<float> (m_height));

    m_densityController = std::make_unique<DensityController> (*m_viewport, 24.0f);
    m_animationSystem   = std::make_unique<AnimationSystem>();
    m_renderSystem      = std::make_unique<RenderSystem>();

    hr = m_renderSystem->Initialize (m_hwnd, m_width, m_height, adapterLuid);
    CHR (hr);

    CBRAExL (charSet.Initialize(), E_FAIL, L"CharacterSet::Initialize failed");

    m_animationSystem->Initialize (*m_viewport, *m_densityController);

    hr = m_renderSystem->BuildGlyphAtlas();
    CHR (hr);

    //  Pin the glyph size: the run must not depend on the DPI of whatever
    //  monitor the harness happens to launch on. Each case then re-applies the
    //  scale it is being captured at.
    ApplyDpiScale (kDpiScales[0]);

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Harness::Shutdown
//
////////////////////////////////////////////////////////////////////////////////

void Harness::Shutdown()
{
    if (m_renderSystem)
    {
        m_renderSystem->Shutdown();
    }

    m_animationSystem.reset();
    m_densityController.reset();
    m_viewport.reset();
    m_renderSystem.reset();

    if (m_hwnd)
    {
        DestroyWindow (m_hwnd);
        m_hwnd = nullptr;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  Harness::ApplyDpiScale
//
//  Sets the display scale exactly as Application does at run time, so a swept
//  frame is the frame a monitor at that scaling would actually show.
//
////////////////////////////////////////////////////////////////////////////////

void Harness::ApplyDpiScale (float dpiScale)
{
    m_renderSystem->SetCharacterScaleOverride (dpiScale);
    m_animationSystem->SetDpiScale            (dpiScale);
    m_densityController->SetDpiScale          (dpiScale);
}





////////////////////////////////////////////////////////////////////////////////
//
//  Harness::ApplyCase
//
////////////////////////////////////////////////////////////////////////////////

void Harness::ApplyCase (const SettingsCase & settingsCase, RenderParams & params)
{
    m_renderSystem->SetGlowIntensity (settingsCase.m_glowIntensityPercent);
    m_renderSystem->SetGlowSize      (settingsCase.m_glowSizePercent);

    params.colorScheme           = settingsCase.m_colorScheme;
    params.customColor           = settingsCase.m_customColor;
    params.glowEnabled           = true;
    params.scanlinesEnabled      = settingsCase.m_scanlinesEnabled;
    params.scanlinesIntensity    = static_cast<float> (ScreenSaverSettings::DEFAULT_SCANLINES_INTENSITY_PERCENT) / 100.0f;
    params.scanlinesLinesPerCell = ScanlineLinesPerCell (settingsCase.m_scanlinesStyle);
    params.elapsedTime           = static_cast<float> (kWarmupFrames) * kStepSeconds;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Harness::RunFrames
//
//  Advances the animation by a fixed step, so the state reached depends only
//  on the seed and the frame count -- never on how fast this machine runs.
//
////////////////////////////////////////////////////////////////////////////////

void Harness::RunFrames (int frameCount)
{
    for (int frame = 0; frame < frameCount; ++frame)
    {
        m_animationSystem->Update (kStepSeconds);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  Harness::RenderOnce
//
////////////////////////////////////////////////////////////////////////////////

HRESULT Harness::RenderOnce (const RenderParams & params)
{
    RenderParams frameParams = params;


    frameParams.streakCount     = static_cast<int> (m_animationSystem->GetActiveStreakCount());
    frameParams.activeHeadCount = static_cast<int> (m_animationSystem->GetActiveHeadCount());

    m_renderSystem->Render (*m_animationSystem, *m_viewport, frameParams);

    return S_OK;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Harness::ReadBackFrame
//
//  Copies the back buffer into a tightly packed BGRA buffer. Runs before
//  Present, because a flip-model back buffer holds nothing afterwards.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT Harness::ReadBackFrame (std::vector<uint8_t> & frame)
{
    HRESULT                                 hr      = S_OK;
    Microsoft::WRL::ComPtr<IDXGISwapChain>  pSwap;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> pBackBuffer;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> pStaging;
    D3D11_TEXTURE2D_DESC                    desc    = {};
    D3D11_MAPPED_SUBRESOURCE                mapped  = {};
    ID3D11DeviceContext                   * pContext = m_renderSystem->GetContext();


    pSwap = m_renderSystem->GetSwapChain();
    CBRA (pSwap != nullptr);

    hr = pSwap->GetBuffer (0, IID_PPV_ARGS (&pBackBuffer));
    CHRA (hr);

    pBackBuffer->GetDesc (&desc);

    desc.Usage          = D3D11_USAGE_STAGING;
    desc.BindFlags      = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags      = 0;

    hr = m_renderSystem->GetDevice()->CreateTexture2D (&desc, nullptr, &pStaging);
    CHRA (hr);

    pContext->CopyResource (pStaging.Get(), pBackBuffer.Get());

    hr = pContext->Map (pStaging.Get(), 0, D3D11_MAP_READ, 0, &mapped);
    CHRA (hr);

    frame.resize (static_cast<size_t> (desc.Width) * desc.Height * 4);

    for (UINT row = 0; row < desc.Height; ++row)
    {
        memcpy (frame.data() + static_cast<size_t> (row) * desc.Width * 4,
                static_cast<const uint8_t *> (mapped.pData) + static_cast<size_t> (row) * mapped.RowPitch,
                static_cast<size_t> (desc.Width) * 4);
    }

    pContext->Unmap (pStaging.Get(), 0);

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  DpiPercent
//
//  The scale as the number Windows shows the user, to build file names.
//
////////////////////////////////////////////////////////////////////////////////

static int DpiPercent (float dpiScale)
{
    return static_cast<int> (dpiScale * 100.0f + 0.5f);
}





////////////////////////////////////////////////////////////////////////////////
//
//  BaselinePath
//
////////////////////////////////////////////////////////////////////////////////

static std::wstring BaselinePath (const std::wstring & directory,
                                  const wchar_t      * pszCase,
                                  float                dpiScale,
                                  const wchar_t      * pszSuffix)
{
    return std::format (L"{}\\{}-dpi{}{}", directory, pszCase, DpiPercent (dpiScale), pszSuffix);
}





////////////////////////////////////////////////////////////////////////////////
//
//  WritePng
//
//  Writes a BGRA frame losslessly. Lossless matters more than it sounds here:
//  a baseline that quietly re-encoded its own pixels would report a
//  difference against itself.
//
////////////////////////////////////////////////////////////////////////////////

static HRESULT WritePng (const std::wstring & path, const std::vector<uint8_t> & bgra, UINT width, UINT height)
{
    HRESULT                                       hr     = S_OK;
    ComPtr<IWICImagingFactory>                    pFactory;
    ComPtr<IWICStream>                            pStream;
    ComPtr<IWICBitmapEncoder>                     pEncoder;
    ComPtr<IWICBitmapFrameEncode>                 pFrame;
    WICPixelFormatGUID                            format = GUID_WICPixelFormat32bppBGRA;
    const UINT                                    stride = width * 4;


    hr = CoCreateInstance (CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS (&pFactory));
    CHRA (hr);

    hr = pFactory->CreateStream (&pStream);
    CHRA (hr);

    hr = pStream->InitializeFromFilename (path.c_str(), GENERIC_WRITE);
    CHRA (hr);

    hr = pFactory->CreateEncoder (GUID_ContainerFormatPng, nullptr, &pEncoder);
    CHRA (hr);

    hr = pEncoder->Initialize (pStream.Get(), WICBitmapEncoderNoCache);
    CHRA (hr);

    hr = pEncoder->CreateNewFrame (&pFrame, nullptr);
    CHRA (hr);

    hr = pFrame->Initialize (nullptr);
    CHRA (hr);

    hr = pFrame->SetSize (width, height);
    CHRA (hr);

    hr = pFrame->SetPixelFormat (&format);
    CHRA (hr);

    CBRAEx (format == GUID_WICPixelFormat32bppBGRA, E_FAIL);

    hr = pFrame->WritePixels (height, stride, static_cast<UINT> (bgra.size()), const_cast<BYTE *> (bgra.data()));
    CHRA (hr);

    hr = pFrame->Commit();
    CHRA (hr);

    hr = pEncoder->Commit();
    CHRA (hr);

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  ReadPng
//
////////////////////////////////////////////////////////////////////////////////

static HRESULT ReadPng (const std::wstring & path, std::vector<uint8_t> & bgra, UINT width, UINT height)
{
    HRESULT                       hr         = S_OK;
    ComPtr<IWICImagingFactory>    pFactory;
    ComPtr<IWICBitmapDecoder>     pDecoder;
    ComPtr<IWICBitmapFrameDecode> pFrame;
    ComPtr<IWICFormatConverter>   pConverter;
    UINT                          fileWidth  = 0;
    UINT                          fileHeight = 0;


    hr = CoCreateInstance (CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS (&pFactory));
    CHRA (hr);

    hr = pFactory->CreateDecoderFromFilename (path.c_str(),
                                              nullptr,
                                              GENERIC_READ,
                                              WICDecodeMetadataCacheOnDemand,
                                              &pDecoder);
    CHR (hr);

    hr = pDecoder->GetFrame (0, &pFrame);
    CHRA (hr);

    hr = pFrame->GetSize (&fileWidth, &fileHeight);
    CHRA (hr);

    CBRAEx (fileWidth == width && fileHeight == height, E_FAIL);

    hr = pFactory->CreateFormatConverter (&pConverter);
    CHRA (hr);

    hr = pConverter->Initialize (pFrame.Get(),
                                 GUID_WICPixelFormat32bppBGRA,
                                 WICBitmapDitherTypeNone,
                                 nullptr,
                                 0.0,
                                 WICBitmapPaletteTypeCustom);
    CHRA (hr);

    bgra.resize (static_cast<size_t> (width) * height * 4);

    hr = pConverter->CopyPixels (nullptr, width * 4, static_cast<UINT> (bgra.size()), bgra.data());
    CHRA (hr);

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  WriteDifferenceImage
//
//  Writes the per-pixel difference as a picture, amplified so it can actually
//  be seen. This is the artifact worth looking at: it says WHERE the render
//  changed, which no summary statistic can.
//
////////////////////////////////////////////////////////////////////////////////

static HRESULT WriteDifferenceImage (const std::wstring         & path,
                                     const std::vector<uint8_t> & baseline,
                                     const std::vector<uint8_t> & candidate,
                                     UINT                         width,
                                     UINT                         height)
{
    std::vector<uint8_t> image (static_cast<size_t> (width) * height * 4);

    for (size_t pixel = 0; pixel < static_cast<size_t> (width) * height; ++pixel)
    {
        for (size_t channel = 0; channel < 3; ++channel)
        {
            const size_t offset = pixel * 4 + channel;
            const int    delta  = std::abs (static_cast<int> (baseline[offset])
                                            - static_cast<int> (candidate[offset]));

            image[offset] = static_cast<uint8_t> (std::min (delta * kDiffAmplify, 255));
        }

        image[pixel * 4 + 3] = 255;
    }

    return WritePng (path, image, width, height);
}





////////////////////////////////////////////////////////////////////////////////
//
//  Harness::RenderCase
//
//  Renders one settings case to a finished frame. Every run starts from the
//  same seed and takes the same number of fixed steps, so two runs of this
//  differ only where the renderer differs.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT Harness::RenderCase (const SettingsCase & settingsCase, float dpiScale, std::vector<uint8_t> & frame)
{
    HRESULT      hr = S_OK;
    RenderParams params;


    ApplyDpiScale (dpiScale);
    ApplyCase     (settingsCase, params);

    RandomSource::Reseed (kReferenceSeed);
    m_animationSystem->ClearAllStreaks();
    RunFrames (kWarmupFrames);

    hr = RenderOnce (params);
    CHR (hr);

    hr = ReadBackFrame (frame);
    CHR (hr);

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Harness::RunReference
//
//  Captures the baseline: one frame per settings case, written out whole.
//  Keeping the frame rather than a summary of it is the point -- a later run
//  can be compared against it in ways nobody thought of today (FR-006).
//
////////////////////////////////////////////////////////////////////////////////

HRESULT Harness::RunReference (const std::wstring & baselineDir)
{
    HRESULT              hr = S_OK;
    std::vector<uint8_t> frame;


    CreateDirectoryW (baselineDir.c_str(), nullptr);

    wprintf (L"case,dpiPercent,meanLuminance,file\n");

    for (float dpiScale : kDpiScales)
    {
        for (const SettingsCase & settingsCase : s_krgCases)
        {
            const std::wstring path = BaselinePath (baselineDir, settingsCase.m_pszName, dpiScale, L".png");

            hr = RenderCase (settingsCase, dpiScale, frame);
            CHR (hr);

            hr = WritePng (path, frame, kFrameWidth, kFrameHeight);
            CHR (hr);

            wprintf (L"%s,%d,%.*f,%s\n",
                     settingsCase.m_pszName,
                     DpiPercent (dpiScale),
                     kPrintPrecision,
                     MeanLuminance (frame, kFrameWidth, kFrameHeight),
                     path.c_str());
        }
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Harness::RunCompare
//
//  Renders each case again and measures how far it has moved from the
//  baseline. A difference image is written per case, because the numbers say
//  how much changed and only the picture says what.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT Harness::RunCompare (const std::wstring & baselineDir)
{
    HRESULT              hr = S_OK;
    std::vector<uint8_t> frame;
    std::vector<uint8_t> baseline;


    wprintf (L"case,dpiPercent,meanLuminance,maxDiff,meanDiff,p99Diff,pixelsOverThreshold,maxDiffAt\n");

    for (float dpiScale : kDpiScales)
    {
        for (const SettingsCase & settingsCase : s_krgCases)
        {
            const std::wstring baselinePath = BaselinePath (baselineDir, settingsCase.m_pszName, dpiScale, L".png");
            const std::wstring diffPath     = BaselinePath (baselineDir, settingsCase.m_pszName, dpiScale, L".diff.png");
            FrameDifference    difference;


            hr = RenderCase (settingsCase, dpiScale, frame);
            CHR (hr);

            hr = ReadPng (baselinePath, baseline, kFrameWidth, kFrameHeight);

            if (FAILED (hr))
            {
                wprintf (L"%s,%d,,,,,,no baseline at %s\n",
                         settingsCase.m_pszName,
                         DpiPercent (dpiScale),
                         baselinePath.c_str());
                hr = S_OK;
                continue;
            }

            difference = CompareFrames (baseline, frame, kFrameWidth, kFrameHeight, kDiffThreshold);

            hr = WriteDifferenceImage (diffPath, baseline, frame, kFrameWidth, kFrameHeight);
            CHR (hr);

            wprintf (L"%s,%d,%.*f,%.0f,%.4f,%.0f,%zu,%ldx%ld\n",
                     settingsCase.m_pszName,
                     DpiPercent (dpiScale),
                     kPrintPrecision,
                     MeanLuminance (frame, kFrameWidth, kFrameHeight),
                     difference.m_maxDifference,
                     difference.m_meanDifference,
                     difference.m_p99Difference,
                     difference.m_pixelsOverThreshold,
                     difference.m_maxDifferenceAt.x,
                     difference.m_maxDifferenceAt.y);
        }
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Harness::RunBenchmark
//
//  Times the GPU, not the wall clock, with timestamp queries around each
//  frame, and reports the mean and the 95th percentile per quality preset
//  (Constitution II, SC-006).
//
//  Frames are not presented, so the numbers are rendering cost alone and are
//  not capped by the display's refresh rate. Blocking on each frame's queries
//  also keeps frames from overlapping, so a per-frame figure means what it
//  says. Both make this a measure of cost rather than of achievable frame
//  rate, which is what a 5% regression gate needs.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT Harness::RunBenchmark (float dpiScale, bool hdr)
{
    HRESULT                             hr        = S_OK;
    ID3D11Device                      * pDevice   = m_renderSystem->GetDevice();
    ID3D11DeviceContext               * pContext  = m_renderSystem->GetContext();
    Microsoft::WRL::ComPtr<ID3D11Query> pDisjoint;
    Microsoft::WRL::ComPtr<ID3D11Query> pStart;
    Microsoft::WRL::ComPtr<ID3D11Query> pEnd;
    D3D11_QUERY_DESC                    disjointDesc  = { D3D11_QUERY_TIMESTAMP_DISJOINT, 0 };
    D3D11_QUERY_DESC                    timestampDesc = { D3D11_QUERY_TIMESTAMP,          0 };


    hr = pDevice->CreateQuery (&disjointDesc, &pDisjoint);
    CHRA (hr);

    hr = pDevice->CreateQuery (&timestampDesc, &pStart);
    CHRA (hr);

    hr = pDevice->CreateQuery (&timestampDesc, &pEnd);
    CHRA (hr);

    //  --hdr: the same frames with a scRGB back buffer, which is what an HDR
    //  monitor costs (T035). The window is created at the origin, so it sits
    //  on the primary monitor; that monitor needs Windows HDR on, or the
    //  switch is refused and the run stops rather than timing SDR under an
    //  HDR label.
    if (hdr)
    {
        hr = m_renderSystem->ReconfigureOutputMode (OutputMode::Hdr);

        if (FAILED (hr) || m_renderSystem->GetOutputMode() != OutputMode::Hdr)
        {
            wprintf (L"--hdr: the swap chain would not switch to scRGB (hr=0x%08X);"
                     L" is Windows HDR on for the primary monitor?\n", static_cast<unsigned> (hr));
            CHR (FAILED (hr) ? hr : E_FAIL);
        }

        wprintf (L"# output=hdr\n");
    }

    wprintf (L"preset,frameWidth,frameHeight,dpiPercent,frames,meanGpuMs,p95GpuMs\n");

    for (QualityPreset preset : s_krgPresets)
    {
        const AdvancedGraphicsValues values = LookupPresetValues (preset);
        RenderParams                 params;
        std::vector<float>           timings;


        //  One configuration per run. Cost is swept by running the benchmark
        //  once per monitor, at that monitor's own resolution and scaling,
        //  rather than by sweeping inside a single run.
        ApplyDpiScale (dpiScale);
        ApplyCase     (s_krgCases[0], params);

        m_renderSystem->SetGlowIntensity  (values.m_glowIntensityPercent);
        m_renderSystem->SetBlurPasses     (values.m_blurPasses);
        m_renderSystem->SetBloomResolution (static_cast<int> (values.m_bloomResolutionDivisor));
        m_renderSystem->SetBlurTaps       (static_cast<int> (values.m_blurTaps));

        RandomSource::Reseed (kReferenceSeed);
        m_animationSystem->ClearAllStreaks();
        RunFrames (kWarmupFrames);

        timings.reserve (kBenchmarkFrames);

        for (int frame = 0; frame < kBenchmarkFrames; ++frame)
        {
            D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData = {};
            UINT64                              startTime    = 0;
            UINT64                              endTime      = 0;


            m_animationSystem->Update (kStepSeconds);

            pContext->Begin (pDisjoint.Get());
            pContext->End   (pStart.Get());

            hr = RenderOnce (params);
            CHR (hr);

            pContext->End (pEnd.Get());
            pContext->End (pDisjoint.Get());

            //  Deliberately NOT presenting. RenderSystem::Present uses a sync
            //  interval of 1, so presenting here would stall the frame against
            //  the display's refresh and the timestamps would measure vsync
            //  rather than rendering: every preset came back at ~12 ms with a
            //  p95 pinned to 16.6 ms, which is 60 Hz, not GPU work. The window
            //  is hidden and nothing reads the back buffer in this mode, so
            //  there is nothing to present.
            while (pContext->GetData (pDisjoint.Get(), &disjointData, sizeof (disjointData), 0) == S_FALSE)
            {
                Sleep (0);
            }

            while (pContext->GetData (pStart.Get(), &startTime, sizeof (startTime), 0) == S_FALSE)
            {
                Sleep (0);
            }

            while (pContext->GetData (pEnd.Get(), &endTime, sizeof (endTime), 0) == S_FALSE)
            {
                Sleep (0);
            }

            //  A disjoint frame means the GPU clock changed mid-measurement;
            //  its number means nothing, so it is dropped rather than averaged
            //  in.
            if (!disjointData.Disjoint && disjointData.Frequency != 0 && endTime > startTime)
            {
                timings.push_back (static_cast<float> (endTime - startTime) * 1000.0f
                                   / static_cast<float> (disjointData.Frequency));
            }
        }

        if (timings.empty())
        {
            wprintf (L"%d,%u,%u,%d,0,,\n",
                     static_cast<int> (preset),
                     m_width,
                     m_height,
                     DpiPercent (dpiScale));
            continue;
        }

        {
            double total = 0.0;
            size_t p95   = static_cast<size_t> (static_cast<double> (timings.size()) * 0.95);

            for (float timing : timings)
            {
                total += timing;
            }

            std::sort (timings.begin(), timings.end());

            p95 = std::min (p95, timings.size() - 1);

            wprintf (L"%d,%u,%u,%d,%zu,%.3f,%.3f\n",
                     static_cast<int> (preset),
                     m_width,
                     m_height,
                     DpiPercent (dpiScale),
                     timings.size(),
                     static_cast<float> (total / static_cast<double> (timings.size())),
                     timings[p95]);
        }
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Harness::RunLuminance
//
//  What WindowsDisplayLuminanceProvider reports for the output this window
//  sits on, and how long one query takes (T032 asks for under a millisecond,
//  since it runs at 1 Hz on every render thread). Meaningful on the hardware
//  adapter only: WARP has no outputs, so it reports the SDR answer at once.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT Harness::RunLuminance()
{
    using namespace std::chrono;

    HRESULT                         hr        = S_OK;
    WindowsDisplayLuminanceProvider provider;
    DisplayLuminance                luminance;
    constexpr int                   kQueries  = 200;
    double                          totalUs   = 0.0;
    double                          worstUs   = 0.0;



    CBRAEx (m_renderSystem && m_renderSystem->GetSwapChain(), E_UNEXPECTED);

    for (int i = 0; i < kQueries; ++i)
    {
        const auto start = steady_clock::now();


        luminance = provider.Query (m_renderSystem->GetSwapChain());

        const double us = duration_cast<duration<double, std::micro>> (steady_clock::now() - start).count();


        totalUs  += us;
        worstUs   = std::max (worstUs, us);
    }

    wprintf (L"device            %s\n", luminance.deviceName.c_str());
    wprintf (L"hdrEnabled        %s\n", luminance.hdrEnabled     ? L"true" : L"false");
    wprintf (L"scRgbSupported    %s\n", luminance.scRgbSupported ? L"true" : L"false");
    wprintf (L"sdrWhiteNits      %.1f\n", luminance.sdrWhiteNits);
    wprintf (L"reportedPeakNits  %.1f\n", luminance.reportedPeakNits);
    wprintf (L"effectivePeakNits %.1f\n", EffectivePeakNits (luminance.reportedPeakNits));
    wprintf (L"sdrWhiteScale     %.3f\n", SdrWhiteScale (luminance.sdrWhiteNits));
    wprintf (L"headroom          %.3f\n", Headroom (EffectivePeakNits (luminance.reportedPeakNits), luminance.sdrWhiteNits));
    wprintf (L"selected mode     %s\n", SelectOutputMode (luminance.hdrEnabled, luminance.scRgbSupported, ScreenSaverMode::Normal) == OutputMode::Hdr ? L"Hdr" : L"Sdr");
    wprintf (L"query time        mean %.1f us, worst %.1f us over %d queries\n", totalUs / kQueries, worstUs, kQueries);
    wprintf (L"stale after       %s\n", provider.IsStale() ? L"true" : L"false");

    // The switch itself, both ways, on this adapter and output.
    {
        const HRESULT toHdr = m_renderSystem->ReconfigureOutputMode (OutputMode::Hdr);


        luminance = provider.Query (m_renderSystem->GetSwapChain());

        wprintf (L"switch to HDR     hr=0x%08X, mode now %s, scRgbSupported now %s\n",
                 static_cast<unsigned> (toHdr),
                 m_renderSystem->GetOutputMode() == OutputMode::Hdr ? L"Hdr" : L"Sdr",
                 luminance.scRgbSupported ? L"true" : L"false");

        const HRESULT toSdr = m_renderSystem->ReconfigureOutputMode (OutputMode::Sdr);


        wprintf (L"switch to SDR     hr=0x%08X, mode now %s\n",
                 static_cast<unsigned> (toSdr),
                 m_renderSystem->GetOutputMode() == OutputMode::Hdr ? L"Hdr" : L"Sdr");
    }


Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  ModeName
//
////////////////////////////////////////////////////////////////////////////////

static const wchar_t * ModeName (RunMode mode)
{
    switch (mode)
    {
        case RunMode::Reference: return L"reference";
        case RunMode::Compare:   return L"compare";
        case RunMode::Benchmark: return L"benchmark";
        case RunMode::Luminance: return L"luminance";
    }

    return L"unknown";
}





////////////////////////////////////////////////////////////////////////////////
//
//  WarpVersion
//
//  WARP ships with Windows, so a Windows update can shift its output. Recording
//  the version alongside a baseline turns "why did the diff light up" from an
//  afternoon into a glance.
//
////////////////////////////////////////////////////////////////////////////////

static std::wstring WarpVersion()
{
    const wchar_t * pszModule = L"d3d10warp.dll";
    DWORD           handle    = 0;
    DWORD           size      = GetFileVersionInfoSizeW (pszModule, &handle);
    std::vector<uint8_t> buffer;
    VS_FIXEDFILEINFO   * pInfo  = nullptr;
    UINT                 length = 0;


    if (size == 0)
    {
        return L"unknown";
    }

    buffer.resize (size);

    if (!GetFileVersionInfoW (pszModule, handle, size, buffer.data()))
    {
        return L"unknown";
    }

    if (!VerQueryValueW (buffer.data(), L"\\", reinterpret_cast<LPVOID *> (&pInfo), &length) || !pInfo)
    {
        return L"unknown";
    }

    return std::format (L"{}.{}.{}.{}",
                        HIWORD (pInfo->dwFileVersionMS),
                        LOWORD (pInfo->dwFileVersionMS),
                        HIWORD (pInfo->dwFileVersionLS),
                        LOWORD (pInfo->dwFileVersionLS));
}





////////////////////////////////////////////////////////////////////////////////
//
//  ParseOptions
//
////////////////////////////////////////////////////////////////////////////////

static bool ParseOptions (int argc, wchar_t * argv[], Options & options)
{
    for (int arg = 1; arg < argc; ++arg)
    {
        const std::wstring_view current (argv[arg]);

        if (current == L"--adapter" && arg + 1 < argc)
        {
            const std::wstring_view value (argv[++arg]);

            if (value == L"warp")
            {
                options.m_useWarp = true;
            }
            else if (value == L"hardware")
            {
                options.m_useWarp = false;
            }
            else
            {
                return false;
            }
        }
        else if (current == L"--mode" && arg + 1 < argc)
        {
            const std::wstring_view value (argv[++arg]);

            if (value == L"reference")
            {
                options.m_mode = RunMode::Reference;
            }
            else if (value == L"compare")
            {
                options.m_mode = RunMode::Compare;
            }
            else if (value == L"benchmark")
            {
                options.m_mode = RunMode::Benchmark;
            }
            else if (value == L"luminance")
            {
                options.m_mode = RunMode::Luminance;
            }
            else
            {
                return false;
            }
        }
        else if (current == L"--hdr")
        {
            options.m_hdr = true;
        }
        else if (current == L"--baseline-dir" && arg + 1 < argc)
        {
            options.m_baselineDir = argv[++arg];
        }
        else if (current == L"--frame" && arg + 1 < argc)
        {
            const std::wstring value (argv[++arg]);
            const size_t       cross = value.find (L'x');

            if (cross == std::wstring::npos)
            {
                return false;
            }

            options.m_frameWidth  = static_cast<UINT> (_wtoi (value.substr (0, cross).c_str()));
            options.m_frameHeight = static_cast<UINT> (_wtoi (value.substr (cross + 1).c_str()));

            if (options.m_frameWidth == 0 || options.m_frameHeight == 0)
            {
                return false;
            }
        }
        else if (current == L"--dpi" && arg + 1 < argc)
        {
            const int percent = _wtoi (argv[++arg]);

            if (percent < 100 || percent > 400)
            {
                return false;
            }

            options.m_dpiScale = static_cast<float> (percent) / 100.0f;
        }
        else
        {
            return false;
        }
    }

    return true;
}





////////////////////////////////////////////////////////////////////////////////
//
//  wmain
//
////////////////////////////////////////////////////////////////////////////////

int wmain (int argc, wchar_t * argv[])
{
    HRESULT hr      = S_OK;
    Options options;
    Harness harness;


    if (!ParseOptions (argc, argv, options))
    {
        wprintf (L"usage: HdrCalibration [--adapter warp|hardware]"
                 L" [--mode reference|compare|benchmark|luminance] [--baseline-dir <path>]"
                 L" [--frame <W>x<H>] [--dpi <percent>] [--hdr]\n"
                 L"       --frame, --dpi and --hdr apply to benchmark mode only\n"
                 L"       --hdr presents scRGB; it needs the window on a monitor with Windows HDR on\n");
        return 1;
    }

    //  Refused rather than ignored: a baseline captured at a different size or
    //  scale would compare against nothing, and silently producing frames that
    //  look right but match no committed baseline is the worse failure.
    if (options.m_mode != RunMode::Benchmark
        && (options.m_frameWidth  != kFrameWidth
            || options.m_frameHeight != kFrameHeight
            || options.m_dpiScale    != kDpiScales[0]
            || options.m_hdr))
    {
        wprintf (L"--frame and --dpi apply to benchmark mode only;"
                 L" reference and compare sweep fixed sizes and scales\n");
        return 1;
    }

    hr = CoInitializeEx (nullptr, COINIT_APARTMENTTHREADED);

    if (FAILED (hr))
    {
        wprintf (L"CoInitializeEx failed: 0x%08X\n", static_cast<unsigned> (hr));
        return 1;
    }

    //  The header is part of the record: a baseline is only comparable against
    //  a run made on the same adapter, and WARP ships with Windows and changes
    //  with it, so its version belongs next to the numbers.
    wprintf (L"# adapter=%s mode=%s frame=%ux%u dpi=%d%% seed=%u warp=%s\n",
             options.m_useWarp ? L"warp" : L"hardware",
             ModeName (options.m_mode),
             options.m_frameWidth,
             options.m_frameHeight,
             DpiPercent (options.m_dpiScale),
             kReferenceSeed,
             WarpVersion().c_str());

    hr = harness.Initialize (options.m_useWarp, options.m_frameWidth, options.m_frameHeight);

    if (FAILED (hr))
    {
        wprintf (L"initialization failed: 0x%08X\n", static_cast<unsigned> (hr));
        harness.Shutdown();
        CoUninitialize();
        return 1;
    }

    switch (options.m_mode)
    {
        case RunMode::Reference:
            hr = harness.RunReference (options.m_baselineDir);
            break;

        case RunMode::Compare:
            hr = harness.RunCompare (options.m_baselineDir);
            break;

        case RunMode::Benchmark:
            hr = harness.RunBenchmark (options.m_dpiScale, options.m_hdr);
            break;

        case RunMode::Luminance:
            hr = harness.RunLuminance();
            break;
    }

    harness.Shutdown();
    CoUninitialize();

    if (FAILED (hr))
    {
        wprintf (L"run failed: 0x%08X\n", static_cast<unsigned> (hr));
        return 1;
    }

    return 0;
}
