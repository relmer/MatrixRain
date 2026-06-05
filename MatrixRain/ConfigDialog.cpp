#include "pch.h"

#include <commctrl.h>
#include <prsht.h>
#pragma comment(lib, "comctl32.lib")

#include "ConfigDialog.h"
#include "..\MatrixRainCore\AdapterSelection.h"
#include "..\MatrixRainCore\Application.h"
#include "..\MatrixRainCore\ApplicationState.h"
#include "..\MatrixRainCore\ConfigDialogController.h"
#include "..\MatrixRainCore\CommandLine.h"
#include "..\MatrixRainCore\MonitorRenderContext.h"
#include "..\MatrixRainCore\RenderSystem.h"
#include "..\MatrixRainCore\WindowsAdapterProvider.h"
#include "resource.h"





struct DialogContext
{
    std::unique_ptr<ConfigDialogController>   m_controller;
    RegistrySettingsProvider                  m_settingsProvider;
    Application                             * m_pApp              = nullptr;
    bool                                      m_ownsContextMemory = false;
    bool                                      m_isScreenSaverCPL  = false;

    // Parallel to IDC_GPU_COMBO entries: each index holds the underlying
    // DXGI adapter description so OnGpuChange can map a selection back to
    // the persistence string.  The OS default adapter is annotated with
    // " (default)" in its display label; no synthetic <system default>
    // entry is added (the user picks the default adapter by name directly).
    std::vector<std::wstring>                 m_gpuAdapterDescriptions;

    // T058/T059 - per-page tooltip control and shared TTF_TRACK tool whose
    // text is updated on each info-button BN_CLICKED.  Stored on the page
    // HWND via SetPropW(kPageTooltipProp) so OnInfoButtonClick / DismissInfoTip
    // can look it up without knowing the page index.

    // Larger font used to render the info-tip ⓘ glyph at 1.5x the dialog's
    // default font size in the BS_OWNERDRAW button paint path.  Created on
    // the first page WM_INITDIALOG; destroyed on sheet WM_DESTROY.
    HFONT                                     m_hInfoTipFont        = nullptr;

    // v1.5 (T029, T032, T033, contracts/propertysheet.md): frame state.
    // m_hSheet is the property-sheet frame HWND (returned by PropertySheetW
    // in modeless mode); both page procs and the title-timer reach it via
    // GetParent or via the SetProp/GetProp keyed to kSheetContextProp.
    // m_applied / m_rolledBack make the per-page PSN_APPLY / PSN_RESET
    // notifications idempotent — only the first page to see each one
    // performs the commit/rollback work.
    HWND                                      m_hSheet              = nullptr;
    bool                                      m_applied             = false;
    bool                                      m_rolledBack          = false;
    WCHAR                                     m_perfTitleFormat[64] = {};
};


// v1.5 (T029): property name used to ferry DialogContext * from the
// property-sheet PSCB_INITIALIZED callback into the per-tick TimerProc
// and the page procs.  Stored on the frame HWND.
static constexpr const wchar_t kSheetContextProp[] = L"MatrixRainSheetCtx";

// v1.5 (T029): per-page tooltip stored on each page HWND so OnInfoButton*
// helpers can reach it without knowing which page they're running on.
static constexpr const wchar_t kPageTooltipProp[]  = L"MatrixRainPageToolt";

// v1.5 (T032): 1 Hz title-update timer ID (frame-scoped).
static constexpr UINT_PTR      kPerfTitleTimerId   = IDT_PERF_TITLE_TIMER;


// Sentinel uId for the single TTF_TRACK tool used by keyboard activation.
static constexpr UINT_PTR kTrackTipUId = 0xC0C00C0Cu;

// Subclass id for the info-button mouse-event relay (TTF_SUBCLASS proved
// unreliable on BS_OWNERDRAW buttons; we explicitly forward mouse events
// to the tooltip via TTM_RELAYEVENT instead).
static constexpr UINT_PTR kInfoButtonSubclassId = 0xC0C00C1Bu;




static LRESULT CALLBACK InfoButtonSubclassProc (HWND     hWnd,
                                                 UINT     msg,
                                                 WPARAM   wParam,
                                                 LPARAM   lParam,
                                                 UINT_PTR uIdSubclass,
                                                 DWORD_PTR dwRefData)
{
    HWND hTooltip = reinterpret_cast<HWND> (dwRefData);


    if (hTooltip)
    {
        switch (msg)
        {
            case WM_MOUSEMOVE:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            {
                MSG ttMsg = {};
                ttMsg.hwnd    = hWnd;
                ttMsg.message = msg;
                ttMsg.wParam  = wParam;
                ttMsg.lParam  = lParam;
                SendMessageW (hTooltip, TTM_RELAYEVENT, 0, reinterpret_cast<LPARAM> (&ttMsg));
                break;
            }
        }
    }

    if (msg == WM_NCDESTROY)
    {
        RemoveWindowSubclass (hWnd, InfoButtonSubclassProc, uIdSubclass);
    }

    return DefSubclassProc (hWnd, msg, wParam, lParam);
}





static DialogContext * GetDialogContext (HWND hDlg)
{
    return reinterpret_cast<DialogContext *> (GetWindowLongPtr (hDlg, DWLP_USER));
}





static ConfigDialogController * GetControllerFromDialog (HWND hDlg)
{
    ConfigDialogController * pController = nullptr;
    DialogContext          * pContext    = GetDialogContext (hDlg);



    if (pContext)
    {
        pController = pContext->m_controller.get();
    }

    return pController;
}





static Application * GetApplicationFromDialog (HWND hDlg)
{
    Application   * pApp     = nullptr;
    DialogContext * pContext = GetDialogContext (hDlg);



    if (pContext)
    {
        pApp = pContext->m_pApp;
    }

    return pApp;
}





////////////////////////////////////////////////////////////////////////////////
//
//  GetInfoTipText — locked infotip strings (per the spec contract,
//  FR-036: descriptive sentence + standardized perf-impact sentence).
//
////////////////////////////////////////////////////////////////////////////////

static const wchar_t * GetInfoTipText (int infoId)
{
    switch (infoId)
    {
        case IDC_QUALITY_PRESET_INFO:
            return L"Picks a graphics quality preset. Higher presets look richer but "
                   L"use more GPU. Custom lets you tune the individual settings below.\r\n"
                   L"\r\n"
                   L"Significant GPU performance impact.";

        case IDC_GLOWINTENSITY_INFO:
            return L"Brightness of the glow effect around bright characters. Setting "
                   L"this to 0% disables the glow effect entirely.\r\n"
                   L"\r\n"
                   L"Significant GPU performance impact.";

        case IDC_GLOWSIZE_INFO:
            return L"Width of the glow halo around bright characters.";

        case IDC_GLOWPASSES_INFO:
            return L"How many times the glow is blurred. Each pass roughly doubles the "
                   L"glow's width.\r\n"
                   L"\r\n"
                   L"Significant GPU performance impact.";

        case IDC_GLOWRES_INFO:
            return L"Resolution the glow is computed at. Lower is much cheaper and only "
                   L"slightly softer; Quarter is about 4x cheaper than Full.\r\n"
                   L"\r\n"
                   L"Significant GPU performance impact.";

        case IDC_GLOWSMOOTH_INFO:
            return L"Number of samples per blur step. Higher gives smoother gradients "
                   L"with no banding.\r\n"
                   L"\r\n"
                   L"Moderate GPU performance impact.";

        default:
            return L"";
    }
}




static bool IsInfoTipControlId (int id)
{
    switch (id)
    {
        case IDC_QUALITY_PRESET_INFO:
        case IDC_GLOWINTENSITY_INFO:
        case IDC_GLOWSIZE_INFO:
        case IDC_GLOWPASSES_INFO:
        case IDC_GLOWRES_INFO:
        case IDC_GLOWSMOOTH_INFO:
            return true;
        default:
            return false;
    }
}




static int TickFrequencyForSliderId (int id)
{
    switch (id)
    {
        case IDC_DENSITY_SLIDER:        return 5;
        case IDC_ANIMSPEED_SLIDER:      return 5;
        case IDC_GLOWINTENSITY_SLIDER:  return 10;
        case IDC_GLOWSIZE_SLIDER:       return 10;  // 50..200 -> 16 ticks (midpoint at 125)
        case IDC_QUALITY_PRESET_SLIDER: return 1;
        case IDC_GLOWPASSES_SLIDER:     return 1;
        case IDC_GLOWRES_SLIDER:        return 1;
        case IDC_GLOWSMOOTH_SLIDER:     return 1;
        default:                        return 1;
    }
}




////////////////////////////////////////////////////////////////////////////////
//
//  CreateAndRegisterTooltip
//
//  Creates a shared WC_TOOLTIPS window for the config dialog and registers
//  every IDC_*_INFO button as a tool with TTF_IDISHWND | TTF_SUBCLASS, with
//  per-tool text supplied via TTN_GETDISPINFO (handled in ConfigDialogProc).
//
////////////////////////////////////////////////////////////////////////////////

static HWND CreateAndRegisterTooltip (HWND hDlg)
{
    static const int kInfoIds[] =
    {
        IDC_QUALITY_PRESET_INFO,
        IDC_GLOWINTENSITY_INFO,
        IDC_GLOWSIZE_INFO,
        IDC_GLOWPASSES_INFO,
        IDC_GLOWRES_INFO,
        IDC_GLOWSMOOTH_INFO,
    };


    HWND hTooltip = CreateWindowExW (WS_EX_TOPMOST,
                                     TOOLTIPS_CLASS,
                                     nullptr,
                                     WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     hDlg, nullptr, nullptr, nullptr);

    if (!hTooltip)
    {
        return nullptr;
    }

    SendMessageW (hTooltip, TTM_SETMAXTIPWIDTH, 0, 300);
    SendMessageW (hTooltip, TTM_ACTIVATE,       TRUE, 0);


    for (int infoId : kInfoIds)
    {
        HWND hCtrl = GetDlgItem (hDlg, infoId);

        if (!hCtrl)
        {
            continue;
        }

        TOOLINFOW ti = { sizeof (TOOLINFOW) };
        ti.uFlags    = TTF_IDISHWND;
        ti.hwnd      = hDlg;
        ti.uId       = (UINT_PTR) hCtrl;
        ti.lpszText  = LPSTR_TEXTCALLBACKW;

        SendMessageW (hTooltip, TTM_ADDTOOLW, 0, (LPARAM) &ti);

        // Manually subclass the button to forward mouse events to the
        // tooltip — TTF_SUBCLASS proved unreliable on BS_OWNERDRAW buttons.
        SetWindowSubclass (hCtrl,
                           InfoButtonSubclassProc,
                           kInfoButtonSubclassId,
                           reinterpret_cast<DWORD_PTR> (hTooltip));
    }


    // Single TTF_TRACK tool used for keyboard-activated tips (T059).  Its
    // text is updated on each info-button BN_CLICKED before we call
    // TTM_TRACKPOSITION + TTM_TRACKACTIVATE.
    TOOLINFOW trackTool = { sizeof (TOOLINFOW) };
    trackTool.uFlags    = TTF_TRACK | TTF_ABSOLUTE;
    trackTool.hwnd      = hDlg;
    trackTool.uId       = kTrackTipUId;
    trackTool.lpszText  = const_cast<LPWSTR> (L"");

    SendMessageW (hTooltip, TTM_ADDTOOLW, 0, (LPARAM) &trackTool);

    return hTooltip;
}




////////////////////////////////////////////////////////////////////////////////
//
//  OnInfoButtonClick — keyboard-activated tooltip for an IDC_*_INFO button
//  (T059).  Updates the shared TTF_TRACK tool's text to the matching infotip
//  string, positions it just below/right of the button, and activates it.
//  Auto-dismisses on a 5-second timer (handled in ConfigDialogProc).
//
////////////////////////////////////////////////////////////////////////////////

static constexpr UINT_PTR kInfoTipDismissTimerId = 0xC0C0C0DEu;

static void OnInfoButtonClick (HWND hDlg, int infoId)
{
    HWND hTooltip = static_cast<HWND> (GetPropW (hDlg, kPageTooltipProp));

    if (!hTooltip)
    {
        return;
    }

    HWND hButton = GetDlgItem (hDlg, infoId);

    if (!hButton)
    {
        return;
    }

    RECT btnRect;
    GetWindowRect (hButton, &btnRect);


    // Update the TTF_TRACK tool's text and position.
    TOOLINFOW ti = { sizeof (TOOLINFOW) };
    ti.hwnd      = hDlg;
    ti.uId       = kTrackTipUId;
    ti.lpszText  = const_cast<LPWSTR> (GetInfoTipText (infoId));

    SendMessageW (hTooltip, TTM_UPDATETIPTEXTW, 0, (LPARAM) &ti);

    SendMessageW (hTooltip,
                  TTM_TRACKPOSITION,
                  0,
                  MAKELPARAM (btnRect.right + 4, btnRect.bottom + 2));

    SendMessageW (hTooltip, TTM_TRACKACTIVATE, TRUE, (LPARAM) &ti);

    SetTimer (hDlg, kInfoTipDismissTimerId, 5000, nullptr);
}




static void DismissInfoTip (HWND hDlg)
{
    HWND hTooltip = static_cast<HWND> (GetPropW (hDlg, kPageTooltipProp));

    if (!hTooltip)
    {
        return;
    }

    TOOLINFOW ti = { sizeof (TOOLINFOW) };
    ti.hwnd      = hDlg;
    ti.uId       = kTrackTipUId;

    SendMessageW (hTooltip, TTM_TRACKACTIVATE, FALSE, (LPARAM) &ti);
    KillTimer (hDlg, kInfoTipDismissTimerId);
}




static std::wstring FormatPercentLabel (int sliderId, int value)
{
    // T041 (US2, FR-007): removed the legacy "0% (glow disabled)" branch.
    // Glow on/off now lives on IDC_GLOW_ENABLED_CHECK; the slider min is
    // back to 1 (see ScreenSaverSettings::MIN_GLOW_INTENSITY_PERCENT).
    (void) sliderId;
    return std::format (L"{}%", value);
}





////////////////////////////////////////////////////////////////////////////////
//
//  ApplyGlowEnabledUI (T042, FR-016, FR-017, research.md R7)
//
//  Mirror m_glowEnabled into EnableWindow on every glow-dependent control
//  across both property-sheet pages.  Cross-tab reach is safe because
//  PSP_PREMATURE forces both page HWNDs to exist immediately after
//  PropertySheetW returns — see plan.md "Property-sheet flags".
//
//  Per spec the Visuals-tab Glow Intensity / Glow Size trios disable, plus
//  the Performance-tab Quality Preset / Glow Passes / Glow Resolution /
//  Glow Smoothness trios.  Static "Glow intensity:" / "Glow size:" prompt
//  labels are IDC_STATIC (no individual ID) so they can't be greyed
//  programmatically; the value labels and info buttons carry the visual
//  cue instead.
//
//  T043 (per-tab tooltip on disabled controls) is deferred — Windows
//  tooltips don't fire on WS_DISABLED controls without a transparent
//  parent-relay tooltip per page, and the greyed-out controls already
//  convey "disabled" clearly.  Tracked as follow-up.
//
////////////////////////////////////////////////////////////////////////////////

static void ApplyGlowEnabledUI (HWND hSheet, bool enabled)
{
    if (!hSheet)
    {
        return;
    }


    HWND hVisuals = PropSheet_IndexToHwnd (hSheet, 0);
    HWND hPerf    = PropSheet_IndexToHwnd (hSheet, 1);

    auto enableIfPresent = [enabled] (HWND hPage, int id)
    {
        if (hPage)
        {
            HWND hCtrl = GetDlgItem (hPage, id);

            if (hCtrl)
            {
                EnableWindow (hCtrl, enabled);
            }
        }
    };


    // Visuals tab — Glow Intensity + Glow Size trios.
    enableIfPresent (hVisuals, IDC_GLOWINTENSITY_SLIDER);
    enableIfPresent (hVisuals, IDC_GLOWINTENSITY_LABEL);
    enableIfPresent (hVisuals, IDC_GLOWINTENSITY_INFO);
    enableIfPresent (hVisuals, IDC_GLOWSIZE_SLIDER);
    enableIfPresent (hVisuals, IDC_GLOWSIZE_LABEL);
    enableIfPresent (hVisuals, IDC_GLOWSIZE_INFO);

    // Performance tab — Quality Preset + Glow Passes/Resolution/Smoothness trios.
    enableIfPresent (hPerf,    IDC_QUALITY_PRESET_SLIDER);
    enableIfPresent (hPerf,    IDC_QUALITY_PRESET_LABEL);
    enableIfPresent (hPerf,    IDC_QUALITY_PRESET_INFO);
    enableIfPresent (hPerf,    IDC_GLOWPASSES_SLIDER);
    enableIfPresent (hPerf,    IDC_GLOWPASSES_LABEL);
    enableIfPresent (hPerf,    IDC_GLOWPASSES_INFO);
    enableIfPresent (hPerf,    IDC_GLOWPASSES_PROMPT);
    enableIfPresent (hPerf,    IDC_GLOWRES_SLIDER);
    enableIfPresent (hPerf,    IDC_GLOWRES_LABEL);
    enableIfPresent (hPerf,    IDC_GLOWRES_INFO);
    enableIfPresent (hPerf,    IDC_GLOWRES_PROMPT);
    enableIfPresent (hPerf,    IDC_GLOWSMOOTH_SLIDER);
    enableIfPresent (hPerf,    IDC_GLOWSMOOTH_LABEL);
    enableIfPresent (hPerf,    IDC_GLOWSMOOTH_INFO);
    enableIfPresent (hPerf,    IDC_GLOWSMOOTH_PROMPT);
}




static const wchar_t * FormatResolutionLabel (int divisor)
{
    switch (divisor)
    {
        case 1: return L"Full";
        case 2: return L"Half";
        case 4: return L"Quarter";
        case 8: return L"Eighth";
        default: return L"Half";
    }
}




static const wchar_t * FormatSmoothnessLabel (int taps)
{
    switch (taps)
    {
        case 5:  return L"Low";
        case 9:  return L"Medium";
        case 13: return L"High";
        default: return L"High";
    }
}




static const wchar_t * FormatQualityPresetLabel (QualityPreset preset)
{
    switch (preset)
    {
        case QualityPreset::Low:    return L"Low";
        case QualityPreset::Medium: return L"Medium";
        case QualityPreset::High:   return L"High";
        case QualityPreset::Custom: return L"Custom";
        default:                    return L"Custom";
    }
}




////////////////////////////////////////////////////////////////////////////////
//
//  InitializeSlider
//
////////////////////////////////////////////////////////////////////////////////

static void InitializeSlider (HWND hDlg, int sliderId, int labelId, int minValue, int maxValue, int currentValue)
{
    SendDlgItemMessageW (hDlg, sliderId, TBM_SETRANGE,    TRUE, MAKELPARAM (minValue, maxValue));
    SendDlgItemMessageW (hDlg, sliderId, TBM_SETTICFREQ,  TickFrequencyForSliderId (sliderId), 0);

    // Speed (1..100) at freq=5 lands the last tick at 96; add an explicit
    // tick at 100 for the documented 21-tick total.
    if (sliderId == IDC_ANIMSPEED_SLIDER)
    {
        SendDlgItemMessageW (hDlg, sliderId, TBM_SETTIC, 0, 100);
    }

    SendDlgItemMessageW (hDlg, sliderId, TBM_SETPOS, TRUE, currentValue);
    SetDlgItemTextW     (hDlg, labelId, FormatPercentLabel (sliderId, currentValue).c_str());
}




////////////////////////////////////////////////////////////////////////////////
//
//  Discrete-slider initializers (Passes / Resolution / Smoothness).  These
//  use the same trackbar control but with small integer ranges and mapped
//  labels rather than percentages.
//
////////////////////////////////////////////////////////////////////////////////

static void InitializePassesSlider (HWND hDlg, int currentPasses)
{
    SendDlgItemMessageW (hDlg, IDC_GLOWPASSES_SLIDER, TBM_SETRANGE, TRUE, MAKELPARAM (1, 4));
    SendDlgItemMessageW (hDlg, IDC_GLOWPASSES_SLIDER, TBM_SETTICFREQ, 1, 0);
    SendDlgItemMessageW (hDlg, IDC_GLOWPASSES_SLIDER, TBM_SETPOS,   TRUE, currentPasses);
    SetDlgItemTextW     (hDlg, IDC_GLOWPASSES_LABEL, std::format (L"{}", currentPasses).c_str());
}


static void InitializeResolutionSlider (HWND hDlg, int currentDivisor)
{
    // Slider position 0..2 maps to divisor 4/2/1 (Quarter/Half/Full).
    // Eighth (divisor 8) is no longer offered through the UI; if a saved
    // configuration still has it, fall back to Quarter on display.
    int pos = 1;  // default Half

    switch (currentDivisor)
    {
        case 8: pos = 0; break;   // Eighth folds onto Quarter for display
        case 4: pos = 0; break;
        case 2: pos = 1; break;
        case 1: pos = 2; break;
    }

    SendDlgItemMessageW (hDlg, IDC_GLOWRES_SLIDER, TBM_SETRANGE, TRUE, MAKELPARAM (0, 2));
    SendDlgItemMessageW (hDlg, IDC_GLOWRES_SLIDER, TBM_SETTICFREQ, 1, 0);
    SendDlgItemMessageW (hDlg, IDC_GLOWRES_SLIDER, TBM_SETPOS,   TRUE, pos);
    SetDlgItemTextW     (hDlg, IDC_GLOWRES_LABEL, FormatResolutionLabel (currentDivisor));
}


static void InitializeSmoothnessSlider (HWND hDlg, int currentTaps)
{
    int pos = 2;  // default High

    switch (currentTaps)
    {
        case 5:  pos = 0; break;
        case 9:  pos = 1; break;
        case 13: pos = 2; break;
    }

    SendDlgItemMessageW (hDlg, IDC_GLOWSMOOTH_SLIDER, TBM_SETRANGE, TRUE, MAKELPARAM (0, 2));
    SendDlgItemMessageW (hDlg, IDC_GLOWSMOOTH_SLIDER, TBM_SETTICFREQ, 1, 0);
    SendDlgItemMessageW (hDlg, IDC_GLOWSMOOTH_SLIDER, TBM_SETPOS,   TRUE, pos);
    SetDlgItemTextW     (hDlg, IDC_GLOWSMOOTH_LABEL, FormatSmoothnessLabel (currentTaps));
}


static int ResolutionSliderPosToDivisor (int pos)
{
    switch (pos)
    {
        case 0: return 4;
        case 1: return 2;
        case 2: return 1;
        default: return 2;
    }
}


static int SmoothnessSliderPosToTaps (int pos)
{
    switch (pos)
    {
        case 0: return 5;
        case 1: return 9;
        case 2: return 13;
        default: return 13;
    }
}





// Color scheme combo entries: capitalized display label paired with the
// lowercase persisted key.  Index order MUST stay in sync between population,
// selection, and read-back so the UI label never leaks into storage/enums.
struct ColorSchemeEntry
{
    const WCHAR * key;
    const WCHAR * label;
};




static constexpr ColorSchemeEntry s_colorSchemeEntries[] =
{
    { L"green", L"Green" },
    { L"blue",  L"Blue"  },
    { L"red",   L"Red"   },
    { L"amber", L"Amber" },
    { L"cycle", L"Cycle" },
};




////////////////////////////////////////////////////////////////////////////////
//
//  ColorSchemeKeyToIndex
//
////////////////////////////////////////////////////////////////////////////////

static int ColorSchemeKeyToIndex (const std::wstring & key)
{
    for (int i = 0; i < static_cast<int> (ARRAYSIZE (s_colorSchemeEntries)); i++)
    {
        if (key == s_colorSchemeEntries[i].key)
            return i;
    }

    return 0;
}




////////////////////////////////////////////////////////////////////////////////
//
//  InitializeColorSchemeCombo
//
////////////////////////////////////////////////////////////////////////////////

static void InitializeColorSchemeCombo (HWND hDlg, const std::wstring & currentScheme)
{
    for (const ColorSchemeEntry & entry : s_colorSchemeEntries)
    {
        SendDlgItemMessageW (hDlg, IDC_COLORSCHEME_COMBO, CB_ADDSTRING, 0, (LPARAM) entry.label);
    }

    SendDlgItemMessageW (hDlg, IDC_COLORSCHEME_COMBO, CB_SETCURSEL, ColorSchemeKeyToIndex (currentScheme), 0);
}





////////////////////////////////////////////////////////////////////////////////
//
//  InitializeGpuCombo
//
//  Enumerate the system's rendering adapters via WindowsAdapterProvider and
//  populate IDC_GPU_COMBO with their real names; the OS default adapter is
//  annotated with " (default)" via FormatAdapterLabel.  The user picks the
//  default adapter by name directly (no synthetic <system default> entry).
//
//  Selection logic:
//   - currentDescription matches an enumerated adapter -> select that entry.
//   - currentDescription is empty or doesn't match -> select the (default)
//     adapter so the UI still highlights what is actually running.
//
////////////////////////////////////////////////////////////////////////////////

static void InitializeGpuCombo (HWND hDlg, DialogContext * pContext, const std::wstring & currentDescription)
{
    WindowsAdapterProvider   provider;
    std::vector<AdapterInfo> adapters       = provider.EnumerateAdapters();
    int                      selected       = -1;
    int                      defaultIndex   = -1;


    pContext->m_gpuAdapterDescriptions.clear();

    for (const AdapterInfo & adapter : adapters)
    {
        std::wstring label = FormatAdapterLabel (adapter);

        SendDlgItemMessageW (hDlg, IDC_GPU_COMBO, CB_ADDSTRING, 0, (LPARAM) label.c_str());

        pContext->m_gpuAdapterDescriptions.push_back (adapter.m_description);

        int idx = static_cast<int> (pContext->m_gpuAdapterDescriptions.size()) - 1;

        if (adapter.m_isDefault)
        {
            defaultIndex = idx;
        }

        if (!currentDescription.empty() && adapter.m_description == currentDescription)
        {
            selected = idx;
        }
    }

    if (selected < 0)
    {
        selected = defaultIndex;
    }

    if (selected >= 0)
    {
        SendDlgItemMessageW (hDlg, IDC_GPU_COMBO, CB_SETCURSEL, selected, 0);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  OnInitDialog
//
////////////////////////////////////////////////////////////////////////////////

static BOOL OnInitDialog (HWND hDlg, LPARAM initParam)
{
    HRESULT                     hr           = S_OK;
    BOOL                        fSuccess     = FALSE;
    DialogContext             * pContext     = nullptr;
    const ScreenSaverSettings * pSettings    = nullptr;
    PROPSHEETPAGEW            * pPsp         = reinterpret_cast<PROPSHEETPAGEW *> (initParam);


    // v1.5 (T029): the page proc receives the PROPSHEETPAGE pointer in
    // WM_INITDIALOG's lParam.  We tucked the DialogContext * into psp->lParam
    // at sheet build time (see BuildPropSheetHeader).
    CBRAEx (pPsp != nullptr, E_UNEXPECTED);

    pContext = reinterpret_cast<DialogContext *> (pPsp->lParam);
    CBRAEx (pContext != nullptr && pContext->m_controller != nullptr, E_UNEXPECTED);

    SetWindowLongPtr (hDlg, DWLP_USER, reinterpret_cast<LONG_PTR> (pContext));

    // v1.5 (T029): centering is owned by the property-sheet frame's
    // PSCB_INITIALIZED callback (the frame is what the user sees moving),
    // not by individual pages.

    pSettings = &pContext->m_controller->GetSettings();
    
    InitializeSlider (hDlg, IDC_DENSITY_SLIDER, IDC_DENSITY_LABEL, 
                      ScreenSaverSettings::MIN_DENSITY_PERCENT, ScreenSaverSettings::MAX_DENSITY_PERCENT, 
                      pSettings->m_densityPercent);
    
    InitializeSlider (hDlg, IDC_ANIMSPEED_SLIDER, IDC_ANIMSPEED_LABEL,
                      ScreenSaverSettings::MIN_ANIMATION_SPEED_PERCENT, ScreenSaverSettings::MAX_ANIMATION_SPEED_PERCENT,
                      pSettings->m_animationSpeedPercent);
    
    InitializeSlider (hDlg, IDC_GLOWINTENSITY_SLIDER, IDC_GLOWINTENSITY_LABEL,
                      ScreenSaverSettings::MIN_GLOW_INTENSITY_PERCENT, ScreenSaverSettings::MAX_GLOW_INTENSITY_PERCENT,
                      pSettings->m_glowIntensityPercent);
    
    InitializeSlider (hDlg, IDC_GLOWSIZE_SLIDER, IDC_GLOWSIZE_LABEL,
                      ScreenSaverSettings::MIN_GLOW_SIZE_PERCENT, ScreenSaverSettings::MAX_GLOW_SIZE_PERCENT,
                      pSettings->m_glowSizePercent);
    
    InitializeColorSchemeCombo (hDlg, pSettings->m_colorSchemeKey);
    InitializeGpuCombo         (hDlg, pContext, pSettings->m_gpuAdapter);

    // Quality preset slider (0=Low, 1=Medium, 2=High, 3=Custom).
    SendDlgItemMessageW (hDlg, IDC_QUALITY_PRESET_SLIDER, TBM_SETRANGE,   TRUE, MAKELPARAM (0, 3));
    SendDlgItemMessageW (hDlg, IDC_QUALITY_PRESET_SLIDER, TBM_SETTICFREQ, 1, 0);
    SendDlgItemMessageW (hDlg, IDC_QUALITY_PRESET_SLIDER, TBM_SETPOS,     TRUE, static_cast<int> (pSettings->m_qualityPreset));
    SetDlgItemTextW     (hDlg, IDC_QUALITY_PRESET_LABEL,  FormatQualityPresetLabel (pSettings->m_qualityPreset));

    InitializePassesSlider      (hDlg, pSettings->m_advancedValues.m_blurPasses);
    InitializeResolutionSlider  (hDlg, static_cast<int> (pSettings->m_advancedValues.m_bloomResolutionDivisor));
    InitializeSmoothnessSlider  (hDlg, static_cast<int> (pSettings->m_advancedValues.m_blurTaps));

    // Per-page tooltip surface for the IDC_*_INFO indicators (only the
    // info buttons actually present on this page get registered tools).
    {
        HWND hTooltip = CreateAndRegisterTooltip (hDlg);

        if (hTooltip)
        {
            SetPropW (hDlg, kPageTooltipProp, hTooltip);
        }
    }

    // Create the 1.5x-size font used by the owner-drawn ⓘ glyphs ONCE per
    // sheet — both pages share the same font.  Created on whichever page
    // initialises first (typically Visuals due to PSP_PREMATURE order).
    if (!pContext->m_hInfoTipFont)
    {
        HFONT hDialogFont = reinterpret_cast<HFONT> (SendMessageW (hDlg, WM_GETFONT, 0, 0));
        LOGFONTW lf       = {};

        if (hDialogFont && GetObjectW (hDialogFont, sizeof (lf), &lf))
        {
            lf.lfHeight = (lf.lfHeight < 0)
                            ? -static_cast<LONG> ((-lf.lfHeight) * 3 / 2)
                            :  static_cast<LONG> ( lf.lfHeight * 3 / 2);

            pContext->m_hInfoTipFont = CreateFontIndirectW (&lf);
        }
    }
    
    CheckDlgButton (hDlg, IDC_STARTFULLSCREEN_CHECK, pSettings->m_startFullscreen     ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton (hDlg, IDC_MULTIMONITOR_CHECK,    pSettings->m_multiMonitorEnabled ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton (hDlg, IDC_SHOWDEBUG_CHECK,       pSettings->m_showDebugStats      ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton (hDlg, IDC_GLOW_ENABLED_CHECK,    pSettings->m_glowEnabled         ? BST_CHECKED : BST_UNCHECKED);

    // Hide fullscreen checkbox in screensaver CPL mode — screensaver always forces fullscreen
    if (pContext->m_isScreenSaverCPL)
    {
        ShowWindow (GetDlgItem (hDlg, IDC_STARTFULLSCREEN_CHECK), SW_HIDE);
    }

    // T044 (US2, research.md R7): mirror initial glow-enabled state into
    // EnableWindow on both pages' glow-dependent controls.  Safe to call
    // from either page proc because PSP_PREMATURE guarantees both HWNDs
    // exist; the function no-ops if PropSheet_IndexToHwnd returns null on
    // the first page's init (the second page's init re-applies).
    {
        HWND hSheet = GetParent (hDlg);

        if (hSheet)
        {
            ApplyGlowEnabledUI (hSheet, pSettings->m_glowEnabled);
        }
    }
    
    fSuccess = TRUE;

Error:
    return fSuccess;
}





////////////////////////////////////////////////////////////////////////////////
//
//  OnHScroll
//
////////////////////////////////////////////////////////////////////////////////

static void OnQualityPresetChange (HWND hDlg);

static BOOL OnHScroll (HWND hDlg, LPARAM lParam)
{
    HRESULT                  hr          = S_OK;
    BOOL                     fSuccess    = FALSE;
    ConfigDialogController * pController = GetControllerFromDialog (hDlg);
    HWND                     hSlider     = (HWND)lParam;
    int                      ctrlId      = 0;
    int                      pos         = 0;



    CBRAEx (pController != nullptr, E_UNEXPECTED);
    CBRAEx (hSlider     != nullptr, E_UNEXPECTED);

    ctrlId = GetDlgCtrlID (hSlider);
    pos    = (int)SendMessageW (hSlider, TBM_GETPOS, 0, 0);
    
    switch (ctrlId)
    {
        case IDC_DENSITY_SLIDER:
            pController->UpdateDensity (pos);
            SetDlgItemTextW (hDlg, IDC_DENSITY_LABEL, FormatPercentLabel (ctrlId, pos).c_str());
            break;
            
        case IDC_ANIMSPEED_SLIDER:
            pController->UpdateAnimationSpeed (pos);
            SetDlgItemTextW (hDlg, IDC_ANIMSPEED_LABEL, FormatPercentLabel (ctrlId, pos).c_str());
            break;
            
        case IDC_GLOWINTENSITY_SLIDER:
        {
            pController->UpdateGlowIntensity (pos);
            SetDlgItemTextW (hDlg, IDC_GLOWINTENSITY_LABEL, FormatPercentLabel (ctrlId, pos).c_str());

            // Glow Intensity is part of the advanced-graphics value set;
            // changing it drifts the preset to Custom (FR-023) the same
            // way moving Passes / Resolution / Smoothness would.
            AdvancedGraphicsValues v = pController->GetSettings().m_advancedValues;
            v.m_glowIntensityPercent = pos;
            pController->UpdateAdvancedGraphicsValues (v);
            {
                QualityPreset p = pController->GetSettings().m_qualityPreset;
                SendDlgItemMessageW (hDlg, IDC_QUALITY_PRESET_SLIDER, TBM_SETPOS, TRUE, static_cast<int> (p));
                SetDlgItemTextW     (hDlg, IDC_QUALITY_PRESET_LABEL,  FormatQualityPresetLabel (p));
            }
            break;
        }
            
        case IDC_GLOWSIZE_SLIDER:
            pController->UpdateGlowSize (pos);
            SetDlgItemTextW (hDlg, IDC_GLOWSIZE_LABEL, FormatPercentLabel (ctrlId, pos).c_str());
            break;

        case IDC_GLOWPASSES_SLIDER:
        {
            AdvancedGraphicsValues v = pController->GetSettings().m_advancedValues;
            v.m_blurPasses = pos;
            pController->UpdateAdvancedGraphicsValues (v);
            SetDlgItemTextW (hDlg, IDC_GLOWPASSES_LABEL, std::format (L"{}", pos).c_str());
            {
                QualityPreset p = pController->GetSettings().m_qualityPreset;
                SendDlgItemMessageW (hDlg, IDC_QUALITY_PRESET_SLIDER, TBM_SETPOS, TRUE, static_cast<int> (p));
                SetDlgItemTextW     (hDlg, IDC_QUALITY_PRESET_LABEL,  FormatQualityPresetLabel (p));
            }
            break;
        }

        case IDC_GLOWRES_SLIDER:
        {
            int divisor = ResolutionSliderPosToDivisor (pos);
            AdvancedGraphicsValues v = pController->GetSettings().m_advancedValues;
            v.m_bloomResolutionDivisor = static_cast<ResolutionDivisor> (divisor);
            pController->UpdateAdvancedGraphicsValues (v);
            SetDlgItemTextW (hDlg, IDC_GLOWRES_LABEL, FormatResolutionLabel (divisor));
            {
                QualityPreset p = pController->GetSettings().m_qualityPreset;
                SendDlgItemMessageW (hDlg, IDC_QUALITY_PRESET_SLIDER, TBM_SETPOS, TRUE, static_cast<int> (p));
                SetDlgItemTextW     (hDlg, IDC_QUALITY_PRESET_LABEL,  FormatQualityPresetLabel (p));
            }
            break;
        }

        case IDC_GLOWSMOOTH_SLIDER:
        {
            int taps = SmoothnessSliderPosToTaps (pos);
            AdvancedGraphicsValues v = pController->GetSettings().m_advancedValues;
            v.m_blurTaps = static_cast<BlurTaps> (taps);
            pController->UpdateAdvancedGraphicsValues (v);
            SetDlgItemTextW (hDlg, IDC_GLOWSMOOTH_LABEL, FormatSmoothnessLabel (taps));
            {
                QualityPreset p = pController->GetSettings().m_qualityPreset;
                SendDlgItemMessageW (hDlg, IDC_QUALITY_PRESET_SLIDER, TBM_SETPOS, TRUE, static_cast<int> (p));
                SetDlgItemTextW     (hDlg, IDC_QUALITY_PRESET_LABEL,  FormatQualityPresetLabel (p));
            }
            break;
        }

        case IDC_QUALITY_PRESET_SLIDER:
            OnQualityPresetChange (hDlg);
            break;
    }
    
    fSuccess = TRUE;

Error:
    return fSuccess;
}





////////////////////////////////////////////////////////////////////////////////
//
//  OnColorSchemeChange
//
////////////////////////////////////////////////////////////////////////////////

static void OnColorSchemeChange (HWND hDlg)
{
    HRESULT                  hr          = S_OK;
    ConfigDialogController * pController = GetControllerFromDialog (hDlg);
    int                      index       = 0;



    CBRAEx (pController != nullptr, E_UNEXPECTED);

    index = (int) SendDlgItemMessageW (hDlg, IDC_COLORSCHEME_COMBO, CB_GETCURSEL, 0, 0);

    CBRAEx (index >= 0 && index < static_cast<int> (ARRAYSIZE (s_colorSchemeEntries)), E_UNEXPECTED);

    pController->UpdateColorScheme (s_colorSchemeEntries[index].key);

Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  OnGpuChange
//
////////////////////////////////////////////////////////////////////////////////

static void OnGpuChange (HWND hDlg)
{
    HRESULT                  hr          = S_OK;
    DialogContext          * pContext    = GetDialogContext (hDlg);
    ConfigDialogController * pController = nullptr;
    int                      index       = 0;



    CBRAEx (pContext != nullptr && pContext->m_controller != nullptr, E_UNEXPECTED);

    pController = pContext->m_controller.get();

    index = (int) SendDlgItemMessageW (hDlg, IDC_GPU_COMBO, CB_GETCURSEL, 0, 0);

    CBRAEx (index >= 0 && index < static_cast<int> (pContext->m_gpuAdapterDescriptions.size()), E_UNEXPECTED);

    pController->UpdateGpuAdapter (pContext->m_gpuAdapterDescriptions[index]);

    // Live preview: post a rebuild so the running app recreates its
    // contexts on the newly-chosen GPU within 1 second (FR-015).
    if (Application * pApp = GetApplicationFromDialog (hDlg))
    {
        pApp->ApplyDisplayModeChange();
    }

Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  OnQualityPresetChange
//
////////////////////////////////////////////////////////////////////////////////

static void OnQualityPresetChange (HWND hDlg)
{
    HRESULT                  hr          = S_OK;
    ConfigDialogController * pController = GetControllerFromDialog (hDlg);
    int                      index       = 0;



    CBRAEx (pController != nullptr, E_UNEXPECTED);

    index = (int) SendDlgItemMessageW (hDlg, IDC_QUALITY_PRESET_SLIDER, TBM_GETPOS, 0, 0);

    CBRAEx (index >= 0 && index <= 3, E_UNEXPECTED);

    pController->UpdateQualityPreset (static_cast<QualityPreset> (index));

    {
        // Reflect the snapped advanced values back into the three sliders and
        // the Glow Intensity slider; the controller already updated them.
        const ScreenSaverSettings & s = pController->GetSettings();

        SetDlgItemTextW (hDlg, IDC_QUALITY_PRESET_LABEL, FormatQualityPresetLabel (s.m_qualityPreset));

        SendDlgItemMessageW (hDlg, IDC_GLOWINTENSITY_SLIDER, TBM_SETPOS, TRUE, s.m_advancedValues.m_glowIntensityPercent);
        SetDlgItemTextW     (hDlg, IDC_GLOWINTENSITY_LABEL,
                             FormatPercentLabel (IDC_GLOWINTENSITY_SLIDER, s.m_advancedValues.m_glowIntensityPercent).c_str());

        InitializePassesSlider     (hDlg, s.m_advancedValues.m_blurPasses);
        InitializeResolutionSlider (hDlg, static_cast<int> (s.m_advancedValues.m_bloomResolutionDivisor));
        InitializeSmoothnessSlider (hDlg, static_cast<int> (s.m_advancedValues.m_blurTaps));
    }

Error:
    return;
}




////////////////////////////////////////////////////////////////////////////////
//
//  OnStartFullscreenCheck
//
////////////////////////////////////////////////////////////////////////////////

static void OnStartFullscreenCheck (HWND hDlg)
{
    HRESULT                  hr          = S_OK;
    ConfigDialogController * pController = GetControllerFromDialog (hDlg);
    bool                     checked     = IsDlgButtonChecked (hDlg, IDC_STARTFULLSCREEN_CHECK) == BST_CHECKED;



    CBRAEx (pController != nullptr, E_UNEXPECTED);

    pController->UpdateStartFullscreen (checked);
    
    // If live overlay mode, immediately apply fullscreen/windowed state
    if (Application * pApp = GetApplicationFromDialog (hDlg))
    {
        DisplayMode newMode = checked ? DisplayMode::Fullscreen : DisplayMode::Windowed;
        pApp->GetApplicationState()->SetDisplayMode (newMode);
        pApp->ApplyDisplayModeChange();
    }

Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  OnMultiMonitorCheck
//
////////////////////////////////////////////////////////////////////////////////

static void OnMultiMonitorCheck (HWND hDlg)
{
    HRESULT                  hr          = S_OK;
    ConfigDialogController * pController = GetControllerFromDialog (hDlg);
    bool                     checked     = IsDlgButtonChecked (hDlg, IDC_MULTIMONITOR_CHECK) == BST_CHECKED;



    CBRAEx (pController != nullptr, E_UNEXPECTED);

    pController->UpdateMultiMonitorEnabled (checked);

    // Live preview: post a rebuild so the running app applies the new
    // multimon gate within the next message-loop tick.  Reuses the same
    // message handler the display-mode toggle relies on.
    if (Application * pApp = GetApplicationFromDialog (hDlg))
    {
        pApp->ApplyDisplayModeChange();
    }

Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  OnShowDebugCheck
//
////////////////////////////////////////////////////////////////////////////////

static void OnShowDebugCheck (HWND hDlg)
{
    HRESULT                  hr          = S_OK;
    ConfigDialogController * pController = GetControllerFromDialog (hDlg);
    bool                     checked     = IsDlgButtonChecked (hDlg, IDC_SHOWDEBUG_CHECK) == BST_CHECKED;



    CBRAEx (pController != nullptr, E_UNEXPECTED);

    pController->UpdateShowDebugStats (checked);
    
    // If live overlay mode, immediately apply debug stats display
    if (Application * pApp = GetApplicationFromDialog (hDlg))
    {
        pApp->GetApplicationState()->SetShowStatistics (checked);
    }

Error:
    return;
}






////////////////////////////////////////////////////////////////////////////////
//
//  OnResetButton
//
////////////////////////////////////////////////////////////////////////////////

static void OnResetButton (HWND hDlg)
{
    HRESULT                     hr          = S_OK;
    ConfigDialogController    * pController = GetControllerFromDialog (hDlg);
    const ScreenSaverSettings * pDefaults   = nullptr;
    int                         schemeIndex = 0;



    CBRAEx (pController != nullptr, E_UNEXPECTED);

    pController->ResetToDefaults();
    
    pDefaults = &pController->GetSettings();
    
    // Update UI controls
    SendDlgItemMessageW (hDlg, IDC_DENSITY_SLIDER, TBM_SETPOS, TRUE, pDefaults->m_densityPercent);
    SetDlgItemTextW     (hDlg, IDC_DENSITY_LABEL, std::format (L"{}%", pDefaults->m_densityPercent).c_str());
    
    SendDlgItemMessageW (hDlg, IDC_ANIMSPEED_SLIDER, TBM_SETPOS, TRUE, pDefaults->m_animationSpeedPercent);
    SetDlgItemTextW     (hDlg, IDC_ANIMSPEED_LABEL, std::format (L"{}%", pDefaults->m_animationSpeedPercent).c_str());
    
    SendDlgItemMessageW (hDlg, IDC_GLOWINTENSITY_SLIDER, TBM_SETPOS, TRUE, pDefaults->m_glowIntensityPercent);
    SetDlgItemTextW     (hDlg, IDC_GLOWINTENSITY_LABEL, std::format (L"{}%", pDefaults->m_glowIntensityPercent).c_str());
    
    SendDlgItemMessageW (hDlg, IDC_GLOWSIZE_SLIDER, TBM_SETPOS, TRUE, pDefaults->m_glowSizePercent);
    SetDlgItemTextW     (hDlg, IDC_GLOWSIZE_LABEL, std::format (L"{}%", pDefaults->m_glowSizePercent).c_str());
    
    // Update color scheme combo box
    schemeIndex = ColorSchemeKeyToIndex (pDefaults->m_colorSchemeKey);
    SendDlgItemMessageW (hDlg, IDC_COLORSCHEME_COMBO, CB_SETCURSEL, schemeIndex, 0);

    // v1.4 additions — multimon, GPU, quality preset slider, advanced sliders.
    CheckDlgButton (hDlg, IDC_MULTIMONITOR_CHECK, pDefaults->m_multiMonitorEnabled ? BST_CHECKED : BST_UNCHECKED);

    {
        DialogContext * pCtx = GetDialogContext (hDlg);

        if (pCtx)
        {
            // Re-resolve the default-marked entry in the existing combo.
            int defaultIdx = 0;

            for (size_t i = 0; i < pCtx->m_gpuAdapterDescriptions.size(); i++)
            {
                if (pCtx->m_gpuAdapterDescriptions[i] == pDefaults->m_gpuAdapter)
                {
                    defaultIdx = static_cast<int> (i);
                    break;
                }
            }

            SendDlgItemMessageW (hDlg, IDC_GPU_COMBO, CB_SETCURSEL, defaultIdx, 0);
        }
    }

    SendDlgItemMessageW (hDlg, IDC_QUALITY_PRESET_SLIDER, TBM_SETPOS, TRUE, static_cast<int> (pDefaults->m_qualityPreset));
    SetDlgItemTextW     (hDlg, IDC_QUALITY_PRESET_LABEL,  FormatQualityPresetLabel (pDefaults->m_qualityPreset));

    InitializePassesSlider      (hDlg, pDefaults->m_advancedValues.m_blurPasses);
    InitializeResolutionSlider  (hDlg, static_cast<int> (pDefaults->m_advancedValues.m_bloomResolutionDivisor));
    InitializeSmoothnessSlider  (hDlg, static_cast<int> (pDefaults->m_advancedValues.m_blurTaps));

    CheckDlgButton (hDlg, IDC_STARTFULLSCREEN_CHECK, pDefaults->m_startFullscreen ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton (hDlg, IDC_SHOWDEBUG_CHECK,       pDefaults->m_showDebugStats  ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton (hDlg, IDC_GLOW_ENABLED_CHECK,    pDefaults->m_glowEnabled     ? BST_CHECKED : BST_UNCHECKED);

    // Re-mirror glow-enabled state into both pages' EnableWindow flags.
    {
        HWND hSheet = GetParent (hDlg);

        if (hSheet)
        {
            ApplyGlowEnabledUI (hSheet, pDefaults->m_glowEnabled);
        }
    }
    
    // If live overlay mode, propagate changes to running application
    // (ResetToDefaults already updated controller settings, now trigger live propagation)
    CBRAEx (pController != nullptr, E_UNEXPECTED);
    if (pController->IsLiveMode())
    {
        pController->UpdateDensity                (pDefaults->m_densityPercent);
        pController->UpdateAnimationSpeed         (pDefaults->m_animationSpeedPercent);
        pController->UpdateGlowIntensity          (pDefaults->m_glowIntensityPercent);
        pController->UpdateGlowSize               (pDefaults->m_glowSizePercent);
        pController->UpdateColorScheme            (pDefaults->m_colorSchemeKey.c_str());
        pController->UpdateAdvancedGraphicsValues (pDefaults->m_advancedValues);
    }

Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  OnCommand
//
////////////////////////////////////////////////////////////////////////////////

static BOOL OnCommand (HWND hDlg, WPARAM wParam)
{
    HRESULT                  hr          = S_OK;
    BOOL                     fSuccess    = FALSE;
    ConfigDialogController * pController = GetControllerFromDialog (hDlg);



    CBRAEx (pController != nullptr, E_UNEXPECTED);

    switch (LOWORD (wParam))
    {
        case IDC_COLORSCHEME_COMBO:
            if (HIWORD (wParam) == CBN_SELCHANGE)
            {
                OnColorSchemeChange (hDlg);
            }
            break;

        case IDC_GPU_COMBO:
            if (HIWORD (wParam) == CBN_SELCHANGE)
            {
                OnGpuChange (hDlg);
            }
            break;

        case IDC_QUALITY_PRESET_INFO:
        case IDC_GLOWINTENSITY_INFO:
        case IDC_GLOWSIZE_INFO:
        case IDC_GLOWPASSES_INFO:
        case IDC_GLOWRES_INFO:
        case IDC_GLOWSMOOTH_INFO:
            // BN_CLICKED on any info button (mouse click OR Space/Enter on
            // a keyboard-focused button) pops the matching infotip via
            // TTM_TRACKACTIVATE.  Auto-dismisses after 5s.
            OnInfoButtonClick (hDlg, LOWORD (wParam));
            break;
            
        case IDC_STARTFULLSCREEN_CHECK:
            OnStartFullscreenCheck (hDlg);
            break;

        case IDC_MULTIMONITOR_CHECK:
            OnMultiMonitorCheck (hDlg);
            break;
            
        case IDC_SHOWDEBUG_CHECK:
            OnShowDebugCheck (hDlg);
            break;

        case IDC_GLOW_ENABLED_CHECK:
        {
            // T044 (US2, FR-016, FR-017): toggle drives the controller AND
            // immediately propagates EnableWindow across both tabs.
            bool                     enabled    = (IsDlgButtonChecked (hDlg, IDC_GLOW_ENABLED_CHECK) == BST_CHECKED);
            ConfigDialogController * pCtrl      = GetControllerFromDialog (hDlg);
            HWND                     hSheet     = GetParent (hDlg);

            if (pCtrl)
            {
                pCtrl->UpdateGlowEnabled (enabled);
            }

            ApplyGlowEnabledUI (hSheet, enabled);
            break;
        }
            
        case IDC_RESET_BUTTON:
            OnResetButton (hDlg);
            break;
    }

Error:
    return fSuccess;
}





////////////////////////////////////////////////////////////////////////////////
//
//  OnDestroy — per-page cleanup only.  The shared DialogContext (controller,
//  app pointer, info-tip font) is owned by the property-sheet frame and
//  freed in PropSheetCallback / DestroySheetContext, not here.
//
////////////////////////////////////////////////////////////////////////////////

static void OnDestroy (HWND hDlg)
{
    HWND hTooltip = static_cast<HWND> (GetPropW (hDlg, kPageTooltipProp));


    if (hTooltip)
    {
        DestroyWindow (hTooltip);
        RemovePropW   (hDlg, kPageTooltipProp);
    }

    SetWindowLongPtr (hDlg, DWLP_USER, 0);
}





////////////////////////////////////////////////////////////////////////////////
//
//  ConfigDialogProc
//
//  Page dialog procedure for IDD_VISUALS_PAGE / IDD_PERFORMANCE_PAGE.
//  v1.5 (T029, T030): both pages share this proc — control handlers
//  dispatch by ID, so a page only handles the controls actually present
//  in its template.  PSN_APPLY / PSN_RESET notifications (frame-owned
//  OK / Cancel buttons) are translated into CommitLiveMode /
//  CancelLiveMode here, with idempotency guards on DialogContext so the
//  second page to see the notification is a no-op.
//
////////////////////////////////////////////////////////////////////////////////

static INT_PTR CALLBACK PageDlgProc (HWND   hDlg,
                                      UINT   message,
                                      WPARAM wParam,
                                      LPARAM lParam)
{
    INT_PTR result = FALSE;



    switch (message)
    {
        case WM_INITDIALOG:
            result = OnInitDialog (hDlg, lParam);
            break;
        
        case WM_HSCROLL:
            result = OnHScroll (hDlg, lParam);
            break;
        
        case WM_COMMAND:
            result = OnCommand (hDlg, wParam);
            break;

        case WM_DRAWITEM:
        {
            // Owner-draw paint for the ⓘ info indicators.  No button frame:
            // we draw only the glyph (transparent background, 1.5x font).
            LPDRAWITEMSTRUCT pdis = reinterpret_cast<LPDRAWITEMSTRUCT> (lParam);

            if (pdis && pdis->CtlType == ODT_BUTTON && IsInfoTipControlId (pdis->CtlID))
            {
                DialogContext * pContext = GetDialogContext (hDlg);
                HFONT           hOldFont = nullptr;
                int             oldBkMode;
                COLORREF        oldTextColor;
                wchar_t         glyph[]  = L"\u24D8";

                // Erase the entire rect with the dialog's face color so any
                // previous focus rect (XOR-drawn by the system) is cleared
                // before we redraw the glyph + optional new focus rect.
                FillRect (pdis->hDC, &pdis->rcItem, GetSysColorBrush (COLOR_3DFACE));

                if (pContext && pContext->m_hInfoTipFont)
                {
                    hOldFont = static_cast<HFONT> (SelectObject (pdis->hDC, pContext->m_hInfoTipFont));
                }

                oldBkMode    = SetBkMode    (pdis->hDC, TRANSPARENT);
                oldTextColor = SetTextColor (pdis->hDC, GetSysColor (COLOR_WINDOWTEXT));

                DrawTextW (pdis->hDC,
                           glyph,
                           1,
                           &pdis->rcItem,
                           DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);

                SetTextColor (pdis->hDC, oldTextColor);
                SetBkMode    (pdis->hDC, oldBkMode);

                if (hOldFont)
                {
                    SelectObject (pdis->hDC, hOldFont);
                }

                if (pdis->itemState & ODS_FOCUS)
                {
                    DrawFocusRect (pdis->hDC, &pdis->rcItem);
                }

                result = TRUE;
            }
            break;
        }

        case WM_NOTIFY:
        {
            LPNMHDR pnmhdr = reinterpret_cast<LPNMHDR> (lParam);

            if (!pnmhdr)
            {
                break;
            }

            // Tooltip text callback — supplies the locked infotip string.
            if (pnmhdr->code == TTN_GETDISPINFOW || pnmhdr->code == TTN_NEEDTEXTW)
            {
                NMTTDISPINFOW * pdi       = reinterpret_cast<NMTTDISPINFOW *> (lParam);
                HWND            hToolHwnd = reinterpret_cast<HWND> (pdi->hdr.idFrom);
                int             toolId    = GetDlgCtrlID (hToolHwnd);

                if (IsInfoTipControlId (toolId))
                {
                    pdi->lpszText = const_cast<LPWSTR> (GetInfoTipText (toolId));
                    result        = TRUE;
                }
                break;
            }

            // v1.5 (T030, T033, FR-004a, contracts/propertysheet.md):
            // property-sheet OK / Cancel notifications.
            DialogContext          * pContext    = GetDialogContext (hDlg);
            ConfigDialogController * pController = pContext ? pContext->m_controller.get() : nullptr;

            if (!pContext || !pController)
            {
                break;
            }

            switch (pnmhdr->code)
            {
                case PSN_APPLY:
                    if (!pContext->m_applied)
                    {
                        pContext->m_applied = true;

                        if (pController->IsLiveMode())
                        {
                            pController->CommitLiveMode();
                        }
                        else
                        {
                            pController->ApplyChanges();
                        }
                    }
                    SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                    result = TRUE;
                    break;

                case PSN_RESET:
                    if (!pContext->m_rolledBack)
                    {
                        pContext->m_rolledBack = true;

                        if (pController->IsLiveMode())
                        {
                            bool          needsRebuild = pController->LiveModeRebuildRequired();
                            Application * pApp         = pContext->m_pApp;

                            pController->CancelLiveMode();

                            if (pApp && needsRebuild)
                            {
                                pApp->ApplyDisplayModeChange();
                            }
                        }
                        else
                        {
                            pController->CancelChanges();
                        }
                    }
                    result = TRUE;
                    break;
            }
            break;
        }

        case WM_TIMER:
            if (wParam == kInfoTipDismissTimerId)
            {
                DismissInfoTip (hDlg);
                result = TRUE;
            }
            break;

        case WM_ACTIVATE:
            // Lose-focus on the dialog dismisses any active TTF_TRACK tip.
            if (LOWORD (wParam) == WA_INACTIVE)
            {
                DismissInfoTip (hDlg);
            }
            break;
        
        case WM_DESTROY:
            OnDestroy (hDlg);
            result = TRUE;
            break;
    }

    return result;
}





////////////////////////////////////////////////////////////////////////////////
//
//  v1.5 property-sheet host (T029, T031, T032, T033)
//
//  Builds the PROPSHEETHEADERW + 2 PROPSHEETPAGEW templates, installs the
//  1 Hz title-update timer in PSCB_INITIALIZED, and tears down the shared
//  DialogContext after the sheet closes.
//
////////////////////////////////////////////////////////////////////////////////

static VOID CALLBACK PerfTitleTimerProc (HWND hSheet, UINT /*msg*/, UINT_PTR /*id*/, DWORD /*time*/)
{
    DialogContext * pContext = static_cast<DialogContext *> (GetPropW (hSheet, kSheetContextProp));


    if (!pContext)
    {
        return;
    }

    // FPS source — primary monitor's render context (FR-010).
    unsigned fps = 0;

    if (pContext->m_pApp)
    {
        MonitorRenderContext * pPrimary = pContext->m_pApp->GetPrimaryRenderContext();
        bool                   hasFps   = false;
        float                  fpsValue = 0.0f;

        if (pPrimary)
        {
            fpsValue = pPrimary->GetPublishedFps (hasFps);

            if (hasFps && fpsValue > 0.0f)
            {
                fps = static_cast<unsigned> (fpsValue + 0.5f);
            }
        }
    }

    // GPU% source — PDH counter shared with the debug overlay (FR-011).
    unsigned gpu     = 0;
    double   gpuLoad = QueryProcessGpuLoadPercent();

    if (gpuLoad >= 0.0)
    {
        gpu = static_cast<unsigned> (gpuLoad + 0.5);
    }

    WCHAR title[64] = {};
    StringCchPrintfW (title,
                      ARRAYSIZE (title),
                      pContext->m_perfTitleFormat,
                      fps,
                      gpu);

    HWND hTab = PropSheet_GetTabControl (hSheet);

    if (!hTab)
    {
        return;
    }

    TCITEMW item = {};
    item.mask    = TCIF_TEXT;
    item.pszText = title;

    TabCtrl_SetItem (hTab, 1 /* Performance is page index 1 */, &item);
}





static int CALLBACK PropSheetCallback (HWND hSheet, UINT uMsg, LPARAM lParam)
{
    UNREFERENCED_PARAMETER (lParam);


    switch (uMsg)
    {
        case PSCB_INITIALIZED:
        {
            DialogContext * pContext = static_cast<DialogContext *> (GetPropW (hSheet, kSheetContextProp));

            if (!pContext)
            {
                break;
            }

            pContext->m_hSheet = hSheet;

            // Update Application's "current dialog" pointer to the sheet
            // frame so the main message loop's IsDialogMessage branch picks
            // up Tab/Enter routing correctly (T031, research.md R1).
            if (pContext->m_pApp)
            {
                pContext->m_pApp->SetConfigDialog (hSheet);
            }

            // Centre the sheet over the application window (live overlay)
            // or the primary monitor (CPL).  Moved here from the per-page
            // OnInitDialog — the frame is what the user sees, not the pages.
            {
                RECT  sheetRect  = {};
                RECT  centerRect = {};
                HWND  appHwnd    = pContext->m_pApp ? pContext->m_pApp->GetMainWindowHwnd() : nullptr;

                GetWindowRect (hSheet, &sheetRect);

                int sheetW = sheetRect.right  - sheetRect.left;
                int sheetH = sheetRect.bottom - sheetRect.top;

                if (appHwnd)
                {
                    GetWindowRect (appHwnd, &centerRect);
                }
                else
                {
                    centerRect.left   = 0;
                    centerRect.top    = 0;
                    centerRect.right  = GetSystemMetrics (SM_CXSCREEN);
                    centerRect.bottom = GetSystemMetrics (SM_CYSCREEN);
                }

                int x = (centerRect.left + centerRect.right  - sheetW) / 2;
                int y = (centerRect.top  + centerRect.bottom - sheetH) / 2;

                SetWindowPos (hSheet, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            }

            // Load the format string once and cache on the context.
            LoadStringW (GetModuleHandleW (nullptr),
                         IDS_PERFTAB_TITLE_FORMAT,
                         pContext->m_perfTitleFormat,
                         ARRAYSIZE (pContext->m_perfTitleFormat));

            // 1 Hz timer; TimerProc form so we don't have to subclass the
            // comctl32-owned frame to catch WM_TIMER.  KillTimer is implicit
            // when the frame is destroyed.
            SetTimer (hSheet, kPerfTitleTimerId, 1000, PerfTitleTimerProc);

            // Fire one immediate tick so the title shows something other
            // than "Performance" before the first second elapses.
            PerfTitleTimerProc (hSheet, WM_TIMER, kPerfTitleTimerId, GetTickCount());
            break;
        }
    }

    return 0;
}





////////////////////////////////////////////////////////////////////////////////
//
//  BuildPropSheet — populate PROPSHEETPAGEW[2] and PROPSHEETHEADERW pointing
//  at the Visuals + Performance templates and at PageDlgProc.
//
////////////////////////////////////////////////////////////////////////////////

static void BuildPropSheet (HINSTANCE         hInstance,
                             HWND              parentHwnd,
                             DialogContext   * pContext,
                             bool              modeless,
                             PROPSHEETPAGEW    psPages[2],
                             PROPSHEETHEADERW & psHeader)
{
    psPages[0]              = {};
    psPages[0].dwSize       = sizeof (PROPSHEETPAGEW);
    psPages[0].dwFlags      = PSP_USETITLE | PSP_PREMATURE;
    psPages[0].hInstance    = hInstance;
    psPages[0].pszTemplate  = MAKEINTRESOURCEW (IDD_VISUALS_PAGE);
    psPages[0].pszTitle     = MAKEINTRESOURCEW (IDS_VISUALS_TAB_TITLE);
    psPages[0].pfnDlgProc   = PageDlgProc;
    psPages[0].lParam       = reinterpret_cast<LPARAM> (pContext);

    psPages[1]              = {};
    psPages[1].dwSize       = sizeof (PROPSHEETPAGEW);
    psPages[1].dwFlags      = PSP_USETITLE | PSP_PREMATURE;
    psPages[1].hInstance    = hInstance;
    psPages[1].pszTemplate  = MAKEINTRESOURCEW (IDD_PERFORMANCE_PAGE);
    psPages[1].pszTitle     = MAKEINTRESOURCEW (IDS_PERFORMANCE_TAB_TITLE_INITIAL);
    psPages[1].pfnDlgProc   = PageDlgProc;
    psPages[1].lParam       = reinterpret_cast<LPARAM> (pContext);

    psHeader                = {};
    psHeader.dwSize         = sizeof (PROPSHEETHEADERW);
    psHeader.dwFlags        = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW | PSH_PROPTITLE | PSH_USECALLBACK
                              | (modeless ? PSH_MODELESS : 0u);
    psHeader.hwndParent     = parentHwnd;
    psHeader.hInstance      = hInstance;
    psHeader.pszCaption     = L"MatrixRain configuration";
    psHeader.nPages         = 2;
    psHeader.nStartPage     = 0;
    psHeader.ppsp           = psPages;
    psHeader.pfnCallback    = PropSheetCallback;
}





////////////////////////////////////////////////////////////////////////////////
//
//  ShowConfigDialog (modal, Control Panel /c:<HWND>)
//
////////////////////////////////////////////////////////////////////////////////

int ShowConfigDialog (HINSTANCE hInstance, const ScreenSaverModeContext & context)
{
    HRESULT          hr         = S_OK;
    DialogContext    dlgContext;
    PROPSHEETPAGEW   psPages[2] = {};
    PROPSHEETHEADERW psHeader   = {};
    HWND             parentHwnd = context.m_previewParentHwnd;
    INT_PTR          result     = -1;
    


    dlgContext.m_controller = std::make_unique<ConfigDialogController> (dlgContext.m_settingsProvider);
    dlgContext.m_isScreenSaverCPL = true;
    hr = dlgContext.m_controller->Initialize();
    CHRA (hr);

    BuildPropSheet (hInstance, parentHwnd, &dlgContext, /*modeless=*/false, psPages, psHeader);

    // Stash the context on a "hint" property of a temp hidden window?  No —
    // PSCB_INITIALIZED runs on the sheet hwnd which we don't have ahead of
    // time.  Workaround: a static thread_local fallback used inside the
    // callback as a bootstrap (cleared on entry).  Modal path is single-
    // threaded so this is safe.
    static thread_local DialogContext * tls_pendingContext = nullptr;
    tls_pendingContext = &dlgContext;

    // Wrapper callback that installs the SetProp on first call, then
    // delegates to PropSheetCallback.  We use a lambda-free static here
    // so it's a plain C callback.
    struct CbHelper
    {
        static int CALLBACK Trampoline (HWND hSheet, UINT uMsg, LPARAM lParam)
        {
            if (uMsg == PSCB_INITIALIZED && tls_pendingContext)
            {
                SetPropW (hSheet, kSheetContextProp, tls_pendingContext);
                tls_pendingContext = nullptr;
            }
            return PropSheetCallback (hSheet, uMsg, lParam);
        }
    };
    psHeader.pfnCallback = CbHelper::Trampoline;

    result = PropertySheetW (&psHeader);

    if (result == -1)
    {
        hr = HRESULT_FROM_WIN32 (GetLastError());
    }

Error:
    if (FAILED (hr))
    {
        MessageBoxW (nullptr, L"Failed to initialize configuration dialog.", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    // PropertySheetW returns ID_PSREBOOT, ID_PSRESTARTWINDOWS, 1 (OK), or 0
    // (Cancel) in modal mode.  Map to IDOK / IDCANCEL for the caller.
    return (result > 0) ? IDOK : IDCANCEL;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CreateConfigDialog (modeless, live overlay)
//
//  Returns the property-sheet frame HWND via phDlg; lifetime of the
//  DialogContext is managed by DestroySheetContext when the frame's
//  WM_DESTROY fires (we observe it via a subclass installed in
//  PSCB_INITIALIZED).
//
////////////////////////////////////////////////////////////////////////////////

namespace
{


LRESULT CALLBACK SheetFrameSubclass (HWND     hSheet,
                                      UINT     msg,
                                      WPARAM   wParam,
                                      LPARAM   lParam,
                                      UINT_PTR uIdSubclass,
                                      DWORD_PTR /*dwRefData*/)
{
    if (msg == WM_DESTROY)
    {
        DialogContext * pContext = static_cast<DialogContext *> (GetPropW (hSheet, kSheetContextProp));

        if (pContext)
        {
            // Tear down per-sheet resources before the frame is gone.
            if (pContext->m_pApp)
            {
                pContext->m_pApp->SetConfigDialog (nullptr);

                if (pContext->m_pApp->GetScreenSaverMode() == ScreenSaverMode::SettingsDialog)
                {
                    PostQuitMessage (0);
                }
            }

            if (pContext->m_hInfoTipFont)
            {
                DeleteObject (pContext->m_hInfoTipFont);
                pContext->m_hInfoTipFont = nullptr;
            }

            RemovePropW (hSheet, kSheetContextProp);

            if (pContext->m_ownsContextMemory)
            {
                delete pContext;
            }
        }
    }
    else if (msg == WM_NCDESTROY)
    {
        RemoveWindowSubclass (hSheet, SheetFrameSubclass, uIdSubclass);
    }

    return DefSubclassProc (hSheet, msg, wParam, lParam);
}


constexpr UINT_PTR kSheetFrameSubclassId = 0xC0C00C2Du;


}  // namespace


HRESULT CreateConfigDialog (HINSTANCE          hInstance,
                             HWND               parentHwnd,
                             Application      * pApplication,
                             ApplicationState * pAppState,
                             HWND             * phDlg)
{
    HRESULT          hr         = S_OK;
    HWND             hSheet     = nullptr;
    PROPSHEETPAGEW   psPages[2] = {};
    PROPSHEETHEADERW psHeader   = {};
    auto             context    = std::make_unique<DialogContext>();


    
    context->m_controller = std::make_unique<ConfigDialogController> (pApplication->GetSettingsProvider());
    hr = context->m_controller->Initialize();
    CHRA (hr);

    hr = context->m_controller->InitializeLiveMode (pAppState);
    CHRA (hr);

    context->m_pApp              = pApplication;
    context->m_ownsContextMemory = true;

    {
        DialogContext * pCtx = context.get();

        BuildPropSheet (hInstance, parentHwnd, pCtx, /*modeless=*/true, psPages, psHeader);

        // Modeless trampoline (mirrors the modal version): bootstrap the
        // SetPropW(kSheetContextProp) on first PSCB_INITIALIZED, install the
        // frame subclass for WM_DESTROY cleanup, then delegate.
        static thread_local DialogContext * tls_pendingContext = nullptr;
        tls_pendingContext = pCtx;

        struct CbHelper
        {
            static int CALLBACK Trampoline (HWND hSheet, UINT uMsg, LPARAM lParam)
            {
                if (uMsg == PSCB_INITIALIZED && tls_pendingContext)
                {
                    SetPropW (hSheet, kSheetContextProp, tls_pendingContext);
                    SetWindowSubclass (hSheet, SheetFrameSubclass, kSheetFrameSubclassId, 0);
                    tls_pendingContext = nullptr;
                }
                return PropSheetCallback (hSheet, uMsg, lParam);
            }
        };
        psHeader.pfnCallback = CbHelper::Trampoline;
    }

    hSheet = reinterpret_cast<HWND> (PropertySheetW (&psHeader));
    CWRA (hSheet);

    context.release();   // ownership transferred to the sheet subclass

    ShowWindow (hSheet, SW_SHOW);

Error:
    if (FAILED (hr))
    {
        MessageBoxW (nullptr, L"Failed to create live overlay configuration dialog.", L"Error", MB_OK | MB_ICONERROR);
        hSheet = nullptr;
    }

    *phDlg = hSheet;

    return hr;
}
