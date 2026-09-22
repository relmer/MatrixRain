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

//  The seed every run starts from. Any fixed value would do; what matters is
//  that it never changes, or reference numbers stop being comparable.
static constexpr uint32_t kReferenceSeed   = 1;

//  Rain is drawn on a black field, so the whole frame's mean luminance is a
//  small number; printing six decimals keeps the 5% calibration threshold
//  (SC-001) readable.
static constexpr int      kPrintPrecision  = 6;





////////////////////////////////////////////////////////////////////////////////
//
//  SettingsCase
//
//  One named point in the settings space that calibration must preserve.
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
//  scanlines at both ends of the Style range, and a custom colour, so a
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

struct Options
{
    bool m_useWarp   = true;
    bool m_benchmark = false;
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
    HRESULT Initialize (bool useWarp);
    void    Shutdown();

    HRESULT RunReference();
    HRESULT RunBenchmark();

private:
    HRESULT CreateHiddenWindow();
    HRESULT ReadBackFrame (std::vector<uint8_t> & frame);
    void    ApplyCase     (const SettingsCase & settingsCase, RenderParams & params);
    void    RunFrames     (int frameCount);
    void    IsolateOneHead();
    HRESULT RenderOnce    (const RenderParams & params);

    static std::optional<LUID> FindWarpAdapterLuid();

    HWND                               m_hwnd = nullptr;
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
                              static_cast<int> (kFrameWidth),
                              static_cast<int> (kFrameHeight),
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

HRESULT Harness::Initialize (bool useWarp)
{
    HRESULT             hr          = S_OK;
    std::optional<LUID> adapterLuid = std::nullopt;
    CharacterSet      & charSet     = CharacterSet::GetInstance();


    if (useWarp)
    {
        adapterLuid = FindWarpAdapterLuid();
        CBRAExL (adapterLuid.has_value(), E_FAIL, L"WARP adapter not available");
    }

    hr = CreateHiddenWindow();
    CHR (hr);

    m_viewport = std::make_unique<Viewport>();
    m_viewport->Resize (static_cast<float> (kFrameWidth), static_cast<float> (kFrameHeight));

    m_densityController = std::make_unique<DensityController> (*m_viewport, 24.0f);
    m_animationSystem   = std::make_unique<AnimationSystem>();
    m_renderSystem      = std::make_unique<RenderSystem>();

    hr = m_renderSystem->Initialize (m_hwnd, kFrameWidth, kFrameHeight, adapterLuid);
    CHR (hr);

    CBRAExL (charSet.Initialize(), E_FAIL, L"CharacterSet::Initialize failed");

    m_animationSystem->Initialize (*m_viewport, *m_densityController);

    hr = m_renderSystem->BuildGlyphAtlas();
    CHR (hr);

    //  Pin the glyph size: the run must not depend on the DPI of whatever
    //  monitor the harness happens to launch on.
    m_renderSystem->SetCharacterScaleOverride (1.0f);
    m_animationSystem->SetDpiScale (1.0f);
    m_densityController->SetDpiScale (1.0f);

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
//  Harness::IsolateOneHead
//
//  Leaves exactly one streak, one character long, parked in the middle of the
//  frame. A halo radius measured on a full field of rain would be the radius
//  of whatever the neighbouring streaks happen to add; measured on a lone head
//  it is the radius of the glow itself.
//
////////////////////////////////////////////////////////////////////////////////

void Harness::IsolateOneHead()
{
    m_animationSystem->ClearAllStreaks();
    m_animationSystem->SpawnStreak();

    //  One step past the drop interval gives the streak its first character
    //  and no trail behind it.
    m_animationSystem->Update (0.31f);

    const std::vector<CharacterStreak> & streaks = m_animationSystem->GetStreaks();

    if (!streaks.empty())
    {
        CharacterStreak * pStreak = const_cast<CharacterStreak *> (&streaks[0]);

        pStreak->SetPosition (Vector3 (static_cast<float> (kFrameWidth)  / 2.0f,
                                       static_cast<float> (kFrameHeight) / 2.0f,
                                       0.0f));
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
//  Harness::RunReference
//
//  Prints one row per settings case: the mean luminance of a full field of
//  rain, and the half-maximum radius of the glow around a single isolated
//  head. Comparing two runs of this is what "the look is unchanged" means in
//  numbers (FR-006).
//
////////////////////////////////////////////////////////////////////////////////

HRESULT Harness::RunReference()
{
    HRESULT              hr = S_OK;
    std::vector<uint8_t> frame;


    wprintf (L"case,meanLuminance,haloRadiusPx,headX,headY,headLuminance\n");

    for (const SettingsCase & settingsCase : s_krgCases)
    {
        RenderParams params;
        float        meanLuminance = 0.0f;
        float        haloRadius    = 0.0f;
        float        headLuminance = 0.0f;
        POINT        head          = {};


        ApplyCase (settingsCase, params);

        //  Field frame: every streak in place, for the exposure number.
        RandomSource::Reseed (kReferenceSeed);
        m_animationSystem->ClearAllStreaks();
        RunFrames (kWarmupFrames);

        hr = RenderOnce (params);
        CHR (hr);

        hr = ReadBackFrame (frame);
        CHR (hr);

        meanLuminance = MeanLuminance (frame, kFrameWidth, kFrameHeight);

        //  Lone-head frame: for the glow's width.
        RandomSource::Reseed (kReferenceSeed);
        IsolateOneHead();

        hr = RenderOnce (params);
        CHR (hr);

        hr = ReadBackFrame (frame);
        CHR (hr);

        //  The head is wherever the renderer actually put it, which is not
        //  the streak's own coordinate: the glyph quad is drawn around it and
        //  the ink sits somewhere inside it. Measuring from the brightest
        //  pixel is both simpler and less likely to drift than re-deriving
        //  that.
        head       = BrightestPixel    (frame, kFrameWidth, kFrameHeight, &headLuminance);
        haloRadius = HaloFalloffRadius (frame, kFrameWidth, kFrameHeight, head);

        wprintf (L"%s,%.*f,%.*f,%ld,%ld,%.*f\n",
                 settingsCase.m_pszName,
                 kPrintPrecision,
                 meanLuminance,
                 kPrintPrecision,
                 haloRadius,
                 head.x,
                 head.y,
                 kPrintPrecision,
                 headLuminance);
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
////////////////////////////////////////////////////////////////////////////////

HRESULT Harness::RunBenchmark()
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

    wprintf (L"preset,frames,meanGpuMs,p95GpuMs\n");

    for (QualityPreset preset : s_krgPresets)
    {
        const AdvancedGraphicsValues values = LookupPresetValues (preset);
        RenderParams                 params;
        std::vector<float>           timings;


        ApplyCase (s_krgCases[0], params);

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

            hr = m_renderSystem->Present();
            CHR (hr);

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
            wprintf (L"%d,0,,\n", static_cast<int> (preset));
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

            wprintf (L"%d,%zu,%.3f,%.3f\n",
                     static_cast<int> (preset),
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
                options.m_benchmark = false;
            }
            else if (value == L"benchmark")
            {
                options.m_benchmark = true;
            }
            else
            {
                return false;
            }
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
        wprintf (L"usage: HdrCalibration [--adapter warp|hardware] [--mode reference|benchmark]\n");
        return 1;
    }

    wprintf (L"# adapter=%s mode=%s frame=%ux%u seed=%u\n",
             options.m_useWarp ? L"warp" : L"hardware",
             options.m_benchmark ? L"benchmark" : L"reference",
             kFrameWidth,
             kFrameHeight,
             kReferenceSeed);

    hr = harness.Initialize (options.m_useWarp);

    if (FAILED (hr))
    {
        wprintf (L"initialization failed: 0x%08X\n", static_cast<unsigned> (hr));
        harness.Shutdown();
        return 1;
    }

    hr = options.m_benchmark ? harness.RunBenchmark() : harness.RunReference();

    harness.Shutdown();

    if (FAILED (hr))
    {
        wprintf (L"run failed: 0x%08X\n", static_cast<unsigned> (hr));
        return 1;
    }

    return 0;
}
